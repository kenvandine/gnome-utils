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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gsearchtool.h"
#include "gsearchtool-support.h"
#include "gsearchtool-callbacks.h"

#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fnmatch.h>
#include <sys/wait.h>
#include <gdk/gdkkeysyms.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>

typedef struct _FindOptionTemplate FindOptionTemplate;
struct _FindOptionTemplate {
	SearchConstraintType type;
	gchar 	 *option;          /* the option string to pass to find or whatever */
	gchar 	 *desc;            /* description */
	gboolean is_selected;     
};
	
static FindOptionTemplate templates[] = {
	{ SEARCH_CONSTRAINT_TEXT, "-exec grep -q '%s' {} \\;", N_("Contains the text"), FALSE },
	{ SEARCH_CONSTRAINT_TEXT, "-user '%s'", N_("Owned by user"), FALSE },
	{ SEARCH_CONSTRAINT_TEXT, "-group '%s'", N_("Owned by group"), FALSE },
	{ SEARCH_CONSTRAINT_BOOL, "\\( -nouser -o -nogroup \\)", N_("Owner is unrecognized"), FALSE },
	{ SEARCH_CONSTRAINT_TIME, "-mtime -%d", N_("Date modified before (days)"), FALSE },
	{ SEARCH_CONSTRAINT_TIME, "-mtime +%d", N_("Date modified after (days)"), FALSE },	
	{ SEARCH_CONSTRAINT_NUMBER, "-size -%dk", N_("Size is less than (kilobytes)"), FALSE },
	{ SEARCH_CONSTRAINT_NUMBER, "-size +%dk", N_("Size is more than (kilobytes)"), FALSE }, 
	{ SEARCH_CONSTRAINT_BOOL, "-size 0c \\( -type f -o -type d \\)", N_("File is empty"), FALSE },
	{ SEARCH_CONSTRAINT_TEXT, "'!' -name '%s'", N_("File is not named"), FALSE },
	{ SEARCH_CONSTRAINT_TEXT, "-regex '%s'", N_("File matches regular expression"), FALSE }, 
	{ SEARCH_CONSTRAINT_BOOL, "-follow", N_("Follow symbolic links"), FALSE },
	{ SEARCH_CONSTRAINT_BOOL, "-xdev", N_("Search other filesystems"), FALSE },
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
	SEARCH_CONSTRAINT_OWNED_BY_USER, 
	SEARCH_CONSTRAINT_OWNED_BY_GROUP,
	SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED,
	SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE,
	SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER,
	SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN,
	SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN,
	SEARCH_CONSTRAINT_FILE_IS_EMPTY,
	SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED,
	SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION,
	SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS,
	SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS
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
	gboolean	follow;
	gboolean	allmounts;
	gchar 		*sortby;
	gboolean 	descending;
	gboolean 	autostart;
} PoptArgument;

struct poptOption options[] = { 
  	{ "name", '\0', POPT_ARG_STRING, &PoptArgument.name, 0, 
	  N_("Set the text of 'file is named'"), NULL},
	{ "path", '\0', POPT_ARG_STRING, &PoptArgument.path, 0, 
	  N_("Set the text of 'look in folder'"), NULL},
  	{ "sortby", '\0', POPT_ARG_STRING, &PoptArgument.sortby, 0, 
	  N_("Sort files by one of the following: name, folder, size, type, or date"), NULL},
  	{ "descending", '\0', POPT_ARG_NONE, &PoptArgument.descending, 0, 
	  N_("Set sort order to descending, the default is ascending"), NULL},
  	{ "autostart", '\0', POPT_ARG_NONE, &PoptArgument.autostart, 0, 
	  N_("Automatically start a search"), NULL},
  	{ "contains", '\0', POPT_ARG_STRING, &PoptArgument.contains, 0, 
	  N_("Select the 'Contains the text' constraint"), NULL},
	{ "user", '\0', POPT_ARG_STRING, &PoptArgument.user, 0, 
	  N_("Select the 'Owned by user' constraint"), NULL},
  	{ "group", '\0', POPT_ARG_STRING, &PoptArgument.group, 0, 
	  N_("Select the 'Owned by group' constraint"), NULL},
  	{ "nouser", '\0', POPT_ARG_NONE, &PoptArgument.nouser, 0, 
	  N_("Select the 'Owner is unrecognized' constraint"), NULL},
  	{ "mtimeless", '\0', POPT_ARG_STRING, &PoptArgument.mtimeless, 0, 
	  N_("Select the 'Date modified before (days)' constraint"), NULL},
  	{ "mtimemore", '\0', POPT_ARG_STRING, &PoptArgument.mtimemore, 0, 
	  N_("Select the 'Date modified after (days)' constraint"), NULL},
  	{ "sizeless", '\0', POPT_ARG_STRING, &PoptArgument.sizeless, 0, 
	  N_("Select the 'Size is less than (kilobytes)' constraint"), NULL},
  	{ "sizemore", '\0', POPT_ARG_STRING, &PoptArgument.sizemore, 0, 
	  N_("Select the 'Size is more than (kilobytes)' constraint"), NULL},
  	{ "empty", '\0', POPT_ARG_NONE, &PoptArgument.empty, 0, 
	  N_("Select the 'File is empty' constraint"), NULL},
  	{ "notnamed", '\0', POPT_ARG_STRING, &PoptArgument.notnamed, 0, 
	  N_("Select the 'File is not named' constraint"), NULL},
  	{ "regex", '\0', POPT_ARG_STRING, &PoptArgument.regex, 0, 
	  N_("Select the 'File matches regular expression' constraint"), NULL},
  	{ "follow", '\0', POPT_ARG_NONE, &PoptArgument.follow, 0, 
	  N_("Select the 'Follow symbolic links' constraint"), NULL},
  	{ "allmounts", '\0', POPT_ARG_NONE, &PoptArgument.allmounts, 0, 
	  N_("Select the 'Search other filesystems' constraint"), NULL},
  	{ NULL,'\0', 0, NULL, 0, NULL, NULL}
};

