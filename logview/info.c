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


#include <glib/gi18n.h>
#include "logview.h"
#include "info.h"
#include "misc.h"

void RepaintLogInfo (LogviewWindow *window);

/* ----------------------------------------------------------------------
   NAME:          LogInfo
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
LogInfo (GtkAction *action, GtkWidget *callback_data)
{
	GtkWidget *frame;
	LogviewWindow *window = LOGVIEW_WINDOW(callback_data);

	if (window->curlog == NULL || window->loginfovisible)
		return;
	
	if (window->info_dialog == NULL) {
		GtkWidget *InfoDialog;
		InfoDialog = gtk_dialog_new_with_buttons (_("Properties"),
							  GTK_WINDOW (window),
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
							  NULL);
		
		g_signal_connect (G_OBJECT (InfoDialog), "response",
				  G_CALLBACK (CloseLogInfo),
				  window);
		g_signal_connect (GTK_OBJECT (InfoDialog), "destroy",
				  G_CALLBACK (QuitLogInfo),
				  window);
		g_signal_connect (GTK_OBJECT (InfoDialog), "delete_event",
				  G_CALLBACK (gtk_true),
				  NULL);
		
		gtk_dialog_set_default_response (GTK_DIALOG (InfoDialog),
						 GTK_RESPONSE_CLOSE);
		gtk_dialog_set_has_separator (GTK_DIALOG(InfoDialog), FALSE);
		gtk_container_set_border_width (GTK_CONTAINER (InfoDialog), 5);
		
		window->info_dialog = InfoDialog;

		frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
		gtk_container_set_border_width (GTK_CONTAINER(frame), 5);

		window->info_text_view = gtk_text_view_new_with_buffer (window->info_buffer);
		gtk_container_add (GTK_CONTAINER (frame), window->info_text_view);

		gtk_text_view_set_editable (GTK_TEXT_VIEW (window->info_text_view), FALSE);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (InfoDialog)->vbox), 
				    frame, TRUE, TRUE, 0);
		
		RepaintLogInfo (window);
	}

	gtk_widget_show_all (window->info_text_view);
	
	gtk_widget_show_all (window->info_dialog);
	window->loginfovisible = TRUE;
}

/* ----------------------------------------------------------------------
   NAME:          RepaintLogInfo
   DESCRIPTION:   Repaint the log info window.
   ---------------------------------------------------------------------- */

void
RepaintLogInfo (LogviewWindow *window)
{
   char *utf8;
   gchar *info_string, *size, *modified, *start_date, *last_date, *num_lines, *tmp;
   
   if (!window->curlog)
	   return;
   
   tmp = g_strdup_printf(_("Size: %ld bytes"), (long) window->curlog->lstats.size);
   size = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf (_("Modified: %s"), ctime (&(window->curlog)->lstats.mtime));
   modified = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("Start Date: %s"), ctime (&(window->curlog)->lstats.startdate));
   start_date = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("Last Date: %s"), ctime (&(window->curlog)->lstats.enddate));
   last_date = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   tmp = g_strdup_printf(_("Number of Lines: %ld)"), window->curlog->lstats.numlines);
   num_lines = LocaleToUTF8 (tmp);
   g_free (tmp);
   
   info_string = g_strdup_printf ("%s\n%s%s%s%s", size, modified, start_date, last_date, num_lines);
   
   g_free (size);
   g_free (modified);
   g_free (start_date);
   g_free (last_date);
   g_free (num_lines);
   
   if (window->info_buffer == NULL)
	   window->info_buffer = gtk_text_buffer_new (NULL);

   gtk_text_buffer_set_text (window->info_buffer, info_string, -1);
   g_free (info_string);

   gtk_text_view_set_buffer (GTK_TEXT_VIEW(window->info_text_view), window->info_buffer);

}

/* ----------------------------------------------------------------------
   NAME:          CloseLogInfo
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
CloseLogInfo (GtkWidget *widget, int arg, gpointer data)
{
   LogviewWindow *window = data;
   if (window->loginfovisible)
      gtk_widget_hide (widget);
   window->loginfovisible = FALSE;
}

void QuitLogInfo (GtkWidget *widget, gpointer data)
{
   LogviewWindow *window = data;
   gtk_widget_destroy (GTK_WIDGET (window->info_dialog));
   window->info_dialog = NULL;
   window->info_buffer = NULL;
   window->info_text_view = NULL;
}

