/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool-callbacks.c
 *
 *  (C) 2002 the Free Software Foundation 
 *
 *  Authors:   	Dennis Cranston  <dennis_cranston@yahoo.com>
 *		George Lebl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */
 
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gsearchtool.h"
#include "gsearchtool-support.h"
#include "gsearchtool-callbacks.h"

#include <signal.h>

static GnomeUIInfo popup_menu[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("_Open..."), 
				NULL, 
				open_file_cb,
				GTK_STOCK_OPEN),
	GNOMEUIINFO_ITEM_STOCK (N_("O_pen Folder..."), 
				NULL, 
				open_folder_cb,
				GTK_STOCK_OPEN),
	GNOMEUIINFO_SEPARATOR,		
	GNOMEUIINFO_ITEM_STOCK (N_("_Save Results As..."), 
				NULL, 
				show_file_selector_cb,
				GTK_STOCK_SAVE_AS),						
	GNOMEUIINFO_END
};

void
die_cb (GnomeClient 	*client, 
	gpointer 	data)
{
	quit_cb (NULL, NULL);
}

void
quit_cb (GtkWidget 	*widget, 
	 gpointer 	data)
{
	if(search_command.running == RUNNING)
	{
		search_command.running = MAKE_IT_QUIT;
		kill (search_command.pid, SIGKILL);
		wait (NULL);
		
		gtk_main_quit ();
		return;
	}
	gtk_main_quit ();
}

void
help_cb (GtkWidget 	*widget, 
	 gpointer 	data) 
{
	GError *error = NULL;

        gnome_help_display_desktop (NULL, "gnome-search-tool", 
				    "gnome-search-tool", NULL, &error);
	if (error) {
		GtkWidget *dialog;

                dialog = gtk_message_dialog_new (GTK_WINDOW (interface.main_window),
                        GTK_DIALOG_DESTROY_WITH_PARENT,
                        GTK_MESSAGE_ERROR,
                        GTK_BUTTONS_CLOSE,
                        _("There was an error displaying help: %s"),
                        error->message);

                g_signal_connect (G_OBJECT (dialog),
                        "response",
                        G_CALLBACK (gtk_widget_destroy), NULL);
			
                gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
                gtk_widget_show (dialog);
                g_error_free (error);
        }
}

void
click_find_cb (GtkWidget	*widget, 
	       gpointer 	data)
{
	gchar *command;

	gtk_widget_set_sensitive (interface.stop_button, FALSE);
	gtk_widget_set_sensitive (interface.find_button, FALSE);

	command = build_search_command ();
	
	if (search_command.lock == FALSE) {	
		gnome_appbar_pop (GNOME_APPBAR (interface.status_bar));
		gnome_appbar_push (GNOME_APPBAR (interface.status_bar), 
				   _("Searching..."));
		search_command.timeout = gtk_timeout_add (100, update_progress_bar, 
							  NULL); 
		search_command.show_hidden_files = FALSE;
		
		spawn_search_command (command);
	}
	
	gtk_widget_set_sensitive (interface.stop_button, TRUE);
	gtk_widget_set_sensitive (interface.find_button, TRUE);
	
	g_free(command);
}

void
click_stop_cb (GtkWidget 	*widget, 
	       gpointer 	data)
{
	gtk_widget_set_sensitive (interface.stop_button, FALSE);
	gtk_widget_set_sensitive (interface.find_button, FALSE);
	
	if(search_command.running == RUNNING) {
		search_command.running = MAKE_IT_STOP;
		kill (search_command.pid, SIGKILL);
		wait (NULL);
	}
	
	gtk_widget_set_sensitive (interface.find_button, TRUE);
	gtk_widget_set_sensitive (interface.find_button, TRUE);
}

void
add_constraint_cb (GtkWidget 	*widget, 
		   gpointer 	data)
{
	SearchConstraint *constraint = g_new(SearchConstraint,1);
	GtkWidget *menu;
	GtkWidget *item;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU(interface.constraint_menu));
	item = gtk_menu_get_active (GTK_MENU(menu));
	
	if (GTK_WIDGET_SENSITIVE(item) == TRUE) {
		interface.geometry.min_height += 30; 
		gtk_window_set_geometry_hints (GTK_WINDOW(interface.main_window), 
					       GTK_WIDGET(interface.main_window),
					       &interface.geometry, GDK_HINT_MIN_SIZE);
		add_constraint (interface.selected_constraint);
	}
}

