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
#if 0
#include "close.xpm"
#endif

#define INFO_WIDTH            350
#define INFO_HEIGHT           140


void LogInfo (GtkWidget * widget, gpointer user_data);
void CloseLogInfo (GtkWidget * widget, GtkWindow ** window);
int RepaintLogInfo (GtkWidget * widget, GdkEventExpose * event);



/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

int loginfovisible;
GtkWidget *InfoDialog;
GtkWidget *info_canvas;
PangoLayout *stat_layout;

extern ConfigData *cfg;
extern GdkGC *gc;
extern Log *curlog;


/* ----------------------------------------------------------------------
   NAME:          LogInfo
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
LogInfo (GtkWidget * widget, gpointer user_data)
{
   GtkWidget *frame;
   GtkWidget *vbox;
   int h1, h2;
   PangoContext *context;
   PangoFontMetrics *metrics;
   PangoFont *font;

   if (curlog == NULL || loginfovisible)
      return;

   InfoDialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_signal_connect (GTK_OBJECT (InfoDialog), "destroy",
		       (GtkSignalFunc) CloseLogInfo,
		       &InfoDialog);
   gtk_window_set_title (GTK_WINDOW (InfoDialog), _("Log stats"));
   gtk_container_set_border_width (GTK_CONTAINER (InfoDialog), 0);
   gtk_widget_set_style (InfoDialog, cfg->main_style);
   gtk_widget_realize (InfoDialog);

   vbox = gtk_vbox_new (FALSE, 2);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
   gtk_container_add (GTK_CONTAINER (InfoDialog), vbox);
   gtk_widget_show (vbox);

   frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_set_border_width (GTK_CONTAINER (frame), 3);
   gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
   gtk_widget_set_style (frame, cfg->main_style);
   gtk_widget_show (frame);

   info_canvas = gtk_drawing_area_new ();
   gtk_signal_connect (GTK_OBJECT (info_canvas), "expose_event",
		       (GtkSignalFunc) RepaintLogInfo, NULL);

   stat_layout = gtk_widget_create_pango_layout (widget, "");
   context = gdk_pango_context_get ();
   pango_context_set_language (context, gtk_get_default_language ());
   font = LoadFont (context, cfg->headingb);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   h1 = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics)) +
	PANGO_PIXELS (pango_font_metrics_get_descent (metrics));
   font = LoadFont (context, cfg->fixedb);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   h2 = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics)) +
	PANGO_PIXELS (pango_font_metrics_get_descent (metrics)) + 2;
   GTK_WIDGET (info_canvas)->requisition.width = INFO_WIDTH;
   GTK_WIDGET (info_canvas)->requisition.height = h1*2+8*h2;
   gtk_widget_queue_resize (GTK_WIDGET (info_canvas));
   gtk_widget_set_events (info_canvas, GDK_EXPOSURE_MASK);
   gtk_widget_set_style (info_canvas, cfg->white_bg_style);
   gtk_container_add (GTK_CONTAINER (frame), info_canvas);
   gtk_widget_show (info_canvas);
   gtk_widget_realize (info_canvas);

   gtk_widget_show (InfoDialog);

   g_object_unref (G_OBJECT (context));
   pango_font_metrics_unref (metrics);
   g_object_unref (G_OBJECT (font));
   loginfovisible = TRUE;
}

static int
get_max_len (void)
{
	int len = 5;
	int i;
	char *strings[] = {
		"Log:",
		"Size:",
		"Modified:",
		"Start date:",
		"Last date:",
		"Num. lines:",
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
   NAME:          RepaintLogInfo
   DESCRIPTION:   Repaint the log info window.
   ---------------------------------------------------------------------- */

