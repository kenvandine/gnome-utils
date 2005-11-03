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

#define MAX_DAY_STRING 512

static GtkWidget *info_label = NULL;
static GtkWidget *info_dialog = NULL;

static void
loginfo_repaint (LogviewWindow *window)
{
   gchar *info_string, *size, *modified, *start_date, *last_date, *num_lines, *tmp, *size_tmp;
   gchar day_string[MAX_DAY_STRING];
   char *utf8;
   Day *day;
   Log *log;
   
   g_assert (LOGVIEW_IS_WINDOW (window));

   log = window->curlog;
   if (log==NULL)
	   return;
   
   tmp = gnome_vfs_format_file_size_for_display (log->lstats->size);
   size_tmp = g_strdup_printf (_("<b>Size</b>: %s"), tmp);
   size = locale_to_utf8 (size_tmp);
   g_free (size_tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf (_("<b>Modified</b>: %s"), ctime (&(log->lstats->mtime)));
   modified = locale_to_utf8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("<b>Number of Lines</b>: %ld"), log->total_lines);
   num_lines = locale_to_utf8 (tmp);
   g_free (tmp);
   
   if (log->days) {
       day = g_list_first (log->days)->data;
       g_date_strftime (day_string, MAX_DAY_STRING, ("%x"), day->date);
       tmp = g_strdup_printf(_("<b>Start Date</b>: %s"), day_string);
       start_date = locale_to_utf8 (tmp);
       g_free (tmp);

       day = g_list_last (log->days)->data;
       g_date_strftime (day_string, MAX_DAY_STRING, ("%x"), day->date);
       tmp = g_strdup_printf(_("<b>Last Date</b>: %s"), day_string);
       last_date = locale_to_utf8 (tmp);
       g_free (tmp);

       info_string = g_strdup_printf ("%s\n%s%s\n%s\n%s", size, modified, start_date, last_date, num_lines);
   } else {
       start_date = NULL;
       last_date = NULL;
       info_string = g_strdup_printf ("%s\n%s%s", size, modified, num_lines);
   }
   
   g_free (size);
   g_free (modified);
   g_free (start_date);
   g_free (last_date);
   g_free (num_lines);

   gtk_label_set_markup (GTK_LABEL (info_label), info_string);
   g_free (info_string);
}

static void
loginfo_quit (GtkWidget *widget, gpointer data)
{
   gtk_widget_hide (widget);
}

static void
loginfo_close (GtkWidget *widget, int arg, gpointer data)
{
    gtk_widget_hide (widget);
}

static GtkWidget *
loginfo_new (GtkWidget *window)
{
    GtkWidget *dialog;

    dialog = gtk_dialog_new_with_buttons (_("Properties"),
                                          GTK_WINDOW (window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                          NULL);
    
    g_signal_connect (G_OBJECT (dialog), "response",
                      G_CALLBACK (loginfo_close),
                      window);
    g_signal_connect (GTK_OBJECT (dialog), "destroy",
                      G_CALLBACK (loginfo_quit),
                      window);
    g_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      G_CALLBACK (gtk_true),
                      NULL);
    
    gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_CLOSE);
    gtk_dialog_set_has_separator (GTK_DIALOG(dialog), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    info_label = gtk_label_new (NULL);
    gtk_label_set_selectable (GTK_LABEL (info_label), TRUE);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
                        info_label, TRUE, TRUE, 0);
		
    return dialog;
}

void
loginfo_show (GtkAction *action, GtkWidget *widget)
{
	LogviewWindow *window = LOGVIEW_WINDOW(widget);

    g_print("in loginfo_show\n");

	if (window->curlog == NULL)
		return;
	
	if (info_dialog == NULL)
        info_dialog = loginfo_new (widget);

	loginfo_repaint (window);
	gtk_widget_show_all (info_dialog);
}
