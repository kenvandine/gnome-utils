/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool.c
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

#define ICON_SIZE 24.0
#define GNOME_SEARCH_TOOL_DEFAULT_ICON_SIZE 48
#define GNOME_SEARCH_TOOL_STOCK "panel-searchtool"
#define LEFT_LABEL_SPACING "     "

#define DEFAULT_ANIMATION_WIDTH   48
#define DEFAULT_ANIMATION_HEIGHT  48
#define DEFAULT_ANIMATION_FRAME    2
#define TOTAL_ANIMATION_FRAMES     3

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <fnmatch.h>

#ifndef FNM_CASEFOLD
#  define FNM_CASEFOLD 0
#endif

#include <gnome.h>

#include "gsearchtool.h"
#include "gsearchtool-support.h"
#include "gsearchtool-callbacks.h"

#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libbonobo.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>

static GtkIconSize gsearchtool_icon_size = 0;

typedef struct _FindOptionTemplate FindOptionTemplate;
struct _FindOptionTemplate {
	SearchConstraintType type;
	gchar 	 *option;          /* the option string to pass to find or whatever */
	gchar 	 *desc;            /* description */
	gboolean is_selected;     
};
	
static FindOptionTemplate templates[] = {
	{ SEARCH_CONSTRAINT_TEXT, "-exec grep -c '%s' {} \\;", N_("Contains the _text"), FALSE },
	{ SEARCH_CONSTRAINT_SEPARATOR, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TIME_LESS, "-mtime -%d", N_("_Date modified less than (days)"), FALSE },
	{ SEARCH_CONSTRAINT_TIME_MORE, "\\( -mtime +%d -o -mtime %d \\)", N_("Date modified more than (da_ys)"), FALSE },
	{ SEARCH_CONSTRAINT_SEPARATOR, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_NUMBER, "-size +%uc", N_("S_ize at least (kilobytes)"), FALSE }, 
	{ SEARCH_CONSTRAINT_NUMBER, "-size -%uc", N_("Si_ze at most (kilobytes)"), FALSE },
	{ SEARCH_CONSTRAINT_BOOL, "-size 0c \\( -type f -o -type d \\)", N_("File is empty"), FALSE },	
	{ SEARCH_CONSTRAINT_SEPARATOR, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TEXT, "-user '%s'", N_("Owned by _user"), FALSE },
	{ SEARCH_CONSTRAINT_TEXT, "-group '%s'", N_("Owned by _group"), FALSE },
	{ SEARCH_CONSTRAINT_BOOL, "\\( -nouser -o -nogroup \\)", N_("Owner is unrecognized"), FALSE },
	{ SEARCH_CONSTRAINT_SEPARATOR, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_TEXT, "'!' -name '*%s*'", N_("Na_me does not contain"), FALSE },
	{ SEARCH_CONSTRAINT_TEXT, "-regex '%s'", N_("Name matches regular e_xpression"), FALSE }, 
	{ SEARCH_CONSTRAINT_SEPARATOR, NULL, NULL, TRUE },
	{ SEARCH_CONSTRAINT_BOOL, "SHOW_HIDDEN_FILES", N_("Show hidden files and folders"), FALSE },
	{ SEARCH_CONSTRAINT_BOOL, "-follow", N_("Follow symbolic links"), FALSE },
	{ SEARCH_CONSTRAINT_BOOL, "INCLUDE_OTHER_FILESYSTEMS", N_("Include other filesystems"), FALSE },
	{ SEARCH_CONSTRAINT_END, NULL, NULL, FALSE}
}; 

static GtkTargetEntry dnd_table[] = {
	{ "STRING",        0, 0 },
	{ "text/plain",    0, 0 },
	{ "text/uri-list", 0, 1 }
};

static guint n_dnds = sizeof (dnd_table) / sizeof (dnd_table[0]);

enum {
	SEARCH_CONSTRAINT_CONTAINS_THE_TEXT,
	SEARCH_CONSTRAINT_SEPARATOR_00, 
	SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE,
	SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER,
	SEARCH_CONSTRAINT_SEPARATOR_01,
	SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN,	
	SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN,
	SEARCH_CONSTRAINT_FILE_IS_EMPTY,
	SEARCH_CONSTRAINT_SEPARATOR_02,	
	SEARCH_CONSTRAINT_OWNED_BY_USER, 
	SEARCH_CONSTRAINT_OWNED_BY_GROUP,
	SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED,
	SEARCH_CONSTRAINT_SEPARATOR_03,	
	SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED,
	SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION,
	SEARCH_CONSTRAINT_SEPARATOR_04,
	SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS,
	SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS,
	SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS,
	SEARCH_CONSTRAINT_MAXIMUM_POSSIBLE
};

struct _PoptArgument {	
	gchar 		*name;
	gchar 		*path;
	gchar 		*contains;
	gchar		*user;
	gchar		*group;
	gboolean	nouser;
	gchar		*mtimeless;
	gchar		*mtimemore;
	gchar  		*sizeless;	
	gchar  		*sizemore;
	gboolean 	empty;
	gchar		*notnamed;
	gchar		*regex;
	gboolean	hidden;
	gboolean	follow;
	gboolean	allmounts;
	gchar 		*sortby;
	gboolean 	descending;
	gboolean 	start;
} PoptArgument;

struct poptOption options[] = { 
  	{ "named", '\0', POPT_ARG_STRING, &PoptArgument.name, 0, NULL, NULL},
	{ "path", '\0', POPT_ARG_STRING, &PoptArgument.path, 0, NULL, NULL},
  	{ "sortby", '\0', POPT_ARG_STRING, &PoptArgument.sortby, 0, NULL, NULL},
  	{ "descending", '\0', POPT_ARG_NONE, &PoptArgument.descending, 0, NULL, NULL},
  	{ "start", '\0', POPT_ARG_NONE, &PoptArgument.start, 0, NULL, NULL},
  	{ "contains", '\0', POPT_ARG_STRING, &PoptArgument.contains, 0, NULL, NULL},
  	{ "mtimeless", '\0', POPT_ARG_STRING, &PoptArgument.mtimeless, 0, NULL, NULL},
  	{ "mtimemore", '\0', POPT_ARG_STRING, &PoptArgument.mtimemore, 0, NULL, NULL},
	{ "sizemore", '\0', POPT_ARG_STRING, &PoptArgument.sizemore, 0, NULL, NULL},
	{ "sizeless", '\0', POPT_ARG_STRING, &PoptArgument.sizeless, 0, NULL, NULL},
	{ "empty", '\0', POPT_ARG_NONE, &PoptArgument.empty, 0, NULL, NULL},
  	{ "user", '\0', POPT_ARG_STRING, &PoptArgument.user, 0, NULL, NULL},
  	{ "group", '\0', POPT_ARG_STRING, &PoptArgument.group, 0, NULL, NULL},
  	{ "nouser", '\0', POPT_ARG_NONE, &PoptArgument.nouser, 0, NULL, NULL},
  	{ "notnamed", '\0', POPT_ARG_STRING, &PoptArgument.notnamed, 0, NULL, NULL},
  	{ "regex", '\0', POPT_ARG_STRING, &PoptArgument.regex, 0, NULL, NULL},
	{ "hidden", '\0', POPT_ARG_NONE, &PoptArgument.hidden, 0, NULL, NULL},
  	{ "follow", '\0', POPT_ARG_NONE, &PoptArgument.follow, 0, NULL, NULL},
  	{ "allmounts", '\0', POPT_ARG_NONE, &PoptArgument.allmounts, 0, NULL, NULL},
  	{ NULL,'\0', 0, NULL, 0, NULL, NULL}
};

static gchar *find_command_default_name_option;
static gchar *locate_command_default_options;
	
