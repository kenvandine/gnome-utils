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

#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "logview.h"

static gboolean
iter_is_visible (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *search_string = data;
	gchar *fields[4];
	gboolean found = FALSE;

	gtk_tree_model_get (model, iter, 0, &(fields[0]), 1, &(fields[1]), 2, &(fields[2]),
			    3, &(fields[3]), -1);
	if (fields[0] != NULL && fields[1]==NULL)
		return TRUE;

	if (fields[1] != NULL)
		found = found || g_strrstr (fields[1], search_string);
	if (fields[2] != NULL)
		found = found || g_strrstr (fields[2], search_string);
	if (fields[3] != NULL) {
		found = found || g_strrstr (fields[3], search_string);
	}
	return found;
}

static void
logview_findbar_entry_changed_cb (GtkEditable *editable,
				  gpointer     data)
{
	LogviewWindow *logview = data;
	GtkTreeModel *model;
	GtkTreeModelFilter *filter = NULL;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *search_string;
	Log *log;

	if (!logview->curlog)
		return;
	log = logview->curlog;

	search_string = gtk_entry_get_text (GTK_ENTRY (logview->find_entry));

	if (strlen (search_string) < 3 && strlen (search_string) > 0) 
		return;

	if (strlen (search_string) == 0) {
		gtk_tree_view_set_model (GTK_TREE_VIEW (logview->view), GTK_TREE_MODEL (log->model));
		return;
	}

	if (filter == NULL) {
		gtk_tree_model_get_iter_first (log->model, &iter);
		path = gtk_tree_model_get_path (log->model, &iter);
		filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (log->model, NULL));
	}
	
	gtk_tree_model_filter_set_visible_func (filter, iter_is_visible, search_string, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (logview->view), GTK_TREE_MODEL (filter));
	gtk_tree_view_expand_all (GTK_TREE_VIEW (logview->view));
	g_object_unref (filter);
}

static gboolean
logview_findbar_hide (GtkWidget *widget, GdkEventFocus *event, LogviewWindow *window)
{
	gtk_widget_hide (window->find_bar);
	log_repaint (window);
	return (FALSE);
}

GtkWidget *
logview_findbar_new (LogviewWindow *window)
{
	GtkWidget *hbox;
	GtkWidget *label;
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);

	label = gtk_label_new_with_mnemonic (_("_Filter:"));
	
	window->find_entry = gtk_entry_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), window->find_entry);
	
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), window->find_entry, FALSE, FALSE, 0);

	gtk_widget_show (hbox);
	gtk_widget_show (window->find_entry);

	g_signal_connect (G_OBJECT (window->find_entry), "changed",
			  G_CALLBACK (logview_findbar_entry_changed_cb), window);
	g_signal_connect (G_OBJECT (window->find_entry), "focus-out-event",
			  G_CALLBACK (logview_findbar_hide), window);

	return hbox;
}
