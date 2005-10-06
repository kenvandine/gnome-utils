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
#include "logview.h"
#include "logrtns.h"
#include "monitor.h"
#include <libgnomevfs/gnome-vfs-ops.h>

static void mon_format_line (char *buffer, int bufsize, LogLine *line);
static gboolean mon_check_logs (gpointer data);

#define MONITORED_LINES 15

void
monitor_stop (LogviewWindow *window)
{
  Log *log;

	if (!window)
		return;
  log = window->curlog;
  if (!log)
    return;

	if (log->mon_handle != NULL) {
		GtkListStore *list;
		list = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));
		gtk_list_store_clear (list);

		window->monitored = FALSE;
		window->curlog->mon_offset = 0;

    gnome_vfs_monitor_cancel (log->mon_handle);
    log->mon_handle = NULL;
		gnome_vfs_close (window->curlog->mon_file_handle);
		window->curlog->mon_file_handle = NULL;
	}
}

void
monitor_callback (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri,
                  const gchar *info_uri, GnomeVFSMonitorEventType event_type,
                  gpointer data)
{
  GnomeVFSResult result;
  LogviewWindow *window = data;

  if (mon_check_logs (data)==FALSE) {
    Log *log = window->curlog;
    if (log == NULL) 
      return;
    
    result = gnome_vfs_monitor_cancel (log->mon_handle);
    log->mon_handle = NULL;
  }
}

void
monitor_start (LogviewWindow *window)
{
	Log *log = window->curlog;
	g_return_if_fail (log);
  GnomeVFSResult result;
	
  result = gnome_vfs_monitor_add (&(log->mon_handle), log->name,
                         GNOME_VFS_MONITOR_FILE, monitor_callback,
                         window);

  if (result != GNOME_VFS_OK) {
    switch (result == GNOME_VFS_ERROR_NOT_SUPPORTED) {
    case GNOME_VFS_ERROR_NOT_SUPPORTED :
      g_print(_("File monitoring is not supported on this file system.\n"));
    default:
      g_print("This file cannot be monitored. !\n");
    }
    return;
  }

	mon_update_display (window);
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
   gchar **buffer_lines, *marked_line;
   int nlines, nlines_to_show;
   GtkTreeIter iter, parent, *parent_pointer = &(parent);
   GtkListStore *list;
   GtkTreePath *path;
   GtkTreeSelection *selection;
	 GtkTreeModel *model;
   gboolean bold;
   Log *log = window->curlog;

   g_return_if_fail (log);

   /* Read either the whole file to get the last page or only the new lines */
   if (log->mon_offset == 0) {
	   buffer_lines = ReadLastPage (log);
	   bold = FALSE;
   } else {
	   buffer_lines = ReadNewLines (log);
	   bold = TRUE;
   }

   if (buffer_lines!=NULL && buffer_lines[0] != NULL) {
	   LogLine *line;
	   int i;

	   list = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));
	   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(window->mon_list_view));
	   gtk_tree_selection_unselect_all (selection);

	   if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list), &parent) == FALSE)
		   parent_pointer = NULL;

	   line = g_new0 (LogLine, 1);

	   /* Count the lines */
	   for (nlines=0; buffer_lines[nlines]!=NULL; nlines++);
	   nlines_to_show = MIN (MONITORED_LINES, nlines);

	   for (i=nlines; i >= (nlines-nlines_to_show); --i) {
		   if (buffer_lines[i] != NULL) {
			   ParseLine (buffer_lines[i], line, TRUE);
			   if (line->date > 0) {
				   mon_format_line (buffer, sizeof (buffer), line);
				   gtk_list_store_insert_before (list, &iter, parent_pointer);
				   if (bold)
					   marked_line = g_markup_printf_escaped ("<b>%s</b>", buffer);
				   else
					   marked_line = g_markup_printf_escaped ("%s", buffer);
				   gtk_list_store_set (list, &iter, 0, marked_line, -1);
				   parent = iter;
				   g_free (marked_line);
			   }
		   }
	   }
	   g_free (line);
	   g_strfreev (buffer_lines);

	   path = gtk_tree_model_get_path (GTK_TREE_MODEL(list), &iter);
	   gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->mon_list_view), path, NULL,
							FALSE, 0, 0);
	   gtk_tree_path_free (path);

		 /* write the log name in bold as well */
		 path = loglist_find_logname (window, log->name);
		 if (path && bold) {
			 GtkTreeIter iter;
			 gchar *label;
			 model =  gtk_tree_view_get_model (GTK_TREE_VIEW(window->treeview));
			 gtk_tree_model_get_iter (model, &iter, path);
			 label = g_strdup_printf ("<b>%s</b>", log->name);
			 gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, label, -1);
		 }
   }
}

/* ----------------------------------------------------------------------
   NAME:	mon_format_line
   DESCRIPTION:	format the output for a log line.
   ---------------------------------------------------------------------- */

static void
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

static gboolean
mon_check_logs (gpointer data)
{
	LogviewWindow *window = data;

	if (!window || !window->curlog->monitored)
		return FALSE;

	if (WasModified (window->curlog))
		mon_update_display (window);
	return TRUE;
}
