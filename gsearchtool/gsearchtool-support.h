/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool-support.h
 *
 *  (C) 2002 the Free Software Foundation 
 *
 *  Authors:	Dennis Cranston  <dennis_cranston@yahoo.com>
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

#ifndef _GSEARCHTOOL_SUPPORT_H_
#define _GSEARCHTOOL_SUPPORT_H_

#ifdef __cplusplus
extern "C" {
#pragma }
#endif

#include <gnome.h>

#define CDDB_DISCLOSURE_TYPE (cddb_disclosure_get_type ())
#define CDDB_DISCLOSURE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDDB_DISCLOSURE_TYPE, CDDBDisclosure))
#define CDDB_DISCLOSURE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CDDB_DISCLOSURE_TYPE, CDDBDisclosureClass))
#define IS_CDDB_DISCLOSURE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDDB_DISCLOSURE_TYPE))
#define IS_CDDB_DISCLOSURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDDB_DISCLOSURE_TYPE))
#define ICON_SIZE 24.0

typedef struct _CDDBDisclosure CDDBDisclosure;
typedef struct _CDDBDisclosureClass CDDBDisclosureClass;
typedef struct _CDDBDisclosurePrivate CDDBDisclosurePrivate;

struct _CDDBDisclosure {
	GtkCheckButton parent;
	CDDBDisclosurePrivate *priv;
};

struct _CDDBDisclosureClass {
	GtkCheckButtonClass parent_class;
};

GType 		
cddb_disclosure_get_type 	(void);

GtkWidget *
cddb_disclosure_new 		(const char *shown,
				 const char *hidden);
void
cddb_disclosure_set_container 	(CDDBDisclosure *disclosure,
				 GtkWidget *container);
gboolean  	
is_path_hidden 			(const gchar *path);

gboolean  	
is_path_in_home_folder 		(const gchar *path);

gboolean  	
is_path_in_mount_folder		(const gchar *path);

gboolean
file_extension_is 		(const char *filename, 
		   		 const char *ext);
gboolean  	
compare_regex	 		(const gchar *regex, 
				 const gchar *string);
gboolean  	
limit_string_to_x_lines		(GString *string, 
				 gint x);
gchar *	
escape_single_quotes 		(const gchar *string);

gint 	 	
count_of_char_in_string		(const gchar *string, 
				 const gchar q);
gchar *    	
get_date 			(const time_t file_time_raw);

gchar *   	
get_readable_date 		(const time_t file_time_raw);

gchar *    	
strdup_strftime 		(const gchar *format, 
				 struct tm *time_pieces); 
gchar *
get_file_type_with_mime_type	(const gchar *filename,
				 const gchar *mimetype);
gchar *
get_file_icon_with_mime_type 	(const gchar *filename,
				 const gchar *mimetype);
gboolean  	
is_nautilus_running 		(void);

gboolean
is_component_action_type 	(const gchar *filename);

gboolean 	
open_file_with_nautilus 	(const gchar *filename);

gboolean  	
open_file_with_application 	(const gchar *filename);

gboolean
launch_file 			(const gchar *filename);

#ifdef __cplusplus
}
#endif

#endif
