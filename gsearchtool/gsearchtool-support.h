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

#include "gsearchtool.h"

#define ICON_SIZE 24

gboolean 
gsearchtool_gconf_get_boolean (const gchar * key);

void
gsearchtool_gconf_set_boolean (const gchar * key, 
                               const gboolean flag);
char *
gsearchtool_gconf_get_string (const gchar * key);

GSList * 
gsearchtool_gconf_get_list (const gchar * key);

gboolean  	
is_path_hidden (const gchar * path);

gboolean  	
is_quick_search_excluded_path (const gchar * path);

gboolean  	
is_second_scan_excluded_path (const gchar * path);

gboolean  	
compare_regex (const gchar * regex, 
               const gchar * string);
gboolean  	
limit_string_to_x_lines (GString * string, 
                         gint x);
gchar *	
escape_single_quotes (const gchar * string);

gchar *	
backslash_special_characters (const gchar * string);

gchar *
remove_mnemonic_character (const gchar * string);

gchar *   	
get_readable_date (const gchar * format_string,
                   const time_t file_time_raw);
gchar *    	
gsearchtool_strdup_strftime (const gchar * format, 
                             struct tm * time_pieces); 
const char *
get_file_type_for_mime_type (const gchar * file,
                             const gchar * mime);
GdkPixbuf *
get_file_pixbuf_for_mime_type (GHashTable * hash,
                               const gchar * file,
                               const gchar * mime);
gboolean 	
open_file_with_nautilus (GtkWidget * window,
                         const gchar * file);
gboolean  	
open_file_with_application (GtkWidget * window,
                            const gchar * file);
gboolean
launch_file (const gchar * file);

gchar *
gsearchtool_get_unique_filename (const gchar * path,
                                 const gchar * suffix);
GtkWidget *
gsearchtool_button_new_with_stock_icon (const gchar * string,
                                        const gchar * stock_id);
#ifdef __cplusplus
}
#endif

#endif /* _GSEARCHTOOL_SUPPORT_H */