void   
setup_case_insensitive_arguments (void)
{
	static gboolean first_pass = TRUE;
	gchar *cmd_stderr;

	if (first_pass != TRUE) {
		return;
	}

	first_pass = FALSE;

	/* check find command for -iname argument compatibility */
	g_spawn_command_line_sync ("find /dev/null -iname 'string'", NULL, &cmd_stderr, NULL, NULL);

	if ((cmd_stderr != NULL) && (strlen (cmd_stderr) == 0)) {
		find_command_default_name_option = g_strdup ("-iname");
		templates[SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED].option = g_strdup ("'!' -iname '*%s*'");
	}
	else {
		find_command_default_name_option = g_strdup ("-name");
	}
	g_free (cmd_stderr);

	/* check grep command for -i argument compatibility */
	g_spawn_command_line_sync ("grep -i 'string' /dev/null", NULL, &cmd_stderr, NULL, NULL);

	if ((cmd_stderr != NULL) && (strlen (cmd_stderr) == 0)) {
		templates[SEARCH_CONSTRAINT_CONTAINS_THE_TEXT].option = g_strdup ("-exec grep -i -c '%s' {} \\;");
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
  
gchar *
setup_find_name_options (gchar *filename)
{
	/* This function builds the name options for the find command.  This in
	   done to insure that the find command returns hidden files and folders. */

	GString *command;
	command = g_string_new ("");

	if (strstr(filename, "*") == NULL) {

		if ((strlen(filename) == 0) || (filename[0] != '.')) {
	 		g_string_append_printf (command, "\\( %s '*%s*' -o %s '.*%s*' \\) ", 
					find_command_default_name_option, filename,
					find_command_default_name_option, filename);
		}
		else {
			g_string_append_printf (command, "\\( %s '*%s*' -o %s '.*%s*' -o %s '%s*' \\) ", 
					find_command_default_name_option, filename,
					find_command_default_name_option, filename,
					find_command_default_name_option, filename);
		}
	}
	else {
		if (filename[0] == '.') {
			g_string_append_printf (command, "\\( %s '%s' -o %s '.*%s' \\) ", 
					find_command_default_name_option, filename,
					find_command_default_name_option, filename);	
		}
		else if (filename[0] != '*') {
			g_string_append_printf (command, "%s '%s' ", 
					find_command_default_name_option, filename);
		}
		else {
			if ((strlen(filename) >= 1) && (filename[1] == '.')) {
				g_string_append_printf (command, "\\( %s '%s' -o %s '%s' \\) ", 
					find_command_default_name_option, filename,
					find_command_default_name_option, &filename[1]);
			}
			else {
				g_string_append_printf (command, "\\( %s '%s' -o %s '.%s' \\) ", 
					find_command_default_name_option, filename,
					find_command_default_name_option, filename);
			}
		}
	}
	return g_string_free (command, FALSE);
}

gchar *
build_search_command (void)
{
	GString *command;
	gchar *file_is_named_utf8;
	gchar *file_is_named_locale;
	gchar *file_is_named_escaped;
	gchar *look_in_folder_utf8;
	gchar *look_in_folder_locale;

	setup_case_insensitive_arguments ();
	
	file_is_named_utf8 = (gchar *) gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry))));

	if (!file_is_named_utf8 || !*file_is_named_utf8) {
		file_is_named_utf8 = g_strdup ("*");
	} 
	else {
		gchar *locale;

		locale = g_locale_from_utf8 (file_is_named_utf8, -1, NULL, NULL, NULL);
		gnome_entry_prepend_history (GNOME_ENTRY(interface.file_is_named_entry), TRUE, file_is_named_utf8);

		if (strstr (locale, "*") == NULL) {
			file_is_named_utf8 = g_strconcat ("*", file_is_named_utf8, "*", NULL);
		}
		
		g_free (locale);
	}
	file_is_named_locale = g_locale_from_utf8 (file_is_named_utf8, -1, NULL, NULL, NULL);
	
	look_in_folder_utf8 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY(interface.look_in_folder_entry), FALSE);
	look_in_folder_locale = g_locale_from_utf8 (look_in_folder_utf8, -1, NULL, NULL, NULL);
	gnome_entry_prepend_history (GNOME_ENTRY(gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry))), 
				     TRUE, look_in_folder_utf8);
	g_free (look_in_folder_utf8);
	
	if (!file_extension_is (look_in_folder_locale, G_DIR_SEPARATOR_S)) {
		gchar *tmp;
		
		tmp = look_in_folder_locale;
		look_in_folder_locale = g_strconcat (look_in_folder_locale, G_DIR_SEPARATOR_S, NULL);
		g_free (tmp);
	}
	search_command.look_in_folder = g_strdup(look_in_folder_locale);
	
	command = g_string_new ("");
	search_command.show_hidden_files = FALSE;
	
	if ((GTK_WIDGET_VISIBLE(interface.additional_constraints) == FALSE) ||
	    (interface.selected_constraints == NULL)) {
	
		gchar *locate;
		gboolean disable_quick_search;
		
		locate = g_find_program_in_path ("locate");
		file_is_named_escaped = escape_single_quotes (file_is_named_locale);
		search_command.file_is_named_pattern = g_strdup(file_is_named_utf8);
		disable_quick_search = gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/disable_quick_search");
		
		if ((disable_quick_search != TRUE) && (locate != NULL) 
		    && (is_path_in_home_folder (look_in_folder_locale) != TRUE) 
		    && (is_path_in_mount_folder (look_in_folder_locale) != TRUE)
		    && (is_path_in_tmp_folder (look_in_folder_locale) != TRUE)) {	
			g_string_append_printf (command, "%s %s '%s*%s'", 
						locate,
						locate_command_default_options,
						look_in_folder_locale,
						file_is_named_escaped);
		} 
		else {
			g_string_append_printf (command, "find \"%s\" '!' -type p %s '%s' -xdev -print", 
						look_in_folder_locale, 
						find_command_default_name_option, 
						file_is_named_escaped);
		}
		g_free (locate);
	} 
	else {
		GList *list;
		gboolean disable_mount_argument = FALSE;
		
		search_command.regex_matching_enabled = FALSE;
		file_is_named_escaped = escape_single_quotes (file_is_named_locale);
		search_command.file_is_named_pattern = g_strdup(file_is_named_utf8);
		
		g_string_append_printf (command, "find \"%s\" '!' -type p  %s", 
					look_in_folder_locale,
					setup_find_name_options (file_is_named_escaped));
	
		for (list = interface.selected_constraints; list != NULL; list = g_list_next (list)) {
			
			SearchConstraint *constraint = list->data;
						
			switch (templates[constraint->constraint_id].type) {
			case SEARCH_CONSTRAINT_BOOL:
				if (strcmp (templates[constraint->constraint_id].option, "INCLUDE_OTHER_FILESYSTEMS") == 0) {
					disable_mount_argument = TRUE;
				}
				else if (strcmp (templates[constraint->constraint_id].option, "SHOW_HIDDEN_FILES") == 0) {
					search_command.show_hidden_files = TRUE;
				}
				else {
					g_string_append_printf(command, "%s ",
						templates[constraint->constraint_id].option);
				}
				break;
			case SEARCH_CONSTRAINT_TEXT:
				if (strcmp (templates[constraint->constraint_id].option, "-regex '%s'") == 0) {
					
					gchar *regex;
					
					regex = escape_single_quotes (constraint->data.text);
					
					if (regex != NULL) {	
						search_command.regex_matching_enabled = TRUE;
						search_command.regex_string = g_locale_from_utf8 (regex, -1, NULL, NULL, NULL);
					}
					
					g_free (regex);
				} 
				else {
					gchar *string;
					gchar *locale;
					
					string = escape_single_quotes (constraint->data.text);
					locale = g_locale_from_utf8 (string, -1, NULL, NULL, NULL);
					
					if (strlen (locale) != 0) { 
						g_string_append_printf (command,
							  	        templates[constraint->constraint_id].option,
						  		        locale);

						g_string_append_c (command, ' ');
					}
					
					g_free(string);
					g_free(locale);					
				}
				break;
			case SEARCH_CONSTRAINT_NUMBER:
				g_string_append_printf (command,
					  		templates[constraint->constraint_id].option,
					  		(constraint->data.number * 1024));
				g_string_append_c (command, ' ');
				break;
			case SEARCH_CONSTRAINT_TIME_LESS:
				g_string_append_printf (command,
					 		templates[constraint->constraint_id].option,
					  		constraint->data.time);
				g_string_append_c (command, ' ');
				break;	
			case SEARCH_CONSTRAINT_TIME_MORE:
				g_string_append_printf (command,
					 		templates[constraint->constraint_id].option,
					  		constraint->data.time,
					  		constraint->data.time);
				g_string_append_c (command, ' ');
				break;
			default:
		        	break;
			}
		}
		search_command.file_is_named_pattern = g_strdup ("*");
		
		if (disable_mount_argument != TRUE) {
			g_string_append (command, "-xdev ");
		}
		
		g_string_append (command, "-print ");
	}
	g_free (file_is_named_locale);
	g_free (look_in_folder_locale);
	
	return g_string_free (command, FALSE);
}

