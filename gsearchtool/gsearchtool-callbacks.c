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

#define SILENT_WINDOW_OPEN_LIMIT 5

#include <gnome.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnome/gnome-desktop-item.h>

#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "gsearchtool.h"
#include "gsearchtool-support.h"
#include "gsearchtool-callbacks.h"

static GnomeUIInfo popup_menu[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("_Open"), 
				NULL, 
				open_file_cb,
				GTK_STOCK_OPEN),
	GNOMEUIINFO_ITEM_STOCK (N_("O_pen Folder"), 
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

gboolean row_selected_by_button_press_event;

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

		dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
		                                     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_BUTTONS_OK,
		                                     _("Could not open help document"),
						     error->message);

                g_signal_connect (G_OBJECT (dialog),
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy), NULL);
		
                gtk_widget_show (dialog);
                g_error_free (error);
        }
}

void
click_check_button_cb (GtkWidget	*widget, 
	       	       gpointer 	data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		gtk_widget_show (interface.additional_constraints);
		gtk_window_set_geometry_hints (GTK_WINDOW (interface.main_window), 
		                               GTK_WIDGET (interface.main_window),
		                               &interface.geometry, 
	                                       GDK_HINT_MIN_SIZE);
	} 
	else {
		GdkGeometry 		default_geometry;
		
		default_geometry.min_height = MINIMUM_WINDOW_HEIGHT;
		default_geometry.min_width  = MINIMUM_WINDOW_WIDTH;
		
		gtk_widget_hide (interface.additional_constraints);
		gtk_window_set_geometry_hints (GTK_WINDOW (interface.main_window), 
		                               GTK_WIDGET (interface.main_window),
		                               &default_geometry, 
		                               GDK_HINT_MIN_SIZE);
	}
}

void
click_find_cb (GtkWidget	*widget, 
	       gpointer 	data)
{
	gchar *command;

	if (search_command.not_running_timeout) {
		return;
	}

	gtk_widget_set_sensitive (interface.stop_button, FALSE);
	gtk_widget_set_sensitive (interface.find_button, FALSE);

	command = build_search_command ();
	
	if (search_command.lock == FALSE) {
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
}

void
size_allocate_cb (GtkWidget	*widget,
		  GtkAllocation *allocation,
	      	  gpointer 	data)
{
	gtk_widget_set_size_request (interface.add_button,
			      	     allocation->width,
			             allocation->height);
}
void
add_constraint_cb (GtkWidget 	*widget, 
		   gpointer 	data)
{
	GtkWidget *menu;
	GtkWidget *item;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU(interface.constraint_menu));
	item = gtk_menu_get_active (GTK_MENU(menu));
	
	if (GTK_WIDGET_SENSITIVE(item)) {
		add_constraint (interface.selected_constraint, NULL, FALSE);
	}
}

void
remove_constraint_cb (GtkWidget 	*widget, 
		      gpointer 		data)
{
	SearchConstraint *constraint = data;
	
      	interface.geometry.min_height -= 35;
	
	gtk_window_set_geometry_hints (GTK_WINDOW(interface.main_window), 
				       GTK_WIDGET(interface.main_window),
				       &interface.geometry, GDK_HINT_MIN_SIZE);
	
	gtk_container_remove (GTK_CONTAINER(interface.constraint), widget->parent);
	
	interface.selected_constraints =
		g_list_remove(interface.selected_constraints, constraint);
	
	set_constraint_selected_state (constraint->constraint_id, FALSE);	
	set_constraint_gconf_boolean (constraint->constraint_id, FALSE);	
}

