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
#include <libgnomevfs/gnome-vfs.h>

struct LogListPriv {
	GtkTreeStore *model;
};

static GObjectClass *parent_class;
GType loglist_get_type (void);

enum {
	LOG_NAME = 0,
	LOG_POINTER,
	LOG_WEIGHT,
	LOG_WEIGHT_SET
};

static GtkTreePath *
loglist_find_directory (LogList *list, gchar *dir)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	gchar *iterdir;
	
	g_assert (LOG_IS_LIST (list));

	model = GTK_TREE_MODEL (list->priv->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gtk_tree_model_get (model, &iter, LOG_NAME, &iterdir, -1);
		if (iterdir) {
			if (g_ascii_strncasecmp (iterdir, dir, -1) == 0) {
				path = gtk_tree_model_get_path (model, &iter);
				break;
			}
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	return path;
}


/* The returned GtkTreePath needs to be freed */

static GtkTreePath *
loglist_find_log (LogList *list, Log *log)
{
	GtkTreeIter iter, child_iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	Log *iterlog;
	
	g_assert (LOG_IS_LIST (list));
	g_assert (log != NULL);       	

	model = GTK_TREE_MODEL (list->priv->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gtk_tree_model_iter_children (model, &child_iter, &iter);
		do {
			gtk_tree_model_get (model, &child_iter, LOG_POINTER, &iterlog, -1);
			if (iterlog == log) {
				path = gtk_tree_model_get_path (model, &child_iter);
				return path;
			}
		} while (gtk_tree_model_iter_next (model, &child_iter));
	} while (gtk_tree_model_iter_next (model, &iter));

	return NULL;
}

static void
loglist_get_log_iter (LogList *list, Log *log, GtkTreeIter *logiter)
{
    GtkTreeModel *model;
    GtkTreePath *path = NULL;

    path = loglist_find_log (list, log);
    if (path) {
	    model = GTK_TREE_MODEL (list->priv->model);
	    gtk_tree_model_get_iter (model, logiter, path);
	    gtk_tree_path_free (path);
    }
}

void
loglist_unbold_log (LogList *list, Log *log)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    model = GTK_TREE_MODEL (list->priv->model);

    loglist_get_log_iter (list, log, &iter);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			LOG_WEIGHT_SET, FALSE, -1);
}

void
loglist_bold_log (LogList *list, Log *log)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    model = GTK_TREE_MODEL (list->priv->model);

    loglist_get_log_iter (list, log, &iter);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			LOG_WEIGHT, PANGO_WEIGHT_BOLD,
			LOG_WEIGHT_SET, TRUE, -1);

    /* if the log is currently shown, put up a timer to unbold it */
    if (logview_get_active_log (log->window) == log)
        g_timeout_add (5000, log_unbold, log);
}

void
loglist_select_log (LogList *list, Log *log)
{
	GtkTreePath *path;
       	
	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (log);

	path = loglist_find_log (list, log);
	if (path) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (list), path);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}
}

void
loglist_remove_log (LogList *list, Log *log)
{
	GtkTreeIter iter, parent;

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    loglist_get_log_iter (list, log, &iter);
    gtk_tree_model_iter_parent (GTK_TREE_MODEL (list->priv->model), &parent, &iter);

    if (gtk_tree_store_remove (list->priv->model, &iter)) {
        GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_select_iter (selection, &iter);
    } else {
	    if (!gtk_tree_model_iter_has_child (GTK_TREE_MODEL (list->priv->model), &parent)) {
		    if (gtk_tree_store_remove (list->priv->model, &parent)) {
			    GtkTreeSelection *selection;
			    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
			    gtk_tree_selection_select_iter (selection, &parent);
		    }
	    }
    }
}

void
loglist_add_directory (LogList *list, gchar *dirname, GtkTreeIter *iter)
{
	gtk_tree_store_append (list->priv->model, iter, NULL);
	gtk_tree_store_set (list->priv->model, iter,
			    LOG_NAME, dirname, LOG_POINTER, NULL, -1);
}

void 
loglist_add_log (LogList *list, Log *log)
{
	GtkTreeIter iter, child_iter;
	GtkTreePath *path;
	gchar *filename, *dirname;

	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (log != NULL);

	dirname = log_extract_dirname (log);
	path = loglist_find_directory (list, dirname);
	if (path) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->model), &iter, path);
		gtk_tree_path_free (path);
	} else
		loglist_add_directory (list, dirname, &iter);

	filename = log_extract_filename (log);
	gtk_tree_store_append (list->priv->model, &child_iter, &iter);
	gtk_tree_store_set (list->priv->model, &child_iter, 
                        LOG_NAME, filename, LOG_POINTER, log, -1);

	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (list)))
		loglist_select_log (list, log);

	g_free (dirname);
	g_free (filename);
}

static void
loglist_selection_changed (GtkTreeSelection *selection, LogviewWindow *logview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean bold;
  gchar *name;
  Log *log;

  g_assert (LOGVIEW_IS_WINDOW (logview));
  g_assert (GTK_IS_TREE_SELECTION (selection));  

  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
      logview_select_log (logview, NULL);
      return;
  }

  gtk_tree_model_get (model, &iter, 
		      LOG_NAME, &name, 
		      LOG_POINTER, &log, 
		      LOG_WEIGHT_SET, &bold, -1);
  logview_select_log (logview, log);
  if (bold)
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
    GtkTreeStore *model;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkCellRenderer *cell;

    list->priv = g_new0 (LogListPriv, 1);

    model = gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
    list->priv->model = model;
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
    g_object_unref (G_OBJECT (model));

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    
    cell = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_attributes (column, cell,
					 "text", LOG_NAME,
					 "weight-set", LOG_WEIGHT_SET,
					 "weight", LOG_WEIGHT,
					 NULL);

    gtk_tree_sortable_set_sort_column_id (list->priv->model, 0, GTK_SORT_ASCENDING);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (list), -1);
}

static void
loglist_finalize (GObject *object)
{
	LogList *list = LOG_LIST (object);
	g_free (list->priv);
	parent_class->finalize (object);
}

static void
loglist_class_init (LogListClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = loglist_finalize;
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
			NULL,/* class_finalize */
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
