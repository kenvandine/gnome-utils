/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * GNOME Search Tool
 *
 *  File:  gsearchtool-support.c
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gsearchtool-support.h"
#include "gsearchtool-callbacks.h"
#include "gsearchtool.h"

#include <regex.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtktogglebutton.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <bonobo-activation/bonobo-activation.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf.h>

#define C_STANDARD_STRFTIME_CHARACTERS "aAbBcdHIjmMpSUwWxXyYZ"
#define C_STANDARD_NUMERIC_STRFTIME_CHARACTERS "dHIjmMSUwWyY"
#define BINARY_EXEC_MIME_TYPE      "application/x-executable-binary"
#define ICON_THEME_DIRECTORY_ICON  "gnome-fs-directory"
#define ICON_THEME_EXECUTABLE_ICON "gnome-fs-executable"

/* START OF THE CDDB FUNCTIONS */

static GtkCheckButtonClass *parent_class = NULL;

struct _CDDBDisclosurePrivate {
	GtkWidget *container;
	char *shown;
	char *hidden;
	
	guint32 expand_id;
	GtkExpanderStyle style;

	int expander_size;
	int direction;
};

static void
finalize (GObject *object)
{
	CDDBDisclosure *disclosure;

	disclosure = CDDB_DISCLOSURE (object);
	if (disclosure->priv == NULL) {
		return;
	}

	g_free (disclosure->priv->hidden);
	g_free (disclosure->priv->shown);

	if (disclosure->priv->container != NULL) {
		g_object_unref (G_OBJECT (disclosure->priv->container));
	}
	
	g_free (disclosure->priv);
	disclosure->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
get_x_y (CDDBDisclosure *disclosure,
	 int *x,
	 int *y,
	 GtkStateType *state_type)
{
	GtkCheckButton *check_button;
	GdkRectangle new_area, restrict_area;
	int indicator_size, indicator_spacing;
	int focus_width;
	int focus_pad;
	gboolean interior_focus;
	GtkWidget *widget = GTK_WIDGET (disclosure);
	GtkAllocation *area = &widget->allocation;
	GtkBin *bin = GTK_BIN (disclosure);
	GtkRequisition child_requisition;
	int width, height;
	
	if (GTK_WIDGET_VISIBLE (disclosure) &&
	    GTK_WIDGET_MAPPED (disclosure)) {
		check_button = GTK_CHECK_BUTTON (disclosure);
		
		gtk_widget_style_get (widget,
				      "interior_focus", &interior_focus,
				      "focus-line-width", &focus_width,
				      "focus-padding", &focus_pad,
				      NULL);
		
		*state_type = GTK_WIDGET_STATE (widget);
		if ((*state_type != GTK_STATE_NORMAL) &&
		    (*state_type != GTK_STATE_PRELIGHT)) {
			*state_type = GTK_STATE_NORMAL;
		}

		if (bin->child) {
			width = bin->child->allocation.x - widget->allocation.x - (2 * GTK_CONTAINER (widget)->border_width);
		} else {
			width = widget->allocation.width;
		}
		
		*x = widget->allocation.x + (width) / 2;
		*y = widget->allocation.y + widget->allocation.height / 2;

		if (interior_focus == FALSE) {
			*x += focus_width + focus_pad;
		}

		*state_type = GTK_WIDGET_STATE (widget) == GTK_STATE_ACTIVE ? GTK_STATE_NORMAL : GTK_WIDGET_STATE (widget);

		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) {
			*x = widget->allocation.x + widget->allocation.width - (indicator_size + *x - widget->allocation.x);
		}
	} else {
		*x = 0;
		*y = 0;
		*state_type = GTK_STATE_NORMAL;
	}
}

