/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */


#include <config.h>
#include <gnome.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "logview.h"
#include "gnome.h"

#define ZOOM_WIDTH            500
#define ZOOM_HEIGHT           300


/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

int zoom_visible;
GtkWidget *zoom_dialog;
GtkWidget *zoom_canvas;
PangoLayout *zoom_layout = NULL;

extern ConfigData *cfg;
extern GdkGC *gc;
extern Log *curlog;
extern char *month[12];
extern GList *regexp_db, *descript_db;
extern GnomeUIInfo view_menu[];

void close_zoom_view (GtkWidget *widget, gpointer data);
void quit_zoom_view (GtkWidget *widget, gpointer data);
void create_zoom_view (GtkWidget *widget, gpointer data);
int match_line_in_db (LogLine *line, GList *db);
void draw_parbox (GdkDrawable *win, PangoFontDescription *font, GdkGC *gc,  \
	     int x, int y, int width, char *text, int max_num_lines);

/* ----------------------------------------------------------------------
   NAME:          create_zoom_view
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
create_zoom_view (GtkWidget *widget, gpointer data)
{

   if (curlog == NULL || zoom_visible)
      return;

   if (zoom_dialog == NULL) {
   	GtkWidget *frame;
   	GtkWidget *vbox;
   	int h1, h2, height;
   	PangoContext *context;
   	PangoFontMetrics *metrics;
   	PangoFont *font;

   	zoom_dialog  = gtk_dialog_new_with_buttons (_("Zoom view"), 
					       	    GTK_WINDOW_TOPLEVEL,
					            GTK_DIALOG_DESTROY_WITH_PARENT,
					            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, 
					            NULL);
  
        g_signal_connect (G_OBJECT (zoom_dialog), "response",
		          G_CALLBACK (close_zoom_view),
			  &zoom_dialog);
   	g_signal_connect (G_OBJECT (zoom_dialog), "destroy",
		          G_CALLBACK (quit_zoom_view),
			  &zoom_dialog);
        g_signal_connect (G_OBJECT (zoom_dialog), "delete_event",
		          G_CALLBACK (gtk_true),
			  NULL);
   	
	gtk_dialog_set_default_response (GTK_DIALOG (zoom_dialog), GTK_RESPONSE_CLOSE);
   	gtk_container_set_border_width (GTK_CONTAINER (zoom_dialog), 0);
   	gtk_widget_set_style (zoom_dialog, cfg->main_style);

   	vbox = gtk_vbox_new (FALSE, 2);
   	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
   	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (zoom_dialog)->vbox), vbox,
                            TRUE, TRUE, 0);
   	gtk_widget_show (vbox);

   	/* Create frame for main view */
   	frame = gtk_frame_new (NULL);
   	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   	gtk_container_set_border_width (GTK_CONTAINER (frame), 3);
   	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
   	gtk_widget_set_style (frame, cfg->main_style);
   	gtk_widget_show (frame);


   	/* Create drawing area where data will be drawn */
   	zoom_canvas = gtk_drawing_area_new ();
   	gtk_signal_connect (GTK_OBJECT (zoom_canvas), "expose_event",
		            (GtkSignalFunc) repaint_zoom, zoom_canvas);

   	zoom_layout = gtk_widget_create_pango_layout (zoom_canvas, "");
   	context = gdk_pango_context_get ();
   	pango_context_set_language (context, gtk_get_default_language ()); 

   	font = LoadFont (context, cfg->headingb);
   	metrics = pango_font_get_metrics (font, pango_context_get_language (context));
   	h1 = PANGO_PIXELS (pango_font_metrics_get_descent (metrics)) +
	     PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));

   	font = LoadFont (context, cfg->fixedb);
   	metrics = pango_font_get_metrics (font, pango_context_get_language (context));
   	h2 = PANGO_PIXELS (pango_font_metrics_get_descent (metrics)) +
	     PANGO_PIXELS (pango_font_metrics_get_ascent (metrics))+2;

   	height = h1*2+14*h2;
   	GTK_WIDGET (zoom_canvas)->requisition.height = height;
   	GTK_WIDGET (zoom_canvas)->requisition.width = ZOOM_WIDTH;
   	gtk_widget_queue_resize (GTK_WIDGET (zoom_canvas));
   	gtk_widget_set_events (zoom_canvas, GDK_EXPOSURE_MASK);
   	/* gtk_widget_set_style (zoom_canvas, cfg->black_bg_style); */
   	gtk_widget_set_style (zoom_canvas, cfg->white_bg_style);
   	gtk_container_add (GTK_CONTAINER (frame), zoom_canvas);
   	gtk_widget_show (zoom_canvas);
   	gtk_widget_realize (zoom_canvas);
   
	g_object_unref (G_OBJECT (context));
   	pango_font_metrics_unref (metrics);
   	g_object_unref (G_OBJECT (font));
   }
   gtk_widget_show (zoom_dialog);
   gtk_widget_queue_draw (zoom_canvas);

   zoom_visible = TRUE;
}

