/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool.c
 *
 *  (C) 1998,2002 the Free Software Foundation 
 *
 *  Authors:    Dennis Cranston  <dennis_cranston@yahoo.com>
 *              George Lebl
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

#include <fnmatch.h>
#ifndef FNM_CASEFOLD
#  define FNM_CASEFOLD 0
#endif

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include <libbonobo.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <gnome.h>

#include "gsearchtool.h"
#include "gsearchtool-callbacks.h"
#include "gsearchtool-support.h"
#include "gsearchtool-spinner.h"

#define GNOME_SEARCH_TOOL_DEFAULT_ICON_SIZE 48
#define GNOME_SEARCH_TOOL_STOCK "panel-searchtool"
#define GNOME_SEARCH_TOOL_REFRESH_DURATION  50000
#define LEFT_LABEL_SPACING "     "

static GObjectClass * parent_class;

typedef enum {
	SEARCH_CONSTRAINT_TYPE_BOOLEAN,
	SEARCH_CONSTRAINT_TYPE_NUMERIC,
	SEARCH_CONSTRAINT_TYPE_TEXT,
	SEARCH_CONSTRAINT_TYPE_DATE_BEFORE,
	SEARCH_CONSTRAINT_TYPE_DATE_AFTER,
	SEARCH_CONSTRAINT_TYPE_SEPARATOR,
	SEARCH_CONSTRAINT_TYPE_NONE
} GSearchConstraintType;

typedef struct _GSearchOptionTemplate GSearchOptionTemplate;

struct _GSearchOptionTemplate {
	GSearchConstraintType type; /* The available option type */
	gchar * option;             /* An option string to pass to the command */
	gchar * desc;               /* The description for display */
	gchar * units;              /* Optional units for display */
	gboolean is_selected;
};