static void
add_file_to_search_results (const gchar 	*file, 
			    GtkListStore 	*store, 
			    GtkTreeIter 	*iter)
{					
	gchar *mime_type = gnome_vfs_get_mime_type (file);
	gchar *description = get_file_type_with_mime_type (file, mime_type);
	gchar *icon_path = get_file_icon_with_mime_type (file, mime_type);
	GnomeVFSFileInfo *vfs_file_info = gnome_vfs_file_info_new ();
	GdkPixbuf *pixbuf = NULL;
	gchar *readable_size, *readable_date;
	gchar *utf8_base_name, *utf8_dir_name;
	gchar *base_name, *dir_name;
	
	if (gtk_tree_view_get_headers_visible (GTK_TREE_VIEW(interface.tree)) == FALSE) {
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(interface.tree), TRUE);
	}
	
	if (icon_path != NULL) {
		pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
	}
	
	if (pixbuf != NULL) {
		GdkPixbuf *scaled;
		int        new_w, new_h;
		int        w, h;
		double     factor;

		/* scale keeping aspect ratio. */

		w = gdk_pixbuf_get_width (pixbuf);
		h = gdk_pixbuf_get_height (pixbuf);
			
		factor = MIN (ICON_SIZE / w, ICON_SIZE / h);
		new_w  = MAX ((int) (factor * w), 1);
		new_h  = MAX ((int) (factor * h), 1);
			
		scaled = gdk_pixbuf_scale_simple (pixbuf,
						  new_w,
						  new_h,
						  GDK_INTERP_BILINEAR);
						  				  
		g_object_unref (G_OBJECT (pixbuf));
		pixbuf = scaled;       	
	}
	gnome_vfs_get_file_info (file, vfs_file_info, GNOME_VFS_FILE_INFO_DEFAULT);
	readable_size = gnome_vfs_format_file_size_for_display (vfs_file_info->size);
	readable_date = get_readable_date (vfs_file_info->mtime);
	
	base_name = g_path_get_basename (file);
	dir_name = g_path_get_dirname (file);
	
	utf8_base_name = g_locale_to_utf8 (base_name, -1, NULL, NULL, NULL);
	utf8_dir_name = g_locale_to_utf8 (dir_name, -1, NULL, NULL, NULL);

	gtk_list_store_append (GTK_LIST_STORE(store), iter); 
	gtk_list_store_set (GTK_LIST_STORE(store), iter,
			    COLUMN_ICON, pixbuf, 
			    COLUMN_NAME, utf8_base_name,
			    COLUMN_PATH, utf8_dir_name,
			    COLUMN_READABLE_SIZE, readable_size,
			    COLUMN_SIZE, (-1) * (gdouble) vfs_file_info->size,
			    COLUMN_TYPE, description,
			    COLUMN_READABLE_DATE, readable_date,
			    COLUMN_DATE, (-1) * (gdouble) vfs_file_info->mtime,
			    COLUMN_NO_FILES_FOUND, FALSE,
			    -1);

	if (pixbuf != NULL) {
		g_object_unref (G_OBJECT(pixbuf));
	}

	gnome_vfs_file_info_unref (vfs_file_info);
	g_free (base_name);
	g_free (dir_name);
	g_free (mime_type);
	g_free (utf8_base_name);
	g_free (utf8_dir_name);
	g_free (icon_path); 
	g_free (readable_size);
	g_free (readable_date);
}

static void
add_no_files_found_message (GtkListStore 	*store, 
			    GtkTreeIter 	*iter)
{
	/* When the list is empty append a 'No Files Found.' message. */
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(interface.tree), FALSE);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(interface.tree));
	gtk_list_store_append (GTK_LIST_STORE(store), iter); 
	gtk_list_store_set (GTK_LIST_STORE(store), iter,
		    	    COLUMN_ICON, NULL, 
			    COLUMN_NAME, _("No files found"),
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
update_search_counts (void)
{
	gchar  *title_bar_string = NULL;
	gchar  *message_string = NULL;
	gchar  *stopped_string = g_strdup ("");
	gchar  *tmp;
	gint   total_files;

	if (search_command.aborted == TRUE) {
		stopped_string = g_strdup (_("(stopped)"));
	}
	
	total_files = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(interface.model), NULL);

	if (total_files == 0) {
		title_bar_string = g_strdup (_("No Files Found"));
		message_string = g_strdup (_("No files found"));
		add_no_files_found_message (interface.model, &interface.iter);
	}
	else if (total_files == 1) {
		title_bar_string = g_strdup (_("One File Found"));
		message_string = g_strdup (_("One file found"));
	}
	else {
		title_bar_string = g_strdup_printf (_("%d Files Found"), total_files);
		message_string = g_strdup_printf (_("%d files found"), total_files);
	}
		
	if (strlen (stopped_string) > 0) {
		tmp = message_string;
		message_string = g_strconcat (message_string, " ", stopped_string, NULL);
		g_free (tmp);
		
		tmp = title_bar_string;
		title_bar_string = g_strconcat (title_bar_string, " ", stopped_string, NULL);
		g_free (tmp);
	}
		
	tmp = title_bar_string;
	title_bar_string = g_strconcat (title_bar_string, " - ", _("Search for Files"), NULL);
	gtk_window_set_title (GTK_WINDOW (interface.main_window), title_bar_string);
	g_free (tmp);
	
	gtk_label_set_text (GTK_LABEL (interface.results_label), message_string);
	
	g_free (title_bar_string);
	g_free (message_string);
	g_free (stopped_string);
}

static GtkWidget *
make_list_of_templates (void)
{
	gboolean ACTIVATE_MENU_OPTION = TRUE;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gint i;

	menu = gtk_menu_new ();

	for(i=0; templates[i].type != SEARCH_CONSTRAINT_END; i++) {
		
		if (templates[i].type == SEARCH_CONSTRAINT_SEPARATOR) {
			menuitem = gtk_separator_menu_item_new ();	
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
			gtk_widget_show (menuitem);
		} 
		else {
			gchar *text = remove_mnemonic_character (_(templates[i].desc));
			menuitem = gtk_menu_item_new_with_label (text);
			g_signal_connect (G_OBJECT(menuitem), "activate",
					  G_CALLBACK(constraint_menu_item_activate_cb),
		        		  (gpointer)(long)i);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
			gtk_widget_show (menuitem);
			g_free (text);
		}
		
		if (templates[i].is_selected) {
			gtk_widget_set_sensitive (menuitem, FALSE);
		}
		else {
			if (ACTIVATE_MENU_OPTION) {
				ACTIVATE_MENU_OPTION = FALSE;
				g_signal_emit_by_name (G_OBJECT(menuitem), "activate", NULL);
			}
		}
	}
	return menu;
}

static void
set_constraint_info_defaults (SearchConstraint *opt)
{	
	switch (templates[opt->constraint_id].type) {
	case SEARCH_CONSTRAINT_BOOL:
		break;
	case SEARCH_CONSTRAINT_TEXT:
		opt->data.text = "";
		break;
	case SEARCH_CONSTRAINT_NUMBER:
		opt->data.number = 0;
		break;
	case SEARCH_CONSTRAINT_TIME_LESS:
	case SEARCH_CONSTRAINT_TIME_MORE:
		opt->data.time = 0;
		break;
	default:
	        break;
	}
}

void
update_constraint_info (SearchConstraint 	*constraint, 
			gchar 			*info)
{
	switch (templates[constraint->constraint_id].type) {
	case SEARCH_CONSTRAINT_TEXT:
		constraint->data.text = info;
		break;
	case SEARCH_CONSTRAINT_NUMBER:
		sscanf (info, "%d", &constraint->data.number);
		break;
	case SEARCH_CONSTRAINT_TIME_LESS:
	case SEARCH_CONSTRAINT_TIME_MORE:
		sscanf (info, "%d", &constraint->data.time);
		break;
	default:
		g_warning (_("Entry changed called for a non entry option!"));
		break;
	}
}

void
set_constraint_selected_state (gint 		constraint_id, 
			       gboolean 	state)
{
	gint index;
	
	templates[constraint_id].is_selected = state;
	
	gtk_option_menu_remove_menu (GTK_OPTION_MENU(interface.constraint_menu));
	gtk_option_menu_set_menu (GTK_OPTION_MENU(interface.constraint_menu), 
				  make_list_of_templates());
	
	for (index=0; templates[index].type != SEARCH_CONSTRAINT_END; index++) {
		if (templates[index].is_selected == FALSE) {
			gtk_option_menu_set_history (GTK_OPTION_MENU(interface.constraint_menu), index);
			interface.selected_constraint = (long)index;
			gtk_widget_set_sensitive (interface.add_button, TRUE);
			return;
		}
	}
	gtk_widget_set_sensitive (interface.add_button, FALSE);
}

void
set_constraint_gconf_boolean (gint constraint_id, gboolean flag)
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

void
add_atk_namedesc (GtkWidget 	*widget, 
		  const gchar 	*name, 
		  const gchar 	*desc)
{
	AtkObject *atk_widget;

	g_return_if_fail (GTK_IS_WIDGET(widget));

	atk_widget = gtk_widget_get_accessible(widget);

	if (name != NULL)
		atk_object_set_name(atk_widget, name);
	if (desc !=NULL)
		atk_object_set_description(atk_widget, desc);
}