gchar *
build_search_command (void)
{
	GString *command;
	gchar *file_is_named_utf8;
	gchar *file_is_named_locale;
	gchar *look_in_folder_utf8;
	gchar *look_in_folder_locale;
	
	file_is_named_utf8 = (gchar *) gtk_entry_get_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry))));

	if (!file_is_named_utf8 || !*file_is_named_utf8) {
		file_is_named_locale = NULL;
	} 
	else {
		file_is_named_locale = g_filename_from_utf8 (file_is_named_utf8, -1, NULL, NULL, NULL);
	}
	
	look_in_folder_utf8 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY(interface.look_in_folder_entry), TRUE);
	look_in_folder_locale = g_filename_from_utf8 (look_in_folder_utf8, -1, NULL, NULL, NULL);
	
	if (!file_extension_is (look_in_folder_locale, G_DIR_SEPARATOR_S)) {
		look_in_folder_locale = g_strconcat (look_in_folder_locale, G_DIR_SEPARATOR_S, NULL);
	}
	
	command = g_string_new ("");
	
	if (GTK_WIDGET_VISIBLE(interface.additional_constraints) == FALSE) {
	
		gchar *locate;
		
		locate = g_find_program_in_path ("locate");
		search_command.file_is_named_pattern = escape_single_quotes (file_is_named_locale);
		
		if ((locate != NULL) && (is_path_in_home_folder (look_in_folder_locale) != TRUE)) {
			g_string_append_printf (command, "%s '%s*%s'", 
						locate, 
						look_in_folder_locale,
						search_command.file_is_named_pattern);
		} 
		else {
			g_string_append_printf (command, "find \"%s\" '!' -type d '!' -type p -name '%s' -xdev -print", 
						look_in_folder_locale, 
						search_command.file_is_named_pattern);
		}
		g_free (locate);
	} 
	else {
		GList *list;
		gboolean disable_mount_argument = FALSE;
		
		search_command.regex_matching_enabled = FALSE;
		search_command.file_is_named_pattern = escape_single_quotes (file_is_named_locale);
		
		if (search_command.file_is_named_pattern == NULL) {
			g_string_append_printf (command, "find \"%s\" '!' -type d '!' -type p -name '%s' ", 
					 	look_in_folder_locale,
						"*");
		}
		else {
			g_string_append_printf (command, "find \"%s\" '!' -type d '!' -type p -name '%s' ", 
					 	look_in_folder_locale,
						search_command.file_is_named_pattern);
		}
	
		for (list = interface.selected_constraints; list != NULL; list = g_list_next (list)) {
			
			SearchConstraint *constraint = list->data;
						
			switch (templates[constraint->constraint_id].type) {
			case SEARCH_CONSTRAINT_BOOL:
				if (strcmp (templates[constraint->constraint_id].option, "-xdev") != 0) {
					g_string_append_printf(command, "%s ",
						       templates[constraint->constraint_id].option);
				}
				else {
					disable_mount_argument = TRUE;
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
					
					g_string_append_printf (command,
						  	        templates[constraint->constraint_id].option,
						  	        locale);
					g_free(string);
					g_free(locale);

					g_string_append_c (command, ' ');
				}
				break;
			case SEARCH_CONSTRAINT_NUMBER:
				g_string_append_printf (command,
					  		templates[constraint->constraint_id].option,
					  		constraint->data.number);
				g_string_append_c (command, ' ');
				break;
			case SEARCH_CONSTRAINT_TIME:
				g_string_append_printf (command,
					 		templates[constraint->constraint_id].option,
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
	const gchar *mime_type = gnome_vfs_mime_type_from_name (file);
	gchar *description = get_file_type_with_mime_type (file, mime_type);
	gchar *icon_path = get_file_icon_with_mime_type (file, mime_type);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
	GnomeVFSFileInfo *vfs_file_info = gnome_vfs_file_info_new ();
	gchar *readable_size, *readable_date, *date;
	gchar *utf8_base_name, *utf8_dir_name;
	gchar *base_name, *dir_name;
	
	if (gtk_tree_view_get_headers_visible (GTK_TREE_VIEW(interface.tree)) == FALSE) {
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(interface.tree), TRUE);
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
	date = get_date (vfs_file_info->mtime);
	
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
			    COLUMN_SIZE, (gdouble) vfs_file_info->size,
			    COLUMN_TYPE, description,
			    COLUMN_READABLE_DATE, readable_date,
			    COLUMN_DATE, date,
			    COLUMN_NO_FILES_FOUND, FALSE,
			    -1);

	gnome_vfs_file_info_unref (vfs_file_info);
	g_object_unref (G_OBJECT(pixbuf));
	g_free (base_name);
	g_free (dir_name);
	g_free (utf8_base_name);
	g_free (utf8_dir_name);
	g_free (icon_path); 
	g_free (readable_size);
	g_free (readable_date);
	g_free (date);
}

static void
add_no_files_found_message (GtkListStore 	*store, 
			    GtkTreeIter 	*iter)
{
	/* When the list is empty append a 'No Files Found.' message. */
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(interface.tree), FALSE);
	gtk_list_store_append (GTK_LIST_STORE(store), iter); 
	gtk_list_store_set (GTK_LIST_STORE(store), iter,
		    	    COLUMN_ICON, NULL, 
			    COLUMN_NAME, _("No files found"),
		    	    COLUMN_PATH, "",
			    COLUMN_READABLE_SIZE, "",
			    COLUMN_SIZE, (gdouble) 0,
			    COLUMN_TYPE, "",
		    	    COLUMN_READABLE_DATE, "",
		    	    COLUMN_DATE, 0,
			    COLUMN_NO_FILES_FOUND, TRUE,
		    	    -1);
}

static GtkWidget *
make_list_of_templates (void)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList    *group = NULL;
	gint i;

	menu = gtk_menu_new ();

	for(i=0; templates[i].type != SEARCH_CONSTRAINT_END; i++) {
		
		menuitem = gtk_radio_menu_item_new_with_label (group,
							       _(templates[i].desc));
		g_signal_connect (G_OBJECT(menuitem), "toggled",
				  G_CALLBACK(constraint_menu_toggled_cb),
		        	  (gpointer)(long)i);
		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(menuitem));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show (menuitem);
		
		if (templates[i].is_selected == TRUE) {
			gtk_widget_set_sensitive (menuitem, FALSE);
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
	case SEARCH_CONSTRAINT_TIME:
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
	case SEARCH_CONSTRAINT_TIME:
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
			break;
		}
	}
}