static GSearchOptionTemplate GSearchOptionTemplates[] = {
	{ SEARCH_CONSTRAINT_TYPE_TEXT, "'!' -type p -exec grep -c '%s' {} \\;", N_("Contains the _text"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_SEPARATOR, NULL, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TYPE_DATE_BEFORE, "-mtime -%d", N_("_Date modified less than"), N_("days"), FALSE },
	{ SEARCH_CONSTRAINT_TYPE_DATE_AFTER, "\\( -mtime +%d -o -mtime %d \\)", N_("Date modified more than"), N_("days"), FALSE },
	{ SEARCH_CONSTRAINT_TYPE_SEPARATOR, NULL, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TYPE_NUMERIC, "\\( -size %uc -o -size +%uc \\)", N_("S_ize at least"), N_("kilobytes"), FALSE },
	{ SEARCH_CONSTRAINT_TYPE_NUMERIC, "\\( -size %uc -o -size -%uc \\)", N_("Si_ze at most"), N_("kilobytes"), FALSE },
	{ SEARCH_CONSTRAINT_TYPE_BOOLEAN, "-size 0c \\( -type f -o -type d \\)", N_("File is empty"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_SEPARATOR, NULL, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TYPE_TEXT, "-user '%s'", N_("Owned by _user"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_TEXT, "-group '%s'", N_("Owned by _group"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_BOOLEAN, "\\( -nouser -o -nogroup \\)", N_("Owner is unrecognized"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_SEPARATOR, NULL, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TYPE_TEXT, "'!' -name '*%s*'", N_("Na_me does not contain"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_TEXT, "-regex '%s'", N_("Name matches regular e_xpression"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_SEPARATOR, NULL, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TYPE_BOOLEAN, "SHOW_HIDDEN_FILES", N_("Show hidden and backup files"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_BOOLEAN, "-follow", N_("Follow symbolic links"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_BOOLEAN, "INCLUDE_OTHER_FILESYSTEMS", N_("Include other filesystems"), NULL, FALSE },
	{ SEARCH_CONSTRAINT_TYPE_NONE, NULL, NULL, NULL, FALSE}
}; 

enum {
	SEARCH_CONSTRAINT_CONTAINS_THE_TEXT,
	SEARCH_CONSTRAINT_TYPE_SEPARATOR_00,
	SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE,
	SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER,
	SEARCH_CONSTRAINT_TYPE_SEPARATOR_01,
	SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN,	
	SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN,
	SEARCH_CONSTRAINT_FILE_IS_EMPTY,
	SEARCH_CONSTRAINT_TYPE_SEPARATOR_02,
	SEARCH_CONSTRAINT_OWNED_BY_USER,
	SEARCH_CONSTRAINT_OWNED_BY_GROUP,
	SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED,
	SEARCH_CONSTRAINT_TYPE_SEPARATOR_03,	
	SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED,
	SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION,
	SEARCH_CONSTRAINT_TYPE_SEPARATOR_04,
	SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS,
	SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS,
	SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS,
	SEARCH_CONSTRAINT_MAXIMUM_POSSIBLE
};

static GtkTargetEntry GSearchDndTable[] = {
	{ "STRING",        0, 0 },
	{ "text/plain",    0, 0 },
	{ "text/uri-list", 0, 1 }
};

static guint GSearchTotalDnds = sizeof (GSearchDndTable) / sizeof (GSearchDndTable[0]);

struct _GSearchPoptArgument {
	gchar * name;
	gchar * path;
	gchar * contains;
	gchar * user;
	gchar * group;
	gboolean nouser;
	gchar * mtimeless;
	gchar * mtimemore;
	gchar * sizeless;	
	gchar * sizemore;
	gboolean empty;
	gchar * notnamed;
	gchar * regex;
	gboolean hidden;
	gboolean follow;
	gboolean allmounts;
	gchar * sortby;
	gboolean descending;
	gboolean start;
} GSearchPoptArgument;

struct poptOption GSearchPoptOptions[] = { 
  	{ "named", '\0', POPT_ARG_STRING, &GSearchPoptArgument.name, 0, NULL, NULL},
	{ "path", '\0', POPT_ARG_STRING, &GSearchPoptArgument.path, 0, NULL, NULL},
  	{ "sortby", '\0', POPT_ARG_STRING, &GSearchPoptArgument.sortby, 0, NULL, NULL},
  	{ "descending", '\0', POPT_ARG_NONE, &GSearchPoptArgument.descending, 0, NULL, NULL},
  	{ "start", '\0', POPT_ARG_NONE, &GSearchPoptArgument.start, 0, NULL, NULL},
  	{ "contains", '\0', POPT_ARG_STRING, &GSearchPoptArgument.contains, 0, NULL, NULL},
  	{ "mtimeless", '\0', POPT_ARG_STRING, &GSearchPoptArgument.mtimeless, 0, NULL, NULL},
  	{ "mtimemore", '\0', POPT_ARG_STRING, &GSearchPoptArgument.mtimemore, 0, NULL, NULL},
	{ "sizemore", '\0', POPT_ARG_STRING, &GSearchPoptArgument.sizemore, 0, NULL, NULL},
	{ "sizeless", '\0', POPT_ARG_STRING, &GSearchPoptArgument.sizeless, 0, NULL, NULL},
	{ "empty", '\0', POPT_ARG_NONE, &GSearchPoptArgument.empty, 0, NULL, NULL},
  	{ "user", '\0', POPT_ARG_STRING, &GSearchPoptArgument.user, 0, NULL, NULL},
  	{ "group", '\0', POPT_ARG_STRING, &GSearchPoptArgument.group, 0, NULL, NULL},
  	{ "nouser", '\0', POPT_ARG_NONE, &GSearchPoptArgument.nouser, 0, NULL, NULL},
  	{ "notnamed", '\0', POPT_ARG_STRING, &GSearchPoptArgument.notnamed, 0, NULL, NULL},
  	{ "regex", '\0', POPT_ARG_STRING, &GSearchPoptArgument.regex, 0, NULL, NULL},
	{ "hidden", '\0', POPT_ARG_NONE, &GSearchPoptArgument.hidden, 0, NULL, NULL},
  	{ "follow", '\0', POPT_ARG_NONE, &GSearchPoptArgument.follow, 0, NULL, NULL},
  	{ "allmounts", '\0', POPT_ARG_NONE, &GSearchPoptArgument.allmounts, 0, NULL, NULL},
  	{ NULL,'\0', 0, NULL, 0, NULL, NULL}
};

static GtkActionEntry GSearchUiEntries[] = {
  { "Open",          GTK_STOCK_OPEN,    N_("_Open"),               NULL, NULL, NULL },
  { "OpenFolder",    GTK_STOCK_OPEN,    N_("O_pen Folder"),        NULL, NULL, NULL },
  { "MoveToTrash",   GTK_STOCK_DELETE,  N_("Mo_ve to Trash"),      NULL, NULL, NULL },
  { "SaveResultsAs", GTK_STOCK_SAVE_AS, N_("_Save Results As..."), NULL, NULL, NULL },
};

static const char * GSearchUiDescription =
"<ui>"
"  <popup name='PopupMenu'>"
"      <menuitem action='Open'/>"
"      <menuitem action='OpenFolder'/>"
"      <separator/>"
"      <menuitem action='MoveToTrash'/>"
"      <separator/>"
"      <menuitem action='SaveResultsAs'/>"
"  </popup>"
"</ui>";

static gchar * find_command_default_name_argument;
static gchar * locate_command_default_options;

static void
setup_case_insensitive_arguments (void)
{
	static gboolean case_insensitive_arguments_setup = TRUE;
	gchar * cmd_stderr;

	if (case_insensitive_arguments_setup != TRUE) {
		return;
	}
	case_insensitive_arguments_setup = FALSE;

	/* check find command for -iname argument compatibility */
	g_spawn_command_line_sync ("find /dev/null -iname 'string'", NULL, &cmd_stderr, NULL, NULL);

	if ((cmd_stderr != NULL) && (strlen (cmd_stderr) == 0)) {
		find_command_default_name_argument = g_strdup ("-iname");
		GSearchOptionTemplates[SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED].option = g_strdup ("'!' -iname '*%s*'");
	}
	else {
		find_command_default_name_argument = g_strdup ("-name");
	}
	g_free (cmd_stderr);

	/* check grep command for -i argument compatibility */
	g_spawn_command_line_sync ("grep -i 'string' /dev/null", NULL, &cmd_stderr, NULL, NULL);

	if ((cmd_stderr != NULL) && (strlen (cmd_stderr) == 0)) {
		GSearchOptionTemplates[SEARCH_CONSTRAINT_CONTAINS_THE_TEXT].option = g_strdup ("'!' -type p -exec grep -i -c '%s' {} \\;");
	}
	g_free (cmd_stderr);

	/* check locate command for -i argument compatibility */
	g_spawn_command_line_sync ("locate -i /dev/null/string", NULL, &cmd_stderr, NULL, NULL);

	if ((cmd_stderr != NULL) && (strlen (cmd_stderr) == 0)) {
		locate_command_default_options = g_strdup ("-i");
	}
	else {
		locate_command_default_options = g_strdup ("");
	}
	g_free (cmd_stderr);
}
  
static gchar *
setup_find_name_options (gchar * file)
{
	/* This function builds the name options for the find command.  This in
	   done to insure that the find command returns hidden files and folders. */

	GString * command;
	command = g_string_new ("");

	if (strstr (file, "*") == NULL) {

		if ((strlen (file) == 0) || (file[0] != '.')) {
	 		g_string_append_printf (command, "\\( %s '*%s*' -o %s '.*%s*' \\) ", 
					find_command_default_name_argument, file,
					find_command_default_name_argument, file);
		}
		else {
			g_string_append_printf (command, "\\( %s '*%s*' -o %s '.*%s*' -o %s '%s*' \\) ", 
					find_command_default_name_argument, file,
					find_command_default_name_argument, file,
					find_command_default_name_argument, file);
		}
	}
	else {
		if (file[0] == '.') {
			g_string_append_printf (command, "\\( %s '%s' -o %s '.*%s' \\) ", 
					find_command_default_name_argument, file,
					find_command_default_name_argument, file);	
		}
		else if (file[0] != '*') {
			g_string_append_printf (command, "%s '%s' ", 
					find_command_default_name_argument, file);
		}
		else {
			if ((strlen (file) >= 1) && (file[1] == '.')) {
				g_string_append_printf (command, "\\( %s '%s' -o %s '%s' \\) ", 
					find_command_default_name_argument, file,
					find_command_default_name_argument, &file[1]);
			}
			else {
				g_string_append_printf (command, "\\( %s '%s' -o %s '.%s' \\) ", 
					find_command_default_name_argument, file,
					find_command_default_name_argument, file);
			}
		}
	}
	return g_string_free (command, FALSE);
}

static gboolean
has_additional_constraints (GSearchWindow * gsearch)
{
	GList * list;
	
	if (gsearch->available_options_selected_list != NULL) {
	
		for (list = gsearch->available_options_selected_list; list != NULL; list = g_list_next (list)) {
			
			GSearchConstraint * constraint = list->data;
		
			switch (GSearchOptionTemplates[constraint->constraint_id].type) {
			case SEARCH_CONSTRAINT_TYPE_BOOLEAN:
			case SEARCH_CONSTRAINT_TYPE_NUMERIC:
			case SEARCH_CONSTRAINT_TYPE_DATE_BEFORE:	
			case SEARCH_CONSTRAINT_TYPE_DATE_AFTER:
				return TRUE;
			case SEARCH_CONSTRAINT_TYPE_TEXT:
				if (strlen (constraint->data.text) > 0) {
					return TRUE;
				} 
			default:
				break;
			}
		}
	}
	return FALSE;
}

gchar *
build_search_command (GSearchWindow * gsearch,
                      gboolean first_pass)
{
	GString * command;
	gchar * file_is_named_utf8;
	gchar * file_is_named_locale;
	gchar * file_is_named_escaped;
	gchar * file_is_named_backslashed;
	gchar * look_in_folder_utf8;
	gchar * look_in_folder_locale;

	setup_case_insensitive_arguments ();
	
	file_is_named_utf8 = g_strdup ((gchar *) gtk_entry_get_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (gsearch->name_contains_entry)))));

	if (!file_is_named_utf8 || !*file_is_named_utf8) {
		g_free (file_is_named_utf8);
		file_is_named_utf8 = g_strdup ("*");
	} 
	else {
		gchar * locale;

		locale = g_locale_from_utf8 (file_is_named_utf8, -1, NULL, NULL, NULL);
		gnome_entry_prepend_history (GNOME_ENTRY (gsearch->name_contains_entry), TRUE, file_is_named_utf8);

		if (strstr (locale, "*") == NULL) {
			gchar *tmp;
		
			tmp = file_is_named_utf8;
			file_is_named_utf8 = g_strconcat ("*", file_is_named_utf8, "*", NULL);
			g_free (tmp);
		}
		
		g_free (locale);
	}
	file_is_named_locale = g_locale_from_utf8 (file_is_named_utf8, -1, NULL, NULL, NULL);
	
	look_in_folder_utf8 = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (gsearch->look_in_folder_button));

	if (look_in_folder_utf8 != NULL) {
		look_in_folder_locale = g_locale_from_utf8 (look_in_folder_utf8, -1, NULL, NULL, NULL);
		g_free (look_in_folder_utf8);
	}
	else {
		/* If for some reason a path was not returned fallback to the user's home directory. */
		look_in_folder_locale = g_strdup (g_get_home_dir ());
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gsearch->look_in_folder_button), look_in_folder_locale);
	}

	if (!g_str_has_suffix (look_in_folder_locale, G_DIR_SEPARATOR_S)) {
		gchar *tmp;
		
		tmp = look_in_folder_locale;
		look_in_folder_locale = g_strconcat (look_in_folder_locale, G_DIR_SEPARATOR_S, NULL);
		g_free (tmp);
	}
	gsearch->command_details->look_in_folder_string = g_strdup (look_in_folder_locale);
	
	command = g_string_new ("");
	gsearch->command_details->is_command_show_hidden_files_enabled = FALSE;
	gsearch->command_details->name_contains_regex_string = NULL;
	gsearch->search_results_date_format_string = NULL;
	gsearch->command_details->name_contains_pattern_string = NULL;
	
	gsearch->command_details->is_command_first_pass = first_pass;
	if (gsearch->command_details->is_command_first_pass == TRUE) {
		gsearch->command_details->is_command_using_quick_mode = FALSE;
	}
	
	if ((GTK_WIDGET_VISIBLE (gsearch->available_options_vbox) == FALSE) ||
	    (has_additional_constraints (gsearch) == FALSE)) {
	
		file_is_named_backslashed = backslash_special_characters (file_is_named_locale);
		file_is_named_escaped = escape_single_quotes (file_is_named_backslashed);
		gsearch->command_details->name_contains_pattern_string = g_strdup (file_is_named_utf8);
		
		if (gsearch->command_details->is_command_first_pass == TRUE) {
		
			gchar * locate;
			gboolean disable_quick_search;
		
			locate = g_find_program_in_path ("locate");
			disable_quick_search = gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/disable_quick_search");		
			gsearch->command_details->is_command_second_pass_enabled = !gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/disable_quick_search_second_pass");
		
			if ((disable_quick_search == FALSE) 
			    && (locate != NULL) 
			    && (is_quick_search_excluded_path (look_in_folder_locale) == FALSE)) {
			    
					g_string_append_printf (command, "%s %s '%s*%s'", 
							locate,
							locate_command_default_options,
							look_in_folder_locale,
							file_is_named_escaped);
					gsearch->command_details->is_command_using_quick_mode = TRUE;
			}
			else {		
				g_string_append_printf (command, "find \"%s\" %s '%s' -xdev -print", 
							look_in_folder_locale, 
							find_command_default_name_argument, 
							file_is_named_escaped);
			}
			g_free (locate);
		}
		else {
			g_string_append_printf (command, "find \"%s\" %s '%s' -xdev -print", 
						look_in_folder_locale, 
						find_command_default_name_argument, 
						file_is_named_escaped);
		}
	}
	else {
		GList * list;
		gboolean disable_mount_argument = FALSE;
		
		gsearch->command_details->is_command_regex_matching_enabled = FALSE;
		file_is_named_backslashed = backslash_special_characters (file_is_named_locale);
		file_is_named_escaped = escape_single_quotes (file_is_named_backslashed);
		
		g_string_append_printf (command, "find \"%s\" %s", 
					look_in_folder_locale,
					setup_find_name_options (file_is_named_escaped));
	
		for (list = gsearch->available_options_selected_list; list != NULL; list = g_list_next (list)) {
			
			GSearchConstraint * constraint = list->data;
						
			switch (GSearchOptionTemplates[constraint->constraint_id].type) {
			case SEARCH_CONSTRAINT_TYPE_BOOLEAN:
				if (strcmp (GSearchOptionTemplates[constraint->constraint_id].option, "INCLUDE_OTHER_FILESYSTEMS") == 0) {
					disable_mount_argument = TRUE;
				}
				else if (strcmp (GSearchOptionTemplates[constraint->constraint_id].option, "SHOW_HIDDEN_FILES") == 0) {
					gsearch->command_details->is_command_show_hidden_files_enabled = TRUE;
				}
				else {
					g_string_append_printf (command, "%s ",
						GSearchOptionTemplates[constraint->constraint_id].option);
				}
				break;
			case SEARCH_CONSTRAINT_TYPE_TEXT:
				if (strcmp (GSearchOptionTemplates[constraint->constraint_id].option, "-regex '%s'") == 0) {
					
					gchar * escaped;
					gchar * regex;
					
					escaped = backslash_special_characters (constraint->data.text);
					regex = escape_single_quotes (escaped);
					
					if (regex != NULL) {	
						gsearch->command_details->is_command_regex_matching_enabled = TRUE;
						gsearch->command_details->name_contains_regex_string = g_locale_from_utf8 (regex, -1, NULL, NULL, NULL);
					}
					
					g_free (escaped);
					g_free (regex);
				} 
				else {
					gchar * escaped;
					gchar * backslashed;
					gchar * locale;
					
					backslashed = backslash_special_characters (constraint->data.text);
					escaped = escape_single_quotes (backslashed);
					
					locale = g_locale_from_utf8 (escaped, -1, NULL, NULL, NULL);
					
					if (strlen (locale) != 0) { 
						g_string_append_printf (command,
							  	        GSearchOptionTemplates[constraint->constraint_id].option,
						  		        locale);

						g_string_append_c (command, ' ');
					}
					
					g_free (escaped);
					g_free (backslashed);
					g_free (locale);					
				}
				break;
			case SEARCH_CONSTRAINT_TYPE_NUMERIC:
				g_string_append_printf (command,
					  		GSearchOptionTemplates[constraint->constraint_id].option,
							(constraint->data.number * 1024),
					  		(constraint->data.number * 1024));
				g_string_append_c (command, ' ');
				break;
			case SEARCH_CONSTRAINT_TYPE_DATE_BEFORE:
				g_string_append_printf (command,
					 		GSearchOptionTemplates[constraint->constraint_id].option,
					  		constraint->data.time);
				g_string_append_c (command, ' ');
				break;	
			case SEARCH_CONSTRAINT_TYPE_DATE_AFTER:
				g_string_append_printf (command,
					 		GSearchOptionTemplates[constraint->constraint_id].option,
					  		constraint->data.time,
					  		constraint->data.time);
				g_string_append_c (command, ' ');
				break;
			default:
		        	break;
			}
		}
		gsearch->command_details->name_contains_pattern_string = g_strdup ("*");
		
		if (disable_mount_argument != TRUE) {
			g_string_append (command, "-xdev ");
		}
		
		g_string_append (command, "-print ");
	}
	g_free (file_is_named_locale);
	g_free (file_is_named_utf8);
	g_free (file_is_named_backslashed);
	g_free (file_is_named_escaped);
	g_free (look_in_folder_locale);

	return g_string_free (command, FALSE);
}

static void
add_file_to_search_results (const gchar * file, 
			    GtkListStore * store, 
			    GtkTreeIter * iter,
			    GSearchWindow * gsearch)
{					
	const char * mime_type; 
	const char * description;
	GdkPixbuf * pixbuf;
	GnomeVFSFileInfo * vfs_file_info;
	gchar * readable_size;
	gchar * readable_date;
	gchar * utf8_base_name;
	gchar * utf8_dir_name;
	gchar * utf8_relative_dir_name;
	gchar * base_name;
	gchar * dir_name;
	gchar * relative_dir_name;
	gchar * look_in_folder;
	
	if (g_hash_table_lookup_extended (gsearch->search_results_filename_hash_table, file, NULL, NULL) == TRUE) {
		return;
	}
	
	g_hash_table_insert (gsearch->search_results_filename_hash_table, g_strdup (file), NULL);
	
	mime_type = gnome_vfs_get_file_mime_type (file, NULL, FALSE);
	 
	description = get_file_type_for_mime_type (file, mime_type);
	pixbuf = get_file_pixbuf_for_mime_type (gsearch->search_results_pixbuf_hash_table, file, mime_type);
	
	if (gtk_tree_view_get_headers_visible (GTK_TREE_VIEW (gsearch->search_results_tree_view)) == FALSE) {
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (gsearch->search_results_tree_view), TRUE);
	}
	
	vfs_file_info = gnome_vfs_file_info_new ();
	gnome_vfs_get_file_info (file, vfs_file_info, GNOME_VFS_FILE_INFO_DEFAULT);
	readable_size = gnome_vfs_format_file_size_for_display (vfs_file_info->size);
	readable_date = get_readable_date (gsearch->search_results_date_format_string, vfs_file_info->mtime);
	
	base_name = g_path_get_basename (file);
	dir_name = g_path_get_dirname (file);
	
	look_in_folder = g_strdup (gsearch->command_details->look_in_folder_string);
	if (strlen (look_in_folder) > 1) {
		gchar * path;

		if (g_str_has_suffix (look_in_folder, G_DIR_SEPARATOR_S) == TRUE) {
			look_in_folder[strlen (look_in_folder) - 1] = '\0'; 
		}
		path = g_path_get_dirname (look_in_folder);
		relative_dir_name = g_strconcat (&dir_name[strlen (path) + 1], NULL);
		g_free (path);
	}
	else {
		relative_dir_name = g_strdup (dir_name);
	}

	utf8_base_name = g_locale_to_utf8 (base_name, -1, NULL, NULL, NULL);
	utf8_dir_name = g_locale_to_utf8 (dir_name, -1, NULL, NULL, NULL);
	utf8_relative_dir_name = g_locale_to_utf8 (relative_dir_name, -1, NULL, NULL, NULL); 
	
	gtk_list_store_append (GTK_LIST_STORE (store), iter); 
	gtk_list_store_set (GTK_LIST_STORE (store), iter,
			    COLUMN_ICON, pixbuf, 
			    COLUMN_NAME, utf8_base_name,
			    COLUMN_RELATIVE_PATH, utf8_relative_dir_name,
			    COLUMN_PATH, utf8_dir_name,
			    COLUMN_READABLE_SIZE, readable_size,
			    COLUMN_SIZE, (-1) * (gdouble) vfs_file_info->size,
			    COLUMN_TYPE, (description != NULL) ? description : mime_type,
			    COLUMN_READABLE_DATE, readable_date,
			    COLUMN_DATE, (-1) * (gdouble) vfs_file_info->mtime,
			    COLUMN_NO_FILES_FOUND, FALSE,
			    -1);

	gnome_vfs_file_info_unref (vfs_file_info);
	g_free (base_name);
	g_free (dir_name);
	g_free (relative_dir_name);
	g_free (utf8_base_name);
	g_free (utf8_dir_name);
	g_free (utf8_relative_dir_name);
	g_free (look_in_folder);
	g_free (readable_size);
	g_free (readable_date);
}

static void
add_no_files_found_message (GSearchWindow * gsearch)
{
	/* When the list is empty append a 'No Files Found.' message. */
	gtk_widget_set_sensitive (GTK_WIDGET (gsearch->search_results_tree_view), FALSE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (gsearch->search_results_tree_view), FALSE);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (gsearch->search_results_tree_view));
	g_object_set (gsearch->search_results_name_cell_renderer,
	              "underline", PANGO_UNDERLINE_NONE,
	              "underline-set", FALSE,
	              NULL);
	gtk_list_store_append (GTK_LIST_STORE (gsearch->search_results_list_store), &gsearch->search_results_iter); 
	gtk_list_store_set (GTK_LIST_STORE (gsearch->search_results_list_store), &gsearch->search_results_iter,
		    	    COLUMN_ICON, NULL, 
			    COLUMN_NAME, _("No files found"),
			    COLUMN_RELATIVE_PATH, "",
		    	    COLUMN_PATH, "",
			    COLUMN_READABLE_SIZE, "",
			    COLUMN_SIZE, (gdouble) 0,
			    COLUMN_TYPE, "",
		    	    COLUMN_READABLE_DATE, "",
		    	    COLUMN_DATE, (gdouble) 0,
			    COLUMN_NO_FILES_FOUND, TRUE,
		    	    -1);
}

void
update_search_counts (GSearchWindow * gsearch)
{
	gchar * title_bar_string = NULL;
	gchar * message_string = NULL;
	gchar * stopped_string = NULL;
	gchar * tmp;
	gint total_files;

	if (gsearch->command_details->command_status == ABORTED) {
		stopped_string = g_strdup (_("(stopped)"));
	}
	
	total_files = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (gsearch->search_results_list_store), NULL);

	if (total_files == 0) {
		title_bar_string = g_strdup (_("No Files Found"));
		message_string = g_strdup (_("No files found"));
		add_no_files_found_message (gsearch);
	}
	else {
		title_bar_string = g_strdup_printf (ngettext ("%d File Found",
					                      "%d Files Found",
					                      total_files),
						    total_files);
		message_string = g_strdup_printf (ngettext ("%d file found",
					                    "%d files found",
					                    total_files),
						  total_files);
	}

	if (stopped_string != NULL) {
		tmp = message_string;
		message_string = g_strconcat (message_string, " ", stopped_string, NULL);
		g_free (tmp);
		
		tmp = title_bar_string;
		title_bar_string = g_strconcat (title_bar_string, " ", stopped_string, NULL);
		g_free (tmp);
	}
		
	tmp = title_bar_string;
	title_bar_string = g_strconcat (title_bar_string, " - ", _("Search for Files"), NULL);
	gtk_window_set_title (GTK_WINDOW (gsearch->window), title_bar_string);
	g_free (tmp);
	
	gtk_label_set_text (GTK_LABEL (gsearch->files_found_label), message_string);
	
	g_free (title_bar_string);
	g_free (message_string);
	g_free (stopped_string);
}

