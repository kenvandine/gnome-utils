
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "logview.h"
#include "logrtns.h"
#include "log_repaint.h"
#include "misc.h"
#include "calendar.h"

enum {
   MESSAGE = 0,
   DAY_POINTER,
   LOG_LINE_WEIGHT,
   LOG_LINE_WEIGHT_SET
};

static gboolean busy_cursor = FALSE;

static gboolean
logview_show_busy_cursor (LogviewWindow *logview)
{
  GdkCursor *cursor;
  if (GTK_WIDGET_VISIBLE (logview->view) && logview->curlog->model == NULL) {
    cursor = gdk_cursor_new (GDK_WATCH);
    gdk_window_set_cursor (GTK_WIDGET (logview)->window, cursor);
    gdk_cursor_unref (cursor);
    gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));
    busy_cursor = TRUE;
  }
  return (FALSE);
}  

static gboolean
logview_show_normal_cursor (LogviewWindow *logview)
{
  if (busy_cursor) {
    gdk_window_set_cursor (GTK_WIDGET (logview)->window, NULL);    
    gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));
    busy_cursor = FALSE;
  }
}

void
row_toggled_cb (GtkTreeView *treeview, GtkTreeIter *iter,
                GtkTreePath *path, gpointer user_data)
{
	GtkTreeModel *model;
	Day *day;

	model = gtk_tree_view_get_model (treeview);
	gtk_tree_model_get (model, iter, DAY_POINTER, &day, -1);
	day->expand = gtk_tree_view_row_expanded (treeview, day->path);
}

static int
tree_path_find_row (GtkTreeModel *model, GtkTreePath *path, gboolean has_date)
{
    int row = 0;
    int j;
    int *indices;
    GtkTreeIter iter;
    GtkTreePath *date_path;

    g_assert (GTK_IS_TREE_MODEL (model));
    g_assert (path);
    
    indices = gtk_tree_path_get_indices (path);
    
    if (has_date) {
        for (j = 0; j < indices[0]; j++) {
            date_path = gtk_tree_path_new_from_indices (j, -1);
            gtk_tree_model_get_iter (model, &iter, date_path);
            row += gtk_tree_model_iter_n_children (model, &iter);
        }
        if (gtk_tree_path_get_depth (path) > 1)
            row += indices[1];
        
    } else
        row = indices[0];

    return (row);
}

void
selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
    int selected_first = -1, selected_last = -1;
    LogviewWindow *logview = data;
    GtkTreePath *selected_path;
    GList *selected_paths, *path;
    GtkTreeModel *model;
    gint row;
    Log *log;
	
    g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    log = logview->curlog;
    if (log == NULL)
      return;

    selected_paths = gtk_tree_selection_get_selected_rows (selection, &model);

    if (selected_paths == NULL)
        return;    

    for (path = selected_paths; path != NULL; path = g_list_next (path)) {		            

        selected_path = path->data;
        
        row = tree_path_find_row (model, selected_path, (log->days != NULL));
        
        if (selected_last == -1 || row > selected_last) {
            selected_last = row;
        }
        if (selected_first == -1 || row < selected_first) {
            selected_first = row;
        }
    }
    
    log->selected_range.first = gtk_tree_path_copy (g_list_first (selected_paths)->data);
    log->selected_range.last = gtk_tree_path_copy (g_list_last (selected_paths)->data);
    log->selected_line_first = selected_first;
    log->selected_line_last = selected_last;

    g_list_foreach (selected_paths, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selected_paths);

    if (log->days) {
      GtkTreeIter iter;
      Day *day;
      
      selected_path = gtk_tree_path_copy (log->selected_range.first);
      if (gtk_tree_path_get_depth (selected_path) > 1)
        gtk_tree_path_up (selected_path);
      gtk_tree_model_get_iter (model, &iter, selected_path);
      gtk_tree_model_get (model, &iter, DAY_POINTER, &day, -1);
      calendar_select_date (CALENDAR (logview->calendar), day->date);
      gtk_tree_path_free (selected_path);
    }
}

static void
logview_update_statusbar (LogviewWindow *logview)
{
   char *statusbar_text;
   char *size, *modified, *index;
   Log *log;

   g_assert (LOGVIEW_IS_WINDOW (logview));

   log = logview->curlog;

   if (log == NULL) {
       gtk_statusbar_pop (GTK_STATUSBAR (logview->statusbar), 0);
       return;
   }
   
   /* ctime returned string has "\n\0" causes statusbar display a invalid char */
   modified = ctime (&(log->stats->file_time));
   index = strrchr (modified, '\n');
   if (index && *index != '\0')
     *index = '\0';

   modified = g_strdup_printf (_("last update: %s"), modified);
   size = gnome_vfs_format_file_size_for_display (log->stats->file_size);
   statusbar_text = g_strdup_printf (_("%d lines (%s) - %s"), 
                                     log->total_lines, size, modified);
                                     
   if (statusbar_text) {
       gtk_statusbar_pop (GTK_STATUSBAR(logview->statusbar), 0);
       gtk_statusbar_push (GTK_STATUSBAR(logview->statusbar), 0, statusbar_text);
       g_free (size);
       g_free (modified);
       g_free (statusbar_text);
   }
}