void
remove_constraint_cb (GtkWidget 	*widget, 
		      gpointer 		data)
{
	SearchConstraint *constraint = data;
	
      	interface.geometry.min_height -= 30;
	
	gtk_window_set_geometry_hints (GTK_WINDOW(interface.main_window), 
				       GTK_WIDGET(interface.main_window),
				       &interface.geometry, GDK_HINT_MIN_SIZE);
	
	gtk_container_remove (GTK_CONTAINER(interface.constraint), widget->parent);
	
	interface.selected_constraints =
		g_list_remove(interface.selected_constraints, constraint);
	
	set_constraint_selected_state (constraint->constraint_id, FALSE);	
}

void
constraint_activate_cb (GtkWidget 	*widget, 
			gpointer 	data)
{
	if ((GTK_WIDGET_VISIBLE (interface.find_button) == TRUE) && 
	    (GTK_WIDGET_SENSITIVE (interface.find_button) == TRUE)) {
		click_find_cb (interface.find_button, NULL);
	}	 
}

void
constraint_update_info_cb (GtkWidget 	*widget, 
			   gpointer 	data)
{
	static gchar *string;
	SearchConstraint *opt = data;
	
	string = (gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
	update_constraint_info(opt, string);
}

void
constraint_menu_toggled_cb(GtkWidget 	*widget, 
			   gpointer 	data)
{
	if (GTK_CHECK_MENU_ITEM(widget)->active)
		interface.selected_constraint = (long)data;
}

void
constraint_entry_changed_cb (GtkWidget 	*widget, 
			     gpointer 	data)
{
	static gchar *file_is_named_string;
	static gchar *look_in_folder_string;
	
	if (GTK_WIDGET_VISIBLE (interface.main_window) == FALSE) {
		return;
	}
	
	if (GTK_WIDGET_VISIBLE (interface.additional_constraints) == FALSE) {
	
		file_is_named_string = 
			(gchar *) gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(interface.file_is_named_entry))));
	
		if (strlen (file_is_named_string) == 0) {
			gtk_widget_set_sensitive (interface.find_button, FALSE);
			return;
		}
	} 
		
	look_in_folder_string = 
		gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(interface.look_in_folder_entry), TRUE);

	if (look_in_folder_string == NULL) {
		gtk_widget_set_sensitive (interface.find_button, FALSE);
		return;
	}
	
	gtk_widget_set_sensitive (interface.find_button, TRUE);
}
	

void
file_is_named_activate_cb (GtkWidget 	*widget, 
			   gpointer 	data)
{
	if ((GTK_WIDGET_VISIBLE (interface.find_button) == TRUE) && 
	    (GTK_WIDGET_SENSITIVE (interface.find_button) == TRUE)) {
		click_find_cb (interface.find_button, NULL);
	}
}

void
open_file_cb (GtkWidget 	*widget, 
	      gpointer 		data)
{
	gchar *file = NULL;
	gchar *locale_file = NULL;
	gchar *utf8_name = NULL;
	gchar *utf8_path = NULL;
	gboolean no_files_found = FALSE;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
		store = interface.model;
		selection = interface.selection;
		
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    			return;
		
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,COLUMN_NAME, &utf8_name,
							       COLUMN_PATH, &utf8_path,
							       COLUMN_NO_FILES_FOUND, &no_files_found,
								-1);

		if (!no_files_found) {
			file = g_build_filename (utf8_path, utf8_name, NULL);
			locale_file = g_locale_from_utf8(file, -1, NULL, NULL, NULL);
			
			if (is_nautilus_running ()) {
				open_file_with_nautilus (locale_file);
			}
			else {
				if (!open_file_with_application(locale_file))	
					gnome_error_dialog_parented(_("No viewer available for this mime type."),
							    	    GTK_WINDOW(interface.main_window));
			}

			g_free (file);
			g_free (locale_file);
		}
		
	g_free (utf8_name);
	g_free (utf8_path);
	
}