static void
intermediate_file_count_update (GSearchWindow * gsearch)
{
	gchar * string;
	gint count;
	
	count = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (gsearch->search_results_list_store), NULL);
	
	if (count > 0) {
		
		string = g_strdup_printf (ngettext ("%d file found",
		                                    "%d files found",
		                                    count),
		                          count);
					  
		gtk_label_set_text (GTK_LABEL (gsearch->files_found_label), string);
		g_free (string);
	}
}

static GtkTreeModel *
gsearch_create_list_of_templates (void)
{
	GtkListStore * store;
	GtkTreeIter iter;
	gint index;

	store = gtk_list_store_new (1, G_TYPE_STRING);

	for (index = 0; GSearchOptionTemplates[index].type != SEARCH_CONSTRAINT_TYPE_NONE; index++) {
		
		if (GSearchOptionTemplates[index].type == SEARCH_CONSTRAINT_TYPE_SEPARATOR) {
		        gtk_list_store_append (store, &iter);
		        gtk_list_store_set (store, &iter, 0, "separator", -1);
		} 
		else {
			gchar * text = remove_mnemonic_character (_(GSearchOptionTemplates[index].desc));
			gtk_list_store_append (store, &iter);
		        gtk_list_store_set (store, &iter, 0, text, -1);
			g_free (text);
		}
	}
	return GTK_TREE_MODEL (store);
}

static void
set_constraint_info_defaults (GSearchConstraint * opt)
{	
	switch (GSearchOptionTemplates[opt->constraint_id].type) {
	case SEARCH_CONSTRAINT_TYPE_BOOLEAN:
		break;
	case SEARCH_CONSTRAINT_TYPE_TEXT:
		opt->data.text = "";
		break;
	case SEARCH_CONSTRAINT_TYPE_NUMERIC:
		opt->data.number = 0;
		break;
	case SEARCH_CONSTRAINT_TYPE_DATE_BEFORE:
	case SEARCH_CONSTRAINT_TYPE_DATE_AFTER:
		opt->data.time = 0;
		break;
	default:
	        break;
	}
}

void
update_constraint_info (GSearchConstraint * constraint, 
			gchar * info)
{
	switch (GSearchOptionTemplates[constraint->constraint_id].type) {
	case SEARCH_CONSTRAINT_TYPE_TEXT:
		constraint->data.text = info;
		break;
	case SEARCH_CONSTRAINT_TYPE_NUMERIC:
		sscanf (info, "%d", &constraint->data.number);
		break;
	case SEARCH_CONSTRAINT_TYPE_DATE_BEFORE:
	case SEARCH_CONSTRAINT_TYPE_DATE_AFTER:
		sscanf (info, "%d", &constraint->data.time);
		break;
	default:
		g_warning (_("Entry changed called for a non entry option!"));
		break;
	}
}

void
set_constraint_selected_state (GSearchWindow * gsearch,
                               gint constraint_id, 
			       gboolean state)
{
	gint index;
	
	GSearchOptionTemplates[constraint_id].is_selected = state;
	
	for (index = 0; GSearchOptionTemplates[index].type != SEARCH_CONSTRAINT_TYPE_NONE; index++) {
		if (GSearchOptionTemplates[index].is_selected == FALSE) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (gsearch->available_options_combo_box), index);
			gtk_widget_set_sensitive (gsearch->available_options_add_button, TRUE);
			gtk_widget_set_sensitive (gsearch->available_options_combo_box, TRUE);
			gtk_widget_set_sensitive (gsearch->available_options_label, TRUE);
			return;
		}
	}
	gtk_widget_set_sensitive (gsearch->available_options_add_button, FALSE);
	gtk_widget_set_sensitive (gsearch->available_options_combo_box, FALSE);
	gtk_widget_set_sensitive (gsearch->available_options_label, FALSE);
}

void
set_constraint_gconf_boolean (gint constraint_id,
                              gboolean flag)
{
	switch (constraint_id) {
	
		case SEARCH_CONSTRAINT_CONTAINS_THE_TEXT:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/contains_the_text",
	   		       	       	       	       flag);
			break;
		case SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/date_modified_less_than",
	   		       	       	       	       flag);
			break;
		case SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/date_modified_more_than",
	   		       	       	       	       flag);
			break;
		case SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/size_at_least",
	   		       	       		       flag);
			break;
		case SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/size_at_most",
	   		       	       	  	       flag);
			break;
		case SEARCH_CONSTRAINT_FILE_IS_EMPTY:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/file_is_empty",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_OWNED_BY_USER:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/owned_by_user",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_OWNED_BY_GROUP:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/owned_by_group",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/owner_is_unrecognized",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/name_does_not_contain",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/name_matches_regular_expression",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/show_hidden_files_and_folders",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/follow_symbolic_links",
	   		       	       	               flag);
			break;
		case SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS:
			gsearchtool_gconf_set_boolean ("/apps/gnome-search-tool/select/include_other_filesystems",
	   		       	       	               flag);
			break;
		
		default:
			break;
	}
}

/*
 * add_atk_namedesc
 * @widget    : The Gtk Widget for which @name and @desc are added.
 * @name      : Accessible Name
 * @desc      : Accessible Description
 * Description: This function adds accessible name and description to a
 *              Gtk widget.
 */

static void
add_atk_namedesc (GtkWidget * widget, 
		  const gchar * name, 
		  const gchar * desc)
{
	AtkObject * atk_widget;

	g_return_if_fail (GTK_IS_WIDGET (widget));

	atk_widget = gtk_widget_get_accessible (widget);

	if (name != NULL)
		atk_object_set_name (atk_widget, name);
	if (desc !=NULL)
		atk_object_set_description (atk_widget, desc);
}

/*
 * add_atk_relation
 * @obj1      : The first widget in the relation @rel_type
 * @obj2      : The second widget in the relation @rel_type.
 * @rel_type  : Relation type which relates @obj1 and @obj2
 * Description: This function establishes Atk Relation between two given
 *              objects.
 */

static void
add_atk_relation (GtkWidget * obj1, 
		  GtkWidget * obj2, 
		  AtkRelationType rel_type)
{
	AtkObject * atk_obj1, * atk_obj2;
	AtkRelationSet * relation_set;
	AtkRelation * relation;

	g_return_if_fail (GTK_IS_WIDGET (obj1));
	g_return_if_fail (GTK_IS_WIDGET (obj2));
	
	atk_obj1 = gtk_widget_get_accessible (obj1);
			
	atk_obj2 = gtk_widget_get_accessible (obj2);
	
	relation_set = atk_object_ref_relation_set (atk_obj1);
	relation = atk_relation_new (&atk_obj2, 1, rel_type);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (G_OBJECT (relation));
	
}