void
constraint_activate_cb (GtkWidget 	*widget, 
			gpointer 	data)
{
	if ((GTK_WIDGET_VISIBLE (interface.find_button)) && 
	    (GTK_WIDGET_SENSITIVE (interface.find_button))) {
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
constraint_menu_item_activate_cb (GtkWidget  *widget, 
                                  gpointer   data)
{
	interface.selected_constraint = (long)data;
}

void
constraint_entry_changed_cb (GtkWidget 	*widget, 
			     gpointer 	data)
{
	static gchar *look_in_folder_string;
	
	if (GTK_WIDGET_VISIBLE (interface.main_window) == FALSE) {
		return;
	}
		
	look_in_folder_string = 
		gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(interface.look_in_folder_entry), FALSE);

	if (look_in_folder_string == NULL) {
		gtk_widget_set_sensitive (interface.find_button, FALSE);
		g_free (look_in_folder_string);
		return;
	}
	
	gtk_widget_set_sensitive (interface.find_button, TRUE);
	g_free (look_in_folder_string);
}
	

void
file_is_named_activate_cb (GtkWidget 	*widget, 
			   gpointer 	data)
{
	if ((GTK_WIDGET_VISIBLE (interface.find_button)) && 
	    (GTK_WIDGET_SENSITIVE (interface.find_button))) {
		click_find_cb (interface.find_button, NULL);
	}
}

void
open_file_cb (GtkWidget 	*widget, 
	      gpointer 		data)
{
	GList *list;
	guint index;

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) == 0) {
		return;
	}
	
	list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION(interface.selection),
						     (GtkTreeModel **)&interface.model);
	
	if (g_list_length (list) > SILENT_WINDOW_OPEN_LIMIT) {
	
		GtkWidget *dialog;
		gchar     *title;
		gchar     *message;
		gint      response;

		title = g_strdup_printf (_("Open %d documents?"), g_list_length (list));
		message = g_strdup_printf (_("This will open %d separate documents. Are you sure you want to do this?"),
		                           g_list_length (list));

		dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
			                             0 /* flags */,
						     GTK_BUTTONS_OK_CANCEL,
						     title,
						     message);
		
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_free (title);        
		g_free (message);
		
		if (response == GTK_RESPONSE_CANCEL) {
			g_list_free (list);
			return;
		}
	}
		
	for (index = 0; index < g_list_length(list); index++) {
	
		gboolean no_files_found = FALSE;
		gchar *utf8_name;
		gchar *utf8_path;
		GtkTreeIter iter;
			
		gtk_tree_model_get_iter (GTK_TREE_MODEL(interface.model), &iter, 
					 g_list_nth(list, index)->data);

		gtk_tree_model_get (GTK_TREE_MODEL(interface.model), &iter,
    			    COLUMN_NAME, &utf8_name,
		    	    COLUMN_PATH, &utf8_path,
		    	    COLUMN_NO_FILES_FOUND, &no_files_found,
		   	    -1);		    

		if (!no_files_found) {
			gchar *file;
			gchar *locale_file;
				
			file = g_build_filename (utf8_path, utf8_name, NULL);
			locale_file = g_locale_from_utf8 (file, -1, NULL, NULL, NULL);
			
			if (is_component_action_type (locale_file) 
		    	    && is_nautilus_running ()) {
				open_file_with_nautilus (locale_file);
			}
			else {
				if (open_file_with_application (locale_file) == FALSE) {
				
					if (launch_file (locale_file) == FALSE) {
						GtkWidget *dialog;
						gchar     *title;
						gchar     *text;

						title = g_strdup_printf (_("Could not open document \"%s\""),
									 g_path_get_basename (file));
						text = g_strdup (_("There is no installed viewer capable of displaying the document."));	
							
						dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
						                                     GTK_DIALOG_DESTROY_WITH_PARENT,
										     GTK_BUTTONS_OK,
										     title,
										     text);
                				g_signal_connect (G_OBJECT (dialog),
                        				"response",
                        				G_CALLBACK (gtk_widget_destroy), NULL);

	                			gtk_widget_show (dialog);
						g_free (title);
						g_free (text);
					}
				}
			} 
			g_free (file);
			g_free (locale_file);
		}
		g_free (utf8_name);
		g_free (utf8_path);
	} 
	g_list_free (list);
}