/*
 * add_atk_relation
 * @obj1      : The first widget in the relation @rel_type
 * @obj2      : The second widget in the relation @rel_type.
 * @rel_type  : Relation type which relates @obj1 and @obj2
 * Description: This function establishes Atk Relation between two given
 *              objects.
 */

void
add_atk_relation (GtkWidget 		*obj1, 
		  GtkWidget 		*obj2, 
		  AtkRelationType 	rel_type)
{
	AtkObject *atk_obj1, *atk_obj2;
	AtkRelationSet *relation_set;
	AtkRelation *relation;
	
	g_return_if_fail (GTK_IS_WIDGET(obj1));
	g_return_if_fail (GTK_IS_WIDGET(obj2));
	
	atk_obj1 = gtk_widget_get_accessible(obj1);
			
	atk_obj2 = gtk_widget_get_accessible(obj2);
	
	relation_set = atk_object_ref_relation_set (atk_obj1);
	relation = atk_relation_new(&atk_obj2, 1, rel_type);
	atk_relation_set_add(relation_set, relation);
	g_object_unref(G_OBJECT (relation));
	
}

gchar *
get_desktop_item_name (void)
{

	GString *gs;
	gchar *file_is_named_utf8;
	gchar *file_is_named_locale;
	gchar *look_in_folder_utf8;
	gchar *look_in_folder_locale;
	GList *list;
	
	gs = g_string_new ("");
	g_string_append (gs, _("Search for Files"));
	g_string_append (gs, " (");
	
	file_is_named_utf8 = (gchar *) gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry))));
	file_is_named_locale = g_locale_from_utf8 (file_is_named_utf8 != NULL ? file_is_named_utf8 : "" , 
	                                           -1, NULL, NULL, NULL);
	g_string_append (gs, g_strdup_printf ("named=%s", file_is_named_locale));
	g_free (file_is_named_locale);

	look_in_folder_utf8 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY(interface.look_in_folder_entry), FALSE);
	look_in_folder_locale = g_locale_from_utf8 (look_in_folder_utf8 != NULL ? look_in_folder_utf8 : "", 
	                                            -1, NULL, NULL, NULL);
	g_string_append (gs, g_strdup_printf ("&path=%s", look_in_folder_locale));
	g_free (look_in_folder_locale);
	g_free (look_in_folder_utf8); 

	if (GTK_WIDGET_VISIBLE(interface.additional_constraints)) {
		for (list = interface.selected_constraints; list != NULL; list = g_list_next (list)) {
			SearchConstraint *constraint = list->data;
			gchar *locale = NULL;
			
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

static gboolean
setup_animation_image (void)
{
	gint	   width  = 0;
	gint	   height = 0;
	GError     *error = NULL;

	interface.current_animation_frame = DEFAULT_ANIMATION_FRAME - 1;

	interface.pixbuf = gdk_pixbuf_new_from_file (GNOME_ICONDIR"/gnome-search-tool/gnome-search-tool-animation.png",
	                                             &error);
	
	if (error) {
		g_warning (_("The animation file is missing or invalid: %s"), error->message);		
		g_error_free (error);
		gtk_drawing_area_size (GTK_DRAWING_AREA (interface.drawing_area), 0, 0);
		return FALSE;
	}

	width  = gdk_pixbuf_get_width (interface.pixbuf); 
	height = gdk_pixbuf_get_height (interface.pixbuf); 

	gtk_drawing_area_size (GTK_DRAWING_AREA (interface.drawing_area), width / TOTAL_ANIMATION_FRAMES, height);
	gtk_widget_queue_draw (interface.drawing_area); 

	return TRUE;
}

static gboolean
animation_expose_event_cb (GtkWidget       *widget,
                           GdkEventExpose  *event,
                           gpointer        *data)
{
	gint width  = 0;
	gint height = 0;

	if (interface.pixbuf == NULL) {
		return FALSE;
	}

	gdk_window_clear_area (widget->window,
	                       event->area.x, event->area.y,
	                       event->area.width, event->area.height);

	gdk_gc_set_clip_rectangle (widget->style->fg_gc[widget->state], &event->area); 

	width  = gdk_pixbuf_get_width (interface.pixbuf); 
	height = gdk_pixbuf_get_height (interface.pixbuf); 

	gdk_pixbuf_render_to_drawable_alpha (interface.pixbuf,
	                                     GDK_DRAWABLE (widget->window),
	                                     (width * interface.current_animation_frame) / TOTAL_ANIMATION_FRAMES, 0, 
	                                     0, 0,
	                                     width / TOTAL_ANIMATION_FRAMES, height,
	                                     GDK_PIXBUF_ALPHA_FULL, 255,
	                                     GDK_RGB_DITHER_MAX,
	                                     0, 0);

	gdk_gc_set_clip_rectangle (widget->style->fg_gc[widget->state], NULL);

        return FALSE;
}

static void
start_animation (void)
{
	gchar *title = NULL;
	
	title = g_strconcat (_("Searching..."), " - ", _("Search for Files"), NULL);
	gtk_window_set_title (GTK_WINDOW (interface.main_window), title);
	
	gtk_label_set_text (GTK_LABEL (interface.results_label), "");
	search_command.animation_timeout = g_timeout_add (250, update_animation_timeout_cb, NULL);
	
	g_free (title);
} 

static void
stop_animation (void)
{
	interface.current_animation_frame = DEFAULT_ANIMATION_FRAME - 1;
	g_source_remove (search_command.animation_timeout);
	gtk_widget_queue_draw (interface.drawing_area);
} 

gboolean
update_animation_timeout_cb (gpointer data)
{
	static gboolean in_forward_progress = TRUE;
	
	if (in_forward_progress) {
		interface.current_animation_frame++;
		if (interface.current_animation_frame >= (TOTAL_ANIMATION_FRAMES - 1)) {
			in_forward_progress = !in_forward_progress;
		}
	}
	else {
		interface.current_animation_frame--;
		if (interface.current_animation_frame <= 0) {
			in_forward_progress = !in_forward_progress;
		}
	}
		      
	gtk_widget_queue_draw (interface.drawing_area);

	return TRUE;
}

void
define_popt_descriptions (void) 
{
	gint i = 0;
	gint j;

	options[i++].descrip = g_strdup (_("Set the text of 'Name contains'"));
	options[i++].descrip = g_strdup (_("Set the text of 'Look in folder'"));
  	options[i++].descrip = g_strdup (_("Sort files by one of the following: name, folder, size, type, or date"));
  	options[i++].descrip = g_strdup (_("Set sort order to descending, the default is ascending"));
  	options[i++].descrip = g_strdup (_("Automatically start a search"));

	for (j = 0; templates[j].type != SEARCH_CONSTRAINT_END; j++) {
		if (templates[j].type != SEARCH_CONSTRAINT_SEPARATOR) {
			gchar *text = remove_mnemonic_character (templates[j].desc);
			options[i++].descrip = g_strdup_printf (_("Select the '%s' constraint"), _(text));
			g_free (text);
		}
	}
}

static gboolean
handle_popt_args (void)
{
	gboolean popt_args_found = FALSE;
	gint sort_by;

	if (PoptArgument.name != NULL) {
		popt_args_found = TRUE;
		gtk_entry_set_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry))),
				    g_locale_to_utf8 (PoptArgument.name, -1, NULL, NULL, NULL));
	}
	if (PoptArgument.path != NULL) {
		popt_args_found = TRUE;
		gtk_entry_set_text (GTK_ENTRY(gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry))),
				    g_locale_to_utf8 (PoptArgument.path, -1, NULL, NULL, NULL));
	}	
	if (PoptArgument.contains != NULL) {
		popt_args_found = TRUE;	
		add_constraint (SEARCH_CONSTRAINT_CONTAINS_THE_TEXT, 
				PoptArgument.contains, TRUE); 	
	}
	if (PoptArgument.mtimeless != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE, 
				PoptArgument.mtimeless, TRUE);		
	}
	if (PoptArgument.mtimemore != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER, 
				PoptArgument.mtimemore, TRUE);
	}
	if (PoptArgument.sizemore != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN,
				PoptArgument.sizemore, TRUE);
	}
	if (PoptArgument.sizeless != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN,
				PoptArgument.sizeless, TRUE);
	}
	if (PoptArgument.empty) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_FILE_IS_EMPTY, NULL, TRUE);
	}
	if (PoptArgument.user != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_OWNED_BY_USER, 
				PoptArgument.user, TRUE); 
	}
	if (PoptArgument.group != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_OWNED_BY_GROUP, 
				PoptArgument.group, TRUE);
	}
	if (PoptArgument.nouser) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED, NULL, TRUE); 
	}
	if (PoptArgument.notnamed != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED, 
				PoptArgument.notnamed, TRUE);
	}
	if (PoptArgument.regex != NULL) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION, 
				PoptArgument.regex, TRUE);
	}
	if (PoptArgument.hidden) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS, NULL, TRUE);
	}
	if (PoptArgument.follow) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS, NULL, TRUE); 
	}
	if (PoptArgument.allmounts) {
		popt_args_found = TRUE;
		add_constraint (SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS, NULL, TRUE);
	}
	if (PoptArgument.sortby != NULL) {
	
		popt_args_found = TRUE;
		if (strcmp (PoptArgument.sortby, "name") == 0) {
			sort_by = COLUMN_NAME;
		}
		else if (strcmp (PoptArgument.sortby, "folder") == 0) {
			sort_by = COLUMN_PATH;
		}
		else if (strcmp (PoptArgument.sortby, "size") == 0) {
			sort_by = COLUMN_SIZE;
		}
		else if (strcmp (PoptArgument.sortby, "type") == 0) {
			sort_by = COLUMN_TYPE;
		}
		else if (strcmp (PoptArgument.sortby, "date") == 0) {
			sort_by = COLUMN_DATE;
		}
		else {
			g_warning (_("Invalid option passed to sortby command line argument.")); 
			sort_by = COLUMN_NAME;
		}
			
		if (PoptArgument.descending) {
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (interface.model), sort_by,
							      GTK_SORT_DESCENDING);
		} 
		else {
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (interface.model), sort_by, 
							      GTK_SORT_ASCENDING);
		}
	}
	if (PoptArgument.start) {
		popt_args_found = TRUE;
		click_find_cb (interface.find_button, NULL);
	}
	return popt_args_found;
}