int
RepaintLogInfo (GtkWidget * widget, GdkEventExpose * event)
{
   static GdkDrawable *canvas;
   char buffer[1024];
   char *utf8;
   int x, y, h, w;
   int ah, dh, afdb, dfdb, df, af;
   int win_width, win_height;
   PangoRectangle logical_rect;
   PangoContext *context;
   PangoFontMetrics *metrics;
   PangoFont *font;
   int max_len;

   canvas = info_canvas->window;
   win_width = info_canvas->allocation.width;
   win_height = info_canvas->allocation.height;

   context = gdk_pango_context_get ();
   pango_context_set_language (context, gtk_get_default_language ());
   font = LoadFont (context, cfg->headingb);
   metrics = pango_font_get_metrics
             (font, pango_context_get_language (context));

   /* Draw title */
   ah = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));
   dh = PANGO_PIXELS (pango_font_metrics_get_descent (metrics));
   h = dh + ah;

   /* Note: we use a real translated string that will appear below to gather
    * the length, that way we'll get semi correct results with a lot more
    * fonts */
   pango_layout_set_font_description (stat_layout, cfg->fixedb);
   pango_layout_set_text (stat_layout, _("Date:"), -1);
   pango_layout_get_pixel_extents (stat_layout, NULL, &logical_rect);
   w = logical_rect.width / g_utf8_strlen (_("Date:"), 1024);
   max_len = get_max_len ();

   x = 5;
   y = ah + 6;
   gdk_gc_set_foreground (gc, &cfg->blue);
   gdk_draw_rectangle (canvas, gc, TRUE, 3, 3, win_width - 6, h+6);
   gdk_gc_set_foreground (gc, &cfg->white);
   pango_layout_set_font_description (stat_layout, cfg->headingb);
   pango_layout_set_text (stat_layout, _("Log information"), -1);
   gdk_draw_layout (canvas, gc, x+3, y-12, stat_layout);
   font = LoadFont (context, cfg->fixedb);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   afdb = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));
   y += 9 + dh + afdb;
   dfdb = PANGO_PIXELS (pango_font_metrics_get_descent (metrics)); 

   /* Draw rectangle */
   h = dfdb + afdb+2;
   gdk_gc_set_foreground (gc, &cfg->blue3);
   font = LoadFont (context, cfg->fixed);
   metrics = pango_font_get_metrics
	     (font, pango_context_get_language (context));
   af = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));
   df = PANGO_PIXELS (pango_font_metrics_get_descent (metrics));
   gdk_draw_rectangle (canvas, gc, TRUE, max_len*w+6, 
		       y - af-3, 
		       win_width - 9 - max_len*w, 
		       win_height - (y - af));
   gdk_gc_set_foreground (gc, &cfg->gray75);
   gdk_draw_rectangle (canvas, gc, TRUE, 3, y-af-3, 
		       max_len*w, win_height - (y - af));

   /* Draw Info */
   gdk_gc_set_foreground (gc, &cfg->black);
   pango_layout_set_font_description (stat_layout, cfg->fixedb);
   pango_layout_set_text (stat_layout, _("Log:"), -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   pango_layout_set_text (stat_layout, _("Size:"), -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   pango_layout_set_text (stat_layout, _("Modified:"), -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   pango_layout_set_text (stat_layout, _("Start date:"), -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   pango_layout_set_text (stat_layout, _("Last date:"), -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   pango_layout_set_text (stat_layout, _("Num. lines:"), -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);

   /* Check that there is at least one log */
   if (curlog == NULL)
      return -1;

   h = dh + ah;
   y = ah + 6;
   y += 9 + dh + afdb;
   x = max_len*w + 6 + 2;
   g_snprintf (buffer, sizeof (buffer), "%s", curlog->name);
   h = dfdb + afdb+2;
   pango_layout_set_font_description (stat_layout, cfg->fixed);
   pango_layout_set_text (stat_layout, buffer, -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   g_snprintf (buffer, sizeof (buffer),
	      _("%ld bytes"), (long) curlog->lstats.size);
   pango_layout_set_text (stat_layout, buffer, -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   g_snprintf (buffer, sizeof (buffer), "%s ", ctime (&curlog->lstats.mtime));
   utf8 = LocaleToUTF8 (buffer);
   pango_layout_set_text (stat_layout, utf8, -1);
   g_free (utf8);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   g_snprintf (buffer, sizeof (buffer), "%s ",
	      ctime (&curlog->lstats.startdate));
   utf8 = LocaleToUTF8 (buffer);
   pango_layout_set_text (stat_layout, utf8, -1);
   g_free (utf8);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   g_snprintf (buffer, sizeof (buffer), "%s ", ctime (&curlog->lstats.enddate));
   utf8 = LocaleToUTF8 (buffer);
   pango_layout_set_text (stat_layout, utf8, -1);
   g_free (utf8);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);
   y += h;
   g_snprintf (buffer, sizeof (buffer), "%ld ", curlog->lstats.numlines);
   pango_layout_set_text (stat_layout, buffer, -1);
   gdk_draw_layout (canvas, gc, x, y, stat_layout);

   pango_font_metrics_unref (metrics);
   g_object_unref (G_OBJECT (context));
   g_object_unref (G_OBJECT (font));
   return TRUE;
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogInfo
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
CloseLogInfo (GtkWidget * widget, GtkWindow ** window)
{
   if (loginfovisible)
      gtk_widget_hide (InfoDialog);
   InfoDialog = NULL;
   loginfovisible = FALSE;

   if (stat_layout != NULL)
      g_object_unref (G_OBJECT (stat_layout));
   stat_layout = NULL;
}

