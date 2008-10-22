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

struct LogviewFindBarPriv
{
	GtkWidget *entry;
	GtkWidget *clear_button;
	gchar *search_string;
	gpointer logview;
};

G_DEFINE_TYPE (LogviewFindBar, logview_findbar, GTK_TYPE_HBOX);

static gboolean
iter_is_visible (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	gboolean found = FALSE;
	gchar *message;
	gpointer day;

	gtk_tree_model_get (model, iter, 0, &message, 1, &day, -1);
	if (day)
		return TRUE;
	if (message)
		found = (g_strstr_len (message, -1, findbar->priv->search_string) != NULL);
	return found;
}

static void
logview_findbar_clear (GtkWidget *widget, gpointer data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	LogviewWindow *logview = LOGVIEW_WINDOW (findbar->priv->logview);
	Log *log = logview->curlog;

	if (log==NULL || log->filter == NULL)
 		return;

	gtk_entry_set_text (GTK_ENTRY (findbar->priv->entry), "");
	gtk_widget_hide (GTK_WIDGET (findbar));
}
	
static  gboolean
logview_findbar_entry_timeout (gpointer data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	LogviewWindow *logview = LOGVIEW_WINDOW (findbar->priv->logview);
	Log *log = logview->curlog;
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (GTK_WIDGET (logview)->window, cursor);
	gdk_cursor_unref (cursor);
	gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));
	
	if (log->filter == NULL) {

		log->filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (log->model, NULL));
		gtk_tree_model_filter_set_visible_func (log->filter, iter_is_visible, findbar, NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW (logview->view), GTK_TREE_MODEL (log->filter));

	} else {
		gtk_tree_model_filter_refilter (log->filter);
	}

	gtk_tree_view_expand_all (GTK_TREE_VIEW (logview->view));

	gdk_window_set_cursor (GTK_WIDGET (logview)->window, NULL);
	gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));

	return FALSE;
}
	

static void
logview_findbar_entry_changed_cb (GtkEditable *editable,
				  gpointer     data)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (data);
	LogviewWindow *logview = LOGVIEW_WINDOW (findbar->priv->logview);
	Log *log = logview->curlog;
	gchar *search_string;    

	search_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (findbar->priv->entry)));
	
	if (strlen (search_string) == 0 && log->filter != NULL) {
		g_object_unref (log->filter);
		log->filter = NULL;
		logview_repaint (logview);
		return;
	}

	if (strlen (search_string) < 3) 
		return;

	findbar->priv->search_string = search_string;

	g_timeout_add (500, (GSourceFunc) logview_findbar_entry_timeout, findbar);
}

void
logview_findbar_grab_focus (LogviewFindBar *findbar)
{
	g_return_if_fail (LOGVIEW_IS_FINDBAR (findbar));
	gtk_widget_grab_focus (findbar->priv->entry);
}

void
logview_findbar_connect (LogviewFindBar *findbar, LogviewWindow *logview)
{
	findbar->priv->logview = logview;

	g_signal_connect (G_OBJECT (findbar->priv->entry), "changed",
			  G_CALLBACK (logview_findbar_entry_changed_cb), findbar);
	g_signal_connect (G_OBJECT (findbar->priv->clear_button), "clicked",
			  G_CALLBACK (logview_findbar_clear), findbar);
}

static void 
logview_findbar_init (LogviewFindBar *findbar)
{
	GtkWidget *label, *button;
	
	findbar->priv = g_new0 (LogviewFindBarPriv, 1);

	gtk_container_set_border_width (GTK_CONTAINER (findbar), 3);

	label = gtk_label_new_with_mnemonic (_("_Filter:"));
	
	findbar->priv->entry = gtk_entry_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), findbar->priv->entry);
	
	findbar->priv->clear_button = gtk_button_new_with_mnemonic (_("_Clear"));
	
	gtk_box_pack_start (GTK_BOX (findbar), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (findbar), findbar->priv->entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (findbar), findbar->priv->clear_button, FALSE, FALSE, 0);

	findbar->priv->search_string = NULL;
}

static void
logview_findbar_finalize (GObject *object)
{
	LogviewFindBar *findbar = LOGVIEW_FINDBAR (object);

	g_free (findbar->priv);
	G_OBJECT_CLASS (logview_findbar_parent_class)->finalize (object);
}

static void
logview_findbar_class_init (LogviewFindBarClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = logview_findbar_finalize;
}

GtkWidget *
logview_findbar_new (void)
{
    GtkWidget *widget;
    widget = g_object_new (LOGVIEW_FINDBAR_TYPE, NULL);
    return widget;
}
