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
#include "logview-findbar.h"

static GObjectClass *parent_class;
GType logview_findbar_get_type (void);

static gboolean
iter_is_visible (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *message;
	gboolean found = FALSE;
	gpointer day;
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);

	gtk_tree_model_get (model, iter, 0, &message, 1, &day, -1);
	if (day != NULL)
		return TRUE;

	if (message != NULL)
		found = g_strrstr (message, findbar->search_string);
	return found;
}

static void
logview_findbar_clear (GtkWidget *widget, gpointer data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	LogviewWindow *logview = LOGVIEW_WINDOW (findbar->logview);
	Log *log = logview->curlog;

	if (log==NULL || log->filter == NULL)
 		return;

	gtk_entry_set_text (GTK_ENTRY (findbar->entry), "");
}
	
static  gboolean
logview_findbar_entry_timeout (gpointer data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	LogviewWindow *logview = LOGVIEW_WINDOW (findbar->logview);
	Log *log = logview->curlog;

	if (log->filter == NULL) {
		log->filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (log->model, NULL));
		gtk_tree_model_filter_set_visible_func (log->filter, iter_is_visible, findbar, NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW (logview->view), GTK_TREE_MODEL (log->filter));
	} else {
		gtk_tree_model_filter_refilter (log->filter);
	}

	gtk_tree_view_expand_all (GTK_TREE_VIEW (logview->view));
	return FALSE;
}
	

static void
logview_findbar_entry_changed_cb (GtkEditable *editable,
				  gpointer     data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	LogviewWindow *logview = LOGVIEW_WINDOW (findbar->logview);
	Log *log = logview->curlog;
	gchar *search_string;    

	search_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (findbar->entry)));
	
	if (strlen (search_string) == 0 && log->filter != NULL) {
		g_object_unref (log->filter);
		log->filter = NULL;
		log_repaint (logview);
		return;
	}

	if (strlen (search_string) < 3) 
		return;

	findbar->search_string = search_string;

	g_timeout_add (500, (GSourceFunc) logview_findbar_entry_timeout, findbar);
}

void
logview_findbar_grab_focus (LogviewFindBar *findbar)
{
	g_return_if_fail (LOGVIEW_IS_FINDBAR (findbar));
	gtk_widget_grab_focus (findbar->entry);
}

void
logview_findbar_connect (LogviewFindBar *findbar, LogviewWindow *logview)
{
	findbar->logview = logview;

	g_signal_connect (G_OBJECT (findbar->entry), "changed",
			  G_CALLBACK (logview_findbar_entry_changed_cb), findbar);
	g_signal_connect (G_OBJECT (findbar->clear_button), "clicked",
			  G_CALLBACK (logview_findbar_clear), findbar);
}

static void 
logview_findbar_init (LogviewFindBar *findbar)
{
	GtkWidget *label, *button;
	
	gtk_container_set_border_width (GTK_CONTAINER (findbar), 3);

	label = gtk_label_new_with_mnemonic (_("_Filter:"));
	
	findbar->entry = gtk_entry_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), findbar->entry);
	
	findbar->clear_button = gtk_button_new_with_mnemonic (_("_Clear"));
	
	gtk_box_pack_start (GTK_BOX (findbar), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (findbar), findbar->entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (findbar), findbar->clear_button, FALSE, FALSE, 0);

	findbar->search_string = NULL;
}

static void
logview_findbar_class_init (LogviewFindBarClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
}

GType
logview_findbar_get_type (void)
{
	static GType object_type = 0;
	
	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (LogviewFindBarClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) logview_findbar_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (LogviewFindBar),
			0,              /* n_preallocs */
			(GInstanceInitFunc) logview_findbar_init
		};

		object_type = g_type_register_static (GTK_TYPE_HBOX, "LogviewFindBar", &object_info, 0);
	}

	return object_type;
}

GtkWidget *
logview_findbar_new (void)
{
    GtkWidget *widget;
    widget = g_object_new (LOGVIEW_FINDBAR_TYPE, NULL);
    return widget;
}
