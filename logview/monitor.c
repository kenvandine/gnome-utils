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
#include "logview.h"
#include "logrtns.h"
#include "monitor.h"

void
monitor_stop (LogviewWindow *window)
{
	if (!window)
		return;
	if (window->timer_tag > 0) {
		GtkListStore *list;
		list = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));
		gtk_list_store_clear (list);

		window->timer_tag = 0;
		window->monitored = FALSE;
		window->curlog->mon_offset = 0;
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
		  GTK_SELECTION_MULTIPLE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	
	gtk_tree_view_column_set_alignment (GTK_TREE_VIEW_COLUMN (clist_column),
					    0.0);
	gtk_widget_show_all (swin);

	window->mon_list_view = (GtkWidget *) clist_view;

	if (window->curlog)
		mon_update_display (window);

	return swin;
}

/* ----------------------------------------------------------------------
   NAME:	go_monitor_log
   DESCRIPTION:	Start monitoring the logs.
   ---------------------------------------------------------------------- */

void
go_monitor_log (LogviewWindow *window)
{
	GnomeVFSResult result;
	Log *log = window->curlog;

	g_return_if_fail (log);
	
	/* Setup timer to check log */
	/* the timeout is automatically stopped when mon_check_logs returns false. */
	window->timer_tag = g_timeout_add (1000, mon_check_logs, window);

	/* Fixme : This doesnt work yet */
	/*
	  gnome_vfs_monitor_add (&(log->mon_handle), log->name, GNOME_VFS_MONITOR_FILE,
			       (GnomeVFSMonitorCallback) mon_check_logs, window);
	*/
}


/* ----------------------------------------------------------------------
   NAME:	mon_update_display
   DESCRIPTION:	Update the monitor display by reading the last page of the log or
   newly-added lines.
   ---------------------------------------------------------------------- */

void
mon_update_display (LogviewWindow *window)
{
   char buffer[1024];
   gchar **buffer_lines;
   int nlines, nlines_to_show;
   GtkTreeIter iter, parent, *parent_pointer = &(parent);
   GtkListStore *list;
   GtkTreePath *path;
   GtkTreeSelection *selection;
   Log *log = window->curlog;

   g_return_if_fail (log);
      
   /* Read either the whole file to get the last page or only the new lines */
   if (log->mon_offset == 0)
	   buffer_lines = ReadLastPage (log);
   else
	   buffer_lines = ReadNewLines (log);

   if (buffer_lines[0] != NULL) {
	   LogLine *line;
	   int i;

	   list = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));
	   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(window->mon_list_view));
	   if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list), &parent) == FALSE)
		   parent_pointer = NULL;
	   gtk_tree_selection_unselect_all (selection);

	   line = g_new0 (LogLine, 1);

	   /* Count the lines */
	   for (nlines=0; buffer_lines[nlines]!=NULL; nlines++);
	   nlines_to_show = MIN (MONITORED_LINES, nlines);

	   for (i=nlines; i >= (nlines-nlines_to_show); --i) {
		   if (buffer_lines[i] != NULL) {
			   ParseLine (buffer_lines[i], line);
			   if (line->date > 0) {
				   mon_format_line (buffer, sizeof (buffer), line);
				   gtk_list_store_insert_before (list, &iter, parent_pointer);
				   gtk_list_store_set (list, &iter, 0, buffer, -1);
				   parent = iter;
				   gtk_tree_selection_select_iter (selection, &iter);
			   }
		   }
	   }
	   g_free (line);
	   g_strfreev (buffer_lines);

	   path = gtk_tree_model_get_path (GTK_TREE_MODEL(list), &iter);
	   gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->mon_list_view), path, NULL,
							FALSE, 0, 0);
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

/*void
mon_check_logs (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri, 
		const gchar *info_uri, GnomeVFSMonitorEventType event_type, 
		gpointer data)*/
gboolean mon_check_logs (gpointer data)
{
	LogviewWindow *window = data;

	if (!window || !window->monitored)
		return FALSE;

	if (WasModified (window->curlog))
		mon_update_display (window);
	return TRUE;
}