void
open_folder_cb (GtkWidget 	*widget, 
		gpointer 	data)
{
	GList *list;
	guint index;
	
	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) == 0) {
		return;
	}
	
	list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION(interface.selection),
						     (GtkTreeModel **)&interface.model);
	
	if (g_list_length (list) > SILENT_WINDOW_OPEN_LIMIT) {
		GtkWidget *dialog;
		gchar     *title;
		gchar     *message;
		gint      response;

		title = g_strdup_printf (_("Open %d folders?"), g_list_length (list));
		message = g_strdup_printf (_("This will open %d separate folders. Are you sure you want to do this?"),
		                           g_list_length (list));

		dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
			                             0 /* flags */,
						     GTK_BUTTONS_OK_CANCEL,
						     title,
								  message);		
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_free (title); 
		g_free (message);
				
		if (response == GTK_RESPONSE_CANCEL) {
			g_list_free (list);
			return;
		}
	}
		
	for (index = 0; index < g_list_length(list); index++) {
		
		gchar *folder_locale;
		gchar *folder_utf8;
		GtkTreeIter iter;
		
		gtk_tree_model_get_iter (GTK_TREE_MODEL(interface.model), &iter, 
					 g_list_nth(list, index)->data);
					 
		gtk_tree_model_get (GTK_TREE_MODEL(interface.model), &iter,
				    COLUMN_PATH, &folder_utf8,
				    -1);
			    							
		folder_locale = g_filename_from_utf8 (folder_utf8, -1, NULL, NULL, NULL);
		
		if (is_nautilus_running ()) {
			open_file_with_nautilus (folder_locale);
		}
		else {
			GtkWidget *dialog;
			gchar *title;
			
			title = g_strdup_printf (_("Could not open folder \"%s\""), folder_utf8);
						       
			dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
		                                             GTK_DIALOG_DESTROY_WITH_PARENT,
				  		             GTK_BUTTONS_OK,
							     title,
							     _("The nautilus file manager is not running."));
			
			g_signal_connect (G_OBJECT (dialog),
                        		  "response",
                        		  G_CALLBACK (gtk_widget_destroy), NULL);

	                gtk_widget_show (dialog);			
			g_free (folder_locale);
			g_free (folder_utf8);
			g_list_free (list);
			return;
		}
		g_free (folder_locale);
		g_free (folder_utf8);
	}
	g_list_free (list);
}

gboolean
file_button_press_event_cb (GtkWidget 		*widget, 
			    GdkEventButton 	*event, 
			    gpointer 		data)
{
	GtkTreePath *path;
	row_selected_by_button_press_event = TRUE;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW(interface.tree))) {
		return FALSE;
	}
	
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(interface.tree), event->x, event->y,
		&path, NULL, NULL, NULL)) {
		
		if ((event->button == 1 || event->button == 2 || event->button == 3)
			&& gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree)), path)) {
			
			row_selected_by_button_press_event = FALSE;
		}
		
		gtk_tree_path_free (path);
	}
	else {
		gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree)));
	}
	
	return !(row_selected_by_button_press_event);
}

gboolean
file_key_press_event_cb  (GtkWidget 		*widget, 
		    	  GdkEventKey	 	*event, 
		     	  gpointer 		data)
{		
	if (event->keyval == GDK_space || event->keyval == GDK_Return) {
		open_file_cb (widget, data);
		return TRUE;
	}
	return FALSE;
}

gboolean
file_button_release_event_cb (GtkWidget 	*widget, 
		      	      GdkEventButton 	*event, 
		      	      gpointer 		data)
{
	gboolean no_files_found = FALSE;	
	GtkTreeIter iter;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW(interface.tree))) {
		return FALSE;
	}
	
	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) == 0) {
		return FALSE;
	}
		
	if (event->button == 1 || event->button == 2) {

		GtkTreePath *path;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(interface.tree), event->x, event->y,
			&path, NULL, NULL, NULL)) {
			if ((event->state & GDK_SHIFT_MASK) || (event->state & GDK_CONTROL_MASK)) {
				if (row_selected_by_button_press_event) {
					gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree)), path);
				}
				else {
					gtk_tree_selection_unselect_path (gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree)), path);
				}
			}
			else {
				gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree)));
				gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree)), path);
			}
		}

		gtk_tree_path_free (path);
	}	
	if (event->button == 3) {	
		
		GtkWidget *popup;
		GList *list;
		
		list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION(interface.selection),
							     (GtkTreeModel **)&interface.model);
		
		gtk_tree_model_get_iter (GTK_TREE_MODEL(interface.model), &iter, 
					 g_list_first (list)->data);
					 
		gtk_tree_model_get (GTK_TREE_MODEL(interface.model), &iter,
			    	    COLUMN_NO_FILES_FOUND, &no_files_found,
			   	    -1);		    
				     
		if (!no_files_found) {
			GtkWidget *save_widget;
				
			popup = gnome_popup_menu_new (popup_menu);
			save_widget = popup_menu[3].widget;
			
			if (search_command.running != NOT_RUNNING) {		    	
		        	gtk_widget_set_sensitive (save_widget, FALSE);
			}
			else {
				gtk_widget_set_sensitive (save_widget, TRUE);
			}
			
			gnome_popup_menu_do_popup (GTK_WIDGET (popup), NULL, NULL, 
						   (GdkEventButton *)event, data, NULL);
						   
		} 
		g_list_free (list);
	}	
	return FALSE;
}

