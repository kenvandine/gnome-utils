
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
#include "logrtns.h"
#include "log_repaint.h"
#include "misc.h"

enum {
   DATE = 0,
   HOSTNAME,
   PROCESS,
   MESSAGE,
   DAY_POINTER
};

void
row_toggled_cb (GtkTreeView *treeview, GtkTreeIter *iter,
                GtkTreePath *path, gpointer user_data)
{
	LogviewWindow *logview = user_data;
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

    g_return_val_if_fail (GTK_IS_TREE_MODEL (model), 0);
    g_return_val_if_fail (path, 0);
    
    indices = gtk_tree_path_get_indices (path);
    
    if (has_date) {
        for (j = 0; j < indices[0]; j++) {
            date_path = gtk_tree_path_new_from_indices (j, -1);
            gtk_tree_model_get_iter (model, &iter, date_path);
            row += gtk_tree_model_iter_n_children (model, &iter) - 1;
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
    g_return_if_fail (logview->curlog);

    log = logview->curlog;
	selected_paths = gtk_tree_selection_get_selected_rows (selection, &model);

    if (selected_paths == NULL)
        return;    

    for (path = selected_paths; path != NULL; path = g_list_next (path)) {		    
        
        selected_path = path->data;
        
        row = tree_path_find_row (model, selected_path, (log->days != NULL));
        
        if (selected_last == -1 || row > selected_last)
            selected_last = row;
        if (selected_first == -1 || row < selected_first)
            selected_first = row;
        
        if (log->caldata) {
            GtkTreeIter iter;
            Day *day;

            if (gtk_tree_path_get_depth (selected_path) > 1)
                gtk_tree_path_up (selected_path);
            gtk_tree_model_get_iter (model, &iter, selected_path);
            gtk_tree_model_get (model, &iter, DAY_POINTER, &day, -1);
            log->caldata->first_pass = TRUE;
            gtk_calendar_select_day (GTK_CALENDAR (logview->calendar), g_date_get_day(day->date));
        }
    }
    
    log->current_path = gtk_tree_path_copy (selected_path);
    log->selected_line_first = selected_first;
    log->selected_line_last = selected_last;
    g_list_foreach (selected_paths, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selected_paths);
}

static void
logview_update_statusbar (LogviewWindow *window)
{
   char *statusbar_text;
   char *size_string;

   g_return_if_fail (LOGVIEW_IS_WINDOW (window));

   if (window->curlog == NULL) { 
       gtk_statusbar_pop (GTK_STATUSBAR (window->statusbar), 0);
       return;
   }

   size_string = gnome_vfs_format_file_size_for_display (window->curlog->lstats->size);
   statusbar_text = g_strdup_printf (_("%d lines (%s)"), 
                                     window->curlog->total_lines, size_string);

   if (statusbar_text) {
       gtk_statusbar_pop (GTK_STATUSBAR(window->statusbar), 0);
       gtk_statusbar_push (GTK_STATUSBAR(window->statusbar), 0, statusbar_text);
       g_free (size_string);
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

	if (log->versions > 0 || log->parent_log != NULL) {
		Log *recent;

		gtk_widget_show (logview->version_bar);

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
tree_view_columns_set_visible (GtkWidget *view, gboolean visible)
{
	int i;
	GtkTreeViewColumn *column;
	for (i=0; i<3; i++) {
		column = gtk_tree_view_get_column (GTK_TREE_VIEW(view), i);
		gtk_tree_view_column_set_visible (column, visible);
	}
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), visible);
}

static void
model_fill_date_iter (GtkTreeStore *model, GtkTreeIter *iter, LogLine *line, Day *day)
{
    gchar *utf8;
    
    utf8 = logline_get_date_string (line);
    gtk_tree_store_set (model, iter, DATE, utf8, DAY_POINTER, day, -1);
    g_free (utf8);
}

#define MAX_TIME_STRING 32

static void
model_fill_iter (GtkTreeStore *model, GtkTreeIter *iter, LogLine *line)
{
    struct tm date = {0};
    /* cached variables */
    static int hour = 0, min = 0, sec = 0;
    static char *date_utf8 = NULL;
    char *hostname_utf8 = NULL, *process_utf8 = NULL, *message_utf8;
    char tmp[MAX_TIME_STRING];

    message_utf8 = LocaleToUTF8 (line->message);
    
    if (line->hour >= 0 && line->min >= 0 && line->sec >= 0) {
        /* Calling strftime is very time-consuming, so cache date_utf8 */
        /* The last date_utf8 will never be freed, but it's a trade-off */
        if (line->hour != hour || line->min != min || line->sec != sec) {
            g_free (date_utf8);
            
            hour = line->hour;
            min = line->min;
            sec = line->sec;

            date.tm_hour = line->hour;
            date.tm_min = line->min;
            date.tm_sec = line->sec;
            date.tm_isdst = 0;
            
            /* Translators: should be only the time, date could be bogus */
            if (strftime (tmp, MAX_TIME_STRING, _("%X"), &date) <= 0) {
                /* as a backup print in 24 hours style */
                date_utf8 = g_strdup_printf ("%02d:%02d:%02d", line->hour, 
                                             line->min, line->sec);
            } else {
                date_utf8 = LocaleToUTF8 (tmp);
            }
        }
        
        hostname_utf8 = LocaleToUTF8 (line->hostname);
        process_utf8 = LocaleToUTF8 (line->process);
        gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                            DATE, date_utf8, HOSTNAME, hostname_utf8,
                            PROCESS, process_utf8, MESSAGE, message_utf8, -1);        
    } else {
        gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                            MESSAGE, message_utf8, -1);        
    }			 
    
    g_free (message_utf8);
    g_free (hostname_utf8);
    g_free (process_utf8);
}

static void
logview_add_new_log_lines (LogviewWindow *window, Log *log)
{
    GtkTreeIter iter, child_iter;
    GtkTreePath *path;
    Day *day;
    int i;
    LogLine *line;

    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
    
    /* Find the last expandable row */
    day = g_list_last (log->days)->data;
    gtk_tree_model_get_iter (log->model, &iter, day->path);
                
    for (i=log->displayed_lines; i<log->total_lines; i++) {
        line = (log->lines)[i];
        gtk_tree_store_append (GTK_TREE_STORE (log->model), &child_iter, &iter);
        model_fill_iter (GTK_TREE_STORE (log->model), &child_iter, line);
    }
    log->displayed_lines = log->total_lines;

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (log->model), &child_iter);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view), path, NULL, TRUE, 1, 1);
}
    
