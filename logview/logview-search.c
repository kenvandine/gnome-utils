/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net>
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
#include "logview-search.h"
#include "logview.h"
#include "gedit-output-window.h"

GtkTreePath *
logview_tree_path_from_key (GtkTreeModel *model, char *key)
{
	gchar **key_array, *date, *message;
	GtkTreeIter iter, child_iter, found_iter;
	int i;
	gboolean found;
	GtkTreePath *path;

	key_array = g_strsplit (key, " | ", 2);

	if (!gtk_tree_model_get_iter_root (model, &iter)) {
		g_strfreev (key_array);
		return NULL;
	}

	found = FALSE;
	
	/* we first loop on each date and then on each message in that date */
	do {
		if (!gtk_tree_model_iter_children (model, &child_iter, &iter))
			return;

		do {
			gtk_tree_model_get (model, &child_iter, 0, &date, 3, &message, -1);
			if ((g_str_equal (date, key_array[0])) && g_str_equal (message, key_array[1])) {
				found_iter = child_iter;
				found = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next (model, &child_iter));
		

		if (found)
			break;
	} while (gtk_tree_model_iter_next (model, &iter));
		    
	if (found) 
		path = gtk_tree_model_get_path (model, &found_iter);
	else
		path = NULL;

	g_strfreev (key_array);

	return path;
}
	
static
gboolean
logview_tree_model_search_iter_foreach (GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer data)
{
	SearchIter *st;
	gchar *found[3];
	GSList *values, *list;
	char *date, *hostname, *process, *message;

	st = (SearchIter *) data;

	if (st->searching == NULL) {
		return TRUE;
	}

	if (st->res == 1)
		gtk_widget_show (GTK_WIDGET (st->output_window));

	while (gtk_events_pending ())
		gtk_main_iteration ();

	gtk_tree_model_get (model, iter, 0, &date, 1, &hostname, 2, &process, 3, &message, -1);

	found[0] = (hostname==NULL ? NULL : g_strrstr ((char*) hostname, (char*) st->pattern));
	found[1] = (process==NULL ? NULL : g_strrstr ((char*) process, (char*) st->pattern));
	found[2] = (message==NULL ? NULL : g_strrstr ((char*) message, (char*) st->pattern));

	if ((found[0] != NULL) || (found[1] != NULL) || (found[2] != NULL)) {
		/* We found the pattern in the tree */
		gchar *key = g_strdup_printf ("%s | %s", date, message);
		gedit_output_window_append_line (st->output_window, key, FALSE);
		g_free (key);
		st->res++;
		return FALSE;
	}

	return FALSE;

}
	
int
logview_tree_model_build_match_list (GtkTreeModel *tree_model, GeditOutputWindow *output_window,
				   const char *pattern, GObject *dialog)
{
	GtkTreeIter iter_root;
	int res;
	SearchIter *st;

	st = g_new0 (SearchIter, 1);
	st->pattern = pattern;
	st->output_window = output_window; 
	st->res = 0;
	st->searching = dialog;

	g_object_add_weak_pointer (st->searching, (gpointer)&st->searching);

	if (!gtk_tree_model_get_iter_root (GTK_TREE_MODEL (tree_model), &iter_root)) {

		/* Ugh, something is terribly wrong */
		return 0;
	}

	gtk_tree_model_foreach (GTK_TREE_MODEL (tree_model),
				logview_tree_model_search_iter_foreach, st);

	res = st->res;
	g_free (st);

	return res;

}