void
open_folder_cb (GtkWidget 	*widget, 
		gpointer 	data)
{
	GtkTreeSelection *selection;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *folder_locale;
	gchar *folder_utf8;
	
	store = interface.model;
	selection = interface.selection;
		
	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {		
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COLUMN_PATH, &folder_utf8, -1);						
		folder_locale = g_filename_from_utf8 (folder_utf8, -1, NULL, NULL, NULL);
		
		if (is_nautilus_running ()) {
			open_file_with_nautilus (folder_locale);
		}
		else {
			gnome_error_dialog_parented (_("The nautilus file manager is not running."),
						       GTK_WINDOW(interface.main_window));
		}
		g_free (folder_locale);
		g_free (folder_utf8);
	}
}

gboolean
click_file_cb 	     (GtkWidget 	*widget, 
		      GdkEventButton 	*event, 
		      gpointer 		data)
{
	gchar *file = NULL;
	gchar *locale_file = NULL;
	gchar *utf8_name = NULL;
	gchar *utf8_path = NULL;
	gboolean no_files_found = FALSE;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	if (event->type==GDK_2BUTTON_PRESS) {
		

		store = interface.model;
		selection = interface.selection;
		
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    			return FALSE;
		
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,COLUMN_NAME, &utf8_name,
							       COLUMN_PATH, &utf8_path,
							       COLUMN_NO_FILES_FOUND, &no_files_found,
								-1);

		if (!no_files_found) {
			file = g_build_filename (utf8_path, utf8_name, NULL);
			locale_file = g_locale_from_utf8(file, -1, NULL, NULL, NULL);
			
			if (is_nautilus_running ()) {
				open_file_with_nautilus (locale_file);
			}
			else {
				if (!open_file_with_application(locale_file))	
					gnome_error_dialog_parented(_("No viewer available for this mime type."),
							    	    GTK_WINDOW(interface.main_window));
			}

			g_free (file);
			g_free (locale_file);
		}
		
		g_free (utf8_name);
		g_free (utf8_path);
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {	
		GtkWidget *popup;
		
		store = interface.model;
		selection = interface.selection;
		
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    			return FALSE;
		
		gtk_tree_model_get (GTK_TREE_MODEL(store),&iter,COLUMN_NAME, &utf8_name,
							        COLUMN_PATH, &utf8_path,
							        COLUMN_NO_FILES_FOUND, &no_files_found,
								-1);
		if (!no_files_found) {
			popup = gnome_popup_menu_new (popup_menu);
		    
			gnome_popup_menu_do_popup (GTK_WIDGET (popup), NULL, NULL, (GdkEventButton *)event,
				 	           data, NULL);
		}
	}
	return FALSE;
}

void  
drag_file_cb  (GtkWidget          *widget,
	       GdkDragContext     *context,
	       GtkSelectionData   *selection_data,
	       guint               info,
	       guint               time,
	       gpointer            data)
{
	gchar 	 		*file = NULL;
	gchar 	 		*locale_file = NULL;
	gchar    		*url_file = NULL;
	gchar 	 		*utf8_name = NULL;
	gchar 	 		*utf8_path = NULL;
	gboolean 		no_files_found = FALSE;
	GtkTreeSelection 	*selection;
	GtkListStore 		*store;
	GtkTreeIter 		iter;
	
	store = interface.model;
	selection = interface.selection;
	
	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;
	
	gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,COLUMN_NAME, &utf8_name,
     		           COLUMN_PATH, &utf8_path,
			   COLUMN_NO_FILES_FOUND, &no_files_found,
		           -1);
			   	   
	file = g_build_filename (utf8_path, utf8_name, NULL);
	locale_file = g_locale_from_utf8 (file, -1, NULL, NULL, NULL);

	if (!no_files_found) {
		url_file = g_strconcat ("file://", locale_file, NULL);	
	} 
	else {
		url_file = g_strconcat (locale_file, NULL);
	}	

	g_free (locale_file);
	g_free (file);

	gtk_selection_data_set (selection_data, 
				selection_data->target,
				8, 
				url_file, 
				strlen (url_file));
	g_free (utf8_name);
	g_free (utf8_path);
	g_free (url_file);
}