gboolean
file_event_after_cb  (GtkWidget 	*widget, 
		      GdkEventButton 	*event, 
		      gpointer 		data)
{	
	GtkTreeIter 	 iter;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW(interface.tree))) {
		return FALSE;
	}
	
	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) == 0) {
		return FALSE;
	}
		
	if (event->type == GDK_2BUTTON_PRESS) {

		gboolean no_files_found = FALSE;
		gchar *utf8_name;
		gchar *utf8_path;	 
		GList *list;
		
		list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION(interface.selection),
							     (GtkTreeModel **)&interface.model);
		
		gtk_tree_model_get_iter (GTK_TREE_MODEL(interface.model), &iter, 
					 g_list_first (list)->data);
					 
		gtk_tree_model_get (GTK_TREE_MODEL(interface.model), &iter,
    				    COLUMN_NAME, &utf8_name,
			    	    COLUMN_PATH, &utf8_path,
			    	    COLUMN_NO_FILES_FOUND, &no_files_found,
			   	    -1);	
		
		if (!no_files_found) {
			
			gchar *file;
			gchar *locale_file;
			
			file = g_build_filename (utf8_path, utf8_name, NULL);
			locale_file = g_locale_from_utf8 (file, -1, NULL, NULL, NULL);
			
			if (is_component_action_type (locale_file) 
			    && is_nautilus_running ()) {
				open_file_with_nautilus (locale_file);
			}
			else {
				if (open_file_with_application (locale_file) == FALSE) {
				
					if (launch_file (locale_file) == FALSE) {
						GtkWidget *dialog;
						gchar     *title;
						gchar     *text;

						title = g_strdup_printf (_("Could not open document \"%s\""), g_path_get_basename (file));
						text = g_strdup (_("There is no installed viewer capable of displaying the document."));	
							
						dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
						                                     GTK_DIALOG_DESTROY_WITH_PARENT,
										     GTK_BUTTONS_OK,
										     title,
										     text);

                				g_signal_connect (G_OBJECT (dialog),
                        				"response",
                        				G_CALLBACK (gtk_widget_destroy), NULL);

	                			gtk_widget_show (dialog);
						g_free (title);
						g_free (text);
					}
				}
			} 
			g_free (file);
			g_free (locale_file);
		}
		g_free (utf8_name);
		g_free (utf8_path);
		g_list_free (list);
	}
	return FALSE;
}

void  
drag_begin_file_cb  (GtkWidget          *widget,
		     GdkDragContext     *context,
		     GtkSelectionData   *selection_data,
		     guint               info,
		     guint               time,
		     gpointer            data)
{	
	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) > 1) {
		gtk_drag_set_icon_stock (context, GTK_STOCK_DND_MULTIPLE, 0, 0);
	}
	else {
		gtk_drag_set_icon_stock (context, GTK_STOCK_DND, 0, 0);
	}
}

void  
drag_file_cb  (GtkWidget          *widget,
	       GdkDragContext     *context,
	       GtkSelectionData   *selection_data,
	       guint               info,
	       guint               time,
	       gpointer            data)
{	
	gchar    	*uri_list = NULL;
	GList 		*list;
	GtkTreeIter 	iter;
	guint 		index;	

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) == 0) {
		return;
	}
	
	list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION(interface.selection),
						     (GtkTreeModel **)&interface.model);
	
	for (index = 0; index < g_list_length (list); index++) {
			   	   
		gboolean   no_files_found = FALSE;
		gchar	   *utf8_name;
		gchar	   *utf8_path;
		gchar	   *file;
		
		gtk_tree_model_get_iter (GTK_TREE_MODEL(interface.model), &iter, 
					 g_list_nth(list, index)->data);
					 
		gtk_tree_model_get (GTK_TREE_MODEL(interface.model), &iter,
    			    COLUMN_NAME, &utf8_name,
		    	    COLUMN_PATH, &utf8_path,
		    	    COLUMN_NO_FILES_FOUND, &no_files_found,
		   	    -1);	
			    
		file = g_build_filename (utf8_path, utf8_name, NULL);

		if (!no_files_found) {
			gchar *tmp_uri = g_filename_to_uri (file, NULL, NULL);
			
			if (uri_list == NULL) {
				uri_list = g_strdup (tmp_uri);
			}
			else{
				uri_list = g_strconcat (uri_list, "\n", tmp_uri, NULL);
			}
			
			gtk_selection_data_set (selection_data, 
						selection_data->target,
						8, 
						uri_list, 
						strlen (uri_list));
			g_free (tmp_uri);
		} 
		else {
			gtk_selection_data_set_text (selection_data, utf8_name, -1);
		}	
		g_free (utf8_name);
		g_free (utf8_path);
		g_free (file);
	}
	g_list_free (list);
	g_free (uri_list);
}

