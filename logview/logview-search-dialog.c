/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net> *
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

#include "logview.h"
#include "logview-search-dialog.h"
#include "logview-search.h"
#include "gedit-output-window.h"
#include <gtk/gtk.h>

extern GtkWidget *view;

#define _(x) gettext (x)
#define N_(x) (x)

static void
logview_search_dialog_response (GtkDialog *dialog, gint response_id)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
logview_search_dialog_class_init (LogviewSearchDialogClass *klass)
{
	GtkDialogClass *dialog_class;

	dialog_class = (GtkDialogClass *)klass;

	dialog_class->response = logview_search_dialog_response;
}

static void
logview_search_not_found_dialog (GtkWidget *window) 
{
	GtkWidget *not_found_dialog;
	
	g_return_if_fail (window);
	not_found_dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE,
						   _("Pattern not found"));
	gtk_dialog_run (GTK_DIALOG (not_found_dialog));
	gtk_widget_destroy (GTK_WIDGET (not_found_dialog));
	
}



static void
logview_search_dialog_search (GtkWidget *button, LogviewSearchDialog *dialog)
{
	LogviewWindow *window; 
	GdkCursor *cursor;

	gchar *pattern;
	int res;
	
	GtkTreeModel *tree_model;
	
	window = g_object_get_data (G_OBJECT (dialog), "logview-window");
	gedit_output_window_clear (GEDIT_OUTPUT_WINDOW (window->output_window));
	window->output_window_type = LOGVIEW_WINDOW_OUTPUT_WINDOW_SEARCH;

	tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (window->view));

	pattern = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->entry)));

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (GTK_WIDGET (dialog)->window, cursor);
	gdk_cursor_unref (cursor);
	gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (dialog)));

	g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer)&dialog);

	res = logview_tree_model_build_match_list  (tree_model,
						    GEDIT_OUTPUT_WINDOW (window->output_window), 
						    pattern,
						    G_OBJECT (dialog));
	
	g_free (pattern);

	if (dialog != NULL) {
		gdk_window_set_cursor (GTK_WIDGET (dialog)->window, NULL);
		gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (dialog)));

		if (res != 0) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
		} else {
			logview_search_not_found_dialog(GTK_WIDGET(window));
		}
	}
}

static void
logview_search_entry_changed (GtkEntry *entry, LogviewSearchDialog *dialog)
{
	gboolean find_sensitive;
	const gchar *text;

	text = gtk_entry_get_text (GTK_ENTRY (entry));
	find_sensitive = text != NULL && text[0] != '\0';
	
	gtk_widget_set_sensitive (dialog->search_button, find_sensitive);
}

static void
logview_search_dialog_init (LogviewSearchDialog *dialog)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 2);
	
	hbox = gtk_hbox_new (FALSE, 12);
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	gtk_window_set_title (GTK_WINDOW (dialog),_("Find"));

	label = gtk_label_new_with_mnemonic (_("_Search for: "));
			
	dialog->entry = gtk_entry_new ();
	g_signal_connect (dialog->entry, "changed",
			  G_CALLBACK (logview_search_entry_changed), dialog);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), GTK_WIDGET(dialog->entry));
	
	dialog->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	gtk_widget_set_sensitive (dialog->search_button, FALSE);
	g_signal_connect (dialog->search_button, "clicked",
			  G_CALLBACK (logview_search_dialog_search), dialog);

	GTK_WIDGET_SET_FLAGS(dialog->search_button, GTK_CAN_DEFAULT);
	gtk_window_set_default(GTK_WINDOW(dialog), dialog->search_button);
	gtk_entry_set_activates_default(GTK_ENTRY(dialog->entry), TRUE);
	
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->entry, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), dialog->search_button, TRUE, TRUE, 0);
	gtk_widget_show_all (GTK_DIALOG(dialog)->action_area);
	gtk_widget_show_all (vbox);
	
	gtk_widget_show_all (hbox);
}

GType
logview_search_dialog_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (LogviewSearchDialogClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) logview_search_dialog_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (LogviewSearchDialog),
			0,              /* n_preallocs */
			(GInstanceInitFunc) logview_search_dialog_init
		};

		object_type = g_type_register_static (GTK_TYPE_DIALOG, "LogviewSearchDialog", &object_info, 0);
	}

	return object_type;
}

GtkWidget *
logview_search_dialog_new (LogviewWindow *parent)
{
	GtkWidget *dialog;

	dialog = g_object_new (LOGVIEW_TYPE_SEARCH_DIALOG, NULL);
	g_object_set_data (G_OBJECT (dialog), "logview-window", parent);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW(parent));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

	return dialog;
}