gchar *
get_desktop_item_name (GSearchWindow * gsearch)
{

	GString * gs;
	gchar * file_is_named_utf8;
	gchar * file_is_named_locale;
	gchar * look_in_folder_utf8;
	gchar * look_in_folder_locale;
	GList * list;

	gs = g_string_new ("");
	g_string_append (gs, _("Search for Files"));
	g_string_append (gs, " (");
	
	file_is_named_utf8 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (gsearch->name_contains_entry))));
	file_is_named_locale = g_locale_from_utf8 (file_is_named_utf8 != NULL ? file_is_named_utf8 : "" , 
	                                           -1, NULL, NULL, NULL);
	g_string_append (gs, g_strdup_printf ("named=%s", file_is_named_locale));
	g_free (file_is_named_locale);

	look_in_folder_utf8 = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (gsearch->look_in_folder_button));
	look_in_folder_locale = g_locale_from_utf8 (look_in_folder_utf8 != NULL ? look_in_folder_utf8 : "", 
	                                            -1, NULL, NULL, NULL);
	g_string_append (gs, g_strdup_printf ("&path=%s", look_in_folder_locale));
	g_free (look_in_folder_locale);
	g_free (look_in_folder_utf8); 

	if (GTK_WIDGET_VISIBLE (gsearch->available_options_vbox)) {
		for (list = gsearch->available_options_selected_list; list != NULL; list = g_list_next (list)) {
			GSearchConstraint * constraint = list->data;
			gchar * locale = NULL;
			
			switch (constraint->constraint_id) {
			case SEARCH_CONSTRAINT_CONTAINS_THE_TEXT:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				g_string_append (gs, g_strdup_printf ("&contains=%s", locale));
				break;
			case SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE:
				g_string_append (gs, g_strdup_printf ("&mtimeless=%d", constraint->data.time));
				break;
			case SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER:
				g_string_append (gs, g_strdup_printf ("&mtimemore=%d", constraint->data.time));
				break;
			case SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN:
				g_string_append (gs, g_strdup_printf ("&sizemore=%u", constraint->data.number));
				break;
			case SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN:
				g_string_append (gs, g_strdup_printf ("&sizeless=%u", constraint->data.number));
				break;
			case SEARCH_CONSTRAINT_FILE_IS_EMPTY:
				g_string_append (gs, g_strdup ("&empty"));
				break;
			case SEARCH_CONSTRAINT_OWNED_BY_USER:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				g_string_append (gs, g_strdup_printf ("&user=%s", locale));
				break;
			case SEARCH_CONSTRAINT_OWNED_BY_GROUP:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				g_string_append (gs, g_strdup_printf ("&group=%s", locale));
				break;
			case SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED:
				g_string_append (gs, g_strdup ("&nouser"));
				break;
			case SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				g_string_append (gs, g_strdup_printf ("&notnamed=%s", locale));
				break;
			case SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				g_string_append (gs, g_strdup_printf ("&regex=%s", locale));
				break;
			case SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS:
				g_string_append (gs, g_strdup ("&hidden"));
				break;
			case SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS:
				g_string_append (gs, g_strdup ("&follow"));
				break;
			case SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS:
				g_string_append (gs, g_strdup ("&allmounts"));
				break;
			default:
				break;
			}
		g_free (locale);
		}		
	}
	g_string_append_c (gs, ')');
	return g_string_free (gs, FALSE);
}

static void
start_animation (GSearchWindow * gsearch)
{
	if (gsearch->command_details->is_command_first_pass == TRUE) {
	
		gchar *title = NULL;
	
		title = g_strconcat (_("Searching..."), " - ", _("Search for Files"), NULL);
		gtk_window_set_title (GTK_WINDOW (gsearch->window), title);
	
		gtk_label_set_text (GTK_LABEL (gsearch->files_found_label), "");
		gsearch_spinner_start (GSEARCH_SPINNER (gsearch->progress_spinner));
	
		g_free (title);
	}
} 

static void
stop_animation (GSearchWindow * gsearch)
{
	gsearch_spinner_stop (GSEARCH_SPINNER (gsearch->progress_spinner));
} 

static void
define_popt_table_options (void) 
{
	gint i = 0;
	gint j;

	GSearchPoptOptions[i++].descrip = g_strdup (_("Set the text of 'Name contains'"));
	GSearchPoptOptions[i++].descrip = g_strdup (_("Set the text of 'Look in folder'"));
  	GSearchPoptOptions[i++].descrip = g_strdup (_("Sort files by one of the following: name, folder, size, type, or date"));
  	GSearchPoptOptions[i++].descrip = g_strdup (_("Set sort order to descending, the default is ascending"));
  	GSearchPoptOptions[i++].descrip = g_strdup (_("Automatically start a search"));

	for (j = 0; GSearchOptionTemplates[j].type != SEARCH_CONSTRAINT_TYPE_NONE; j++) {
		if (GSearchOptionTemplates[j].type != SEARCH_CONSTRAINT_TYPE_SEPARATOR) {
			gchar *text = remove_mnemonic_character (GSearchOptionTemplates[j].desc);
			GSearchPoptOptions[i++].descrip = g_strdup_printf (_("Select the '%s' constraint"), _(text));
			g_free (text);
		}
	}
}

static gboolean
handle_popt_args (GSearchWindow * gsearch)
{
	gboolean popt_args_found = FALSE;
	gint sort_by;

	if (GSearchPoptArgument.name != NULL) {
		popt_args_found = TRUE;
		gtk_entry_set_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (gsearch->name_contains_entry))),
				    g_locale_to_utf8 (GSearchPoptArgument.name, -1, NULL, NULL, NULL));
	}
	if (GSearchPoptArgument.path != NULL) {
		popt_args_found = TRUE;
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gsearch->look_in_folder_button),
		                               g_locale_to_utf8 (GSearchPoptArgument.path, -1, NULL, NULL, NULL));
	}	
	if (GSearchPoptArgument.contains != NULL) {
		popt_args_found = TRUE;	
		add_constraint (gsearch, SEARCH_CONSTRAINT_CONTAINS_THE_TEXT, 
				GSearchPoptArgument.contains, TRUE); 	
	}
	if (GSearchPoptArgument.mtimeless != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE, 
				GSearchPoptArgument.mtimeless, TRUE);		
	}
	if (GSearchPoptArgument.mtimemore != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER, 
				GSearchPoptArgument.mtimemore, TRUE);
	}
	if (GSearchPoptArgument.sizemore != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN,
				GSearchPoptArgument.sizemore, TRUE);
	}
	if (GSearchPoptArgument.sizeless != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN,
				GSearchPoptArgument.sizeless, TRUE);
	}
	if (GSearchPoptArgument.empty) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_FILE_IS_EMPTY, NULL, TRUE);
	}
	if (GSearchPoptArgument.user != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_OWNED_BY_USER, 
				GSearchPoptArgument.user, TRUE); 
	}
	if (GSearchPoptArgument.group != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_OWNED_BY_GROUP, 
				GSearchPoptArgument.group, TRUE);
	}
	if (GSearchPoptArgument.nouser) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED, NULL, TRUE); 
	}
	if (GSearchPoptArgument.notnamed != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED, 
				GSearchPoptArgument.notnamed, TRUE);
	}
	if (GSearchPoptArgument.regex != NULL) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION, 
				GSearchPoptArgument.regex, TRUE);
	}
	if (GSearchPoptArgument.hidden) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS, NULL, TRUE);
	}
	if (GSearchPoptArgument.follow) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS, NULL, TRUE); 
	}
	if (GSearchPoptArgument.allmounts) {
		popt_args_found = TRUE;
		add_constraint (gsearch, SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS, NULL, TRUE);
	}
	if (GSearchPoptArgument.sortby != NULL) {
	
		popt_args_found = TRUE;
		if (strcmp (GSearchPoptArgument.sortby, "name") == 0) {
			sort_by = COLUMN_NAME;
		}
		else if (strcmp (GSearchPoptArgument.sortby, "folder") == 0) {
			sort_by = COLUMN_PATH;
		}
		else if (strcmp (GSearchPoptArgument.sortby, "size") == 0) {
			sort_by = COLUMN_SIZE;
		}
		else if (strcmp (GSearchPoptArgument.sortby, "type") == 0) {
			sort_by = COLUMN_TYPE;
		}
		else if (strcmp (GSearchPoptArgument.sortby, "date") == 0) {
			sort_by = COLUMN_DATE;
		}
		else {
			g_warning (_("Invalid option passed to sortby command line argument.")); 
			sort_by = COLUMN_NAME;
		}
			
		if (GSearchPoptArgument.descending) {
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (gsearch->search_results_list_store), sort_by,
							      GTK_SORT_DESCENDING);
		} 
		else {
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (gsearch->search_results_list_store), sort_by, 
							      GTK_SORT_ASCENDING);
		}
	}
	if (GSearchPoptArgument.start) {
		popt_args_found = TRUE;
		click_find_cb (gsearch->find_button, (gpointer) gsearch);
	}
	return popt_args_found;
}

static gboolean
handle_search_command_stdout_io (GIOChannel * ioc, 
				 GIOCondition condition, 
				 gpointer data) 
{
	GSearchWindow * gsearch = data;
	gboolean broken_pipe = FALSE;

	if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) { 
	
		GError * error = NULL;
		GTimer * timer;
		GString * string;
		GdkRectangle prior_rect;
		GdkRectangle after_rect;
		gulong duration;
		gint look_in_folder_string_length;
		
		string = g_string_new (NULL);
		look_in_folder_string_length = strlen (gsearch->command_details->look_in_folder_string);

		timer = g_timer_new ();
		g_timer_start (timer);

		while (ioc->is_readable != TRUE);

		do {
			gchar * utf8 = NULL;
			gchar * filename = NULL;
			gint status;
			
			if (gsearch->command_details->command_status == MAKE_IT_STOP) {
				broken_pipe = TRUE;
				break;
			} 
			else if (gsearch->command_details->command_status != RUNNING) {
			 	break;
			}
			
			do {
				status = g_io_channel_read_line_string (ioc, string, NULL, &error);
				
				if (status == G_IO_STATUS_EOF) {
					broken_pipe = TRUE;
				}
				else if (status == G_IO_STATUS_AGAIN) {
					if (gtk_events_pending ()) {
						intermediate_file_count_update (gsearch);
						while (gtk_events_pending ()) {
							if (gsearch->command_details->command_status == MAKE_IT_QUIT) {
								return FALSE;
							}
							gtk_main_iteration (); 
						}	
 				
					}
				}
				
			} while (status == G_IO_STATUS_AGAIN && broken_pipe == FALSE);
			
			if (broken_pipe == TRUE) {
				break;
			}
			
			if (status != G_IO_STATUS_NORMAL) {
				if (error != NULL) {
					g_warning ("handle_search_command_stdout_io(): %s", error->message);
					g_error_free (error);
				}
				continue;
			}		
			
			string = g_string_truncate (string, string->len - 1);
			if (string->len <= 1) {
				continue;
			}
			
			utf8 = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
			if (utf8 == NULL) {
				continue;
			}
			
			if (strncmp (string->str, gsearch->command_details->look_in_folder_string, look_in_folder_string_length) == 0) { 
			
				if (strlen (string->str) != look_in_folder_string_length) {
			
					filename = g_path_get_basename (utf8);
			
					if (fnmatch (gsearch->command_details->name_contains_pattern_string, filename, FNM_NOESCAPE | FNM_CASEFOLD ) != FNM_NOMATCH) {
						if (gsearch->command_details->is_command_show_hidden_files_enabled) {
							if (gsearch->command_details->is_command_regex_matching_enabled == FALSE) {
								add_file_to_search_results (string->str, gsearch->search_results_list_store, &gsearch->search_results_iter, gsearch);
							} 
							else if (compare_regex (gsearch->command_details->name_contains_regex_string, filename)) {
								add_file_to_search_results (string->str, gsearch->search_results_list_store, &gsearch->search_results_iter, gsearch);
							}
						}
						else if ((is_path_hidden (string->str) == FALSE) && (!g_str_has_suffix (string->str, "~"))) {
							if (gsearch->command_details->is_command_regex_matching_enabled == FALSE) {
								add_file_to_search_results (string->str, gsearch->search_results_list_store, &gsearch->search_results_iter, gsearch);
							} 
							else if (compare_regex (gsearch->command_details->name_contains_regex_string, filename)) {
								add_file_to_search_results (string->str, gsearch->search_results_list_store, &gsearch->search_results_iter, gsearch);
							}
						}
					}
				}
			}
			g_free (utf8);
			g_free (filename);

			gtk_tree_view_get_visible_rect (GTK_TREE_VIEW (gsearch->search_results_tree_view), &prior_rect);
			
			if (prior_rect.y == 0) {
				gtk_tree_view_get_visible_rect (GTK_TREE_VIEW (gsearch->search_results_tree_view), &after_rect);
				if (after_rect.y <= 40) {  /* limit this hack to the first few pixels */
					gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (gsearch->search_results_tree_view), -1, 0);
				}	
			}

			g_timer_elapsed (timer, &duration);

			if (duration > GNOME_SEARCH_TOOL_REFRESH_DURATION) {
				if (gtk_events_pending ()) {
					intermediate_file_count_update (gsearch);
					while (gtk_events_pending ()) {
						if (gsearch->command_details->command_status == MAKE_IT_QUIT) {
							return FALSE;
						}
						gtk_main_iteration ();		
					}		
				}
				g_timer_reset (timer);
			}
			
		} while (g_io_channel_get_buffer_condition (ioc) == G_IO_IN);
		
		g_string_free (string, TRUE);
		g_timer_destroy (timer);
	}

	if (condition != G_IO_IN || broken_pipe == TRUE) { 

		g_io_channel_shutdown (ioc, TRUE, NULL);
		
		if ((gsearch->command_details->command_status != MAKE_IT_STOP)
		     && (gsearch->command_details->is_command_using_quick_mode == TRUE) 
		     && (gsearch->command_details->is_command_first_pass == TRUE) 
		     && (gsearch->command_details->is_command_second_pass_enabled == TRUE)
		     && (is_second_scan_excluded_path (gsearch->command_details->look_in_folder_string) == FALSE)) { 
		    
			gchar * command;
			
			command = build_search_command (gsearch, FALSE);
			spawn_search_command (gsearch, command);
			g_free (command);
		}
		else {
			gsearch->command_details->command_status = (gsearch->command_details->command_status == MAKE_IT_STOP) ? ABORTED : STOPPED;
			gsearch->command_details->is_command_timeout_enabled = TRUE;
			g_hash_table_destroy (gsearch->search_results_pixbuf_hash_table);
			g_hash_table_destroy (gsearch->search_results_filename_hash_table);
			g_timeout_add (500, not_running_timeout_cb, (gpointer) gsearch); 

			update_search_counts (gsearch); 
			stop_animation (gsearch);

			gtk_window_set_default (GTK_WINDOW (gsearch->window), gsearch->find_button);
			gtk_widget_set_sensitive (gsearch->available_options_vbox, TRUE);
			gtk_widget_set_sensitive (gsearch->show_more_options_expander, TRUE);
			gtk_widget_set_sensitive (gsearch->name_and_folder_table, TRUE);
			gtk_widget_set_sensitive (gsearch->find_button, TRUE);
			gtk_widget_set_sensitive (gsearch->search_results_save_results_as_item, TRUE);
			gtk_widget_hide (gsearch->stop_button);
			gtk_widget_show (gsearch->find_button); 

			/* Free the gchar fields of search_command structure. */
			g_free (gsearch->command_details->look_in_folder_string);
			g_free (gsearch->command_details->name_contains_pattern_string);
			g_free (gsearch->command_details->name_contains_regex_string);
			g_free (gsearch->search_results_date_format_string);
		}
		return FALSE;
	}
	return TRUE;
}

