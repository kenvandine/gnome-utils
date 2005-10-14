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
#include "info.h"
#include "misc.h"

static void
loginfo_repaint (LogviewWindow *window, GtkWidget *label)
{
   char *utf8;
   gchar *info_string, *size, *modified, *start_date, *last_date, *num_lines, *tmp, *size_tmp;
   
   if (!window->curlog)
	   return;
   
   tmp = gnome_vfs_format_file_size_for_display (window->curlog->lstats.size);
   size_tmp = g_strdup_printf (_("<b>Size</b>: %s"), tmp);
   size = LocaleToUTF8 (size_tmp);
   g_free (size_tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf (_("<b>Modified</b>: %s"), ctime (&(window->curlog)->lstats.mtime));
   modified = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("<b>Start Date</b>: %s"), ctime (&(window->curlog)->lstats.startdate));
   start_date = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("<b>Last Date</b>: %s"), ctime (&(window->curlog)->lstats.enddate));
   last_date = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("<b>Number of Lines</b>: %ld"), window->curlog->total_lines);
   num_lines = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   info_string = g_strdup_printf ("%s\n%s%s%s%s", size, modified, start_date, last_date, num_lines);
   g_free (size);
   g_free (modified);
   g_free (start_date);
   g_free (last_date);
   g_free (num_lines);

   gtk_label_set_markup (GTK_LABEL (label), info_string);
   g_free (info_string);
}

static void
loginfo_quit (GtkWidget *widget, gpointer data)
{
   LogviewWindow *window = data;
   gtk_widget_hide (widget);
   window->loginfovisible = FALSE;
}

static void
loginfo_close (GtkWidget *widget, int arg, gpointer data)
{
   loginfo_quit (widget, data);
}

void
loginfo_new (GtkAction *action, GtkWidget *callback_data)
{
	static GtkWidget *label;
	static GtkWidget *info_dialog;
	LogviewWindow *window = LOGVIEW_WINDOW(callback_data);

	if (window->curlog == NULL || window->loginfovisible)
		return;
	
	if (info_dialog == NULL) {
		info_dialog = gtk_dialog_new_with_buttons (_("Properties"),
							  GTK_WINDOW (window),
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
							  NULL);
		
		g_signal_connect (G_OBJECT (info_dialog), "response",
				  G_CALLBACK (loginfo_close),
				  window);
		g_signal_connect (GTK_OBJECT (info_dialog), "destroy",
				  G_CALLBACK (loginfo_quit),
				  window);
		g_signal_connect (GTK_OBJECT (info_dialog), "delete_event",
				  G_CALLBACK (gtk_true),
				  NULL);
		
		gtk_dialog_set_default_response (GTK_DIALOG (info_dialog),
						 GTK_RESPONSE_CLOSE);
		gtk_dialog_set_has_separator (GTK_DIALOG(info_dialog), FALSE);
		gtk_container_set_border_width (GTK_CONTAINER (info_dialog), 10);
		
		label = gtk_label_new (NULL);
		gtk_label_set_selectable (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info_dialog)->vbox), 
				    label, TRUE, TRUE, 0);
		gtk_window_set_resizable (GTK_WINDOW (info_dialog), FALSE);
		
	}

	loginfo_repaint (window, label);
	gtk_widget_show_all (info_dialog);
	window->loginfovisible = TRUE;
}
