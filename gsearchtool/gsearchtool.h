/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool.h
 *
 *  (C) 1998,2002 the Free Software Foundation 
 *
 *  Authors:	George Lebl
 *		Dennis Cranston  <dennis_cranston@yahoo.com>
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

#ifndef _GSEARCHTOOL_H_
#define _GSEARCHTOOL_H_

#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
#define G_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#define GNOME_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#pragma }
#endif

typedef enum {
	SEARCH_CONSTRAINT_END, 
	SEARCH_CONSTRAINT_BOOL, 
	SEARCH_CONSTRAINT_TEXT,
	SEARCH_CONSTRAINT_NUMBER,
	SEARCH_CONSTRAINT_TIME,
	SEARCH_CONSTRAINT_SEPARATOR
} SearchConstraintType;

typedef enum {
	NOT_RUNNING,
	RUNNING,
	MAKE_IT_STOP,
	MAKE_IT_QUIT
} RunStatus;

typedef enum {
	COLUMN_ICON,
	COLUMN_NAME,
	COLUMN_PATH,
	COLUMN_READABLE_SIZE,
	COLUMN_SIZE,
	COLUMN_TYPE,
	COLUMN_READABLE_DATE,
	COLUMN_DATE,
	COLUMN_NO_FILES_FOUND,
	NUM_COLUMNS
} ResultColumn;
	  
struct _SearchStruct {
	gint			pid;
	gint 	        	timeout;
	gchar			*look_in_folder;
	gchar           	*file_is_named_pattern;
	gchar 	  		*regex_string;
	gboolean		lock;	
	gboolean		show_hidden_files;
	gboolean		regex_matching_enabled;
	RunStatus        	running;
} search_command;

struct _InterfaceStruct {
	GtkWidget		*file_is_named_entry;
	GtkWidget 		*look_in_folder_entry;
	GtkWidget		*find_button;
	GtkWidget		*stop_button;
	GtkWidget 		*save_button;
	GtkWidget 		*main_window;	
	GtkWidget		*table;	
	GtkWidget 		*file_selector;
	GtkWidget 		*status_bar;
	GtkWidget   		*progress_bar;
	GtkWidget	 	*disclosure;
	GtkWidget		*add_button;
	GtkWidget 		*additional_constraints;
	GtkWidget		*constraint_menu;
	GtkWidget 		*constraint;
	GtkWidget       	*results;
	GtkWidget        	*tree;
	GtkListStore     	*model;	
	GtkTreeSelection 	*selection;
	GtkTreeIter       	iter;
	GdkGeometry 		geometry;
	GtkSizeGroup 	 	*constraint_size_group;
	GList 			*selected_constraints;
	gchar 		 	*save_results_file;	
	gint 		  	selected_constraint;
	GnomeIconTheme	 	*icon_theme;
	gboolean  	  	is_gail_loaded;
} interface;

typedef struct _SearchConstraint SearchConstraint;
struct _SearchConstraint {
	gint constraint_id;	/* the index of the template this uses */
	union {
		gchar 	*text; 	/* this is a char string of data */
		gint 	time; 	/* time data   */
		gint 	number; /* number data */
	} data;
};

gchar *
build_search_command 		(void);

void
spawn_search_command 		(gchar *command);

void  		
add_constraint 			(gint constraint_id,
				 gchar *value);

void  		
remove_constraint 		(gint constraint_id);

void  		
update_constraint_info 		(SearchConstraint *constraint, 
				 gchar *info);
void
set_constraint_selected_state	(gint		constraint_id, 
				 gboolean	state);
gboolean
update_progress_bar 		(gpointer data);

#ifdef __cplusplus
}
#endif

#endif
