/*
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@gnome.org>
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

#include "logview-manager.h"
#include "logview-log.h"

#include "logview-loglist.h"

struct _LogviewLoglistPrivate {
  GtkTreeStore *model;
  LogviewManager *manager;
};

G_DEFINE_TYPE (LogviewLoglist, logview_loglist, GTK_TYPE_TREE_VIEW);

#define GET_PRIVATE(o)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOG_LIST_TYPE, LogListPriv))

enum {
	LOG_OBJECT = 0,
	LOG_NAME,
	LOG_WEIGHT,
	LOG_WEIGHT_SET
};

static GtkTreeIter *
logview_loglist_find_log (LogviewLoglist *list, LogviewLog *log)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeIter *retval = NULL;
  LogviewLog *current;

  model = GTK_TREE_MODEL (list->priv->model);

  if (!gtk_tree_model_get_iter_first (model, &iter)) {
    return NULL;
  }

  do {
    gtk_tree_model_get (model, &iter, LOG_OBJECT, &current, -1);
    if (current == log) {
      retval = gtk_tree_iter_copy (&iter);
    }
    g_object_unref (current);
  } while (gtk_tree_model_iter_next (model, &iter) && retval == NULL);

  return retval;
}

static void
log_changed_cb (LogviewLog *log,
                gpointer user_data)
{
  LogviewLoglist *list = user_data;
  LogviewLog *active;
  GtkTreeIter *iter;

  active = logview_manager_get_active_log (list->priv->manager);

  if (log == active) {
    g_object_unref (active);
    return;
  }

  iter = logview_list_find_log (list, log);

  if (!iter) {
    return;
  }

  /* make the log bold in the list */
  gtk_tree_store_set (list->priv->model, iter,
                      LOG_WEIGHT, PANGO_WEIGHT_BOLD,
                      LOG_WEIGHT_SET, TRUE, -1);
}

static void
manager_active_changed_cb (LogviewManager *manager,
                           LogviewLog *log,
                           gpointer user_data)
{
  /* TODO: implement */
}

static void
manager_log_closed_cb (LogviewManager *manager,
                       LogviewLog *log,
                       gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeIter *iter;
  gboolean res;

  iter = logview_list_find_log (list, log);

  if (!iter) {
    return;
  }

  g_signal_handlers_disconnect_by_func (log, log_changed_cb, list);

  res = gtk_tree_store_remove (list->priv->model, iter);
  if (res) {
    GtkTreeSelection *selection;

    /* iter now points to the next valid row */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
    gtk_tree_selection_select_iter (selection, iter);
  } else {
    /* FIXME: what shall we do here? */
  }

  gtk_tree_iter_free (iter);
}

static void
manager_log_added_cb (LogviewManager *manager,
                      LogviewLog *log,
                      gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeIter iter;

  gtk_tree_store_append (list->priv->model, &iter, NULL);
  gtk_tree_store_set (list->priv->model, &iter,
                      LOG_OBJECT, log,
                      LOG_NAME, logview_log_get_display_name (log), -1);

  g_signal_connect (log, "log-changed",
                    G_CALLBACK (log_changed_cb), list);
}

static void
tree_selection_changed_cb (GtkTreeSelection *selection,
                           gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeModel *model;
  GtkTreeIter iter;
  LogviewLog *log;
  gboolean is_bold;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
      return;
  }

  gtk_tree_model_get (model, &iter, LOG_OBJECT, &log,
                      LOG_WEIGHT_SET, &is_bold);
  logview_manager_set_active_log (list->priv->manager, log);

  if (is_bold) {
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                        LOG_WEIGHT_SET, FALSE, -1);
  }

  g_object_unref (log);
}

static void
do_finalize (GObject *obj)
{
  LogviewLoglist *list = LOGVIEW_LOGLIST (obj);

  g_object_unref (list->priv->model);

  G_OBJECT_CLASS (logview_loglist_parent_class)->finalize (obj);
}

static void 
logview_loglist_init (LogviewLoglist *list)
{
  GtkTreeStore *model;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkCellRenderer *cell;

  list->priv = GET_PRIVATE (list); 

  model = gtk_tree_store_new (4, LOGVIEW_TYPE_LOG, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
  list->priv->model = model;
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (tree_selection_changed_cb), list);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", LOG_NAME,
                                       "weight-set", LOG_WEIGHT_SET,
                                       "weight", LOG_WEIGHT,
                                       NULL);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list->priv->model), 0, GTK_SORT_ASCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (list), -1);

  list->priv->manager = logview_manager_get ();

  g_signal_connect (list->priv->manager, "log-added",
                    G_CALLBACK (manager_log_added_cb), list);
  g_signal_connect (list->priv->manager, "log-closed",
                    G_CALLBACK (manager_log_closed_cb), list);
  g_signal_connect (list->priv->manager, "active-changed",
                    G_CALLBACK (manager_active_changed_cb), list);
}

static void
logview_loglist_class_init (LogviewLoglistClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  oclass->finalize = do_finalize;

  g_type_class_add_private (klass, sizeof (LogviewLoglistPrivate));
}

GtkWidget *
loglist_new (void)
{
  GtkWidget *widget;
  widget = g_object_new (LOG_LIST_TYPE, NULL);
  return widget;
}