static void
setup_app_progress_bar (void)
{
	interface.status_bar = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP(interface.main_window), interface.status_bar);
	interface.progress_bar = GTK_WIDGET(gnome_appbar_get_progress (GNOME_APPBAR(interface.status_bar)));
	g_object_set (G_OBJECT(interface.progress_bar), "pulse_step", 0.1, NULL);
}

gboolean
update_progress_bar (gpointer data)
{
	if (GTK_IS_PROGRESS_BAR(interface.progress_bar) == TRUE) {
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (interface.progress_bar));
	}
	return TRUE;
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

void 
handle_popt_args (void)
{
	gint sort_by;

	if (PoptArgument.name != NULL) {
		gtk_entry_set_text (GTK_ENTRY(gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry))),
				    g_locale_to_utf8 (PoptArgument.name, -1, NULL, NULL, NULL));
		gtk_widget_set_sensitive (interface.find_button, TRUE);
	}
	if (PoptArgument.path != NULL) {
		gtk_entry_set_text (GTK_ENTRY(gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry))),
				    g_locale_to_utf8 (PoptArgument.path, -1, NULL, NULL, NULL));
	}	
	if (PoptArgument.contains != NULL) {
		add_constraint (SEARCH_CONSTRAINT_CONTAINS_THE_TEXT, 
				PoptArgument.contains); 
	}
	if (PoptArgument.user != NULL) {
		add_constraint (SEARCH_CONSTRAINT_OWNED_BY_USER, 
				PoptArgument.user); 
	}
	if (PoptArgument.group != NULL) {
		add_constraint (SEARCH_CONSTRAINT_OWNED_BY_GROUP, 
				PoptArgument.group); 
	}
	if (PoptArgument.nouser == TRUE) {
		add_constraint (SEARCH_CONSTRAINT_OWNER_IS_UNRECOGNIZED, NULL); 
	}
	if (PoptArgument.mtimeless != NULL) {
		add_constraint (SEARCH_CONSTRAINT_DATE_MODIFIED_BEFORE, 
				PoptArgument.mtimeless); 
	}
	if (PoptArgument.mtimemore != NULL) {
		add_constraint (SEARCH_CONSTRAINT_DATE_MODIFIED_AFTER, 
				PoptArgument.mtimemore); 
	}
	if (PoptArgument.sizeless != NULL) {
		add_constraint (SEARCH_CONSTRAINT_SIZE_IS_LESS_THAN,
				PoptArgument.sizeless);
	}
	if (PoptArgument.sizemore != NULL) {
		add_constraint (SEARCH_CONSTRAINT_SIZE_IS_MORE_THAN,
				PoptArgument.sizemore);
	}
	if (PoptArgument.empty == TRUE) {
		add_constraint (SEARCH_CONSTRAINT_FILE_IS_EMPTY, NULL);
	}
	if (PoptArgument.notnamed != NULL) {
		add_constraint (SEARCH_CONSTRAINT_FILE_IS_NOT_NAMED, 
				PoptArgument.notnamed); 
	}
	if (PoptArgument.regex != NULL) {
		add_constraint (SEARCH_CONSTRAINT_FILE_MATCHES_REGULAR_EXPRESSION, 
				PoptArgument.regex); 
	}
	if (PoptArgument.follow == TRUE) {
		add_constraint (SEARCH_CONSTRAINT_FOLLOW_SYMBOLIC_LINKS, NULL); 
	}
	if (PoptArgument.allmounts == TRUE) {
		add_constraint (SEARCH_CONSTRAINT_SEARCH_OTHER_FILESYSTEMS, NULL); 
	}
	if (PoptArgument.sortby != NULL) {
	
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
			
		if (PoptArgument.descending == TRUE) {
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (interface.model), sort_by,
							      GTK_SORT_DESCENDING);
		} 
		else {
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (interface.model), sort_by, 
							      GTK_SORT_ASCENDING);
		}
	}
	if (PoptArgument.autostart == TRUE) {
		click_find_cb (interface.find_button, NULL);
	}
}