static gboolean
handle_search_command_stdout_io (GIOChannel 	*ioc, 
				 GIOCondition	condition, 
				 gpointer 	data) 
{
	struct _SearchStruct *search_data = data;
	gboolean             broken_pipe = FALSE;

	if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) { 
	
		GError       *error = NULL;
		GString      *string;
		GdkRectangle prior_rect;
		GdkRectangle after_rect;
		gint	     look_in_folder_string_length;
		
		string = g_string_new (NULL);
		look_in_folder_string_length = strlen (search_data->look_in_folder);

		while (ioc->is_readable != TRUE);

		do {
			gint  status;
			gchar *utf8 = NULL;
			gchar *filename = NULL;
			
			if (search_data->running == MAKE_IT_STOP) {
				broken_pipe = TRUE;
				break;
			} 
			else if (search_data->running != RUNNING) {
			 	break;
			}
			
			do {
				status = g_io_channel_read_line_string (ioc, string, NULL, &error);
				
				if (status == G_IO_STATUS_EOF) {
					broken_pipe = TRUE;
				}
				
				while (gtk_events_pending ()) {
					if (search_data->running == MAKE_IT_QUIT) {
						return FALSE;
					}
					gtk_main_iteration (); 				
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
			
			if (strncmp (string->str, search_data->look_in_folder, look_in_folder_string_length) == 0) { 
			
				if (strlen (string->str) != look_in_folder_string_length) {
			
					filename = g_path_get_basename (utf8);
			
					if (fnmatch (search_data->file_is_named_pattern, filename, FNM_NOESCAPE | FNM_CASEFOLD ) != FNM_NOMATCH) {
						if (search_data->show_hidden_files) {
							if (search_data->regex_matching_enabled == FALSE) {
								add_file_to_search_results (string->str, interface.model, &interface.iter);
							} 
							else if (compare_regex (search_data->regex_string, filename)) {
								add_file_to_search_results (string->str, interface.model, &interface.iter);
							}
						}
						else if (is_path_hidden (string->str) == FALSE) {
							if (search_data->regex_matching_enabled == FALSE) {
								add_file_to_search_results (string->str, interface.model, &interface.iter);
							} 
							else if (compare_regex (search_data->regex_string, filename)) {
								add_file_to_search_results (string->str, interface.model, &interface.iter);
							}
						}
					}
				}
			}
			g_free (utf8);
			g_free (filename);
			
			gtk_tree_view_get_visible_rect (GTK_TREE_VIEW(interface.tree), &prior_rect);
			
			while (gtk_events_pending ()) {
				if (search_data->running == MAKE_IT_QUIT) {
					return FALSE;
				}
				gtk_main_iteration (); 				
			}
			
			if (prior_rect.y == 0) {
				gtk_tree_view_get_visible_rect (GTK_TREE_VIEW(interface.tree), &after_rect);
				if (after_rect.y <= 40) {  /* limit this hack to the first few pixels */
					gtk_tree_view_scroll_to_point (GTK_TREE_VIEW(interface.tree), -1, 0);
				}
			}	
			
		} while (g_io_channel_get_buffer_condition (ioc) == G_IO_IN);
		
		g_string_free (string, TRUE);
	}

	if (condition != G_IO_IN || broken_pipe == TRUE) { 
		
		if (search_data->running == MAKE_IT_STOP) {
			search_data->aborted = TRUE;
		}
		
		search_data->lock = FALSE;
		search_data->running = NOT_RUNNING;
		search_data->not_running_timeout = TRUE;
		g_timeout_add (500, not_running_timeout_cb, NULL);

		update_search_counts ();
		stop_animation ();

		gtk_window_set_default (GTK_WINDOW(interface.main_window), interface.find_button);
		gtk_widget_set_sensitive (interface.additional_constraints, TRUE);
		gtk_widget_set_sensitive (interface.expander, TRUE);
		gtk_widget_set_sensitive (interface.table, TRUE);
		gtk_widget_set_sensitive (interface.find_button, TRUE);
		gtk_widget_hide (interface.stop_button);
		gtk_widget_show (interface.find_button);

		g_io_channel_shutdown (ioc, TRUE, NULL);
		return FALSE;
	}
	return TRUE;
}

static gboolean
handle_search_command_stderr_io (GIOChannel 	*ioc, 
				 GIOCondition 	condition, 
				 gpointer 	data) 
{	
	struct _SearchStruct *search_data = data;
	static GString       *error_msgs = NULL;
	static gboolean      truncate_error_msgs = FALSE;
	gboolean             broken_pipe = FALSE;
	
	if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) { 
	
		GString         *string;
		GError          *error = NULL;
		gchar           *utf8 = NULL;
		
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
	
				while (gtk_events_pending ()) {
					if (search_data->running == MAKE_IT_QUIT) {
						break;
					}
					gtk_main_iteration ();
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
				
				GtkWidget *dialog;
				
				error_msgs = g_string_prepend (error_msgs, 
				     _("While searching the following errors were reported.\n\n"));
				
				if (truncate_error_msgs) {
					error_msgs = g_string_append (error_msgs, 
				     		_("\n... Too many errors to display ..."));
				}
				
				dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
				                                     GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_BUTTONS_OK,
				                                     _("Error while searching for files"),
				                                     error_msgs->str);
				
				g_signal_connect (G_OBJECT (dialog),
					"response",
					G_CALLBACK (gtk_widget_destroy), NULL);
				
				gtk_widget_show (dialog);
	
			}
			truncate_error_msgs = FALSE;
			g_string_truncate (error_msgs, 0);
		}
		g_io_channel_shutdown(ioc, TRUE, NULL);
		return FALSE;
	}
	return TRUE;
}