void
logview_update_version_bar (LogviewWindow *logview)
{
	Log *log;
	int i;
	gchar *label;

	log = logview->curlog;
    if (log == NULL) {
        gtk_widget_hide (logview->version_bar);
        return;
    }

	if (log->versions > 0 || log->parent_log != NULL) {
		Log *recent;

		gtk_widget_show_all (logview->version_bar);

		if (log->current_version > 0)
			recent = log->parent_log;
		else
			recent = log;
			
		for (i=5; i>-1; i--)
			gtk_combo_box_remove_text (GTK_COMBO_BOX (logview->version_selector), i);

		gtk_combo_box_append_text (GTK_COMBO_BOX (logview->version_selector), _("Current"));

		for (i=0; i<(recent->versions); i++) {
			label = g_strdup_printf ("Archive %d", i+1);
			gtk_combo_box_append_text (GTK_COMBO_BOX (logview->version_selector),
                                                   label);
			g_free (label);
		}
                
		gtk_combo_box_set_active (GTK_COMBO_BOX (logview->version_selector), 
                                          log->current_version);

	} else {
		gtk_widget_hide (logview->version_bar);
	}
}

static void
model_fill_date_iter (GtkTreeStore *model, GtkTreeIter *iter, Day *day)
{
    gchar *utf8;

    g_assert (GTK_IS_TREE_STORE (model));
    
    utf8 = date_get_string (day->date);
    gtk_tree_store_set (model, iter, MESSAGE, utf8, DAY_POINTER, day, -1);
    g_free (utf8);
}

static gboolean
logview_unbold_rows (Log *log)
{
  LogviewWindow *logview;
  TreePathRange *bold_rows;
  GtkTreeIter iter;
  GtkTreePath *path;

  g_assert (log);
  logview = log->window;

  if (log->bold_rows_list == NULL)
    return FALSE;

  if (logview->curlog != log)
    return TRUE;
  
  bold_rows = g_list_first (log->bold_rows_list)->data;
  g_assert (bold_rows != NULL);

  for (path = bold_rows->first; 
       gtk_tree_path_compare (path, bold_rows->last)<=0;
       gtk_tree_path_next (path)) {
    gtk_tree_model_get_iter (log->model, &iter, path);
    gtk_tree_store_set (GTK_TREE_STORE (log->model), &iter,
                        LOG_LINE_WEIGHT, PANGO_WEIGHT_NORMAL,
                        LOG_LINE_WEIGHT_SET, TRUE, -1);
  }
  
  gtk_tree_path_free (bold_rows->first);
  gtk_tree_path_free (bold_rows->last);
  log->bold_rows_list = g_list_remove (log->bold_rows_list, bold_rows);
  g_free (bold_rows);

  return FALSE;
}

static void
log_add_new_log_lines (Log *log)
{
    GtkTreeIter iter, child_iter, *iter_ptr;
    GtkTreePath *path;
    TreePathRange *bold_rows;
    Day *day;
    int i;
    gchar *line;

    g_assert (log);

    if (log_read_new_lines (log) == FALSE)
      return;

    /* Find the last expandable row */
    if (log->days) {
      day = g_slist_last (log->days)->data;
      gtk_tree_model_get_iter (log->model, &iter, day->path);
      iter_ptr = &iter;
    } else {
      iter_ptr = NULL;
    }  
 
    for (i=log->displayed_lines; i<log->total_lines; i++) {

        line = log->lines[i];

        if (line != NULL) {
            gtk_tree_store_append (GTK_TREE_STORE (log->model), &child_iter, iter_ptr);
            gtk_tree_store_set (GTK_TREE_STORE (log->model), &child_iter,
                                MESSAGE, line, 
                                DAY_POINTER, NULL,
                                LOG_LINE_WEIGHT, PANGO_WEIGHT_BOLD,
                                LOG_LINE_WEIGHT_SET, TRUE, -1);
            /* Remember the first and last bold lines in the model to
               unset them later */
            if (i==log->displayed_lines) {
              bold_rows = g_new0 (TreePathRange, 1);
              bold_rows->first = gtk_tree_model_get_path (log->model, &child_iter);
            }
            if (i == log->total_lines-1) {
              bold_rows->last = gtk_tree_model_get_path (log->model, &child_iter);
            }
        }
    }
    log->displayed_lines = log->total_lines;
    log->bold_rows_list = g_list_append (log->bold_rows_list, bold_rows);

    g_timeout_add (5000, (GSourceFunc) logview_unbold_rows, log);
}
    