static gboolean
handle_search_command_stdout_io (GIOChannel 	*ioc, 
				 GIOCondition	condition, 
				 gpointer 	data) 
{
	struct _SearchStruct *search_data = data;

	if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) { 
	
		GError       *error = NULL;
		GString      *string;
		GdkRectangle prior_rect;
		GdkRectangle after_rect;
		
		string = g_string_new (NULL);

		while (ioc->is_readable != TRUE);

		do {
			gint  status;
			gchar *locale = NULL;
			gchar *filename = NULL;
			
			if (search_data->running != RUNNING) {
			 	break;
			}
			
			do {
				status = g_io_channel_read_line_string (ioc, string, NULL, &error);
				
				while (gtk_events_pending ()) {
					if (search_data->running == MAKE_IT_QUIT) {
						return FALSE;
					}
					gtk_main_iteration (); 				
				}
				
			} while (status == G_IO_STATUS_AGAIN);
			
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
			
			locale = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
			if (locale == NULL) {
				continue;
			}
			
			filename = g_path_get_basename (locale);
			
			if (fnmatch (search_data->file_is_named_pattern, filename, FNM_NOESCAPE) != FNM_NOMATCH) {
				if (search_data->show_hidden_files == TRUE) {
					if (search_data->regex_matching_enabled == FALSE) {
						add_file_to_search_results (string->str, interface.model, &interface.iter);
					} 
					else if (compare_regex (search_data->regex_string, filename) == TRUE) {
						add_file_to_search_results (string->str, interface.model, &interface.iter);
					}
				}
				else if (is_path_hidden (string->str) == FALSE) {
					if (search_data->regex_matching_enabled == FALSE) {
						add_file_to_search_results (string->str, interface.model, &interface.iter);
					} 
					else if (compare_regex (search_data->regex_string, filename) == TRUE) {
						add_file_to_search_results (string->str, interface.model, &interface.iter);
					}
				}
			}
			g_free (locale);
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
			
		} while (g_io_channel_get_buffer_condition(ioc) == G_IO_IN);
		
		g_string_free (string, TRUE);
	}

	if (condition != G_IO_IN) { 
	
		gint   total_files;
		gchar *status_bar_string = NULL;
		gchar *search_status = g_strdup ("");
		
		if (search_data->running == MAKE_IT_STOP) {
			search_status = g_strdup (_("(aborted)"));
		}
		
		search_data->lock = FALSE;
		search_data->running = NOT_RUNNING;
		
		total_files = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(interface.model), NULL);

		if (total_files == 0) {
			status_bar_string = g_strdup_printf (_("No files found %s"), 
							     search_status);
			add_no_files_found_message (interface.model, &interface.iter);
		}
		else if (total_files == 1) {
			status_bar_string = g_strdup_printf (_("One file found %s"),
							     search_status);
		}
		else {
			status_bar_string = g_strdup_printf (_("%d files found %s"), total_files,
							     search_status);
		}

		gnome_appbar_pop (GNOME_APPBAR (interface.status_bar));
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (interface.progress_bar), 0.0);
		gnome_appbar_push (GNOME_APPBAR (interface.status_bar), status_bar_string);
		gtk_timeout_remove (search_data->timeout);

		gtk_widget_set_sensitive (interface.find_button, TRUE);
		gtk_widget_hide (interface.stop_button);
		gtk_widget_show (interface.find_button);

		g_io_channel_shutdown (ioc, TRUE, NULL);
		g_free (status_bar_string);
		
		return FALSE;
	}
	return TRUE;
}

