/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include "logview.h"
#include "misc.h"
#include "loglist.h"
#include "logrtns.h"

extern gboolean restoration_complete;

static GObjectClass *parent_class;
GType loglist_get_type (void);

enum {
    LOG_NAME = 0,
    LOG_POINTER = 1
};

static void
loglist_get_log_iter (LogList *list, Log *log, GtkTreeIter *logiter)
{
    GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
    Log *iterlog;

    g_assert (LOG_IS_LIST (list));
    g_assert (log != NULL);

    model = GTK_TREE_MODEL (list->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	do {
		gtk_tree_model_get (model, &iter, LOG_POINTER, &iterlog, -1);
        if (iterlog == log) {
			path = gtk_tree_model_get_path (model, &iter);
            gtk_tree_model_get_iter (model, logiter, path);
            gtk_tree_path_free (path);
			return;
		}
	} while (gtk_tree_model_iter_next (model, &iter));
}

/* The returned GtkTreePath needs to be freed */

static GtkTreePath *
loglist_find_log_from_name (LogList *list, gchar *name)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	Log *log;
	
	g_assert (LOG_IS_LIST (list));
	g_assert (name != NULL);
	
	model = GTK_TREE_MODEL (list->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gtk_tree_model_get (model, &iter, LOG_POINTER, &log, -1);
		if (g_ascii_strncasecmp (name, log->name, 255) == 0) {
			path = gtk_tree_model_get_path (model, &iter);
			return path;
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	return NULL;
}

void
loglist_unbold_log (LogList *list, Log *log)
{
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL (list->model);

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    loglist_get_log_iter (list, log, &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, LOG_NAME, log->name, -1);
}

void
loglist_bold_log (LogList *list, Log *log)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *markup;
    
    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    loglist_get_log_iter (list, log, &iter);
    model = GTK_TREE_MODEL (list->model);
    markup = g_markup_printf_escaped ("<b>%s</b>", log->name);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, LOG_NAME, markup, -1);
    g_free (markup);

    /* if the log is currently shown, put up a timer to unbold it */
    if (logview_get_active_log (log->window) == log)
        g_timeout_add (5000, log_unbold, log);
}

void
loglist_select_log_from_name (LogList *list, gchar *name)
{
	GtkTreePath *path;
       	
	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (name != NULL);

	g_print("in loglist_select_log_from_name\n");
	
	path = loglist_find_log_from_name (list, name);
	if (path) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}
}

void
loglist_remove_log (LogList *list, Log *log)
{
    GtkTreeIter iter;

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    loglist_get_log_iter (list, log, &iter);

    if (gtk_list_store_remove (list->model, &iter)) {
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
        gtk_tree_selection_select_iter (selection, &iter);
    }
}

void 
loglist_add_log (LogList *list, Log *log)
{
	GtkTreeIter iter;

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

	gtk_list_store_append (list->model, &iter);
	gtk_list_store_set (list->model, &iter, 
                        LOG_NAME, log->name, LOG_POINTER, log, -1);

	if (restoration_complete) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		gtk_tree_selection_select_iter (selection, &iter);
	}
}

static void
loglist_selection_changed (GtkTreeSelection *selection, LogviewWindow *logview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name;
  Log *log;

  g_assert (LOGVIEW_IS_WINDOW (logview));
  g_assert (GTK_IS_TREE_SELECTION (selection));  

  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
      logview_select_log (logview, NULL);
      return;
  }

  gtk_tree_model_get (model, &iter, LOG_NAME, &name, LOG_POINTER, &log, -1);
  logview_select_log (logview, log);      
  if (g_str_has_prefix (name, "<b>"))
      g_timeout_add (5000, log_unbold, log);
}

void
loglist_connect (LogList *list, LogviewWindow *logview)
{
    GtkTreeSelection *selection;

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
    g_signal_connect (G_OBJECT (selection), "changed",
                      G_CALLBACK (loglist_selection_changed), logview);
}

static void 
loglist_init (LogList *list)
{
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;

    model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_list_store_clear (model);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
    list->model = model;
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
    g_object_unref (G_OBJECT (model));
    
    cell = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("words", cell, "markup", 0, NULL);
    
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (list), -1);
}

static void
loglist_class_init (LogListClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
}

GType
loglist_get_type (void)
{
	static GType object_type = 0;
	
	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (LogListClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) loglist_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (LogList),
			0,              /* n_preallocs */
			(GInstanceInitFunc) loglist_init
		};

		object_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "LogList", &object_info, 0);
	}

	return object_type;
}

GtkWidget *
loglist_new (void)
{
    GtkWidget *widget;
    widget = g_object_new (LOG_LIST_TYPE, NULL);
    return widget;
}
