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

#include <gnome.h>

#include <regex.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-ops.h> 
#include <bonobo-activation/bonobo-activation.h>

#include "gsearchtool-support.h"
#include "gsearchtool-callbacks.h"
#include "gsearchtool.h"

#define C_STANDARD_STRFTIME_CHARACTERS "aAbBcdHIjmMpSUwWxXyYZ"
#define C_STANDARD_NUMERIC_STRFTIME_CHARACTERS "dHIjmMSUwWyY"
#define BINARY_EXEC_MIME_TYPE      "application/x-executable-binary"
#define ICON_THEME_EXECUTABLE_ICON "gnome-fs-executable"
#define ICON_THEME_REGULAR_ICON    "gnome-fs-regular"


/* START OF THE GCONF FUNCTIONS */

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

gboolean 
gsearchtool_gconf_get_boolean (const gchar *key)
{
	gboolean result;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, FALSE);
	
	client = gsearchtool_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, FALSE);
	
	result = gconf_client_get_bool (client, key, &error);
	
	if (gsearchtool_gconf_handle_error (&error)) {
		result = FALSE;
	}
	
	return result;
}

void
gsearchtool_gconf_set_boolean (const gchar *key, const gboolean flag)
{
	GConfClient *client;
	GError *error = NULL;
	
	g_return_if_fail (key != NULL);
	
	client = gsearchtool_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_set_bool (client, key, flag, &error);
	
	gsearchtool_gconf_handle_error (&error);
}

/* START OF GENERIC GNOME-SEARCH-TOOL FUNCTIONS */

gboolean 
is_path_hidden (const gchar *path)
{
	gint results = FALSE;
	gchar *sub_str;
	gchar *hidden_path_substr = g_strconcat (G_DIR_SEPARATOR_S, ".", NULL);
	
	sub_str = g_strstr_len (path, strlen (path), hidden_path_substr);

	if (sub_str != NULL) {
		gchar *gnome_desktop_str = g_strconcat (G_DIR_SEPARATOR_S, ".gnome-desktop", G_DIR_SEPARATOR_S, NULL);

		/* exclude the .gnome-desktop folder */
		if (strncmp (sub_str, gnome_desktop_str, strlen (gnome_desktop_str)) == 0) {
			sub_str++;
			results = (g_strstr_len (sub_str, strlen (sub_str), hidden_path_substr) != NULL);
		}
		else {
			results = TRUE;
		}
		
		g_free (gnome_desktop_str);
	}

	g_free (hidden_path_substr);
	return results;
}

gboolean 
is_path_in_home_folder (const gchar *path)
{
	return (g_strstr_len (path, strlen (g_get_home_dir ()), g_get_home_dir ()) != NULL);
}

gboolean 
is_path_in_mount_folder (const gchar *path)
{
	return (g_strstr_len (path, strlen ("/mnt/"), "/mnt/") != NULL);
}

gboolean 
is_path_in_tmp_folder (const gchar *path)
{
	return (g_strstr_len (path, strlen ("/tmp/"), "/tmp/") != NULL);
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

gchar *
remove_mnemonic_character (const gchar *string)
{
	GString *gs;
	gboolean first_mnemonic = TRUE;
	
	if (string == NULL) {
		return NULL;
	}
	
	gs = g_string_new ("");
	for(; *string; string++) {
		if ((first_mnemonic) && (*string == '_')) {
			first_mnemonic = FALSE;
			continue;
		}
		g_string_append_c(gs, *string);
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
			
	/* Translators:  Below are the strings displayed in the 'Date Modified'
	   column of the list view.  The format of this string can vary depending 
	   on age of a file.  Please modify the format of the timestamp to match
	   your locale.  For example, to display 24 hour time replace the '%-I' 
	   with '%-H' and remove the '%p'.  (See bugzilla report #120434.) */
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

	if (g_file_test (filename, G_FILE_TEST_IS_SYMLINK)) {
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

	if (filename == NULL || mimetype == NULL) {
		icon_name = g_strdup (ICON_THEME_REGULAR_ICON);
	}
	else if ((g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE)) &&
	         (g_ascii_strcasecmp (mimetype, "application/x-executable-binary") == 0)) {
		icon_name = g_strdup (ICON_THEME_EXECUTABLE_ICON);
	}
	else {
		icon_name = gnome_icon_lookup (interface.icon_theme, NULL, filename, NULL, 
					       NULL, mimetype, 0, NULL);
	}

	icon_path = gnome_icon_theme_lookup_icon (interface.icon_theme, icon_name, ICON_SIZE, 
						  &icon_data, &base_size);
	g_free (icon_name);
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
		gint  argc;
		gchar **argv;
		gchar *command_line;
		
		command_line = g_strdup_printf("%s '%s'", mimeApp->command, 
			escape_single_quotes (filename));
		
		g_shell_parse_argv (command_line, &argc, &argv, NULL); 
			
		if (mimeApp->requires_terminal) {
			gnome_prepend_terminal_to_vector(&argc, &argv);	
		}
		
		gnome_execute_async(NULL, argc, argv);	
		gnome_vfs_mime_application_free(mimeApp);
		g_free (command_line);	
		g_strfreev (argv);
		return TRUE;
	}
	return FALSE;
}

gboolean
launch_file (const gchar *filename)
{
	gchar *mime_type = gnome_vfs_get_mime_type (filename);	
	gboolean result = FALSE;
	
	if ((g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE)) &&
	    (g_ascii_strcasecmp (mime_type, BINARY_EXEC_MIME_TYPE) == 0)) { 
		result = g_spawn_command_line_async (filename, NULL);
	}
	
	g_free (mime_type);
	return result;
}