static gboolean
handle_search_command_stderr_io (GIOChannel 	*ioc, 
				 GIOCondition 	condition, 
				 gpointer 	data) 
{
	gboolean   truncate_error_msgs = FALSE;
	gchar      *locale = NULL;
	GError     *error = NULL;
	GtkWidget  *dialog;
	GString    *error_msgs;
	GString    *string;
	
	error_msgs = g_string_new (NULL);
	string = g_string_new (NULL);

	while (ioc->is_readable != TRUE);

	while (g_io_channel_read_line_string (ioc, string, NULL, &error) == G_IO_STATUS_NORMAL) {
	
		if (truncate_error_msgs == FALSE) {
			if ((strstr (string->str, "ermission denied") == NULL) &&
			    (strstr (string->str, "No such file or directory") == NULL) &&
			    (strncmp (string->str, "grep: ", 6) != 0) &&
			    (strcmp (string->str, "find: ") != 0)){
				locale = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
				error_msgs = g_string_append (error_msgs, locale); 
				truncate_error_msgs = limit_string_to_x_lines (error_msgs, 20);
			}
		}
		
		while (gtk_events_pending ()) {
			gtk_main_iteration ();
		}
	}
	
	if (error != NULL) {
		g_warning ("handle_search_command_stderr_io(): %s", error->message);
		g_error_free (error);
	}
	
	if (error_msgs->len > 0) {	
		error_msgs = g_string_prepend (error_msgs, 
			     _("While searching the following errors were reported.\n\n"));
				
		if (truncate_error_msgs == TRUE) {
			error_msgs = g_string_append (error_msgs, 
				     _("\n... Too many errors to display ..."));
		}
		
		gnome_app_error (GNOME_APP (interface.main_window), error_msgs->str);	
	}

	g_io_channel_shutdown(ioc, TRUE, NULL);
	g_string_free (error_msgs, TRUE);	
	g_string_free (string, TRUE);
	g_free (locale);
	
	return FALSE;
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
	
	if (!g_shell_parse_argv (command, NULL, &argv, &error)) {
		GtkWidget *dialog;

		gnome_appbar_pop (GNOME_APPBAR (interface.status_bar));
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (interface.progress_bar), 0.0);
		gnome_appbar_push (GNOME_APPBAR (interface.status_bar), "");
		gtk_timeout_remove (search_command.timeout);
		
		dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("Sorry, could not start the search.\n%s\n"), 
						error->message);
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (interface.main_window));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
		return;
	}	

	if (!g_spawn_async_with_pipes (g_get_home_dir (), argv, NULL, 
				       (G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
				       NULL, NULL, &search_command.pid, NULL, &child_stdout, 
				       &child_stderr, &error)) {
		GtkWidget *dialog;
		
		gnome_appbar_pop (GNOME_APPBAR (interface.status_bar));
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (interface.progress_bar), 0.0);
		gnome_appbar_push (GNOME_APPBAR (interface.status_bar), "");
		gtk_timeout_remove (search_command.timeout);
		
		dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("Sorry, could not start the search.\n%s\n"), 
						error->message);
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (interface.main_window));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
		return;
	}
	
	search_command.lock = TRUE;
	search_command.running = RUNNING; 

	gtk_widget_show (interface.stop_button);
	gtk_widget_set_sensitive (interface.stop_button, TRUE);
	gtk_widget_hide (interface.find_button);
	gtk_widget_set_sensitive (interface.find_button, FALSE);
	gtk_widget_set_sensitive (interface.results, TRUE);

	gtk_tree_view_scroll_to_point (GTK_TREE_VIEW(interface.tree), 0, 0);
	gtk_list_store_clear (GTK_LIST_STORE(interface.model));
	
	ioc_stdout = g_io_channel_unix_new (child_stdout);
	ioc_stderr = g_io_channel_unix_new (child_stderr);

	g_io_channel_set_encoding (ioc_stdout, NULL, NULL);
	g_io_channel_set_encoding (ioc_stderr, NULL, NULL);
	
	g_io_channel_set_flags (ioc_stdout, G_IO_FLAG_NONBLOCK, NULL);
	
	g_io_add_watch (ioc_stdout, G_IO_IN | G_IO_HUP,
			handle_search_command_stdout_io, &search_command);	
	g_io_add_watch (ioc_stderr, G_IO_HUP,
			handle_search_command_stderr_io, NULL);

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
	
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

	switch(templates[opt->constraint_id].type) {
	case SEARCH_CONSTRAINT_BOOL:
		{
			gchar *desc = g_strconcat (_(templates[opt->constraint_id].desc), ".", NULL);
			label = gtk_label_new (desc);
			g_free (desc);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);
		}
		break;
	case SEARCH_CONSTRAINT_TEXT:
	case SEARCH_CONSTRAINT_NUMBER:
	case SEARCH_CONSTRAINT_TIME:
		{
			gchar *desc = g_strconcat (_(templates[opt->constraint_id].desc), ":", NULL);
			label = gtk_label_new (desc);
			g_free (desc);
		}
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);
		
		entry = gtk_entry_new();

		if (interface.is_gail_loaded)
		{
			add_atk_namedesc (GTK_WIDGET(entry), _("Search Rule Value Entry"), 
					  _("Enter a value for search rule"));
			add_atk_relation (entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
			add_atk_relation (GTK_WIDGET(label), entry, ATK_RELATION_LABEL_FOR);
		}
		
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
		
		if (value != NULL) {
			gtk_entry_set_text (GTK_ENTRY(entry), value);
		}
		
		break;
	default:
		/* This should never happen.  If it does, there is a bug */
		label = gtk_label_new("???");
		gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, GNOME_PAD_SMALL);
	        break;
	}
	
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE); 
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(remove_constraint_cb), opt);
        gtk_size_group_add_widget (interface.constraint_size_group, button);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, GNOME_PAD_SMALL);
	
	if (interface.is_gail_loaded) {
		gchar *desc = g_strdup_printf (_("Click to Remove the '%s' Rule"), 
					       _(templates[opt->constraint_id].desc));
		add_atk_namedesc (GTK_WIDGET(button), NULL, desc);
		g_free (desc);
	}
	return hbox;
}