static gboolean
expand_collapse_timeout (gpointer data)
{
	GdkRectangle area;
	GtkWidget *widget = data;
	CDDBDisclosure *disclosure = data;
	GtkStateType state_type;
	int x, y;
	
	gdk_window_invalidate_rect (widget->window, &widget->allocation, TRUE);
	get_x_y (disclosure, &x, &y, &state_type);
	
	gtk_paint_expander (widget->style,
			    widget->window,
			    state_type,
			    &widget->allocation,
			    widget,
			    "disclosure",
			    x, y,
			    disclosure->priv->style);

	disclosure->priv->style += disclosure->priv->direction;
	if ((int) disclosure->priv->style > (int) GTK_EXPANDER_EXPANDED) {
		disclosure->priv->style = GTK_EXPANDER_EXPANDED;

		if (disclosure->priv->container != NULL) {
			gtk_widget_show (disclosure->priv->container);
		}

		g_object_set (G_OBJECT (disclosure),
			      "label", disclosure->priv->hidden,
			      NULL);
			      
		constraint_entry_changed_cb (NULL, NULL);	      
			      
		return FALSE;
	} else if ((int) disclosure->priv->style < (int) GTK_EXPANDER_COLLAPSED) {
		disclosure->priv->style = GTK_EXPANDER_COLLAPSED;

		if (disclosure->priv->container != NULL) {
			gtk_widget_hide (disclosure->priv->container);
		}

		g_object_set (G_OBJECT (disclosure),
			      "label", disclosure->priv->shown,
			      NULL);

		constraint_entry_changed_cb (NULL, NULL);

		return FALSE;
	} else {
		return TRUE;
	}
}

static void
do_animation (CDDBDisclosure *disclosure,
	      gboolean opening)
{
	if (disclosure->priv->expand_id > 0) {
		gtk_timeout_remove (disclosure->priv->expand_id);
	}

	disclosure->priv->direction = opening ? 1 : -1;
	disclosure->priv->expand_id = g_timeout_add (50, expand_collapse_timeout, disclosure);
}

static void
toggled (GtkToggleButton *tb)
{
	CDDBDisclosure *disclosure;

	disclosure = CDDB_DISCLOSURE (tb);
	do_animation (disclosure, gtk_toggle_button_get_active (tb));

	if (disclosure->priv->container == NULL) {
		return;
	}
}

static void
draw_indicator (GtkCheckButton *check,
		GdkRectangle *area)
{
	GtkWidget *widget = GTK_WIDGET (check);
	CDDBDisclosure *disclosure = CDDB_DISCLOSURE (check);
	GtkStateType state_type;
	int x, y;

	get_x_y (disclosure, &x, &y, &state_type);
	gtk_paint_expander (widget->style,
			    widget->window,
			    state_type,
			    area,
			    widget,
			    "treeview",
			    x, y,
			    disclosure->priv->style);
}

static void
class_init (CDDBDisclosureClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkCheckButtonClass *button_class;
	GtkToggleButtonClass *toggle_class;
	
	object_class = G_OBJECT_CLASS (klass);
	widget_class = GTK_WIDGET_CLASS (klass);
	button_class = GTK_CHECK_BUTTON_CLASS (klass);
	toggle_class = GTK_TOGGLE_BUTTON_CLASS (klass);
	
	toggle_class->toggled = toggled;
	button_class->draw_indicator = draw_indicator;

	object_class->finalize = finalize;

	parent_class = g_type_class_peek_parent (klass);

	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_int ("expander_size",
								   _("Expander Size"),
								   _("Size of the expander arrow"),
								   0, G_MAXINT,
								   10, G_PARAM_READABLE));
}

static void
init (CDDBDisclosure *disclosure)
{
	disclosure->priv = g_new0 (CDDBDisclosurePrivate, 1);
	disclosure->priv->expander_size = 10;
}

GType
cddb_disclosure_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (CDDBDisclosureClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (CDDBDisclosure), 0, (GInstanceInitFunc) init
		};

		type = g_type_register_static (GTK_TYPE_CHECK_BUTTON, "CDDBDisclosure", &info, 0);
	}

	return type;
}

GtkWidget *
cddb_disclosure_new (const char *shown,
		     const char *hidden)
{
	CDDBDisclosure *disclosure;

	disclosure = g_object_new (cddb_disclosure_get_type (), "label", shown, NULL);

	disclosure->priv->shown = g_strdup (shown);
	disclosure->priv->hidden = g_strdup (hidden);
	return GTK_WIDGET (disclosure);
}

void
cddb_disclosure_set_container (CDDBDisclosure *disclosure,
			       GtkWidget *container)
{
	g_object_ref (G_OBJECT (container));
	disclosure->priv->container = container;
}

/* START OF THE GCONF FUNCTIONS */

static GConfClient *global_gconf_client = NULL;

