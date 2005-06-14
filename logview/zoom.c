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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "logview.h"
#include "zoom.h"
#include "misc.h"
#include "desc_db.h"

/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

extern char *month[12];
extern GList *regexp_db, *descript_db;

void
close_zoom_view (LogviewWindow *window)
{
   if (window->zoom_visible) {
	   GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/ViewMenu/ShowDetails");
	   gtk_action_activate (action);
   }
}

static void
zoom_close_button (GtkWidget *zoom_dialog, int arg, gpointer data)
{
	close_zoom_view (data);
}

static void
quit_zoom_view (GtkWidget *zoom_dialog, gpointer data)
{
	close_zoom_view (data);
}

/* ----------------------------------------------------------------------
   NAME:          create_zoom_view
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
create_zoom_view (LogviewWindow *window)
{
   if (window->curlog == NULL || window->zoom_visible)
      return;

   if (window->zoom_dialog == NULL) {
	GtkWidget *zoom_dialog;
   	zoom_dialog  = gtk_dialog_new_with_buttons (_("Entry Detail"), 
					       	    GTK_WINDOW_TOPLEVEL, 
					            GTK_DIALOG_DESTROY_WITH_PARENT,
					            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, 
					            NULL);
        g_signal_connect (G_OBJECT (zoom_dialog), "response",
		          G_CALLBACK (zoom_close_button),
			  window);
   	g_signal_connect (G_OBJECT (zoom_dialog), "destroy",
		          G_CALLBACK (quit_zoom_view),
			  window);
        g_signal_connect (G_OBJECT (zoom_dialog), "delete_event",
		          G_CALLBACK (gtk_true),
			  NULL);
	gtk_dialog_set_has_separator (GTK_DIALOG (zoom_dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (zoom_dialog), FALSE);
	gtk_dialog_set_default_response (GTK_DIALOG (zoom_dialog), GTK_RESPONSE_CLOSE);
   	gtk_container_set_border_width (GTK_CONTAINER (zoom_dialog), 5);
	window->zoom_dialog = zoom_dialog;
   }

   repaint_zoom(window);
   gtk_window_present (GTK_WINDOW (window->zoom_dialog));
   window->zoom_visible = TRUE; 
}

/* ----------------------------------------------------------------------
   NAME:          repaint_zoom
   DESCRIPTION:   Repaint the zoom window.
   ---------------------------------------------------------------------- */

int
repaint_zoom (LogviewWindow *window)
{
   LogLine *line;
   struct tm date = {0};
   char buffer[1024];
   GtkWidget *zoom_dialog = window->zoom_dialog;
   gchar *date_string, *description=NULL, *label_text;

   g_return_val_if_fail (window->curlog, FALSE);
   
   if (window->zoom_label == NULL) {
	   window->zoom_label = gtk_label_new (NULL);
	   gtk_label_set_selectable (GTK_LABEL (window->zoom_label), TRUE);
	   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window->zoom_dialog)->vbox),
			       window->zoom_label, TRUE, TRUE, 0);
	   gtk_widget_show (window->zoom_label);
   }

   if (window->curlog->current_line_no <= 0 || 
       ((line = (window->curlog->lines)[window->curlog->current_line_no]) == NULL)) {
	   return FALSE;
   }

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
           date_string = g_strdup_printf ("%s %d %02d:%02d:%02d",
					  _(month[(int)line->month]),
					  (int) line->date,
					  (int) line->hour,
					  (int) line->min,
					  (int) line->sec);
   } else
           date_string = LocaleToUTF8 (buffer);
   
   if (match_line_in_db (line, regexp_db))
	   if (find_tag_in_db (line, descript_db))
		   description = LocaleToUTF8 (line->description->description);
   if (description == NULL)
	   description = g_strdup_printf("");
   
   label_text = g_strdup_printf ("<b>Date</b> : %s\n"
				 "<b>Process</b> : %s\n"
				 "<b>Message</b> : %s\n"
				 "<b>Description</b> : %s\n",
				 date_string, LocaleToUTF8 (line->process),
				 LocaleToUTF8 (line->message),
				 description);

   gtk_label_set_markup (GTK_LABEL(window->zoom_label), label_text);

   g_free (date_string);
   g_free (description);
   g_free (label_text);

   return TRUE;
}