static gboolean
handle_search_command_stderr_io (GIOChannel * ioc, 
				 GIOCondition condition, 
				 gpointer data) 
{	
	GSearchWindow * gsearch = data;
	static GString * error_msgs = NULL;
	static gboolean truncate_error_msgs = FALSE;
	gboolean broken_pipe = FALSE;
	
	if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) { 
	
		GString * string;
		GError * error = NULL;
		gchar * utf8 = NULL;
		
		string = g_string_new (NULL);
		
		if (error_msgs == NULL) {
			error_msgs = g_string_new (NULL);
		}

		while (ioc->is_readable != TRUE);

		do {
			gint status;
		
			do {
				status = g_io_channel_read_line_string (ioc, string, NULL, &error);
	
				if (status == G_IO_STATUS_EOF) {
					broken_pipe = TRUE;
				}
				else if (status == G_IO_STATUS_AGAIN) {
					if (gtk_events_pending ()) {
						intermediate_file_count_update (gsearch);
						while (gtk_events_pending ()) {
							if (gsearch->command_details->command_status == MAKE_IT_QUIT) {
								break;
							}
							gtk_main_iteration ();

						}
					}
				}
				
			} while (status == G_IO_STATUS_AGAIN && broken_pipe == FALSE);
			
			if (broken_pipe == TRUE) {
				break;
			}
			
			if (status != G_IO_STATUS_NORMAL) {
				if (error != NULL) {
					g_warning ("handle_search_command_stderr_io(): %s", error->message);
					g_error_free (error);
				}
				continue;
			}
			
			if (truncate_error_msgs == FALSE) {
				if ((strstr (string->str, "ermission denied") == NULL) &&
			   	    (strstr (string->str, "No such file or directory") == NULL) &&
			   	    (strncmp (string->str, "grep: ", 6) != 0) &&
			 	    (strcmp (string->str, "find: ") != 0)) { 
					utf8 = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
					error_msgs = g_string_append (error_msgs, utf8); 
					truncate_error_msgs = limit_string_to_x_lines (error_msgs, 20);
				}
			}
		
		} while (g_io_channel_get_buffer_condition (ioc) == G_IO_IN);
		
		g_string_free (string, TRUE);
		g_free (utf8);
	}
	
	if (condition != G_IO_IN || broken_pipe == TRUE) { 
	
		if (error_msgs != NULL) {

			if (error_msgs->len > 0) {	
				
				GtkWidget * dialog;
				
				if (truncate_error_msgs) {
					error_msgs = g_string_append (error_msgs, 
				     		_("\n... Too many errors to display ..."));
				}
				
				if (gsearch->command_details->is_command_using_quick_mode != TRUE) {

					GtkWidget * hbox;
					GtkWidget * spacer;
					GtkWidget * expander;
					GtkWidget * label;

					dialog = gtk_message_dialog_new (GTK_WINDOW (gsearch->window),
					                                 GTK_DIALOG_DESTROY_WITH_PARENT,
					                                 GTK_MESSAGE_ERROR,
					                                 GTK_BUTTONS_OK,
					                                 _("The search results may be invalid."
									   "  There were errors while performing this search."));
					gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "");

					gtk_window_set_title (GTK_WINDOW (dialog), "");
					gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

					hbox = gtk_hbox_new (0, FALSE);

					spacer = gtk_label_new ("     ");
					gtk_box_pack_start (GTK_BOX (hbox), spacer, FALSE, FALSE, 0);

					expander = gtk_expander_new_with_mnemonic (_("Show more _details"));
					gtk_container_set_border_width (GTK_CONTAINER (expander), 6);
					gtk_expander_set_spacing (GTK_EXPANDER (expander), 6);
					gtk_box_pack_start (GTK_BOX (hbox), expander, TRUE, TRUE, 0);

					label = gtk_label_new (error_msgs->str);
					gtk_container_add (GTK_CONTAINER (expander), label);

					gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
					gtk_widget_show_all (hbox);
					
					g_signal_connect (G_OBJECT (dialog),
					                  "response",
					                   G_CALLBACK (gtk_widget_destroy), NULL);
					
					gtk_widget_show (dialog);
				}
				else {
					GtkWidget * button;
					GtkWidget * hbox;
					GtkWidget * spacer;
					GtkWidget * expander;
					GtkWidget * label;

					dialog = gtk_message_dialog_new (GTK_WINDOW (gsearch->window),
					                                 GTK_DIALOG_DESTROY_WITH_PARENT,
					                                 GTK_MESSAGE_ERROR,
					                                 GTK_BUTTONS_CANCEL,
					                                 _("The search results may be invalid."
									   "  Do you want to disable the quick search feature?"));
					gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "");

					gtk_window_set_title (GTK_WINDOW (dialog), "");
					gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

					hbox = gtk_hbox_new (0, FALSE);

					spacer = gtk_label_new ("     ");
					gtk_box_pack_start (GTK_BOX (hbox), spacer, FALSE, FALSE, 0);

					expander = gtk_expander_new_with_mnemonic (_("Show more _details"));
					gtk_container_set_border_width (GTK_CONTAINER (expander), 6);
					gtk_expander_set_spacing (GTK_EXPANDER (expander), 6);
					gtk_box_pack_start (GTK_BOX (hbox), expander, TRUE, TRUE, 0);

					label = gtk_label_new (error_msgs->str);
					gtk_container_add (GTK_CONTAINER (expander), label);

					gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
					gtk_widget_show_all (hbox);

					button = gsearchtool_button_new_with_stock_icon (_("Disable _Quick Search"), GTK_STOCK_OK);
					GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
					gtk_widget_show (button);

					gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
					gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
				
					g_signal_connect (G_OBJECT (dialog),
					                  "response",
					                  G_CALLBACK (disable_quick_search_cb), (gpointer) gsearch);
					
					gtk_widget_show (dialog);
				}
			}
			truncate_error_msgs = FALSE;
			g_string_truncate (error_msgs, 0);
		}
		g_io_channel_shutdown (ioc, TRUE, NULL);
		return FALSE;
	}
	return TRUE;
}

static void
child_command_set_pgid_cb (gpointer data)
{
	if (setpgid (0, 0) < 0) {
		g_print (_("Failed to set process group id of child %d: %s.\n"), 
		         getpid (), g_strerror (errno));
	}
}