void
spawn_search_command (gchar *command)
{
	GIOChannel  *ioc_stdout;
	GIOChannel  *ioc_stderr;	
	GError      *error = NULL;
	gchar       **argv  = NULL;
	gint        child_stdout;
	gint        child_stderr;
	
	start_animation ();
	
	if (!g_shell_parse_argv (command, NULL, &argv, &error)) {
		GtkWidget *dialog;

		stop_animation ();

		dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
		                                     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_BUTTONS_OK,
					             _("Error parsing the search command"),
					             (error != NULL) ? error->message : "");

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
		g_strfreev (argv);
		return;
	}	

	if (!g_spawn_async_with_pipes (g_get_home_dir (), argv, NULL, 
				       G_SPAWN_SEARCH_PATH,
				       NULL, NULL, &search_command.pid, NULL, &child_stdout, 
				       &child_stderr, &error)) {
		GtkWidget *dialog;
		
		stop_animation ();
						
		dialog = gsearchtool_hig_dialog_new (GTK_WINDOW (interface.main_window),
		                                     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_BUTTONS_OK,
					             _("Error running the search command"),
					             (error != NULL) ? error->message : "");

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
		return;
	}
	
	search_command.lock = TRUE;
	search_command.aborted = FALSE;
	search_command.running = RUNNING; 

	gtk_window_set_default (GTK_WINDOW(interface.main_window), interface.stop_button);
	gtk_widget_show (interface.stop_button);
	gtk_widget_set_sensitive (interface.stop_button, TRUE);
	gtk_widget_hide (interface.find_button);
	gtk_widget_set_sensitive (interface.find_button, FALSE);
	gtk_widget_set_sensitive (interface.results, TRUE);
	gtk_widget_set_sensitive (interface.additional_constraints, FALSE);
	gtk_widget_set_sensitive (interface.expander, FALSE);
	gtk_widget_set_sensitive (interface.table, FALSE);

	gtk_tree_view_scroll_to_point (GTK_TREE_VIEW(interface.tree), 0, 0);
	gtk_list_store_clear (GTK_LIST_STORE(interface.model));
	
	ioc_stdout = g_io_channel_unix_new (child_stdout);
	ioc_stderr = g_io_channel_unix_new (child_stderr);

	g_io_channel_set_encoding (ioc_stdout, NULL, NULL);
	g_io_channel_set_encoding (ioc_stderr, NULL, NULL);
	
	g_io_channel_set_flags (ioc_stdout, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_flags (ioc_stderr, G_IO_FLAG_NONBLOCK, NULL);
	
	g_io_add_watch (ioc_stdout, G_IO_IN | G_IO_HUP,
			handle_search_command_stdout_io, &search_command);	
	g_io_add_watch (ioc_stderr, G_IO_IN | G_IO_HUP,
			handle_search_command_stderr_io, &search_command);

	g_io_channel_unref (ioc_stdout);
	g_io_channel_unref (ioc_stderr);
}
  
static GtkWidget *
create_constraint_box (SearchConstraint *opt, gchar *value)
{
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *button;
	
	hbox = gtk_hbox_new (FALSE, 12);

	switch(templates[opt->constraint_id].type) {
	case SEARCH_CONSTRAINT_BOOL:
		{
			gchar *text = remove_mnemonic_character (templates[opt->constraint_id].desc);
			gchar *desc = g_strconcat (LEFT_LABEL_SPACING, _(text), ".", NULL);
			label = gtk_label_new (desc);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			g_free (desc);
			g_free (text);
		}
		break;
	case SEARCH_CONSTRAINT_TEXT:
	case SEARCH_CONSTRAINT_NUMBER:
	case SEARCH_CONSTRAINT_TIME_LESS:
	case SEARCH_CONSTRAINT_TIME_MORE:
		{
			gchar *desc = g_strconcat (LEFT_LABEL_SPACING, _(templates[opt->constraint_id].desc), ":", NULL);
			label = gtk_label_new_with_mnemonic (desc);
			g_free (desc);
		}
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		
		if (templates[opt->constraint_id].type == SEARCH_CONSTRAINT_TEXT) {
			entry = gtk_entry_new();
			if (value != NULL) {
				gtk_entry_set_text (GTK_ENTRY(entry), value);
				opt->data.text = g_strdup (value);
			}
		}
		else {
			entry = gtk_spin_button_new_with_range (0, 999999999, 1);
			if (value != NULL) {
				gtk_spin_button_set_value (GTK_SPIN_BUTTON(entry), atoi(value));
				opt->data.time = atoi(value);
				opt->data.number = atoi(value);
			}
		}
		
		if (interface.is_gail_loaded)
		{
			add_atk_namedesc (GTK_WIDGET(entry), _("Search Rule Value Entry"), 
					  _("Enter a value for search rule"));
			add_atk_relation (entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
			add_atk_relation (GTK_WIDGET(label), entry, ATK_RELATION_LABEL_FOR);
		}
		
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (entry));
		
		g_signal_connect (G_OBJECT(entry), "changed",
			 	  G_CALLBACK(constraint_update_info_cb), opt);
		
		g_signal_connect (G_OBJECT (entry), "changed",
				  G_CALLBACK (constraint_entry_changed_cb),
				  NULL);
		
		g_signal_connect (G_OBJECT (entry), "activate",
				  G_CALLBACK (constraint_activate_cb),
				  NULL);
				  
		/* add label and text field */
		gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
		
		break;
	default:
		/* This should never happen.  If it does, there is a bug */
		label = gtk_label_new("???");
		gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	        break;
	}
	
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT); 
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(remove_constraint_cb), opt);
        gtk_size_group_add_widget (interface.constraint_size_group, button);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	
	if (interface.is_gail_loaded) {
		gchar *text = remove_mnemonic_character (templates[opt->constraint_id].desc);
		gchar *desc = g_strdup_printf (_("Click to Remove the '%s' Rule"), _(text));
		add_atk_namedesc (GTK_WIDGET(button), NULL, desc);
		g_free (desc);
		g_free (text);
	}
	return hbox;
}

void
add_constraint (gint constraint_id, gchar *value, gboolean show_constraint)
{
	SearchConstraint *constraint = g_new(SearchConstraint,1);
	GtkWidget *widget;

	if (show_constraint) {
		if (GTK_WIDGET_VISIBLE (interface.additional_constraints) == FALSE) {
			gtk_expander_set_expanded (GTK_EXPANDER (interface.expander), TRUE);
			gtk_widget_show (interface.additional_constraints);
		}
	} 
	
	interface.geometry.min_height += 35;
	
	if (GTK_WIDGET_VISIBLE (interface.additional_constraints)) {
		gtk_window_set_geometry_hints (GTK_WINDOW (interface.main_window), 
		                               GTK_WIDGET (interface.main_window),
		                               &interface.geometry, 
		                               GDK_HINT_MIN_SIZE);
	}
		
	constraint->constraint_id = constraint_id;
	set_constraint_info_defaults (constraint);
	set_constraint_gconf_boolean (constraint->constraint_id, TRUE);
	
	widget = create_constraint_box (constraint, value);
	gtk_box_pack_start (GTK_BOX(interface.constraint), widget, FALSE, FALSE, 0);
	gtk_widget_show_all (widget);
	
	interface.selected_constraints =
		g_list_append (interface.selected_constraints, constraint);
		
	set_constraint_selected_state (constraint->constraint_id, TRUE);
}

static GtkWidget *
create_additional_constraint_section (void)
{
	GtkWidget *hbox;
	GtkWidget *label;
	gchar *desc;

	interface.constraint = gtk_vbox_new (FALSE, 6);	

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_end (GTK_BOX(interface.constraint), hbox, FALSE, FALSE, 0);

	desc = g_strconcat (LEFT_LABEL_SPACING, _("A_vailable options:"), NULL);
	label = gtk_label_new_with_mnemonic (desc);
	g_free (desc);

	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	interface.constraint_menu = gtk_option_menu_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), GTK_WIDGET(interface.constraint_menu));
	gtk_option_menu_set_menu (GTK_OPTION_MENU(interface.constraint_menu), make_list_of_templates());
	gtk_box_pack_start (GTK_BOX(hbox), interface.constraint_menu, TRUE, TRUE, 0);

	if (interface.is_gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET (interface.constraint_menu), _("Search Rules Menu"), 
				  _("Select a search rule from the menu"));
		add_atk_relation (interface.constraint_menu, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
	}

	interface.add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	GTK_WIDGET_UNSET_FLAGS (interface.add_button, GTK_CAN_DEFAULT);
	interface.constraint_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (interface.constraint_size_group, interface.add_button);
	
	g_signal_connect (G_OBJECT(interface.add_button),"clicked",
			  G_CALLBACK(add_constraint_cb), NULL);
	
	gtk_box_pack_end (GTK_BOX(hbox), interface.add_button, FALSE, FALSE, 0); 
	
	return interface.constraint;
}