static void
global_client_free (void)
{
	if (global_gconf_client == NULL) {
		return;
	}
	
	g_object_unref (global_gconf_client);
	global_gconf_client = NULL;
}

gboolean
gsearchtool_gconf_handle_error (GError **error)
{
	g_return_val_if_fail (error != NULL, FALSE);

	if (*error != NULL) {
		g_warning (_("GConf error:\n  %s"), (*error)->message);
		g_error_free (*error);
		*error = NULL;
		return TRUE;
	}
	return FALSE;
}

GConfClient *
gsearchtool_gconf_client_get_global (void)
{
	/* Initialize gconf if needed */
	if (!gconf_is_initialized ()) {
		char *argv[] = { "eel-preferences", NULL };
		GError *error = NULL;
		
		if (!gconf_init (1, argv, &error)) {
			if (gsearchtool_gconf_handle_error (&error)) {
				return NULL;
			}
		}
	}
	
	if (global_gconf_client == NULL) {
		global_gconf_client = gconf_client_get_default ();
	}
	
	return global_gconf_client;
}

char *
gsearchtool_gconf_get_string (const gchar *key)
{
	gchar *result;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, NULL);
	
	client = gsearchtool_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);
	
	result = gconf_client_get_string (client, key, &error);
	
	if (gsearchtool_gconf_handle_error (&error)) {
		result = g_strdup ("");
	}
	
	return result;
}

/* START OF GENERIC GNOME-SEARCH-TOOL FUNCTIONS */

gboolean 
is_path_hidden (const gchar *path)
{
	gint results;
	gchar *hidden_path_substr = g_strconcat (G_DIR_SEPARATOR_S, ".", NULL);
	
	results = (g_strstr_len (path, strlen (path), hidden_path_substr) != NULL);
	g_free (hidden_path_substr);
	return results;
}

gboolean 
is_path_in_home_folder (const gchar *path)
{
	return (g_strstr_len (path, strlen (g_get_home_dir ()), g_get_home_dir ()) != NULL);
}

gboolean
file_extension_is (const char *filename, 
		   const char *ext)
{
	int filename_l, ext_l;
	
	filename_l = strlen (filename);
	ext_l = strlen (ext);
	
	if (filename_l < ext_l)
		return FALSE;
	return strcasecmp (filename + filename_l - ext_l, ext) == 0;
}

gboolean
compare_regex (const gchar *regex, 
	       const gchar *string)
{
	regex_t regexec_pattern;
	
	regcomp (&regexec_pattern, regex, REG_NOSUB);
	
	if (regexec (&regexec_pattern, string, 0, 0, 0) != REG_NOMATCH) {
		return TRUE;
	}
	return FALSE;
}

gboolean
limit_string_to_x_lines (GString *string, 
			 gint x)
{
	int i;
	int count = 0;
	for (i = 0; string->str[i] != '\0'; i++) {
		if (string->str[i] == '\n') {
			count++;
			if (count == x) {
				g_string_truncate (string, i);
				return TRUE;
			}
		}
	}
	return FALSE;
}

gchar *
escape_single_quotes (const gchar *string)
{
	GString *gs;

	if (string == NULL) {
		return NULL;
	}

	if (count_of_char_in_string (string, '\'') == 0) {
		return g_strdup(string);
	}
	gs = g_string_new ("");
	for(; *string; string++) {
		if (*string == '\'') {
			g_string_append(gs, "'\\''");
		}
		else {
			g_string_append_c(gs, *string);
		}
	}
	return g_string_free (gs, FALSE);
}

gint
count_of_char_in_string (const gchar *string, 
			 const gchar q)
{
	int cnt = 0;
	for(; *string; string++) {
		if (*string == q) cnt++;
	}
	return cnt;
}

gchar *
get_date (const time_t file_time_raw)
{
	struct tm *file_time;
	const char *format = _("%Y%m%d%H%M%S");

	file_time = localtime (&file_time_raw);
	return strdup_strftime (format, file_time);
}