void
spawn_search_command (GSearchWindow * gsearch, 
                      gchar * command)
{
	GIOChannel * ioc_stdout;
	GIOChannel * ioc_stderr;	
	GError * error = NULL;
	gchar ** argv  = NULL;
	gint child_stdout;
	gint child_stderr;
	
	start_animation (gsearch);
	
	if (!g_shell_parse_argv (command, NULL, &argv, &error)) {
		GtkWidget * dialog;

		stop_animation (gsearch);

		dialog = gtk_message_dialog_new (GTK_WINDOW (gsearch->window),
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_OK,
		                                 _("Error parsing the search command."));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          (error == NULL) ? "" : error->message);

		gtk_window_set_title (GTK_WINDOW (dialog), "");
		gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
		gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
		g_strfreev (argv);
		
		/* Free the gchar fields of search_command structure. */
		g_free (gsearch->command_details->look_in_folder_string);
		g_free (gsearch->command_details->name_contains_pattern_string);
		g_free (gsearch->command_details->name_contains_regex_string);
		g_free (gsearch->search_results_date_format_string);
		return;
	}	

	if (!g_spawn_async_with_pipes (g_get_home_dir (), argv, NULL, 
				       G_SPAWN_SEARCH_PATH,
				       child_command_set_pgid_cb, NULL, &gsearch->command_details->command_pid, NULL, &child_stdout, 
				       &child_stderr, &error)) {
		GtkWidget * dialog;
		
		stop_animation (gsearch);
	
		dialog = gtk_message_dialog_new (GTK_WINDOW (gsearch->window),
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_OK,
		                                 _("Error running the search command."));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          (error == NULL) ? "" : error->message);  

		gtk_window_set_title (GTK_WINDOW (dialog), "");
		gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
		gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
		g_strfreev (argv);
		
		/* Free the gchar fields of search_command structure. */
		g_free (gsearch->command_details->look_in_folder_string);
		g_free (gsearch->command_details->name_contains_pattern_string);
		g_free (gsearch->command_details->name_contains_regex_string);
		g_free (gsearch->search_results_date_format_string);
		return;
	}
	
	if (gsearch->command_details->is_command_first_pass == TRUE) {
		gchar * click_to_activate_pref;
		
		gsearch->command_details->command_status = RUNNING; 
		gsearch->is_search_results_single_click_to_activate = FALSE;
		gsearch->search_results_pixbuf_hash_table = g_hash_table_new (g_str_hash, g_str_equal);
		gsearch->search_results_filename_hash_table = g_hash_table_new (g_str_hash, g_str_equal);
	
		/* Get value of nautilus date_format key */
		gsearch->search_results_date_format_string = gsearchtool_gconf_get_string ("/apps/nautilus/preferences/date_format");

		/* Get value of nautilus click behaivor (single or double click to activate items) */
		click_to_activate_pref = gsearchtool_gconf_get_string ("/apps/nautilus/preferences/click_policy");
		
		if (strncmp (click_to_activate_pref, "single", 6) == 0) { 
			/* Format name column for single click to activate items */
			gsearch->is_search_results_single_click_to_activate = TRUE;		
			g_object_set (gsearch->search_results_name_cell_renderer,
			       "underline", PANGO_UNDERLINE_SINGLE,
			       "underline-set", TRUE,
			       NULL);
		}
		else {
			/* Format name column for double click to activate items */
			g_object_set (gsearch->search_results_name_cell_renderer,
			       "underline", PANGO_UNDERLINE_NONE,
			       "underline-set", FALSE,
			       NULL);
		}
		
		gtk_window_set_default (GTK_WINDOW (gsearch->window), gsearch->stop_button);
		gtk_widget_show (gsearch->stop_button);
		gtk_widget_set_sensitive (gsearch->stop_button, TRUE);
		gtk_widget_hide (gsearch->find_button);
		gtk_widget_set_sensitive (gsearch->find_button, FALSE);
		gtk_widget_set_sensitive (gsearch->search_results_save_results_as_item, FALSE);
		gtk_widget_set_sensitive (gsearch->search_results_vbox, TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (gsearch->search_results_tree_view), TRUE);
		gtk_widget_set_sensitive (gsearch->available_options_vbox, FALSE);
		gtk_widget_set_sensitive (gsearch->show_more_options_expander, FALSE);
		gtk_widget_set_sensitive (gsearch->name_and_folder_table, FALSE);

		gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (gsearch->search_results_tree_view), 0, 0);	
		gtk_list_store_clear (GTK_LIST_STORE (gsearch->search_results_list_store));
		g_free (click_to_activate_pref);
	}
	
	ioc_stdout = g_io_channel_unix_new (child_stdout);
	ioc_stderr = g_io_channel_unix_new (child_stderr);

	g_io_channel_set_encoding (ioc_stdout, NULL, NULL);
	g_io_channel_set_encoding (ioc_stderr, NULL, NULL);
	
	g_io_channel_set_flags (ioc_stdout, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_flags (ioc_stderr, G_IO_FLAG_NONBLOCK, NULL);
	
	g_io_add_watch (ioc_stdout, G_IO_IN | G_IO_HUP,
			handle_search_command_stdout_io, gsearch);	
	g_io_add_watch (ioc_stderr, G_IO_IN | G_IO_HUP,
			handle_search_command_stderr_io, gsearch);

	g_io_channel_unref (ioc_stdout);
	g_io_channel_unref (ioc_stderr);
	g_strfreev (argv);
}
  
static GtkWidget *
create_constraint_box (GSearchWindow * gsearch,
                       GSearchConstraint * opt,
                       gchar * value)
{
	GtkWidget * hbox;
	GtkWidget * label;
	GtkWidget * entry;
	GtkWidget * entry_hbox;
	GtkWidget * button;
	
	hbox = gtk_hbox_new (FALSE, 12);

	switch (GSearchOptionTemplates[opt->constraint_id].type) {
	case SEARCH_CONSTRAINT_TYPE_BOOLEAN:
		{
			gchar * text = remove_mnemonic_character (GSearchOptionTemplates[opt->constraint_id].desc);
			gchar * desc = g_strconcat (LEFT_LABEL_SPACING, _(text), ".", NULL);
			label = gtk_label_new (desc);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
			g_free (desc);
			g_free (text);
		}
		break;
	case SEARCH_CONSTRAINT_TYPE_TEXT:
	case SEARCH_CONSTRAINT_TYPE_NUMERIC:
	case SEARCH_CONSTRAINT_TYPE_DATE_BEFORE:
	case SEARCH_CONSTRAINT_TYPE_DATE_AFTER:
		{
			gchar * desc = g_strconcat (LEFT_LABEL_SPACING, _(GSearchOptionTemplates[opt->constraint_id].desc), ":", NULL);
			label = gtk_label_new_with_mnemonic (desc);
			g_free (desc);
		}
		
		/* add description label */
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		
		if (GSearchOptionTemplates[opt->constraint_id].type == SEARCH_CONSTRAINT_TYPE_TEXT) {
			entry = gtk_entry_new ();
			if (value != NULL) {
				gtk_entry_set_text (GTK_ENTRY (entry), value);
				opt->data.text = value;
			}
		}
		else {
			entry = gtk_spin_button_new_with_range (0, 999999999, 1);
			if (value != NULL) {
				gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), atoi (value));
				opt->data.time = atoi (value);
				opt->data.number = atoi (value);
			}
		}
		
		if (gsearch->is_window_accessible)
		{
			gchar * text = remove_mnemonic_character (GSearchOptionTemplates[opt->constraint_id].desc);
			gchar * desc = g_strdup_printf (_("'%s' entry"), _(text));
			add_atk_namedesc (GTK_WIDGET (entry), desc, _("Enter a value for search rule"));
			g_free (desc);
			g_free (text);
		}
		
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (entry));
		
		g_signal_connect (G_OBJECT (entry), "changed",
			 	  G_CALLBACK (constraint_update_info_cb), opt);
		
		g_signal_connect (G_OBJECT (entry), "activate",
				  G_CALLBACK (constraint_activate_cb),
				  (gpointer) gsearch);
				  
		/* add text field */
		entry_hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), entry_hbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (entry_hbox), entry, TRUE, TRUE, 0);
		
		/* add units label */
		if (GSearchOptionTemplates[opt->constraint_id].units != NULL)
		{
			label = gtk_label_new_with_mnemonic (_(GSearchOptionTemplates[opt->constraint_id].units));
			gtk_box_pack_start (GTK_BOX (entry_hbox), label, FALSE, FALSE, 0);
		}
		
		break;
	default:
		/* This should never happen.  If it does, there is a bug */
		label = gtk_label_new ("???");
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	        break;
	}
	
	button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT); 
	
	{
		GList * list = NULL;
		
		list = g_list_append (list, (gpointer) gsearch);
		list = g_list_append (list, (gpointer) opt);
		
	
		g_signal_connect (G_OBJECT (button), "clicked",
		                  G_CALLBACK (remove_constraint_cb),
		                  (gpointer) list);
			 
	}
	gtk_size_group_add_widget (gsearch->available_options_button_size_group, button);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	
	if (gsearch->is_window_accessible) {
		gchar *text = remove_mnemonic_character (GSearchOptionTemplates[opt->constraint_id].desc);
		gchar *desc = g_strdup_printf (_("Click to Remove the '%s' Rule"), _(text));
		add_atk_namedesc (GTK_WIDGET (button), NULL, desc);
		g_free (desc);
		g_free (text);
	}
	return hbox;
}

void
add_constraint (GSearchWindow * gsearch,
                gint constraint_id,
                gchar * value,
                gboolean show_constraint)
{
	GSearchConstraint * constraint = g_new (GSearchConstraint,1);
	GtkWidget * widget;

	if (show_constraint) {
		if (GTK_WIDGET_VISIBLE (gsearch->available_options_vbox) == FALSE) {
			gtk_expander_set_expanded (GTK_EXPANDER (gsearch->show_more_options_expander), TRUE);
			gtk_widget_show (gsearch->available_options_vbox);
		}
	} 
	
	gsearch->window_geometry.min_height += WINDOW_HEIGHT_STEP;
	
	if (GTK_WIDGET_VISIBLE (gsearch->available_options_vbox)) {
		gtk_window_set_geometry_hints (GTK_WINDOW (gsearch->window), 
		                               GTK_WIDGET (gsearch->window),
		                               &gsearch->window_geometry, 
		                               GDK_HINT_MIN_SIZE);
	}
		
	constraint->constraint_id = constraint_id;
	set_constraint_info_defaults (constraint);
	set_constraint_gconf_boolean (constraint->constraint_id, TRUE);
	
	widget = create_constraint_box (gsearch, constraint, value);
	gtk_box_pack_start (GTK_BOX (gsearch->available_options_vbox), widget, FALSE, FALSE, 0);
	gtk_widget_show_all (widget);
	
	gsearch->available_options_selected_list =
		g_list_append (gsearch->available_options_selected_list, constraint);
		
	set_constraint_selected_state (gsearch, constraint->constraint_id, TRUE);
}

static void
set_sensitive (GtkCellLayout * cell_layout,
               GtkCellRenderer * cell,
               GtkTreeModel * tree_model,
               GtkTreeIter * iter,
               gpointer data)
{
	GtkTreePath * path;
	gint index;

	path = gtk_tree_model_get_path (tree_model, iter);
	index = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	g_object_set (cell, "sensitive", !(GSearchOptionTemplates[index].is_selected), NULL);
}

static gboolean
is_separator (GtkTreeModel * model,
              GtkTreeIter * iter,
              gpointer data)
{
	GtkTreePath * path;
	gint index;

	path = gtk_tree_model_get_path (model, iter);
	index = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	return (GSearchOptionTemplates[index].type == SEARCH_CONSTRAINT_TYPE_SEPARATOR);
}

static void
create_additional_constraint_section (GSearchWindow * gsearch)
{
	GtkCellRenderer * renderer;
	GtkTreeModel * model;
	GtkWidget * hbox;
	gchar * desc;

	gsearch->available_options_vbox = gtk_vbox_new (FALSE, 6);	

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_end (GTK_BOX (gsearch->available_options_vbox), hbox, FALSE, FALSE, 0);

	desc = g_strconcat (LEFT_LABEL_SPACING, _("A_vailable options:"), NULL);
	gsearch->available_options_label = gtk_label_new_with_mnemonic (desc);
	g_free (desc);

	gtk_box_pack_start (GTK_BOX (hbox), gsearch->available_options_label, FALSE, FALSE, 0);
	
	model = gsearch_create_list_of_templates ();
	gsearch->available_options_combo_box = gtk_combo_box_new_with_model (model);
	g_object_unref (model);

	gtk_label_set_mnemonic_widget (GTK_LABEL (gsearch->available_options_label), GTK_WIDGET (gsearch->available_options_combo_box));
	gtk_combo_box_set_active (GTK_COMBO_BOX (gsearch->available_options_combo_box), 0);
	gtk_box_pack_start (GTK_BOX (hbox), gsearch->available_options_combo_box, TRUE, TRUE, 0);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (gsearch->available_options_combo_box),
	                            renderer,
	                            TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (gsearch->available_options_combo_box), renderer,
	                                "text", 0,
	                                NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (gsearch->available_options_combo_box),
	                                    renderer,
	                                    set_sensitive,
	                                    NULL, NULL);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (gsearch->available_options_combo_box), 
	                                      is_separator, NULL, NULL);

	if (gsearch->is_window_accessible)
	{
		add_atk_namedesc (GTK_WIDGET (gsearch->available_options_combo_box), _("Search Rules Menu"), 
				  _("Select a search rule from the menu"));
	}

	gsearch->available_options_add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	GTK_WIDGET_UNSET_FLAGS (gsearch->available_options_add_button, GTK_CAN_DEFAULT);
	gsearch->available_options_button_size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	gtk_size_group_add_widget (gsearch->available_options_button_size_group, gsearch->available_options_add_button);
	
	g_signal_connect (G_OBJECT (gsearch->available_options_add_button),"clicked",
			  G_CALLBACK (add_constraint_cb), (gpointer) gsearch);
	
	gtk_box_pack_end (GTK_BOX (hbox), gsearch->available_options_add_button, FALSE, FALSE, 0); 
}

