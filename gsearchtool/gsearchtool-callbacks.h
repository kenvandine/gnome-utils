/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool-callbacks.h
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
 
#ifndef _GSEARCHTOOL_CALLBACKS_H_
#define _GSEARCHTOOL_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#pragma }
#endif


void	
die_cb 				(GnomeClient 	*client, 
				 gpointer 	data);						 						 
void	
quit_cb 			(GtkWidget 	*widget, 
				 gpointer 	data);
void    
help_cb 			(GtkWidget 	*widget, 
				 gpointer 	data);
void   
click_find_cb	 		(GtkWidget 	*widget, 
				 gpointer 	data);
void
click_stop_cb 			(GtkWidget 	*widget, 
	       			 gpointer 	data);
void
click_check_button_cb		(GtkWidget	*widget, 
				 gpointer 	data);
void   
size_allocate_cb 		(GtkWidget 	*widget,
				 GtkAllocation  *allocation, 
				 gpointer 	data);
void    
add_constraint_cb		(GtkWidget 	*widget, 
				 gpointer 	data);
void    
remove_constraint_cb		(GtkWidget 	*widget, 
				 gpointer 	data);
void    
constraint_activate_cb 		(GtkWidget 	*widget, 
				 gpointer 	data);
void    
constraint_update_info_cb 	(GtkWidget 	*widget, 
				 gpointer 	data);	
void    
constraint_menu_item_activate_cb(GtkWidget 	*widget,
                                 gpointer 	data);
void    
constraint_entry_changed_cb 	(GtkWidget 	*widget, 
				 gpointer 	data);
void    
file_is_named_activate_cb 	(GtkWidget 	*widget, 
				 gpointer 	data);
void 	
open_file_cb 			(GtkWidget 	*widget, 
				 gpointer 	data);
void	
open_folder_cb 			(GtkWidget 	*widget, 
				 gpointer 	data);
void  
drag_begin_file_cb  		(GtkWidget          *widget,
	      			 GdkDragContext     *context,
	       			 GtkSelectionData   *selection_data,
	       			 guint               info,
	       			 guint               time,
	       			 gpointer            data);
void  
drag_file_cb  			(GtkWidget          *widget,
	      			 GdkDragContext     *context,
	       			 GtkSelectionData   *selection_data,
	       			 guint               info,
	       			 guint               time,
	       			 gpointer            data);
void  
drag_data_animation_cb		(GtkWidget          *widget,
	      			 GdkDragContext     *context,
	       			 GtkSelectionData   *selection_data,
	       			 guint               info,
	       			 guint               time,
	       			 gpointer            data);
void    
show_file_selector_cb 		(GtkWidget 	*widget,
				 gpointer 	data);
void   	
save_results_cb 		(GtkFileSelection   *selector, 
				 gpointer 	    user_data, 
				 gpointer 	    data);
void    
save_session_cb 		(GnomeClient 	    *client, 
				 gint 		    phase,
  				 GnomeRestartStyle  save_style, 
				 gint 		    shutdown,
      				 GnomeInteractStyle interact_style, 
				 gint 		    fast, 
				 gpointer 	    client_data);
gboolean  
key_press_cb 			(GtkWidget 	*widget, 
				 GdkEventKey 	*event,
				 gpointer 	data);	
gboolean    
look_in_folder_key_press_cb 	(GtkWidget 	*widget, 
				 GdkEventKey    *event,
				 gpointer 	data);
				 
gboolean
file_button_release_event_cb	(GtkWidget 	*widget,
				 GdkEventButton *event,
				 gpointer 	data);

gboolean
file_event_after_cb	        (GtkWidget 	*widget,
				 GdkEventButton *event,
				 gpointer 	data);  
gboolean	
file_button_press_event_cb	(GtkWidget 	*widget, 
				 GdkEventButton *event, 
				 gpointer 	data);
gboolean	
file_key_press_event_cb		(GtkWidget 	*widget,
				 GdkEventKey    *event,
				 gpointer 	data);
gboolean
not_running_timeout_cb 		(gpointer data);

#ifdef __cplusplus
}
#endif

#endif