gchar *
get_readable_date (const time_t file_time_raw)
{
	struct tm *file_time;
	gchar *format;
	GDate *today;
	GDate *file_date;
	guint32 file_date_age;
	gchar *readable_date;

	file_time = localtime (&file_time_raw);
	file_date = g_date_new_dmy (file_time->tm_mday,
			       file_time->tm_mon + 1,
			       file_time->tm_year + 1900);
	
	today = g_date_new ();
	g_date_set_time (today, time (NULL));

	file_date_age = g_date_get_julian (today) - g_date_get_julian (file_date);
	 
	g_date_free (today);
	g_date_free (file_date);
			
	/* the format varies depending on age of file. */
	if (file_date_age == 0)	{
		format = g_strdup(_("today at %-I:%M %p"));
	} else if (file_date_age == 1) {
		format = g_strdup(_("yesterday at %-I:%M %p"));
	} else if (file_date_age < 7) {
		format = g_strdup(_("%m/%-d/%y, %-I:%M %p"));
	} else {
		format = g_strdup(_("%m/%-d/%y, %-I:%M %p"));
	}
	
	readable_date = strdup_strftime (format, file_time);
	g_free (format);
	
	return readable_date;
}  

gchar *
strdup_strftime (const gchar *format, 
		 struct tm *time_pieces)
{
	/* this function is based on eel */
	GString *string;
	const char *remainder, *percent;
	char code[3], buffer[512];
	char *piece, *result, *converted;
	size_t string_length;
	gboolean strip_leading_zeros, turn_leading_zeros_to_spaces;

	converted = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
	g_return_val_if_fail (converted != NULL, NULL);
	
	string = g_string_new ("");
	remainder = converted;

	for (;;) {
		percent = strchr (remainder, '%');
		if (percent == NULL) {
			g_string_append (string, remainder);
			break;
		}
		g_string_append_len (string, remainder,
				     percent - remainder);

		remainder = percent + 1;
		switch (*remainder) {
		case '-':
			strip_leading_zeros = TRUE;
			turn_leading_zeros_to_spaces = FALSE;
			remainder++;
			break;
		case '_':
			strip_leading_zeros = FALSE;
			turn_leading_zeros_to_spaces = TRUE;
			remainder++;
			break;
		case '%':
			g_string_append_c (string, '%');
			remainder++;
			continue;
		case '\0':
			g_warning ("Trailing %% passed to strdup_strftime");
			g_string_append_c (string, '%');
			continue;
		default:
			strip_leading_zeros = FALSE;
			turn_leading_zeros_to_spaces = FALSE;
			break;
		}
		
		if (strchr (C_STANDARD_STRFTIME_CHARACTERS, *remainder) == NULL) {
			g_warning ("strdup_strftime does not support "
				   "non-standard escape code %%%c",
				   *remainder);
		}

		code[0] = '%';
		code[1] = *remainder;
		code[2] = '\0';
		string_length = strftime (buffer, sizeof (buffer),
					  code, time_pieces);
		if (string_length == 0) {
			buffer[0] = '\0';
		}

		piece = buffer;
		if (strip_leading_zeros || turn_leading_zeros_to_spaces) {
			if (strchr (C_STANDARD_NUMERIC_STRFTIME_CHARACTERS, *remainder) == NULL) {
				g_warning ("strdup_strftime does not support "
					   "modifier for non-numeric escape code %%%c%c",
					   remainder[-1],
					   *remainder);
			}
			if (*piece == '0') {
				do {
					piece++;
				} while (*piece == '0');
				if (!g_ascii_isdigit (*piece)) {
				    piece--;
				}
			}
			if (turn_leading_zeros_to_spaces) {
				memset (buffer, ' ', piece - buffer);
				piece = buffer;
			}
		}
		remainder++;
		g_string_append (string, piece);
	}
	
	result = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
	g_string_free (string, TRUE);
	g_free (converted);
	return result;
}

gchar *
get_file_type_with_mime_type (const gchar *filename, 
			      const gchar *mimetype)
{
	if (filename == NULL || mimetype == NULL) {
		return (gchar *)gnome_vfs_mime_get_description (GNOME_VFS_MIME_TYPE_UNKNOWN);
	}

	if (g_file_test (filename, G_FILE_TEST_IS_SYMLINK) == TRUE) {
		if (g_ascii_strcasecmp (mimetype, GNOME_VFS_MIME_TYPE_UNKNOWN) == 0) {
			GnomeVFSFileInfo *file_info;
		
			file_info = gnome_vfs_file_info_new ();
			gnome_vfs_get_file_info (filename, file_info, GNOME_VFS_FILE_INFO_DEFAULT);
		
			if (g_file_test (file_info->symlink_name, G_FILE_TEST_EXISTS) != TRUE) {
				gnome_vfs_file_info_unref (file_info);
				return _("link (broken)");
			}
			
			gnome_vfs_file_info_unref (file_info);
		}
		return g_strdup_printf (_("link to %s"), 
			(gchar *)gnome_vfs_mime_get_description (mimetype));
	}

	return (gchar *)gnome_vfs_mime_get_description (mimetype);
} 