GtkWidget *
create_search_results_section (void)
{
	GtkWidget 	  *label;
	GtkWidget 	  *vbox;
	GtkWidget	  *hbox;
	GtkWidget	  *window;	
	GtkTreeViewColumn *column;	
	GtkCellRenderer   *renderer;
	
	vbox = gtk_vbox_new (FALSE, 6);	
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic (_("S_earch results:"));
	g_object_set (G_OBJECT(label), "xalign", 0.0, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	
	interface.results_label = gtk_label_new (NULL);
	g_object_set (G_OBJECT (interface.results_label), "xalign", 1.0, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), interface.results_label, FALSE, FALSE, 0);
	
	window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(window), GTK_SHADOW_NONE);	
	gtk_container_set_border_width (GTK_CONTAINER(window), 0);
	gtk_widget_set_size_request (window, 530, 160); 
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(window),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
	
	interface.model = gtk_list_store_new (NUM_COLUMNS, 
					      GDK_TYPE_PIXBUF, 
					      G_TYPE_STRING, 
					      G_TYPE_STRING, 
					      G_TYPE_STRING,
					      G_TYPE_DOUBLE,
					      G_TYPE_STRING,
					      G_TYPE_STRING,
					      G_TYPE_DOUBLE,
					      G_TYPE_BOOLEAN);
	
	interface.tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(interface.model));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(interface.tree), FALSE);						
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(interface.tree), TRUE);
  	g_object_unref (G_OBJECT(interface.model));
	
	interface.selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree));
	
	gtk_tree_selection_set_mode (GTK_TREE_SELECTION(interface.selection),
				     GTK_SELECTION_MULTIPLE);
				     
	gtk_drag_source_set (interface.tree, 
			     GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			     dnd_table, n_dnds, 
			     GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);	

	g_signal_connect (G_OBJECT (interface.tree), 
			  "drag_data_get",
			  G_CALLBACK (drag_file_cb), 
			  NULL);
			  
	g_signal_connect (G_OBJECT (interface.tree),
			  "drag_begin",
			  G_CALLBACK (drag_begin_file_cb),
			  NULL);	  
			  
	g_signal_connect (G_OBJECT(interface.tree), 
			  "event_after",
		          G_CALLBACK(file_event_after_cb), 
			  NULL);
			  
	g_signal_connect (G_OBJECT(interface.tree), 
			  "button_release_event",
		          G_CALLBACK(file_button_release_event_cb), 
			  NULL);
			  		  
	g_signal_connect (G_OBJECT(interface.tree), 
			  "button_press_event",
		          G_CALLBACK(file_button_press_event_cb), 
			  NULL);		   

	g_signal_connect (G_OBJECT(interface.tree),
			  "key_press_event",
			  G_CALLBACK(file_key_press_event_cb),
			  NULL);
			  
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), interface.tree);
	
	gtk_container_add (GTK_CONTAINER(window), GTK_WIDGET(interface.tree));
	gtk_box_pack_end (GTK_BOX (vbox), window, TRUE, TRUE, 0);
	
	/* create the name column */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", COLUMN_ICON,
                                             NULL);
					     
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_NAME,
					     NULL);				     
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);				     
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);

	/* create the folder column */
	renderer = gtk_cell_renderer_text_new (); 
	column = gtk_tree_view_column_new_with_attributes (_("Folder"), renderer,
							   "text", COLUMN_PATH,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_PATH); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);
	
	/* create the size column */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set( renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer,
							   "text", COLUMN_READABLE_SIZE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);
 
	/* create the type column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer,
							   "text", COLUMN_TYPE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_TYPE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);

	/* create the date modified column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date Modified"), renderer,
							   "text", COLUMN_READABLE_DATE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);

	return vbox;
}

GtkWidget *
create_main_window (void)
{
	GtkTargetEntry  drag_types[] = { { "text/uri-list", 0, 0 } };
	gchar 		*locale_string;
	gchar		*utf8_string;
	GtkWidget 	*hbox;	
	GtkWidget	*vbox;
	GtkWidget 	*entry;
	GtkWidget	*label;
	GtkWidget 	*folder_entry;
	GtkWidget 	*button;
	GtkWidget 	*window;

	window = gtk_vbox_new (FALSE, 6);	
	gtk_container_set_border_width (GTK_CONTAINER(window), 12);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX(window), hbox, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	interface.drawing_area = gtk_drawing_area_new ();
	gtk_box_pack_start (GTK_BOX (vbox), interface.drawing_area, TRUE, FALSE, 0);

	gtk_drawing_area_size (GTK_DRAWING_AREA (interface.drawing_area),
	                       DEFAULT_ANIMATION_WIDTH,
			       DEFAULT_ANIMATION_HEIGHT);
			  
	gtk_drag_source_set (interface.drawing_area, 
			     GDK_BUTTON1_MASK,
			     drag_types, 
			     G_N_ELEMENTS (drag_types), 
			     GDK_ACTION_COPY);
			     
	gtk_drag_source_set_icon_stock (interface.drawing_area,
	                                GTK_STOCK_FIND);
			  
	g_signal_connect (G_OBJECT (interface.drawing_area), 
			  "drag_data_get",
			  G_CALLBACK (drag_data_animation_cb), 
			  interface.main_window);

	g_signal_connect (interface.drawing_area, "expose-event",
			  G_CALLBACK (animation_expose_event_cb), NULL);
	
	interface.table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE(interface.table), 6);
	gtk_table_set_col_spacings (GTK_TABLE(interface.table), 12);
	gtk_container_add (GTK_CONTAINER(hbox), interface.table);
	
	label = gtk_label_new_with_mnemonic (_("_Name contains:"));
	gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT(label), "xalign", 0.0, NULL);
	
	gtk_table_attach (GTK_TABLE(interface.table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 1);

	interface.file_is_named_entry = gnome_entry_new ("gsearchtool-file-entry");
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry)));
	gnome_entry_set_max_saved (GNOME_ENTRY(interface.file_is_named_entry), 10);
	gtk_table_attach (GTK_TABLE(interface.table), interface.file_is_named_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0, 0);
	entry =  gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry));
       
	if (GTK_IS_ACCESSIBLE (gtk_widget_get_accessible(interface.file_is_named_entry)))
	{
		interface.is_gail_loaded = TRUE;
		add_atk_namedesc (interface.file_is_named_entry, _("Name Contains Entry"), _("Enter the file name you want to search"));
		add_atk_namedesc (entry, _("Name Contains Entry"), _("Enter the file name you want to search"));
		add_atk_relation (interface.file_is_named_entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
	}	
     
	g_signal_connect (G_OBJECT (interface.file_is_named_entry), "activate",
			  G_CALLBACK (file_is_named_activate_cb),
			  NULL);
			  
	g_signal_connect (G_OBJECT (interface.file_is_named_entry), "changed",
			  G_CALLBACK (constraint_entry_changed_cb),
			  NULL);		 
			  
	label = gtk_label_new_with_mnemonic (_("_Look in folder:"));
	gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT(label), "xalign", 0.0, NULL);
	
	gtk_table_attach(GTK_TABLE(interface.table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	
	interface.look_in_folder_entry = gnome_file_entry_new ("gsearchtool-folder-entry", _("Browse"));
	gnome_file_entry_set_directory_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry), TRUE);
	entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry));
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);
	folder_entry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry));
	gtk_table_attach (GTK_TABLE(interface.table), interface.look_in_folder_entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0, 0);
	
	if (interface.is_gail_loaded)
	{ 
		add_atk_namedesc (GTK_WIDGET(folder_entry), _("Look in Folder Entry"), _("Enter the folder name where you want to start the search"));
		add_atk_namedesc (GTK_WIDGET(entry), _("Look in Folder Entry"), _("Enter the folder name where you want to start the search"));
		add_atk_relation (folder_entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY); 
	}
	
	g_signal_connect (G_OBJECT (entry), "key_press_event",
			  G_CALLBACK (look_in_folder_key_press_cb),
			  NULL);
	
	g_signal_connect (G_OBJECT (interface.look_in_folder_entry), "changed",
			  G_CALLBACK (constraint_entry_changed_cb),
			  NULL);
			  
	locale_string = g_get_current_dir ();
	utf8_string = g_filename_to_utf8 (locale_string, -1, NULL, NULL, NULL);
	
	gtk_entry_set_text (GTK_ENTRY(entry), utf8_string);
	g_free (locale_string);
	g_free (utf8_string);
	
	interface.expander = gtk_expander_new_with_mnemonic (_("Show more _options"));
	gtk_box_pack_start (GTK_BOX(window), interface.expander, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (interface.expander),"notify::expanded",
			  G_CALLBACK (click_expander_cb), NULL);
	
	interface.additional_constraints = create_additional_constraint_section ();
	gtk_box_pack_start (GTK_BOX(window), GTK_WIDGET(interface.additional_constraints), FALSE, FALSE, 0);
	
	if (interface.is_gail_loaded)
	{ 
		add_atk_namedesc (GTK_WIDGET (interface.expander), _("Show more options"), _("Expands or collapses a list of search options."));
		add_atk_relation (GTK_WIDGET (interface.additional_constraints), GTK_WIDGET (interface.expander), ATK_RELATION_CONTROLLED_BY);
		add_atk_relation (GTK_WIDGET (interface.expander), GTK_WIDGET (interface.additional_constraints), ATK_RELATION_CONTROLLER_FOR);
	}
	
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (window), vbox, TRUE, TRUE, 0);
	
	interface.results = create_search_results_section ();
	gtk_widget_set_sensitive (GTK_WIDGET (interface.results), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), interface.results, TRUE, TRUE, 0);
	
	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	gtk_box_set_spacing (GTK_BOX(hbox), 10);
	button = gtk_button_new_from_stock (GTK_STOCK_HELP);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX(hbox), button, TRUE);
	g_signal_connect (G_OBJECT(button), "clicked",
			  G_CALLBACK(help_cb), NULL);
	
	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	g_signal_connect (G_OBJECT(button), "clicked",
			  G_CALLBACK(quit_cb), NULL);
	
	gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);
	
	interface.find_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	interface.stop_button = gtk_button_new_from_stock (GTK_STOCK_STOP);

	g_signal_connect (G_OBJECT(interface.find_button),"clicked",
			  G_CALLBACK(click_find_cb), NULL);
    	g_signal_connect (G_OBJECT(interface.find_button),"size_allocate",
			  G_CALLBACK(size_allocate_cb), NULL);
	g_signal_connect (G_OBJECT(interface.stop_button),"clicked",
			  G_CALLBACK(click_stop_cb), NULL);
	gtk_box_pack_end (GTK_BOX(hbox), interface.stop_button, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX(hbox), interface.find_button, FALSE, FALSE, 0);
	gtk_widget_set_sensitive (interface.stop_button, FALSE);
	gtk_widget_set_sensitive (interface.find_button, TRUE);
	
	if (interface.is_gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET(interface.find_button), NULL, _("Click to Start the search"));
		add_atk_namedesc (GTK_WIDGET(interface.stop_button), NULL, _("Click to Stop the search"));
	}

	return window;
}

static void
register_gsearchtool_icon (GtkIconFactory *factory)
{
	GtkIconSource	*source;
	GtkIconSet	*icon_set;

	source = gtk_icon_source_new ();

	gtk_icon_source_set_filename (source, GNOME_ICONDIR"/"GNOME_SEARCH_TOOL_ICON);

	icon_set = gtk_icon_set_new ();
	gtk_icon_set_add_source (icon_set, source);

	gtk_icon_factory_add (factory, GNOME_SEARCH_TOOL_STOCK, icon_set);

	gtk_icon_set_unref (icon_set);

	gtk_icon_source_free (source);	
}

static void
gsearchtool_init_stock_icons ()
{
	GtkIconFactory	*factory;

	gsearchtool_icon_size = gtk_icon_size_register ("panel-menu",
							GNOME_SEARCH_TOOL_DEFAULT_ICON_SIZE,
							GNOME_SEARCH_TOOL_DEFAULT_ICON_SIZE);

	factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (factory);

	register_gsearchtool_icon (factory);

	g_object_unref (factory);
}

void
set_clone_command (gint *argcp, gchar ***argvp, gpointer client_data, gboolean escape_values)
{
	gchar **argv;
	gchar *file_is_named_utf8;
	gchar *file_is_named_locale;
	gchar *look_in_folder_utf8;
	gchar *look_in_folder_locale;
	gchar *tmp;
	GList *list;
	gint  i = 0;

	argv = g_new0 (gchar*, SEARCH_CONSTRAINT_MAXIMUM_POSSIBLE);

	argv[i++] = (gchar *) client_data;

	file_is_named_utf8 = (gchar *) gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry))));
	file_is_named_locale = g_locale_from_utf8 (file_is_named_utf8 != NULL ? file_is_named_utf8 : "" , 
	                                           -1, NULL, NULL, NULL);
	if (escape_values)
		tmp = g_shell_quote (file_is_named_locale);
	else
		tmp = g_strdup (file_is_named_locale);
	argv[i++] = g_strdup_printf ("--named=%s", tmp);	
	g_free (tmp);
	g_free (file_is_named_locale);

	look_in_folder_utf8 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY(interface.look_in_folder_entry), FALSE);
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

	if (GTK_WIDGET_VISIBLE(interface.additional_constraints)) {
		for (list = interface.selected_constraints; list != NULL; list = g_list_next (list)) {
			SearchConstraint *constraint = list->data;
			gchar *locale = NULL;
			
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

void
handle_gconf_settings (void)
{
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/show_additional_options")) {
		if (GTK_WIDGET_VISIBLE (interface.additional_constraints) == FALSE) {
			gtk_expander_set_expanded (GTK_EXPANDER (interface.expander), TRUE);
			gtk_widget_show (interface.additional_constraints);
		}
	}
		      
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/contains_the_text")) {
		add_constraint (SEARCH_CONSTRAINT_CONTAINS_THE_TEXT, "", FALSE); 
	}
		
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/date_modified_less_than")) {
		add_constraint (SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/date_modified_more_than")) {
		add_constraint (SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/size_at_least")) {
		add_constraint (SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/size_at_most")) {
		add_constraint (SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/file_is_empty")) {
		add_constraint (SEARCH_CONSTRAINT_FILE_IS_EMPTY, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/owned_by_user")) {
		add_constraint (SEARCH_CONSTRAINT_OWNED_BY_USER, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/owned_by_group")) {
		add_constraint (SEARCH_CONSTRAINT_OWNED_BY_GROUP, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/owner_is_unrecognized")) {
		add_constraint (SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/name_does_not_contain")) {
		add_constraint (SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/name_matches_regular_expression")) {
		add_constraint (SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION, "", FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/show_hidden_files_and_folders")) {
		add_constraint (SEARCH_CONSTRAINT_SHOW_HIDDEN_FILES_AND_FOLDERS, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/follow_symbolic_links")) {
		add_constraint (SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS, NULL, FALSE); 
	}
	
	if (gsearchtool_gconf_get_boolean ("/apps/gnome-search-tool/select/include_other_filesystems")) {
		add_constraint (SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS, NULL, FALSE); 
	}
}

int
main (int 	argc, 
      char 	*argv[])
{
	GnomeProgram *gsearchtool;
	GnomeClient  *client;
	GtkWidget    *window;

	memset (&interface, 0, sizeof (interface));

	global_gconf_client = NULL;
	interface.geometry.min_height = MINIMUM_WINDOW_HEIGHT;
	interface.geometry.min_width  = MINIMUM_WINDOW_WIDTH;

	/* initialize the i18n stuff */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_set_application_name (_("Search for Files"));

	define_popt_descriptions ();

	gsearchtool = gnome_program_init ("gnome-search-tool", VERSION, 
					  LIBGNOMEUI_MODULE, 
					  argc, 
					  argv, 
					  GNOME_PARAM_POPT_TABLE, options,
					  GNOME_PARAM_POPT_FLAGS, 0,
					  GNOME_PARAM_APP_DATADIR, DATADIR,  
					  NULL);
	gsearchtool_init_stock_icons ();				  
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/"GNOME_SEARCH_TOOL_ICON);

	if (!bonobo_init_full (&argc, argv, bonobo_activation_orb_get (), NULL, NULL))
		g_error (_("Cannot initialize bonobo."));

	bonobo_activate ();

	interface.main_window = gnome_app_new ("gnome-search-tool", _("Search for Files"));
	gtk_window_set_wmclass (GTK_WINDOW(interface.main_window), "gnome-search-tool", "gnome-search-tool");
	gtk_window_set_policy (GTK_WINDOW(interface.main_window), TRUE, TRUE, TRUE);

	g_signal_connect (G_OBJECT(interface.main_window), "delete_event", G_CALLBACK(quit_cb), NULL);
	g_signal_connect (G_OBJECT(interface.main_window), "key_press_event", G_CALLBACK(key_press_cb), NULL);

	client = gnome_master_client ();
			 
	g_signal_connect (client, "save_yourself", G_CALLBACK (save_session_cb),
			  (gpointer)argv[0]);

	g_signal_connect (client, "die", G_CALLBACK (die_cb), NULL);

	window = create_main_window ();
	gnome_app_set_contents (GNOME_APP(interface.main_window), window);

	gtk_window_set_geometry_hints (GTK_WINDOW(interface.main_window), GTK_WIDGET(interface.main_window),
				       &interface.geometry, GDK_HINT_MIN_SIZE);

	gtk_window_set_position (GTK_WINDOW(interface.main_window), GTK_WIN_POS_CENTER);

	gtk_window_set_focus (GTK_WINDOW(interface.main_window), 
		GTK_WIDGET(gnome_entry_gtk_entry(GNOME_ENTRY(interface.file_is_named_entry))));

	GTK_WIDGET_SET_FLAGS (interface.find_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS (interface.stop_button, GTK_CAN_DEFAULT); 
	gtk_window_set_default (GTK_WINDOW(interface.main_window), interface.find_button);

	gtk_widget_show_all (window);
	gtk_widget_hide (interface.additional_constraints);
	gtk_widget_hide (interface.stop_button);
	
	gtk_widget_show (interface.main_window);
	
	interface.icon_theme = gnome_icon_theme_new ();
	gnome_icon_theme_set_allow_svg (interface.icon_theme, TRUE);

	setup_animation_image ();
	
	if (handle_popt_args () == FALSE) {
		handle_gconf_settings (); 
	}	
		 
	gtk_main ();

	return 0;
}