void
show_file_selector_cb (GtkWidget 	*widget, 
		       gpointer		data)
{
	if (search_command.running != NOT_RUNNING) {
		return;
	}
	
	interface.file_selector = gtk_file_selection_new(_("Save Search Results As..."));
		
	if (interface.save_results_file) gtk_file_selection_set_filename(GTK_FILE_SELECTION(interface.file_selector), 
									interface.save_results_file);

	g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(interface.file_selector)->ok_button), "clicked",
				G_CALLBACK (save_results_cb), NULL);
	
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (interface.file_selector)->ok_button),
                             	"clicked",
                             	G_CALLBACK (gtk_widget_destroy), 
                             	(gpointer) interface.file_selector); 

   	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (interface.file_selector)->cancel_button),
                             	"clicked",
                             	G_CALLBACK (gtk_widget_destroy),
                             	(gpointer) interface.file_selector); 

	gtk_window_set_modal (GTK_WINDOW(interface.file_selector), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW(interface.file_selector), GTK_WINDOW(interface.main_window));
	gtk_window_set_position (GTK_WINDOW (interface.file_selector), GTK_WIN_POS_MOUSE);

	gtk_widget_show (GTK_WIDGET(interface.file_selector));
}

void
save_results_cb (GtkFileSelection *selector, 
		 gpointer 	  user_data, 
		 gpointer 	  data)
{
	FILE *fp;
	GtkListStore *store;
	GtkTreeIter iter;
	gint n_children, i;
	
	store = interface.model;
	
	interface.save_results_file = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(interface.file_selector));
	
	if (access (interface.save_results_file, F_OK) == 0) {
		GtkWidget *dialog;
		gint response;
		
		dialog = gtk_message_dialog_new
			(GTK_WINDOW (interface.file_selector),
			 0 /* flags */,
			 GTK_MESSAGE_QUESTION,
			 GTK_BUTTONS_YES_NO,
			 _("File %s already exists. Overwrite?"),
			 interface.save_results_file);
		gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW (interface.main_window));
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		
		gtk_widget_destroy (GTK_WIDGET(dialog));
		
		if (response != GTK_RESPONSE_YES) return;
	}
	
	if (fp = fopen(interface.save_results_file, "w")) {
	
		if (gtk_tree_model_get_iter_root(GTK_TREE_MODEL(store), &iter)) {
			
			n_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL);
			
			for (i = 0; i < n_children; i++)
			{
				gchar *utf8_path, *utf8_name, *file;
				
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_PATH, &utf8_path, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_NAME, &utf8_name, -1);
				gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);	
				
				file = g_build_filename (utf8_path, utf8_name, NULL);					    
				fprintf(fp, "%s\n", file);
				
				g_free(utf8_path);
				g_free(utf8_name);
				g_free(file);
			}
		}		 
		fclose(fp);
	} 
	else {
		gnome_app_error(GNOME_APP(interface.main_window), _("Cannot save the results file."));
	}
}

void
save_session_cb (GnomeClient 	    *client, 
		 gint 		    phase,
 	      	 GnomeRestartStyle  save_style, 
		 gint 		    shutdown,
 	      	 GnomeInteractStyle interact_style, 
		 gint 		    fast,
 	      	 gpointer 	    client_data)
{
 	gchar *argv[] = { NULL };
 
 	argv[0] = (gchar *) client_data;
 	gnome_client_set_clone_command (client, 1, argv);
 	gnome_client_set_restart_command (client, 1, argv);	
}

gboolean
escape_key_press_cb (GtkWidget    	*widget, 
		     GdkEventKey	*event, 
		     gpointer 	data)
{
	g_return_if_fail (GTK_IS_WIDGET(widget));

	if (event->keyval == GDK_Escape)
	{
		if (search_command.running == RUNNING)
		{
			search_command.running = MAKE_IT_STOP;
			return FALSE;	
		}
	}
	return FALSE;
}

gboolean
look_in_folder_key_press_cb (GtkWidget    	*widget, 
		     	     GdkEventKey	*event, 
		     	     gpointer 		data)
{
	g_return_if_fail (GTK_IS_WIDGET(widget));

	if (event->keyval == GDK_Return)
	{
		if ((GTK_WIDGET_VISIBLE (interface.find_button) == TRUE) && 
		    (GTK_WIDGET_SENSITIVE (interface.find_button) == TRUE)) {
			click_find_cb (interface.find_button, NULL);
		}
	}
	return FALSE;
}
