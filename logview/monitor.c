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
#include <time.h>
#include "logview.h"
#include "logrtns.h"
#include "monitor.h"
#include "misc.h"

void
monitor_stop (LogviewWindow *window)
{
	if (window->timer_tag > 0) {
		window->timer_tag = 0;
		window->monitored = FALSE;
	}
}
	
GtkWidget *monitor_create_widget (LogviewWindow *window)
{
	GtkWidget *clist_view, *swin;
	GtkListStore *clist;
	GtkCellRenderer *clist_cellrenderer;
	GtkTreeViewColumn *clist_column;
	GtkTreeIter newiter;
   
	/* Create destination list */
	clist = gtk_list_store_new (1, G_TYPE_STRING); 
	clist_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (clist));
	clist_cellrenderer = gtk_cell_renderer_text_new ();
	clist_column = gtk_tree_view_column_new_with_attributes
		(NULL, clist_cellrenderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (clist_view), clist_column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (clist_view), FALSE);
	swin = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (clist_view));
	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN
					      (clist_column), 300); 
	gtk_tree_selection_set_mode
		( (GtkTreeSelection *)gtk_tree_view_get_selection
		  (GTK_TREE_VIEW (clist_view)),
		  GTK_SELECTION_BROWSE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	
	gtk_tree_view_column_set_alignment (GTK_TREE_VIEW_COLUMN (clist_column),
					    0.0);
	gtk_widget_show_all (swin);

	window->mon_list_view = (GtkWidget *) clist_view;

	if (window->curlog)
		mon_read_last_page (window);

	return swin;
}

/* ----------------------------------------------------------------------
   NAME:	go_monitor_log
   DESCRIPTION:	Start monitoring the logs.
   ---------------------------------------------------------------------- */

void
go_monitor_log (LogviewWindow *window)
{
	g_return_if_fail (window->curlog);
	/* Setup timer to check log */
	/* the timeout is automatically stopped when mon_check_logs returns false. */
	window->timer_tag = g_timeout_add (1000, mon_check_logs, window);
}

/* ----------------------------------------------------------------------
   NAME:	mon_read_last_page
   DESCRIPTION:	read last lines of the log into the monitor window.
   ---------------------------------------------------------------------- */

void
mon_read_last_page (LogviewWindow *window)
{
   char buffer[1024];
   int i;
   GtkTreeIter iter;
   GtkListStore *list;
   GtkTreePath *path;
   gint num_of_lines_to_display;
   Log *log = window->curlog;

   g_return_if_fail (log);
   g_return_if_fail (window->mon_list_view);
 
   log->mon_numlines = 0;
   
   if (!log->total_lines)
       return; 

   if (log->total_lines < 5)
       num_of_lines_to_display = log->total_lines;
   else
       num_of_lines_to_display = 5;

   for (i = num_of_lines_to_display; i; --i) {
       mon_format_line (buffer, sizeof(buffer), 
                        (log->lines)[log->total_lines - i]);
       list = (GtkListStore *)
               gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));
       gtk_list_store_append (list, &iter);
       gtk_list_store_set (list, &iter, 0, buffer, -1);
       log->mon_numlines++;
   }
   path = gtk_tree_model_get_path (GTK_TREE_MODEL (list), &iter);
   gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->mon_list_view),
       path, NULL, TRUE, 0, 0);
   gtk_tree_path_free (path);
 
}

/* ----------------------------------------------------------------------
   NAME:	mon_read_new_lines
   DESCRIPTION:	Read last lines in the log file and add to the monitor
   		window.
   ---------------------------------------------------------------------- */

static void
mon_read_new_lines (LogviewWindow *window)
{
   char buffer[1024];
   int i;
   gint lines_read = 0;
   LogLine **lines;
   GtkTreeIter iter;
   GtkTreePath *path;
   GtkListStore *list;
   Log *log = window->curlog;

   g_return_if_fail (log);

   fseek (log->fp, log->offset_end, SEEK_SET);

   list = (GtkListStore *)
   gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));

   /* Read new lines */
   lines_read = ReadPageDown (log, &lines, FALSE);

   for (i = 0;  i < lines_read; ++i ) {
	   mon_format_line (buffer, sizeof (buffer), lines[i]);
	   gtk_list_store_insert_before (list, &iter, NULL);
	   gtk_list_store_set (list, &iter, 0, buffer, -1);
	   ++log->mon_numlines; 
   } 
   if (lines_read) {
       path = gtk_tree_model_get_path (GTK_TREE_MODEL (list), &iter);
       gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->mon_list_view),
           path, NULL, TRUE, 0, 0);
       gtk_tree_path_free (path);
   }

}

/* ----------------------------------------------------------------------
   NAME:	mon_format_line
   DESCRIPTION:	format the output for a log line.
   ---------------------------------------------------------------------- */

void
mon_format_line (char *buffer, int bufsize, LogLine *line)
{
    if (line == NULL)
        return;

	g_snprintf (buffer, bufsize, "%s/%s  %s:%s:%s %s %s", 
		        LocaleToUTF8 (g_strdup_printf ("%2d", line->date)),
                LocaleToUTF8 (g_strdup_printf ("%2d", line->month+1)),
		        LocaleToUTF8 (g_strdup_printf ("%2d", line->hour)),
                LocaleToUTF8 (g_strdup_printf ("%02d", line->min)), 
                LocaleToUTF8 (g_strdup_printf ("%02d", line->sec)),
		        LocaleToUTF8 (line->process),
                LocaleToUTF8 (line->message));

}

/* ----------------------------------------------------------------------
   NAME:	mon_check_logs
   DESCRIPTION:	Routinly called to check wheter the logs have changed.
   ---------------------------------------------------------------------- */

gboolean
mon_check_logs (gpointer data)
{
	LogviewWindow *window = data;
	g_return_if_fail (window->curlog);

	if (!window->monitored)
		return FALSE;

	if (WasModified (window->curlog))
		mon_read_new_lines (window);
	return TRUE;
}