static void
logview_show_model (LogviewWindow *window, Log *log)
{    
    GList *days;
    Day *day;

    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
    g_return_if_fail (GTK_IS_TREE_MODEL (log->model));

    gtk_tree_view_set_model (GTK_TREE_VIEW (window->view), log->model);
    if (log->days != NULL) {
        for (days=log->days; days != NULL; days = g_list_next(days)) {
            day = days->data;
            if (day->expand)
                gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view),
                                          day->path, FALSE);
        }
    }
}

static void
log_scroll_and_focus_path (LogviewWindow *window, Log *log, GtkTreePath *path)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
    g_return_if_fail (path);

    gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->view), path, NULL, FALSE); 
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view), path, NULL, TRUE, 
                                  1, 1);
}

static void
logview_create_model_no_date (LogviewWindow *window, Log *log)
{
    int i;
    LogLine *line;
    GtkTreePath *last_path;
    GtkTreeIter iter;
    
    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
        
    log->model = GTK_TREE_MODEL(gtk_tree_store_new (5, G_TYPE_STRING, G_TYPE_STRING,
                                                    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));
    for (i=log->total_lines-1; i>=0; i--) {
        line = (log->lines)[i];
        gtk_tree_store_prepend (GTK_TREE_STORE (log->model), &iter, NULL);
        gtk_tree_store_set (GTK_TREE_STORE (log->model), &iter,                            
                            MESSAGE, line->message, -1);
        if (i == log->total_lines-1)
            log->current_path = gtk_tree_model_get_path (log->model, &iter);
    }
}

static void
logview_create_model (LogviewWindow *window, Log *log)
{
    GtkTreeIter iter, child_iter;
    GtkTreePath *path;
    LogLine *line;
    GList *days;
    Day *day;
    int i;

    gchar *path_string;

    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log->total_lines > 0);

    log->model = GTK_TREE_MODEL(gtk_tree_store_new (5, G_TYPE_STRING, G_TYPE_STRING,
                                                    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));

    /* Cycle on the days in the log */
    /* It's not worth it to do this with prepend instead of append */

    for (days = log->days; days != NULL; days = g_list_next (days)) {
        day = days->data;
        line = (log->lines)[day->first_line];
 
        /* Peut-etre faire tourner une progressbar ici ? */
       
        gtk_tree_store_append (GTK_TREE_STORE (log->model), &iter, NULL);
        model_fill_date_iter (GTK_TREE_STORE (log->model), &iter, line, day);
        path = gtk_tree_model_get_path (log->model, &iter);
        day->path = gtk_tree_path_copy (path);
        gtk_tree_path_free (path);
        path = NULL;

        /* Now cycle on the lines for the studied day */
        for (i = day->last_line-1; i >= day->first_line; i--) {
            line = (log->lines)[i];
            gtk_tree_store_prepend (GTK_TREE_STORE (log->model), &child_iter, &iter);
            model_fill_iter (GTK_TREE_STORE (log->model), &child_iter, line);
        }                
        
        if (days == g_list_last (days)) {
            day->expand = TRUE;
            log->current_path = gtk_tree_path_copy (day->path);
        } else {
            day->expand = FALSE;
        }
            
    }

    log->displayed_lines = log->total_lines;
}

void
log_repaint (LogviewWindow *window)
{
    Log *log;
    g_return_if_fail (LOGVIEW_IS_WINDOW (window));

    logview_update_statusbar (window);
    logview_set_window_title (window);
    
    if (window->curlog) {
        log = window->curlog;

        if ((log->displayed_lines > 0) &&
            (log->displayed_lines != log->total_lines))
            logview_add_new_log_lines (window, log);
        else {

            if (log->model == NULL) {
                if (log->days != NULL)
                    logview_create_model (window, log);
                else
                    logview_create_model_no_date (window, log);
            }
            
            logview_show_model (window, log);
            tree_view_columns_set_visible (window->view, (log->days != NULL));
            log_scroll_and_focus_path (window, log, log->current_path);
        }
    } else
        gtk_tree_view_set_model (GTK_TREE_VIEW (window->view), NULL);    
}