static int
get_max_len (void)
{
	int len = 5;
	int i;
	char *strings[] = {
		"Date:",
		"Process:",
		"Message:",
		"Description:",
		NULL
	};
	for (i = 0; strings[i] != NULL; i++) {
		int l = g_utf8_strlen (_(strings[i]), 1024);
		if (l > len)
			len = l;
	}
	return len+1;
}


/* ----------------------------------------------------------------------
   NAME:          repaint_zoom
   DESCRIPTION:   Repaint the zoom window.
   ---------------------------------------------------------------------- */

int
repaint_zoom (GtkWidget * widget, GdkEventExpose * event)
{
   static GdkDrawable *canvas;
   char buffer[1024];
   int x , y , h, w, win_width;
   int win_height;
   GdkRectangle *area;
   LogLine *line;
   struct tm date = {0};
   int af, df, afdb, dfdb, ah, dh;
   PangoContext *context;
   PangoFontMetrics *metrics;
   PangoFont *font;
   PangoRectangle logical_rect;
   char *utf8;
   int max_len;

   canvas = zoom_canvas->window;
   win_width = zoom_canvas->allocation.width;
   win_height = zoom_canvas->allocation.height;


   /* Get area that was exposed */
   area = NULL;
   if (event != NULL)
       area = &event->area;

   /* Draw title */
   context = gdk_pango_context_get ();
   pango_context_set_language (context, gtk_get_default_language ());
   font = LoadFont (context, cfg->headingb);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   ah = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));
   dh = PANGO_PIXELS (pango_font_metrics_get_descent (metrics));
   h = dh + ah;
   /* Note: we use a real translated string that will appear below to gather
    * the length, that way we'll get semi correct results with a lot more
    * fonts */
   pango_layout_set_text (zoom_layout, _("Date:"), -1);
   pango_layout_set_font_description (zoom_layout, cfg->fixedb);
   pango_layout_get_pixel_extents (zoom_layout, NULL, &logical_rect);
   w = logical_rect.width / g_utf8_strlen (_("Date:"), 1024);
   max_len = get_max_len ();

   x = 5;
   y = ah + 6;
   gdk_gc_set_foreground (gc, &cfg->blue);
   gdk_draw_rectangle (canvas, gc, TRUE, 3, 3, win_width - 6, h+6);
   gdk_gc_set_foreground (gc, &cfg->white);
   if (curlog != NULL)
     g_snprintf (buffer, sizeof (buffer),
		 _("Log line detail for %s"), curlog->name);
   else
     g_snprintf (buffer, sizeof (buffer),
		 _("Log line detail for <No log loaded>"));
   pango_layout_set_text (zoom_layout, buffer, -1);
   pango_layout_set_font_description (zoom_layout, cfg->headingb);
   gdk_draw_layout (canvas, gc, x, y-12, zoom_layout);
   font = LoadFont (context, cfg->fixedb);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   afdb = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));
   dfdb = PANGO_PIXELS (pango_font_metrics_get_descent (metrics));
   y += 9 + dh + afdb;

   /* Draw Info */
   gdk_gc_set_foreground (gc, &cfg->black);
   /* gdk_gc_set_foreground (gc, &cfg->white); */

   h = dfdb + afdb+2;

   /* Draw bg. rectangles */
   font = LoadFont (context, cfg->fixed);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   af = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));
   df = PANGO_PIXELS (pango_font_metrics_get_descent (metrics));

   gdk_gc_set_foreground (gc, &cfg->blue3);
   gdk_draw_rectangle (canvas, gc, TRUE, max_len*w+6, 
		       y - af-3, 
		       win_width - 9 - max_len*w, 
		       win_height - (y - af) );
   gdk_gc_set_foreground (gc, &cfg->gray75);
   gdk_draw_rectangle (canvas, gc, TRUE, 3, 
		       y-af-3, 
		       max_len*w, win_height - (y - af) );

   gdk_gc_set_foreground (gc, &cfg->black);
   pango_layout_set_text (zoom_layout, _("Date:"), -1);
   pango_layout_set_font_description (zoom_layout, cfg->fixedb);
   gdk_draw_layout (canvas, gc, x, y, zoom_layout);
   y += h;
   pango_layout_set_text (zoom_layout, _("Process:"), -1);
   gdk_draw_layout (canvas, gc, x, y, zoom_layout);
   y += h;
   pango_layout_set_text (zoom_layout, _("Message:"), -1);
   gdk_draw_layout (canvas, gc, x, y, zoom_layout);
   y += 4*h;
   pango_layout_set_text (zoom_layout, _("Description:"), -1);
   gdk_draw_layout (canvas, gc, x, y-8, zoom_layout);

   /* Check that there is at least one log */
   if (curlog == NULL)
      return -1;

   /* Draw line info */
   h = dh + ah;
   y = ah + 6;
   y += 9 + dh + afdb;
   x = max_len*w + 6 + 2;
   h = dfdb + afdb;

   line = &(curlog->pointerpg->line[curlog->pointerln]);

   date.tm_mon = line->month;
   date.tm_year = 70 /* bogus */;
   date.tm_mday = line->date;
   date.tm_hour = line->hour;
   date.tm_min = line->min;
   date.tm_sec = line->sec;
   date.tm_isdst = 0;

   /* Translators: do not include year, it would be bogus */
   if (strftime (buffer, sizeof (buffer), _("%B %e %X"), &date) <= 0) {
	   /* as a backup print in US style */
	   utf8 = g_strdup_printf ("%s %d %02d:%02d:%02d",
				   _(month[(int)line->month]), 
				   (int) line->date,
				   (int) line->hour,
				   (int) line->min,
				   (int) line->sec);
   } else {
	   utf8 = LocaleToUTF8 (buffer);
   }
   pango_layout_set_font_description (zoom_layout, cfg->fixed);
   pango_layout_set_text (zoom_layout, utf8, -1);
   g_free (utf8);
   gdk_draw_layout (canvas, gc, x, y, zoom_layout);
   y += h;
   g_snprintf (buffer, sizeof (buffer), "%s ", line->process);
   utf8 = LocaleToUTF8 (buffer);
   pango_layout_set_text (zoom_layout, utf8, -1);
   g_free (utf8);
   gdk_draw_layout (canvas, gc, x, y+2, zoom_layout);
   y += h;
   utf8 = LocaleToUTF8 (line->message);
   draw_parbox (canvas, cfg->fixed, gc, x, y+4, 380, utf8, 4);
   g_free (utf8);
   y += 4*h;
   if (match_line_in_db (line, regexp_db))
     {
       if (find_tag_in_db (line, descript_db))
	 draw_parbox (canvas, cfg->fixed, gc, x, y+4, 360,
			  line->description->description, 8);
     }

   g_object_unref (G_OBJECT (context));
   pango_font_metrics_unref (metrics);
   g_object_unref (G_OBJECT (font));
   return TRUE;
}