static GtkWidget *
create_search_results_section (GSearchWindow * gsearch)
{
	GtkWidget * label;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * window;	
	GtkTreeViewColumn * column;	
	GtkCellRenderer * renderer;
	
	vbox = gtk_vbox_new (FALSE, 6);	
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic (_("S_earch results:"));
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	
	gsearch->files_found_label = gtk_label_new (NULL);
	g_object_set (G_OBJECT (gsearch->files_found_label), "xalign", 1.0, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), gsearch->files_found_label, FALSE, FALSE, 0);
	
	window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (window), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (window), 0);
	gtk_widget_set_size_request (window, 530, 160); 
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
	
	gsearch->search_results_list_store = gtk_list_store_new (NUM_COLUMNS, 
					      GDK_TYPE_PIXBUF, 
					      G_TYPE_STRING, 
					      G_TYPE_STRING, 
					      G_TYPE_STRING,
					      G_TYPE_STRING,
					      G_TYPE_DOUBLE,
					      G_TYPE_STRING,
					      G_TYPE_STRING,
					      G_TYPE_DOUBLE,
					      G_TYPE_BOOLEAN);
	
	gsearch->search_results_tree_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (gsearch->search_results_list_store)));

	gtk_tree_view_set_headers_visible (gsearch->search_results_tree_view, FALSE);						
	gtk_tree_view_set_rules_hint (gsearch->search_results_tree_view, TRUE);
  	g_object_unref (G_OBJECT (gsearch->search_results_list_store));
	
	gsearch->search_results_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gsearch->search_results_tree_view));
	
	gtk_tree_selection_set_mode (GTK_TREE_SELECTION (gsearch->search_results_selection),
				     GTK_SELECTION_MULTIPLE);
				     
	gtk_drag_source_set (GTK_WIDGET (gsearch->search_results_tree_view), 
			     GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			     GSearchDndTable, GSearchTotalDnds, 
			     GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);	

	g_signal_connect (G_OBJECT (gsearch->search_results_tree_view), 
			  "drag_data_get",
			  G_CALLBACK (drag_file_cb), 
			  (gpointer) gsearch);
			  
	g_signal_connect (G_OBJECT (gsearch->search_results_tree_view),
			  "drag_begin",
			  G_CALLBACK (drag_begin_file_cb),
			  (gpointer) gsearch);	  
			  
	g_signal_connect (G_OBJECT (gsearch->search_results_tree_view), 
			  "event_after",
		          G_CALLBACK (file_event_after_cb), 
			  (gpointer) gsearch);
			  
	g_signal_connect (G_OBJECT (gsearch->search_results_tree_view), 
			  "button_release_event",
		          G_CALLBACK (file_button_release_event_cb), 
			  (gpointer) gsearch);
			  		  
	g_signal_connect (G_OBJECT (gsearch->search_results_tree_view), 
			  "button_press_event",
		          G_CALLBACK (file_button_press_event_cb), 
			  (gpointer) gsearch->search_results_tree_view);		   

	g_signal_connect (G_OBJECT (gsearch->search_results_tree_view),
			  "key_press_event",
			  G_CALLBACK (file_key_press_event_cb),
			  (gpointer) gsearch);
			  
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (gsearch->search_results_tree_view));
	
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (gsearch->search_results_tree_view));
	gtk_box_pack_end (GTK_BOX (vbox), window, TRUE, TRUE, 0);
	
	/* create the name column */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", COLUMN_ICON,
                                             NULL);
					     
	gsearch->search_results_name_cell_renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, gsearch->search_results_name_cell_renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, gsearch->search_results_name_cell_renderer,
                                             "text", COLUMN_NAME,
					     NULL);				     
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);				     
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME); 
	gtk_tree_view_append_column (GTK_TREE_VIEW (gsearch->search_results_tree_view), column);

	/* create the folder column */
	renderer = gtk_cell_renderer_text_new (); 
	column = gtk_tree_view_column_new_with_attributes (_("Folder"), renderer,
							   "text", COLUMN_RELATIVE_PATH,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_PATH); 
	gtk_tree_view_append_column (GTK_TREE_VIEW (gsearch->search_results_tree_view), column);
	
	/* create the size column */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer,
							   "text", COLUMN_READABLE_SIZE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (gsearch->search_results_tree_view), column);
 
	/* create the type column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer,
							   "text", COLUMN_TYPE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_TYPE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (gsearch->search_results_tree_view), column);

	/* create the date modified column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date Modified"), renderer,
							   "text", COLUMN_READABLE_DATE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (gsearch->search_results_tree_view), column);

	return vbox;
}

static void
register_gsearchtool_icon (GtkIconFactory * factory)
{
	GtkIconSource * source;
	GtkIconSet * icon_set;

	source = gtk_icon_source_new ();

	gtk_icon_source_set_icon_name (source, GNOME_SEARCH_TOOL_ICON);

	icon_set = gtk_icon_set_new ();
	gtk_icon_set_add_source (icon_set, source);

	gtk_icon_factory_add (factory, GNOME_SEARCH_TOOL_STOCK, icon_set);

	gtk_icon_set_unref (icon_set);

	gtk_icon_source_free (source);	
}

static void
gsearchtool_init_stock_icons (void)
{
	GtkIconFactory * factory;
	GtkIconSize gsearchtool_icon_size;

	gsearchtool_icon_size = gtk_icon_size_register ("panel-menu",
							GNOME_SEARCH_TOOL_DEFAULT_ICON_SIZE,
							GNOME_SEARCH_TOOL_DEFAULT_ICON_SIZE);

	factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (factory);

	register_gsearchtool_icon (factory);

	g_object_unref (factory);
}

void
set_clone_command (GSearchWindow * gsearch,
                   gint * argcp,
                   gchar *** argvp,
                   gpointer client_data, 
                   gboolean escape_values)
{
	gchar ** argv;
	gchar * file_is_named_utf8;
	gchar * file_is_named_locale;
	gchar * look_in_folder_utf8;
	gchar * look_in_folder_locale;
	gchar * tmp;
	GList * list;
	gint  i = 0;

	argv = g_new0 (gchar*, SEARCH_CONSTRAINT_MAXIMUM_POSSIBLE);

	argv[i++] = (gchar *) client_data;

	file_is_named_utf8 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (gsearch->name_contains_entry))));
	file_is_named_locale = g_locale_from_utf8 (file_is_named_utf8 != NULL ? file_is_named_utf8 : "" , 
	                                           -1, NULL, NULL, NULL);
	if (escape_values)
		tmp = g_shell_quote (file_is_named_locale);
	else
		tmp = g_strdup (file_is_named_locale);
	argv[i++] = g_strdup_printf ("--named=%s", tmp);	
	g_free (tmp);
	g_free (file_is_named_locale);

	look_in_folder_utf8 = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (gsearch->look_in_folder_button));
	look_in_folder_locale = g_locale_from_utf8 (look_in_folder_utf8 != NULL ? look_in_folder_utf8 : "", 
	                                            -1, NULL, NULL, NULL);
	g_free (look_in_folder_utf8);
	
	if (escape_values)
		tmp = g_shell_quote (look_in_folder_locale);
	else
		tmp = g_strdup (look_in_folder_locale);
	argv[i++] = g_strdup_printf ("--path=%s", tmp);
	g_free (tmp);
	g_free (look_in_folder_locale);

	if (GTK_WIDGET_VISIBLE (gsearch->available_options_vbox)) {
		for (list = gsearch->available_options_selected_list; list != NULL; list = g_list_next (list)) {
			GSearchConstraint * constraint = list->data;
			gchar * locale = NULL;
			
			switch (constraint->constraint_id) {
			case SEARCH_CONSTRAINT_CONTAINS_THE_TEXT:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				if (escape_values)
					tmp = g_shell_quote (locale);
				else
					tmp = g_strdup (locale);
				argv[i++] = g_strdup_printf ("--contains=%s", tmp);
				g_free (tmp);
				break;
			case SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE:
				argv[i++] = g_strdup_printf ("--mtimeless=%d", constraint->data.time);
				break;
			case SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER:
				argv[i++] = g_strdup_printf ("--mtimemore=%d", constraint->data.time);
				break;
			case SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN:
				argv[i++] = g_strdup_printf ("--sizemore=%u", constraint->data.number);
				break;
			case SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN:
				argv[i++] = g_strdup_printf ("--sizeless=%u", constraint->data.number);
				break;
			case SEARCH_CONSTRAINT_FILE_IS_EMPTY:
				argv[i++] = g_strdup ("--empty");
				break;
			case SEARCH_CONSTRAINT_OWNED_BY_USER:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				if (escape_values)
					tmp = g_shell_quote (locale);
				else
					tmp = g_strdup (locale);
				argv[i++] = g_strdup_printf ("--user=%s", tmp);
				g_free (tmp);
				break;
			case SEARCH_CONSTRAINT_OWNED_BY_GROUP:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				if (escape_values)
					tmp = g_shell_quote (locale);
				else
					tmp = g_strdup (locale);
				argv[i++] = g_strdup_printf ("--group=%s", tmp);
				g_free (tmp);
				break;
			case SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED:
				argv[i++] = g_strdup ("--nouser");
				break;
			case SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				if (escape_values)
					tmp = g_shell_quote (locale);
				else
					tmp = g_strdup (locale);
				argv[i++] = g_strdup_printf ("--notnamed=%s", tmp);
				g_free (tmp);
				break;
			case SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION:
				locale = g_locale_from_utf8 (constraint->data.text, -1, NULL, NULL, NULL);
				if (escape_values)
					tmp = g_shell_quote (locale);
				else
					tmp = g_strdup (locale);
				argv[i++] = g_strdup_printf ("--regex=%s", tmp);
				g_free (tmp);
				break;
			case SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS:
				argv[i++] = g_strdup ("--hidden");
				break;
			case SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS:
				argv[i++] = g_strdup ("--follow");
				break;
			case SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS:
				argv[i++] = g_strdup ("--allmounts");
				break;
			default:
				break;
			}
			g_free (locale);
		}		
	}
	*argvp = argv;
	*argcp = i;
}

static void
handle_gconf_settings (GSearchWindow * gsearch)
{
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/show_additional_options")) {
		if (GTK_WIDGET_VISIBLE (gsearch->available_options_vbox) == FALSE) {
			gtk_expander_set_expanded (GTK_EXPANDER (gsearch->show_more_options_expander), TRUE);
			gtk_widget_show (gsearch->available_options_vbox);
		}
	}
		      
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/contains_the_text")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_CONTAINS_THE_TEXT, "", FALSE); 
	}
		
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/date_modified_less_than")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/date_modified_more_than")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/size_at_least")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/size_at_most")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/file_is_empty")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_FILE_IS_EMPTY, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/owned_by_user")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_OWNED_BY_USER, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/owned_by_group")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_OWNED_BY_GROUP, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/owner_is_unrecognized")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/name_does_not_contain")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/name_matches_regular_expression")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/show_hidden_files_and_folders")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/follow_symbolic_links")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/include_other_filesystems")) {
		add_constraint (gsearch, SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS, NULL, FALSE); 
	}
}

static void
gsearchtool_ui_manager (GSearchWindow * gsearch) 
{
	GtkActionGroup * action_group;
	GtkAccelGroup * accel_group;
	GtkAction * action;
	GError * error = NULL;

	action_group = gtk_action_group_new ("PopupActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, GSearchUiEntries, G_N_ELEMENTS (GSearchUiEntries), gsearch->window);
			
	gsearch->window_ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (gsearch->window_ui_manager, action_group, 0);
				
	accel_group = gtk_ui_manager_get_accel_group (gsearch->window_ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (gsearch->window), accel_group);
			
	if (!gtk_ui_manager_add_ui_from_string (gsearch->window_ui_manager, GSearchUiDescription, -1, &error)) {
      		g_message ("Building menus failed: %s", error->message);
		g_error_free (error);
      		exit (EXIT_FAILURE);
	}
	
	action = gtk_ui_manager_get_action (gsearch->window_ui_manager, "/PopupMenu/Open");
	g_signal_connect (G_OBJECT (action),
	                  "activate",
	                  G_CALLBACK (open_file_cb),
	                  (gpointer) gsearch);

	action = gtk_ui_manager_get_action (gsearch->window_ui_manager, "/PopupMenu/OpenFolder");
	g_signal_connect (G_OBJECT (action),
	                  "activate",
	                  G_CALLBACK (open_folder_cb), 
	                  (gpointer) gsearch);

	action = gtk_ui_manager_get_action (gsearch->window_ui_manager, "/PopupMenu/MoveToTrash");
	g_signal_connect (G_OBJECT (action), 
	                  "activate",
	                  G_CALLBACK (move_to_trash_cb), 
	                  (gpointer) gsearch);			  
	  
	action = gtk_ui_manager_get_action (gsearch->window_ui_manager, "/PopupMenu/SaveResultsAs");
	g_signal_connect (G_OBJECT (action), 
	                  "activate",
	                  G_CALLBACK (show_file_selector_cb), 
	                  (gpointer) gsearch);

	gsearch->search_results_popup_menu = gtk_ui_manager_get_widget (gsearch->window_ui_manager, 
	                                                                "/PopupMenu");							   
	gsearch->search_results_save_results_as_item = gtk_ui_manager_get_widget (gsearch->window_ui_manager,
	                                                                          "/PopupMenu/SaveResultsAs");
}

static GtkWidget *
gsearch_app_create (GSearchWindow * gsearch)
{
	GtkTargetEntry drag_types[] = {{ "text/uri-list", 0, 0 }};
	gchar * locale_string;
	gchar * utf8_string;
	GtkWidget * hbox;	
	GtkWidget * vbox;
	GtkWidget * entry;
	GtkWidget * label;
	GtkWidget * button;
	GtkWidget * container;
	
	gsearch->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gsearch->command_details = g_new0 (GSearchCommandDetails, 1);
	gsearch->window_geometry.min_height = MINIMUM_WINDOW_HEIGHT;
	gsearch->window_geometry.min_width  = MINIMUM_WINDOW_WIDTH;
	
	gtk_window_set_position (GTK_WINDOW (gsearch->window), GTK_WIN_POS_CENTER);	
	gtk_window_set_geometry_hints (GTK_WINDOW (gsearch->window), GTK_WIDGET (gsearch->window),
				       &gsearch->window_geometry, GDK_HINT_MIN_SIZE);
	
	container = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (gsearch->window), container);	
	gtk_container_set_border_width (GTK_CONTAINER (container), 12);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (container), hbox, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	gsearch->progress_spinner = gsearch_spinner_new ();
	gtk_widget_show (gsearch->progress_spinner);
	gtk_box_pack_start (GTK_BOX (vbox), gsearch->progress_spinner, FALSE, FALSE, 0);
	gsearch_spinner_set_small_mode (GSEARCH_SPINNER (gsearch->progress_spinner), FALSE);
			  
	gtk_drag_source_set (gsearch->progress_spinner, 
			     GDK_BUTTON1_MASK,
			     drag_types, 
			     G_N_ELEMENTS (drag_types), 
			     GDK_ACTION_COPY);
			     
	gtk_drag_source_set_icon_stock (gsearch->progress_spinner, GTK_STOCK_FIND);
	
	g_signal_connect (G_OBJECT (gsearch->progress_spinner), 
			  "drag_data_get",
			  G_CALLBACK (drag_data_animation_cb), 
			  (gpointer) gsearch);
	
	gsearch->name_and_folder_table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (gsearch->name_and_folder_table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (gsearch->name_and_folder_table), 12);
	gtk_container_add (GTK_CONTAINER (hbox), gsearch->name_and_folder_table);
	
	label = gtk_label_new_with_mnemonic (_("_Name contains:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	
	gtk_table_attach (GTK_TABLE (gsearch->name_and_folder_table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 1);

	gsearch->name_contains_entry = gnome_entry_new ("gsearchtool-file-entry");
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), gsearch->name_contains_entry);
	gnome_entry_set_max_saved (GNOME_ENTRY (gsearch->name_contains_entry), 10);
	gtk_table_attach (GTK_TABLE (gsearch->name_and_folder_table), gsearch->name_contains_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0, 0);
	entry =  gnome_entry_gtk_entry (GNOME_ENTRY (gsearch->name_contains_entry));
       
	if (GTK_IS_ACCESSIBLE (gtk_widget_get_accessible (gsearch->name_contains_entry)))
	{
		gsearch->is_window_accessible = TRUE;
		add_atk_namedesc (gsearch->name_contains_entry, NULL, _("Enter the file name you want to search."));
		add_atk_namedesc (entry, _("Name Contains Entry"), _("Enter the file name you want to search."));
	}	
	g_signal_connect (G_OBJECT (gsearch->name_contains_entry), "activate",
			  G_CALLBACK (name_contains_activate_cb),
			  (gpointer) gsearch);	 
			  
	label = gtk_label_new_with_mnemonic (_("_Look in folder:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	
	gtk_table_attach (GTK_TABLE (gsearch->name_and_folder_table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	
	gsearch->look_in_folder_button = gtk_file_chooser_button_new (_("Browse"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (gsearch->look_in_folder_button));		
	gtk_table_attach (GTK_TABLE (gsearch->name_and_folder_table), gsearch->look_in_folder_button, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0, 0);

	if (gsearch->is_window_accessible)
	{ 
		add_atk_namedesc (GTK_WIDGET (gsearch->look_in_folder_button), NULL, _("Select the folder where you want to start the search."));
	}
			  
	locale_string = g_get_current_dir ();
	utf8_string = g_filename_to_utf8 (locale_string, -1, NULL, NULL, NULL);

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gsearch->look_in_folder_button), utf8_string);

	g_free (locale_string);
	g_free (utf8_string);

	gsearch->show_more_options_expander = gtk_expander_new_with_mnemonic (_("Show more _options"));
	gtk_box_pack_start (GTK_BOX (container), gsearch->show_more_options_expander, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (gsearch->show_more_options_expander), "notify::expanded",
			  G_CALLBACK (click_expander_cb), (gpointer) gsearch);

	create_additional_constraint_section (gsearch);
	gtk_box_pack_start (GTK_BOX (container), GTK_WIDGET (gsearch->available_options_vbox), FALSE, FALSE, 0);
	
	if (gsearch->is_window_accessible)
	{ 
		add_atk_namedesc (GTK_WIDGET (gsearch->show_more_options_expander), _("Show more options"), _("Expands or collapses a list of search options."));
		add_atk_relation (GTK_WIDGET (gsearch->available_options_vbox), GTK_WIDGET (gsearch->show_more_options_expander), ATK_RELATION_CONTROLLED_BY);
		add_atk_relation (GTK_WIDGET (gsearch->show_more_options_expander), GTK_WIDGET (gsearch->available_options_vbox), ATK_RELATION_CONTROLLER_FOR);
	}
	
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (container), vbox, TRUE, TRUE, 0);
	
	gsearch->search_results_vbox = create_search_results_section (gsearch);
	gtk_widget_set_sensitive (GTK_WIDGET (gsearch->search_results_vbox), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), gsearch->search_results_vbox, TRUE, TRUE, 0);
	
	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	gtk_box_set_spacing (GTK_BOX (hbox), 10);
	button = gtk_button_new_from_stock (GTK_STOCK_HELP);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (hbox), button, TRUE);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (click_help_cb), (gpointer) gsearch->window);
	
	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (click_close_cb), (gpointer) gsearch->command_details);
	
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	/* Find and Stop buttons... */
	gsearch->find_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	gsearch->stop_button = gtk_button_new_from_stock (GTK_STOCK_STOP);

	GTK_WIDGET_SET_FLAGS (gsearch->find_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS (gsearch->stop_button, GTK_CAN_DEFAULT);

	gtk_box_pack_end (GTK_BOX (hbox), gsearch->stop_button, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), gsearch->find_button, FALSE, FALSE, 0);

	gtk_widget_set_sensitive (gsearch->stop_button, FALSE);
	gtk_widget_set_sensitive (gsearch->find_button, TRUE);

	g_signal_connect (G_OBJECT (gsearch->find_button), "clicked",
	                  G_CALLBACK (click_find_cb), (gpointer) gsearch);
    	g_signal_connect (G_OBJECT (gsearch->find_button), "size_allocate",
	                  G_CALLBACK (size_allocate_cb), (gpointer) gsearch->available_options_add_button);
	g_signal_connect (G_OBJECT (gsearch->stop_button), "clicked",
	                  G_CALLBACK (click_stop_cb), (gpointer) gsearch);
	
	if (gsearch->is_window_accessible) {
		add_atk_namedesc (GTK_WIDGET (gsearch->find_button), NULL, _("Click to Start the search"));
		add_atk_namedesc (GTK_WIDGET (gsearch->stop_button), NULL, _("Click to Stop the search"));
	}

	gtk_widget_show_all (container);
	gtk_widget_hide (gsearch->available_options_vbox);
	gtk_widget_hide (gsearch->stop_button); 

	gtk_window_set_focus (GTK_WINDOW (gsearch->window), 
		GTK_WIDGET (gnome_entry_gtk_entry (GNOME_ENTRY (gsearch->name_contains_entry)))); 
 
	gtk_window_set_default (GTK_WINDOW (gsearch->window), gsearch->find_button);
	
	return gsearch->window;
}