gchar *
gsearchtool_unique_filename (const gchar *dir,
                             const gchar *suffix)
{
	const gint num_of_words = 12;
	gchar      *words[] = {
		    "foo",
		    "bar",
		    "blah",
	   	    "gegl",
		    "frobate",
		    "hadjaha",
		    "greasy",
		    "hammer",
		    "eek",
		    "larry",
		    "curly",
		    "moe",
		    NULL};
	gchar      *retval = NULL;
	gboolean   exists = TRUE;

	while (exists) {
		gchar *filename;
		gint   rnd;
		gint   word;

		rnd = rand ();
		word = rand () % num_of_words;

		filename = g_strdup_printf ("%s-%010x%s",
					    words [word],
					    (guint) rnd,
					    suffix);

		g_free (retval);
		retval = g_strconcat (dir, "/", filename, NULL);
		exists = g_file_test (retval, G_FILE_TEST_EXISTS);
		g_free (filename);
	}
	return retval;
}

GtkWidget*
gsearchtool_hig_dialog_new (GtkWindow      *parent,
                            GtkDialogFlags flags,
                            GtkButtonsType buttons,
                            const gchar    *header,
                            const gchar    *message)
{
	GtkWidget *dialog;
	GtkWidget *dialog_vbox;
	GtkWidget *dialog_action_area;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *image;
	gchar     *title;

	dialog = gtk_dialog_new ();
	
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_title (GTK_WINDOW (dialog), "");
  
	dialog_vbox = GTK_DIALOG (dialog)->vbox;
	gtk_box_set_spacing (GTK_BOX (dialog_vbox), 14);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_widget_show (hbox);

	if (buttons != GTK_BUTTONS_OK_CANCEL) {
		image = gtk_image_new_from_stock ("gtk-dialog-error", GTK_ICON_SIZE_DIALOG);
	}
	else {
		image = gtk_image_new_from_stock ("gtk-dialog-question", GTK_ICON_SIZE_DIALOG);
	}
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_widget_show (image);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show (vbox);
	
	title = g_strconcat ("<b>", header, "</b>", NULL);
	label = gtk_label_new (title);  
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_widget_show (label);
	g_free (title);
	
	label = gtk_label_new (message);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_widget_show (label);
	
	dialog_action_area = GTK_DIALOG (dialog)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area), GTK_BUTTONBOX_END);

	switch (buttons) 
  	{		
		case GTK_BUTTONS_OK_CANCEL:
	
			button = gtk_button_new_from_stock ("gtk-cancel");
  			gtk_widget_show (button);
  			gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CANCEL);
  			GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

		  	button = gtk_button_new_from_stock ("gtk-ok");
  			gtk_widget_show (button);
  			gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
  			GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
			break;
		
		case GTK_BUTTONS_OK:
		
			button = gtk_button_new_from_stock ("gtk-ok");
			gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
			GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
			gtk_widget_show (button);
			break;
		
		default:
			g_warning ("Unhandled GtkButtonsType");
			break;
  	}

	if (parent != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
	}
	if (flags & GTK_DIALOG_MODAL) {
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	}
	if (flags & GTK_DIALOG_DESTROY_WITH_PARENT) {
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	}
	
  	return dialog;
}