/* ----------------------------------------------------------------------
   NAME:          draw_parbox
   DESCRIPTION:   Draw the given text in a box of a given width breaking
                  words. The width is given in pixels.
   ---------------------------------------------------------------------- */

void
draw_parbox (GdkDrawable *win, PangoFontDescription *font, GdkGC *gc, 
	     int x, int y, int width, char *text, int max_num_lines)
{
   char *p1, *p2, *p3, c;
   char *buffer;
   int  ypos, done;
   PangoContext *context;
   PangoFontMetrics *metrics;
   PangoRectangle logical_rect;
   PangoFont *pfont;

   ypos = y;

   context = gdk_pango_context_get ();
   pango_context_set_language (context, gtk_get_default_language ());
   pfont = LoadFont (context, font);
   metrics = pango_font_get_metrics
	     (pfont, pango_context_get_language (context));
   pango_layout_set_font_description (zoom_layout, font);

   /* Copy string and modify our buffer string */
   buffer = g_strdup(text);

   done = FALSE;
   p1 = p2 = buffer;
   while (!done)
     {
       c = *p1; *p1 = 0;
       pango_layout_set_text (zoom_layout, p2, -1);
       pango_layout_get_pixel_extents (zoom_layout, NULL, &logical_rect);
       while ( logical_rect.width < width && c != 0)
         {
	   *p1 = c;
	   p1 = g_utf8_next_char (p1);
	   c = *p1;
	   *p1 = 0;
	   pango_layout_set_text (zoom_layout, p2, -1);
	   pango_layout_get_pixel_extents (zoom_layout, NULL, &logical_rect);
         }
       switch (c)
         {
         case ' ':
	   p1 = g_utf8_next_char (p1);
	   pango_layout_set_text (zoom_layout, p2, -1);
	   gdk_draw_layout (win, gc, x, ypos, zoom_layout);
	   break;
         case '\0':
	   done = TRUE;
	   pango_layout_set_text (zoom_layout, p2, -1);
	   gdk_draw_layout (win, gc, x, ypos, zoom_layout);
	   break;
         default:
	   p3 = p1;
	   while (*p1 != ' ' && p1 > p2)
	     p1 = g_utf8_prev_char (p1);
	   if (p1 == p2)
	     {
	       pango_layout_set_text (zoom_layout, p2, -1);
	       gdk_draw_layout (win, gc, x, ypos, zoom_layout);
               *p3 = c;
               p1 = p3;
               break;
	     }
	   else
	     {
               *p3 = c;
               *p1 = 0;
	       p1 = g_utf8_next_char (p1);
	       pango_layout_set_text (zoom_layout, p2, -1);
	       gdk_draw_layout (win, gc, x, ypos, zoom_layout);
	     }
	   break;
         }
       ypos += PANGO_PIXELS (pango_font_metrics_get_descent (metrics)) +
	       PANGO_PIXELS (pango_font_metrics_get_ascent (metrics)); 
       p2 = p1;
     }
   
   g_object_unref (G_OBJECT (pfont));
   pango_font_metrics_unref (metrics);
   g_object_unref (G_OBJECT (context));
   g_free (buffer);

}


/* ----------------------------------------------------------------------
   NAME:          CloseLogInfo
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
close_zoom_view (GtkWidget *widget, gpointer data)
{
   if (zoom_visible) {
      gtk_widget_hide (GTK_WIDGET (zoom_dialog));
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view_menu[1].widget), FALSE);
/*
      if (G_IS_OBJECT (zoom_layout))
         g_object_unref (G_OBJECT (zoom_layout)); */
   }
   zoom_dialog = NULL;
   zoom_visible = FALSE;
}

void
quit_zoom_view (GtkWidget *widget, gpointer data) {

	if (zoom_layout != NULL)
		g_object_unref (G_OBJECT (zoom_layout));
	zoom_layout = NULL;
	zoom_dialog = NULL;
}
