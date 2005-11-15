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
	LOG_POINTER
};

static GtkTreePath *
loglist_find_directory (LogList *list, GnomeVFSURI *uri)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	GnomeVFSURI *iteruri;
	gchar *iterdir;
	
	g_assert (LOG_IS_LIST (list));

	model = GTK_TREE_MODEL (list->priv->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gtk_tree_model_get (model, &iter, LOG_NAME, &iterdir, -1);
		if (iterdir) {
			iteruri = gnome_vfs_uri_new (iterdir);
			if (gnome_vfs_uri_equal (uri, iteruri)) {
				path = gtk_tree_model_get_path (model, &iter);
				gnome_vfs_uri_unref (iteruri);
				return path;
			}
			gnome_vfs_uri_unref (iteruri);
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	return NULL;
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
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL (list->priv->model);
    gchar *filename;

    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    loglist_get_log_iter (list, log, &iter);
    filename = log_extract_filename (log);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LOG_NAME, filename, -1);
    g_free (filename);
}

void
loglist_bold_log (LogList *list, Log *log)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *markup, *filename;
    
    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    loglist_get_log_iter (list, log, &iter);
    model = GTK_TREE_MODEL (list->priv->model);

    filename = log_extract_filename (log);
    markup = g_markup_printf_escaped ("<b>%s</b>", filename);
    g_free (filename);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LOG_NAME, markup, -1);
    g_free (markup);

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
loglist_add_log (LogList *list, Log *log)
{
	GtkTreeIter iter, child_iter;
	GtkTreePath *path;
	gchar *dir, *file;
	GnomeVFSURI *uri, *dir_uri;

	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (log != NULL);

	uri = gnome_vfs_uri_new (log->name);
	dir_uri = gnome_vfs_uri_get_parent (uri);

	path = loglist_find_directory (list, dir_uri);
	gnome_vfs_uri_unref (dir_uri);

	if (path) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->model), &iter, path);
		gtk_tree_path_free (path);
	} else {
		dir = gnome_vfs_uri_extract_dirname (uri);
	
		gtk_tree_store_append (list->priv->model, &iter, NULL);
		gtk_tree_store_set (list->priv->model, &iter,
				    LOG_NAME, dir, LOG_POINTER, NULL, -1);
		
		g_free (dir);
	}

	file = gnome_vfs_uri_extract_short_name (uri);
	gtk_tree_store_append (list->priv->model, &child_iter, &iter);
	gtk_tree_store_set (list->priv->model, &child_iter, 
                        LOG_NAME, file, LOG_POINTER, log, -1);
	g_free (file);

	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (list))) {
		GtkTreeSelection *selection;
		GtkTreePath *path;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (list->priv->model), &child_iter);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (list), path);
		gtk_tree_selection_select_iter (selection, &child_iter);
	}

	gnome_vfs_uri_unref (uri);
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
  if (g_str_has_prefix (name, "<b>") && g_str_has_suffix (name, "</b>"))
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

    model = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
    list->priv->model = model;
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
    g_object_unref (G_OBJECT (model));

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    
    cell = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("words", cell, "markup", 0, NULL);
    
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