void
add_constraint (gint constraint_id, gchar *value)
{
	SearchConstraint *constraint = g_new(SearchConstraint,1);
	GtkWidget *widget;
	
	if (GTK_WIDGET_VISIBLE (interface.additional_constraints) == FALSE) {
		g_signal_emit_by_name (G_OBJECT(interface.disclosure), "clicked", 0);
		gtk_widget_show (interface.additional_constraints);
	}
	
	interface.geometry.min_height += 30; 
	gtk_window_set_geometry_hints (GTK_WINDOW(interface.main_window), 
				       GTK_WIDGET(interface.main_window),
				       &interface.geometry, GDK_HINT_MIN_SIZE);
	
	constraint->constraint_id = constraint_id;
	set_constraint_info_defaults (constraint);
	
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
	GtkWidget *vbox1; 
	GtkWidget *vbox2;
	GtkWidget *label;
	GtkWidget *button;

	vbox1 = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER(vbox1), GNOME_PAD_SMALL);

	interface.constraint = gtk_vbox_new (FALSE, 0); 
	vbox2 = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	
	gtk_box_pack_end (GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic (_("A_vailable search constraints:"));
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);
	
	interface.constraint_menu = gtk_option_menu_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), GTK_WIDGET(interface.constraint_menu));
	gtk_option_menu_set_menu (GTK_OPTION_MENU(interface.constraint_menu), make_list_of_templates());
	gtk_box_pack_start (GTK_BOX(hbox), interface.constraint_menu, TRUE, TRUE, GNOME_PAD_SMALL);

	if (interface.is_gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET (interface.constraint_menu), _("Search Rules Menu"), 
				  _("Select a search rule from the menu"));
		add_atk_relation (interface.constraint_menu, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
	}

	button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	interface.constraint_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_widget_set_size_request (GTK_WIDGET(button), 78, -1);
	gtk_size_group_add_widget (interface.constraint_size_group, button);
	
	g_signal_connect (G_OBJECT(button),"clicked",
			  G_CALLBACK(add_constraint_cb),NULL);
	
	gtk_box_pack_end (GTK_BOX(hbox), button, FALSE, FALSE, GNOME_PAD_SMALL); 
		
	gtk_box_pack_start (GTK_BOX(vbox1), interface.constraint, TRUE, TRUE, 0);
	
	gtk_container_add (GTK_CONTAINER(vbox1), GTK_WIDGET(vbox2));
	
	return vbox1;
}