gchar *
get_file_icon_with_mime_type (const gchar *filename, 
			      const gchar *mimetype) 
{
	const GnomeIconData *icon_data;
	gchar *icon_name = NULL;	
	gchar *icon_path = NULL;
	int base_size;

	if (g_file_test (filename, G_FILE_TEST_IS_DIR) == TRUE) {
		icon_name = g_strdup (ICON_THEME_DIRECTORY_ICON);
	} 
	else if ((g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE) == TRUE) &&
		 (g_ascii_strcasecmp (mimetype, "application/x-executable-binary") == 0)) {
		icon_name = g_strdup (ICON_THEME_EXECUTABLE_ICON);
	}
	else {
		icon_name = gnome_icon_lookup (interface.icon_theme, NULL, filename, NULL, 
					       NULL, mimetype, 0, NULL);
	}

	icon_path = gnome_icon_theme_lookup_icon (interface.icon_theme, icon_name, ICON_SIZE, 
						  &icon_data, &base_size);
	return icon_path;	
}

gboolean
is_nautilus_running (void)
{
	CORBA_Environment ev;
	CORBA_Object obj;
	gboolean ret;
	
	CORBA_exception_init (&ev); 
	obj = bonobo_activation_activate_from_id ("OAFIID:Nautilus_Factory",
		Bonobo_ACTIVATION_FLAG_EXISTING_ONLY, NULL, &ev);
		
	ret = !CORBA_Object_is_nil (obj, &ev);
	
	CORBA_Object_release (obj, &ev);	
	CORBA_exception_free (&ev);
	
	return ret;
}

gboolean
is_component_action_type (const gchar *filename)
{
	gchar *mimeType = gnome_vfs_get_mime_type (filename);	
	GnomeVFSMimeActionType actionType = gnome_vfs_mime_get_default_action_type (mimeType);
	
	g_free (mimeType);
	
	if (actionType == GNOME_VFS_MIME_ACTION_TYPE_COMPONENT) {
		return TRUE;
	}
	return FALSE;
}

gboolean
open_file_with_nautilus (const gchar *filename)
{
	int argc = 5;
	char **argv = g_new0 (char *, argc);
	
	argv[0] = "nautilus";
	argv[1] = "--sm-disable";
	argv[2] = "--no-desktop";
	argv[3] = "--no-default-window";
	argv[4] = (gchar *)filename;
	
	gnome_execute_async(NULL, argc, argv);
	g_free(argv);
	
	return TRUE;
}

gboolean
open_file_with_application (const gchar *filename)
{
	gchar *mimeType = gnome_vfs_get_mime_type (filename);	
	GnomeVFSMimeApplication *mimeApp = gnome_vfs_mime_get_default_application (mimeType);
	
	g_free (mimeType);
		
	if (mimeApp) {
		char **argv;
		int argc = 2;

		argv = g_new0 (char *, argc);
		argv[0] = mimeApp->command;
		argv[1] = (gchar *)filename;	
		
		if (mimeApp->requires_terminal) 
			gnome_prepend_terminal_to_vector(&argc, &argv);	
		
		gnome_execute_async(NULL, argc, argv);	
		gnome_vfs_mime_application_free(mimeApp);	
		g_free(argv);
		return TRUE;
	}
	return FALSE;
}

gboolean
launch_file (const gchar *filename)
{
	gchar *mime_type = gnome_vfs_get_mime_type (filename);	
	gboolean result = FALSE;
	
	if ((g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE) == TRUE) &&
	    (g_ascii_strcasecmp (mime_type, BINARY_EXEC_MIME_TYPE) == 0)) { 
		result = g_spawn_command_line_async (filename, NULL);
	}
	
	g_free (mime_type);
	return result;
}