void  
drag_data_animation_cb (GtkWidget          *widget,
                        GdkDragContext     *context,
                        GtkSelectionData   *selection_data,
                        guint               info,
                        guint               time,
                        gpointer            data)
{
	GnomeDesktopItem *ditem;
	GString          *command = g_string_new ("");
	gchar            *uri;
	gchar            *path;
	gchar            *disk;
	gchar            *scheme;
	gchar		 *desktop_item_name = NULL;
	gchar            **argv;
	gint             argc, i;
	
	set_clone_command (&argc, &argv, "gnome-search-tool", TRUE);
	
	if (argc == 0) {
		return;
	}
	
	for (i = 0; i < argc; i++) {
		command = g_string_append (command, argv[i]);
		command = g_string_append_c (command, ' ');
	}
	command = g_string_append (command, "--start");
		
	disk = g_locale_from_utf8 (command->str, -1, NULL, NULL, NULL);
	uri = gnome_vfs_make_uri_from_input_with_dirs (disk, GNOME_VFS_MAKE_URI_DIR_HOMEDIR);
	scheme = gnome_vfs_get_uri_scheme (uri);
	
	ditem = gnome_desktop_item_new ();
	
	gnome_desktop_item_set_entry_type (ditem, GNOME_DESKTOP_ITEM_TYPE_APPLICATION);
	gnome_desktop_item_set_string (ditem, GNOME_DESKTOP_ITEM_EXEC, command->str);
	
	desktop_item_name = get_desktop_item_name ();
	
	gnome_desktop_item_set_string (ditem, GNOME_DESKTOP_ITEM_NAME, desktop_item_name);
	gnome_desktop_item_set_boolean (ditem, GNOME_DESKTOP_ITEM_TERMINAL, FALSE);					
	gnome_desktop_item_set_string (ditem, GNOME_DESKTOP_ITEM_ICON, GNOME_SEARCH_TOOL_ICON);
	gnome_desktop_item_set_boolean (ditem, "StartupNotify", TRUE);
	
	g_string_free (command, TRUE);
	g_free (desktop_item_name);
	g_free (uri);

	path = gsearchtool_unique_filename (g_get_tmp_dir(), ".desktop");
	gnome_desktop_item_set_location (ditem, path);

	uri = gnome_vfs_get_uri_from_local_path (path);
		
	if (gnome_desktop_item_save (ditem, NULL, FALSE, NULL)) {
		gtk_selection_data_set (selection_data,
					selection_data->target, 8,
					uri, strlen (uri));
	}
	gnome_desktop_item_unref (ditem);

	g_free (uri);
	g_free (path);
	g_free (disk);
	g_free (scheme);
}