static void
logview_scroll_and_focus_path (LogviewWindow *logview, Log *log)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));
    
    if (log == NULL)
      return;

    if (log->selected_range.first != NULL) {
      GtkTreeSelection *selection;
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->view));
      gtk_tree_selection_select_range (selection, log->selected_range.first,
                                       log->selected_range.last);
    }

    if (log->bold_rows_list != NULL) {
      TreePathRange *bold_rows;
      bold_rows = g_list_last (log->bold_rows_list)->data;      
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (logview->view),
                                    bold_rows->last,
                                    NULL, TRUE, 1.0, 1);   
    } else {
      if (log->visible_first != NULL) {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (logview->view),
                                    log->visible_first,
                                    NULL, TRUE, 0.0, 1);   
      } 
    }    
}

static void
log_fill_model_no_date (Log *log, GtkTreeModel *model)
{
    int i;
    gchar *line;
    GtkTreeIter iter;
    
    g_assert (log);

    for (i=log->total_lines-1; i>=0; i--) {
        line = log->lines[i];
        gtk_tree_store_prepend (GTK_TREE_STORE (model), &iter, NULL);
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter,                            
                            MESSAGE, line, DAY_POINTER, NULL, -1);
        if (i == (log->total_lines-1)) {
  	    GtkTreePath *path;
	    path = gtk_tree_model_get_path (model, &iter);
            log->selected_range.first = gtk_tree_path_copy (path);
            log->selected_range.last = gtk_tree_path_copy (path);
	    log->visible_first = gtk_tree_path_copy (path);
	    gtk_tree_path_free (path);
        }
    }
}

static void
log_fill_day_iter (Log *log, GtkTreeModel *model, Day *day, GtkTreeIter *iter)
{
  GtkTreeIter child_iter;
  int i;

  for (i = day->last_line; i >= day->first_line; i--) {
    gtk_tree_store_prepend (GTK_TREE_STORE (model), &child_iter, iter);
    gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,                            
                        MESSAGE, log->lines[i], DAY_POINTER, NULL, -1);
  }
}

static void
log_fill_model_with_date (Log *log, GtkTreeModel *model)
{
    GtkTreeIter iter;
    GSList *days;
    Day *day;
    int i;

    g_assert (log->total_lines > 0);

    /* Cycle on the days in the log */
    /* It's not worth it to prepend instead of append */

    for (days = log->days; days != NULL; days = g_slist_next (days)) {
        day = days->data;
 
        gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
        model_fill_date_iter (GTK_TREE_STORE (model), &iter, day);
        day->path = gtk_tree_model_get_path (model, &iter);
        
        log_fill_day_iter (log, model, day, &iter);
        day->expand = FALSE; 
        
        while (gtk_events_pending ())
          gtk_main_iteration ();
    }

    day->expand = TRUE;
    log->selected_range.first = gtk_tree_path_copy (day->path);
    log->selected_range.last = gtk_tree_path_copy (day->path);
    log->visible_first = gtk_tree_path_copy (day->path);

    log->displayed_lines = log->total_lines;
}

static void
log_create_model (Log *log)
{
  GtkTreeModel *model;

  g_assert (log != NULL);
  model = GTK_TREE_MODEL(gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_POINTER,
                                                  G_TYPE_INT, G_TYPE_BOOLEAN));
  if (log->days != NULL)
    log_fill_model_with_date (log, model);
  else
    log_fill_model_no_date (log, model);
  log->model = model;
}  

static void
logview_set_log_model (LogviewWindow *window, Log *log)
{    
    GSList *days;
    Day *day;

    g_assert (LOGVIEW_IS_WINDOW (window));
    g_assert (log);
    g_assert (GTK_IS_TREE_MODEL (log->model));

    if (log->filter != NULL)
        gtk_tree_view_set_model (GTK_TREE_VIEW (window->view), GTK_TREE_MODEL (log->filter));
    else
        gtk_tree_view_set_model (GTK_TREE_VIEW (window->view), log->model);

    if (log->days != NULL) {
        for (days=log->days; days != NULL; days = g_slist_next(days)) {
            day = days->data;
            if (day->expand)
                gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view),
                                          day->path, FALSE);
        }
    }
}

void
logview_repaint (LogviewWindow *logview)
{
    Log *log;
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    
    log = logview->curlog;

    logview_update_statusbar (logview);
    logview_set_window_title (logview);
      
    if (log == NULL) {
      gtk_tree_view_set_model (GTK_TREE_VIEW (logview->view), NULL);
      return;
    }

    if (log->model == NULL) {
      g_timeout_add (200, (GSourceFunc) logview_show_busy_cursor, logview);
      log_create_model (log);
      logview_show_normal_cursor (logview);
    }      
            
    if (log->needs_refresh) {
      log_add_new_log_lines (log);
      log->needs_refresh = FALSE;
    }

    if (gtk_tree_view_get_model (GTK_TREE_VIEW (logview->view)) != log->model)
      logview_set_log_model (logview, log);

    logview_scroll_and_focus_path (logview, log);
}