static void
gsearch_window_finalize (GObject * object)
{
        parent_class->finalize (object);
}

static void
gsearch_window_class_init (GSearchWindowClass * klass)
{
	GObjectClass * object_class = (GObjectClass *) klass;

	object_class->finalize = gsearch_window_finalize;
	parent_class = g_type_class_peek_parent (klass);
}

GType
gsearch_window_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (GSearchWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) gsearch_window_class_init,
			NULL,
			NULL,
			sizeof (GSearchWindow),
			0,
			(GInstanceInitFunc) gsearch_app_create
		};
		object_type = g_type_register_static (GTK_TYPE_WINDOW, "GSearchWindow", &object_info, 0);
	}
	return object_type;
}

int
main (int argc, 
      char * argv[])
{
	GSearchWindow * gsearch;
	GnomeProgram * program;
	GnomeClient * client;
	GtkWidget * window;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	define_popt_table_options ();

	program = gnome_program_init ("gnome-search-tool", 
	                              VERSION,
	                              LIBGNOMEUI_MODULE,
	                              argc, argv,
	                              GNOME_PARAM_POPT_TABLE, GSearchPoptOptions,
	                              GNOME_PARAM_POPT_FLAGS, 0,
	                              GNOME_PARAM_APP_DATADIR, DATADIR,
	                              NULL);

	g_set_application_name (_("Search for Files"));
	gtk_window_set_default_icon_name (GNOME_SEARCH_TOOL_ICON);

	gsearchtool_init_stock_icons ();

	window = g_object_new (GSEARCH_TYPE_WINDOW, NULL);
	gsearch = GSEARCH_WINDOW (window);
	gsearchtool_ui_manager (gsearch);

	gtk_window_set_wmclass (GTK_WINDOW (gsearch->window), "gnome-search-tool", "gnome-search-tool");
	gtk_window_set_policy (GTK_WINDOW (gsearch->window), TRUE, TRUE, TRUE);

	g_signal_connect (G_OBJECT (gsearch->window), "delete_event",
	                            G_CALLBACK (quit_cb),
	                            (gpointer) gsearch->command_details);
	g_signal_connect (G_OBJECT (gsearch->window), "key_press_event",
	                            G_CALLBACK (key_press_cb), 
	                            (gpointer) gsearch);

	if (!bonobo_init_full (&argc, argv, bonobo_activation_orb_get (), NULL, NULL)) {
		g_error (_("Cannot initialize bonobo."));
	}
	bonobo_activate ();

	if ((client = gnome_master_client ()) != NULL) {
		g_signal_connect (client, "save_yourself",
		                  G_CALLBACK (save_session_cb),
		                  (gpointer) argv[0]);
		g_signal_connect (client, "die",
		                  G_CALLBACK (die_cb),
		                  (gpointer) gsearch->command_details);
	}

	gtk_widget_show (gsearch->window);

	if (handle_popt_args (gsearch) == FALSE) {
		handle_gconf_settings (gsearch);
	}

	gtk_main ();
	return 0;
}