void
show_file_selector_cb (GtkWidget 	*widget, 
		       gpointer		data)
{
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
	gtk_window_set_position (GTK_WINDOW (interface.file_selector), GTK_WIN_POS_CENTER_ON_PARENT);

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
	gchar *utf8;
	
	store = interface.model;
	
	interface.save_results_file = (gchar *)gtk_file_selection_get_filename (GTK_FILE_SELECTION (interface.file_selector));
	utf8 = g_filename_to_utf8 (interface.save_results_file, -1, NULL, NULL, NULL);
	
	if (g_file_test (interface.save_results_file, G_FILE_TEST_EXISTS)) {
		
		if (g_file_test (interface.save_results_file, G_FILE_TEST_IS_DIR)) {
			GtkWidget *dialog;
			gchar *title_msg;
			gchar *error_msg;
			
			title_msg = g_strdup_printf (_("Could not save document \"\" to %s"), 
		                                     interface.save_results_file);
					     
			error_msg = g_strdup_printf (_("You did not select a document name or the name selected was a folder.  The document is not saved."));

			dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
			                                     GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_BUTTONS_OK,
							     title_msg,
							     error_msg);

			g_signal_connect (G_OBJECT (dialog),
					"response",
					G_CALLBACK (gtk_widget_destroy), NULL);
					
			gtk_widget_show (dialog);
			g_free (title_msg);
			g_free (error_msg);
			
			return;	
		}
		else {
			GtkWidget *dialog;
			gchar     *text;
			gint      response;
				 
			text = g_strdup_printf (_("The contents of the file \"%s\" will be lost."), utf8);
			
			dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
			                                     0 /* flags */,
							     GTK_BUTTONS_OK_CANCEL,
							     _("Overwrite an existing file?"),
							     text);
			
			response = gtk_dialog_run (GTK_DIALOG (dialog));
		
			gtk_widget_destroy (GTK_WIDGET(dialog));
			g_free (text);
			
			if (response != GTK_RESPONSE_OK) return;
		}
	}
	
	if ((fp = fopen (interface.save_results_file, "w")) != NULL) {
	
		if (gtk_tree_model_get_iter_root(GTK_TREE_MODEL(store), &iter)) {
			
			n_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL);
			
			for (i = 0; i < n_children; i++)
			{
				gchar *utf8_path, *utf8_name, *utf8_file, *locale_file;
				
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_PATH, &utf8_path, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_NAME, &utf8_name, -1);
				gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);	
   
				utf8_file = g_build_filename (utf8_path, utf8_name, NULL);					    
				locale_file = g_filename_from_utf8 (utf8_file, -1, NULL, NULL, NULL);	
				fprintf(fp, "%s\n", locale_file);
				
				g_free(utf8_path);
				g_free(utf8_name);
				g_free(utf8_file);
				g_free(locale_file);
			}
		}		 
		fclose(fp);
	} 
	else {
		GtkWidget *dialog;
		gchar *title_msg;
		gchar *error_msg;
		
		title_msg = g_strdup_printf (_("Could not save document \"%s\" to %s"), 
		                             g_path_get_basename (utf8), g_path_get_dirname (utf8));
					     
		error_msg = g_strdup_printf ("%s",
		_("You may not have write permissions to the document.  The document is not saved."));
						    						
		dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
		                                     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_BUTTONS_OK,
						     title_msg,
						     error_msg);

		g_signal_connect (G_OBJECT (dialog),
				"response",
				G_CALLBACK (gtk_widget_destroy), NULL);
					
		gtk_widget_show (dialog);
		g_free (title_msg);
		g_free (error_msg);
	}
	g_free (utf8);
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
	char **argv;
	int argc;

	set_clone_command (&argc, &argv, client_data, FALSE);
	gnome_client_set_clone_command (client, argc, argv);
	gnome_client_set_restart_command (client, argc, argv);	
}

gboolean
key_press_cb (GtkWidget    	*widget, 
	      GdkEventKey	*event, 
	      gpointer 		data)
{
	g_return_val_if_fail (GTK_IS_WIDGET(widget), FALSE);

	if (event->keyval == GDK_Escape) {
		if (search_command.running == RUNNING) {
			click_stop_cb (widget, NULL);
		}
		else if (search_command.not_running_timeout == FALSE) {
			quit_cb (widget, NULL);
		}
	}
	else if (event->keyval == GDK_F10) {
		if (event->state & GDK_SHIFT_MASK) {
			gboolean no_files_found = FALSE;
			GtkWidget *popup;
			GtkTreeIter iter;
			GList *list;
			
			if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION(interface.selection)) == 0) {
				return FALSE;
			}
		
			list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION(interface.selection),
						     (GtkTreeModel **)&interface.model);
			
			gtk_tree_model_get_iter (GTK_TREE_MODEL(interface.model), &iter, 
						 g_list_first(list)->data);
		
			gtk_tree_model_get (GTK_TREE_MODEL(interface.model), &iter,
					    COLUMN_NO_FILES_FOUND, &no_files_found,
				    	    -1);
				    
			if (!no_files_found) {
				popup = gnome_popup_menu_new (popup_menu);
		    
				gnome_popup_menu_do_popup (GTK_WIDGET (popup), NULL, NULL,
							   (GdkEventButton *)event, data, NULL);
			}
		}
	}
	return FALSE;
}

gboolean
look_in_folder_key_press_cb (GtkWidget    	*widget, 
		     	     GdkEventKey	*event, 
		     	     gpointer 		data)
{
	g_return_val_if_fail (GTK_IS_WIDGET(widget), FALSE);

	if (event->keyval == GDK_Return)
	{
		if ((GTK_WIDGET_VISIBLE (interface.find_button)) && 
		    (GTK_WIDGET_SENSITIVE (interface.find_button))) {
			click_find_cb (interface.find_button, NULL);
		}
	}
	return FALSE;
}

gboolean
not_running_timeout_cb (gpointer data)
{
	search_command.not_running_timeout = FALSE;
	return FALSE;
}