GtkWidget *
create_search_results_section (void)
{
	GtkWidget 	  *label;
	GtkWidget 	  *frame;	
	GtkTreeViewColumn *column;	
	GtkCellRenderer   *renderer;
	
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_NONE); 
	
	label = gtk_label_new_with_mnemonic (_("S_earch Results"));
	gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT(label), "xalign", 0.0, NULL);
	gtk_frame_set_label_widget (GTK_FRAME(frame), label);
	
	interface.results = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_sensitive (GTK_WIDGET(interface.results), FALSE); 
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(interface.results), GTK_SHADOW_IN);	
	gtk_container_set_border_width (GTK_CONTAINER(interface.results), GNOME_PAD_SMALL);
	gtk_widget_set_usize (interface.results, 530, 200); 
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(interface.results),
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
					      G_TYPE_STRING,
					      G_TYPE_BOOLEAN);
	
	interface.tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(interface.model));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(interface.tree), FALSE);						
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(interface.tree), TRUE);
  	g_object_unref (G_OBJECT(interface.model));
	
	interface.selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(interface.tree));
	
	gtk_drag_source_set (interface.tree, 
			     GDK_BUTTON1_MASK,
			     dnd_table, n_dnds, 
			     GDK_ACTION_COPY);	

	g_signal_connect (G_OBJECT (interface.tree), 
			  "drag_data_get",
			  G_CALLBACK (drag_file_cb), 
			  NULL);
			  
	g_signal_connect (G_OBJECT(interface.tree), 
			  "button_press_event",
		          G_CALLBACK(click_file_cb), 
			  NULL);		   

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), interface.tree);
	
	gtk_container_add (GTK_CONTAINER(interface.results), GTK_WIDGET(interface.tree));
	gtk_container_add (GTK_CONTAINER(frame), GTK_WIDGET(interface.results));
	
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
					     
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);				     
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);

	/* create the folder column */
	renderer = gtk_cell_renderer_text_new (); 
	column = gtk_tree_view_column_new_with_attributes (_("Folder"), renderer,
							   "text", COLUMN_PATH,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
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
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);
 
	/* create the type column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer,
							   "text", COLUMN_TYPE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_TYPE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);

	/* create the date modified column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date Modified"), renderer,
							   "text", COLUMN_READABLE_DATE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(interface.tree), column);

	return frame;
}

GtkWidget *
create_main_window (void)
{
	gchar 		*string;
	GtkWidget 	*hbox;	
	GtkWidget 	*label;
	GtkWidget 	*entry;
	GtkWidget 	*folder_entry;
	GtkWidget 	*button;
	GtkWidget 	*hsep;
	GtkWidget 	*table;
	GtkWidget 	*results;
	GtkWidget 	*window;
	GtkWidget	*image;

	window = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER(window), GNOME_PAD);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX(window), hbox, FALSE, FALSE, GNOME_PAD_SMALL);
	
	image = gtk_image_new_from_file (GNOME_ICONDIR"/gnome-searchtool.png");
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, GNOME_PAD_SMALL);
	
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE(table), GNOME_PAD);
	gtk_table_set_col_spacings (GTK_TABLE(table), GNOME_PAD);
	gtk_container_add (GTK_CONTAINER(hbox), table);
	
	label = gtk_label_new_with_mnemonic (_("File is _named:"));
	gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT(label), "xalign", 0.0, NULL);
	
	gtk_table_attach (GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 1);

	interface.file_is_named_entry = gnome_entry_new ("gsearchtool-file-entry");
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry)));
	gnome_entry_set_max_saved (GNOME_ENTRY(interface.file_is_named_entry), 10);
	gtk_table_attach (GTK_TABLE(table), interface.file_is_named_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	entry =  gnome_entry_gtk_entry (GNOME_ENTRY(interface.file_is_named_entry));
       
	if (GTK_IS_ACCESSIBLE (gtk_widget_get_accessible(interface.file_is_named_entry)))
	{
		interface.is_gail_loaded = TRUE;
		add_atk_namedesc (interface.file_is_named_entry, _("File Name Entry"), _("Enter the file name you want to search"));
		add_atk_namedesc (entry, _("File Name Entry"), _("Enter the file name you want to search"));
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
	
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	
	interface.look_in_folder_entry = gnome_file_entry_new ("gsearchtool-folder-entry", _("Browse"));
	gnome_file_entry_set_directory_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry), TRUE);
	entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry));
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);
	folder_entry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY(interface.look_in_folder_entry));
	gtk_table_attach (GTK_TABLE(table), interface.look_in_folder_entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	
	if (interface.is_gail_loaded)
	{ 
		add_atk_namedesc (GTK_WIDGET(folder_entry), _("Starting folder entry"), _("Enter the folder name where you want to start the search"));
		add_atk_namedesc (GTK_WIDGET(entry), _("Starting folder entry"), _("Enter the folder name where you want to start the search"));
		add_atk_relation (folder_entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY); 
	}
	
	g_signal_connect (G_OBJECT (entry), "key_press_event",
			  G_CALLBACK (look_in_folder_key_press_cb),
			  NULL);
	
	g_signal_connect (G_OBJECT (interface.look_in_folder_entry), "changed",
			  G_CALLBACK (constraint_entry_changed_cb),
			  NULL);
			  
	string = g_get_current_dir ();
	gtk_entry_set_text (GTK_ENTRY(entry), string);
	g_free (string);
	
	interface.disclosure = cddb_disclosure_new (_("Additional Constraints"), _("Additional Constraints"));
	gtk_box_pack_start (GTK_BOX(window), interface.disclosure, FALSE, FALSE, 0);
	
	interface.additional_constraints = create_additional_constraint_section ();
  	cddb_disclosure_set_container (CDDB_DISCLOSURE(interface.disclosure), interface.additional_constraints);

	gtk_box_pack_start (GTK_BOX(window), GTK_WIDGET(interface.additional_constraints), FALSE, FALSE, 0);
	
	hsep = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX(window),hsep,FALSE,FALSE,0);
	
	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start (GTK_BOX(window), hbox, FALSE, TRUE, GNOME_PAD_SMALL);

	gtk_box_set_spacing (GTK_BOX(hbox), GNOME_PAD);
	button = gtk_button_new_from_stock (GTK_STOCK_HELP);
	gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, GNOME_PAD_SMALL);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX(hbox), button, TRUE);
	g_signal_connect (G_OBJECT(button), "clicked",
			  G_CALLBACK(help_cb), NULL);
	
	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	g_signal_connect (G_OBJECT(button), "clicked",
			  G_CALLBACK(quit_cb), NULL);
	
	gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, GNOME_PAD_SMALL);
	
	interface.find_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	interface.stop_button = gtk_button_new_from_stock (GTK_STOCK_STOP);

	g_signal_connect (G_OBJECT(interface.find_button),"clicked",
			  G_CALLBACK(click_find_cb), NULL);
	g_signal_connect (G_OBJECT(interface.stop_button),"clicked",
			  G_CALLBACK(click_stop_cb), NULL);
	gtk_box_pack_end (GTK_BOX(hbox), interface.stop_button, FALSE, FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_end (GTK_BOX(hbox), interface.find_button, FALSE, FALSE, GNOME_PAD_SMALL);
	gtk_widget_set_sensitive (interface.stop_button, FALSE);
	gtk_widget_set_sensitive (interface.find_button, FALSE);
	
	if (interface.is_gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET(interface.find_button), NULL, _("Click to Start the search"));
		add_atk_namedesc (GTK_WIDGET(interface.stop_button), NULL, _("Click to Stop the search"));
	}

	results = create_search_results_section ();
	gtk_box_pack_start (GTK_BOX(window), results, TRUE, TRUE, GNOME_PAD_SMALL);

	return window;
}

int
main (int 	argc, 
      char 	*argv[])
{
	GnomeProgram *gsearchtool;
	GnomeClient  *client;
	GtkWidget    *window;

	interface.geometry.min_height = 310;
	interface.geometry.min_width  = 422;

	/* initialize the i18n stuff */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gsearchtool = gnome_program_init ("gnome-search-tool", VERSION, 
					  LIBGNOMEUI_MODULE, 
					  argc, 
					  argv, 
					  GNOME_PARAM_POPT_TABLE, options,
					  GNOME_PARAM_POPT_FLAGS, 0,
					  GNOME_PARAM_APP_DATADIR, DATADIR,  
					  NULL);
					  
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-searchtool.png");

	if (!bonobo_init (bonobo_activation_orb_get (), NULL, NULL))
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

	setup_app_progress_bar ();

	gtk_window_set_geometry_hints (GTK_WINDOW(interface.main_window), GTK_WIDGET(interface.main_window),
				       &interface.geometry, GDK_HINT_MIN_SIZE);

	gtk_window_set_type_hint (GTK_WINDOW(interface.main_window),GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_window_set_focus (GTK_WINDOW(interface.main_window), 
		GTK_WIDGET(gnome_entry_gtk_entry(GNOME_ENTRY(interface.file_is_named_entry))));

	GTK_WIDGET_SET_FLAGS (interface.find_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS (interface.stop_button, GTK_CAN_DEFAULT); 
	gtk_window_set_default (GTK_WINDOW(interface.main_window), interface.find_button);

	gtk_widget_show_all (window);
	gtk_widget_hide (interface.additional_constraints);
	gtk_widget_hide (interface.stop_button);
	
	gtk_widget_show (interface.main_window);
	
	handle_popt_args ();
		
	gtk_main ();

	return 0;
}
