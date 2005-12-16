/* gdict-window.c - main application window
 *
 * This file is part of GNOME Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib/goption.h>
#include <glib/gi18n.h>
#include <libgnome/libgnome.h>
#include <libgnome/gnome-help.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprint/gnome-print-pango.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>

#include "gdict.h"

#include "gdict-print.h"
#include "gdict-pref-dialog.h"
#include "gdict-about.h"
#include "gdict-window.h"

#define GDICT_WINDOW_MIN_WIDTH	  400
#define GDICT_WINDOW_MIN_HEIGHT	  330

enum
{
  PROP_0,
  
  PROP_SOURCE_LOADER,
  PROP_WINDOW_ID,
  PROP_PRINT_FONT,
  PROP_WORD
};

enum
{
  CREATED,
  
  LAST_SIGNAL
};

static guint gdict_window_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry drop_types[] =
{
  { "text/plain",    0, 0 },
  { "TEXT",          0, 0 },
  { "STRING",        0, 0 },
  { "UTF8_STRING",   0, 0 },
};
static const guint n_drop_types = G_N_ELEMENTS (drop_types);



G_DEFINE_TYPE (GdictWindow, gdict_window, GTK_TYPE_WINDOW);


/* shows an error dialog making it transient for @parent */
static void
show_error_dialog (GtkWindow   *parent,
		   const gchar *message,
		   const gchar *detail)
{
  GtkWidget *dialog;
  
  dialog = gtk_message_dialog_new (parent,
  				   GTK_DIALOG_DESTROY_WITH_PARENT,
  				   GTK_MESSAGE_ERROR,
  				   GTK_BUTTONS_OK,
  				   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), "");
  
  if (detail)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
  					      "%s", detail);
  
  if (parent->group)
    gtk_window_group_add_window (parent->group, GTK_WINDOW (dialog));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

static void
gdict_window_finalize (GObject *object)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  if (window->client)
    g_object_unref (window->client);
  
  if (window->context)
    {
      g_signal_handler_disconnect (window->context, window->lookup_start_id);
      g_signal_handler_disconnect (window->context, window->lookup_end_id);
      g_signal_handler_disconnect (window->context, window->error_id);

      g_object_unref (window->context);
    }
  
  if (window->loader)
    g_object_unref (window->loader);
  
  if (window->ui_manager)
    g_object_unref (window->ui_manager);
  
  g_object_unref (window->action_group);

  g_free (window->source_name);  
  g_free (window->print_font);
  g_free (window->word);
  
  G_OBJECT_CLASS (gdict_window_parent_class)->finalize (object);
}

static void
gdict_window_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      if (window->loader)
        g_object_unref (window->loader);
      window->loader = g_value_get_object (value);
      g_object_ref (window->loader);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_window_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      g_value_set_object (value, window->loader);
      break;
    case PROP_WINDOW_ID:
      g_value_set_uint (value, window->window_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_window_cmd_file_new (GtkAction   *action,
			   GdictWindow *window)
{
  GtkWidget *new_window;
  
  new_window = gdict_window_new (window->loader);
  gtk_widget_show (new_window);
  
  g_signal_emit (window, gdict_window_signals[CREATED], 0, new_window);
}

static void
gdict_window_cmd_save_as (GtkAction   *action,
			  GdictWindow *window)
{
  GtkWidget *dialog;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  dialog = gtk_file_chooser_dialog_new (_("Save a Copy"),
  					GTK_WINDOW (window),
  					GTK_FILE_CHOOSER_ACTION_SAVE,
  					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
  					NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  /* default to user's home */
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("Untitled document"));
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;
      gchar *text;
      gsize len;
      GError *write_error = NULL;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      
      text = gdict_defbox_get_text (GDICT_DEFBOX (window->defbox), &len);
      
      g_file_set_contents (filename,
      			   text,
      			   len,
      			   &write_error);
      if (write_error)
        {
          gchar *message;
          
          message = g_strdup_printf (_("Error while writing to '%s'"), filename);
          
          show_error_dialog (GTK_WINDOW (window),
          		     message,
          		     write_error->message);
          
          g_error_free (write_error);
          g_free (message);
        }
      
      g_free (text);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
gdict_window_cmd_file_print (GtkAction   *action,
			     GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_show_print_dialog (GTK_WINDOW (window),
  			   _("Print"),
  			   GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_file_close_window (GtkAction   *action,
				    GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gtk_widget_destroy (GTK_WIDGET (window));
}

static void
gdict_window_cmd_edit_copy (GtkAction   *action,
			    GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_copy_to_clipboard (GDICT_DEFBOX (window->defbox),
		  		  gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

static void
gdict_window_cmd_edit_select_all (GtkAction   *action,
				  GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_select_all (GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_edit_find (GtkAction   *action,
			    GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_defbox_set_show_find (GDICT_DEFBOX (window->defbox), TRUE);
}

static void
gdict_window_cmd_edit_find_next (GtkAction   *action,
				 GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_find_next (GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_edit_find_previous (GtkAction   *action,
				     GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_find_previous (GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_edit_preferences (GtkAction   *action,
				   GdictWindow *window)
{
  GtkWidget *pref_dialog;
  
  pref_dialog = gdict_pref_dialog_new (GTK_WINDOW (window),
  				       _("Preferences"),
  				       window->loader);
  
  gtk_dialog_run (GTK_DIALOG (pref_dialog));
  
  gtk_widget_destroy (pref_dialog);
}

static void
gdict_window_cmd_go_first_def (GtkAction   *action,
			       GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  window->last_definition = 0;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_go_previous_def (GtkAction   *action,
				  GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  if (window->last_definition == 0)
    return;
  
  window->last_definition -= 1;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_go_next_def (GtkAction   *action,
			      GdictWindow *window)
{
  gint max;
  g_assert (GDICT_IS_WINDOW (window));
  
  if (window->max_definition == -1)
    window->max_definition = gdict_defbox_count_definitions (GDICT_DEFBOX (window->defbox)) - 1;
    
  if (window->last_definition == window->max_definition)
    return;
  
  window->last_definition += 1;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_go_last_def (GtkAction   *action,
			      GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  if (window->max_definition == -1)
    window->last_definition = gdict_defbox_count_definitions (GDICT_DEFBOX (window->defbox)) - 1;
  
  window->last_definition = window->max_definition;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_help_contents (GtkAction   *action,
				GdictWindow *window)
{
  GError *err = NULL;
  
  g_return_if_fail (GDICT_IS_WINDOW (window));
  
  gnome_help_display_desktop_on_screen (NULL,
  					"gnome-dictionary",
  					"gnome-dictionary",
  					"gnome-dictionary-application",
  					gtk_widget_get_screen (GTK_WIDGET (window)),
  					&err);
  if (err)
    {
      show_error_dialog (GTK_WINDOW (window),
      			 _("There was an error displaying help"),
      			 err->message);
      g_error_free (err);
    }
}

static void
gdict_window_cmd_help_about (GtkAction   *action,
			     GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_show_about_dialog (GTK_WIDGET (window));
}

static void
gdict_window_cmd_lookup (GtkAction   *action,
			 GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gtk_widget_grab_focus (window->entry);
}

static void
gdict_window_cmd_escape (GtkAction   *action,
			 GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_defbox_set_show_find (GDICT_DEFBOX (window->defbox), FALSE);
}

static const GtkActionEntry entries[] =
{
  { "File", NULL, N_("_File") },
  { "Edit", NULL, N_("_Edit") },
  { "Go", NULL, N_("_Go") },
  { "Help", NULL, N_("_Help") },

  /* File menu */
  { "FileNew", GTK_STOCK_NEW, N_("_New"), "<control>N",
    N_("New look up"), G_CALLBACK (gdict_window_cmd_file_new) },
  { "FileSaveAs", GTK_STOCK_SAVE_AS, N_("_Save a Copy..."), NULL, NULL,
    G_CALLBACK (gdict_window_cmd_save_as) },
  { "FilePrint", GTK_STOCK_PRINT, N_("_Print..."), "<control>P",
    N_("Print this document"), G_CALLBACK (gdict_window_cmd_file_print) },
  { "FileCloseWindow", GTK_STOCK_CLOSE, NULL, "<control>W", NULL,
    G_CALLBACK (gdict_window_cmd_file_close_window) },

  /* Edit menu */
  { "EditCopy", GTK_STOCK_COPY, NULL, "<control>C", NULL,
    G_CALLBACK (gdict_window_cmd_edit_copy) },
  { "EditSelectAll", NULL, N_("Select _All"), "<control>A", NULL,
    G_CALLBACK (gdict_window_cmd_edit_select_all) },
  { "EditFind", GTK_STOCK_FIND, NULL, "<control>F",
    N_("Find a word or phrase in the document"),
    G_CALLBACK (gdict_window_cmd_edit_find) },
  { "EditFindNext", NULL, N_("Find Ne_xt"), "<control>G", NULL,
    G_CALLBACK (gdict_window_cmd_edit_find_next) },
  { "EditFindPrevious", NULL, N_("Find Pre_vious"), "<control><shift>G", NULL,
    G_CALLBACK (gdict_window_cmd_edit_find_previous) },
  { "EditPreferences", NULL, N_("_Preferences"), NULL, NULL,
    G_CALLBACK (gdict_window_cmd_edit_preferences) },

  /* Go menu */
  { "GoPreviousDef", GTK_STOCK_GO_BACK, N_("_Previous Definition"), "<control>Page_Up",
    N_("Go to the previous definition"), G_CALLBACK (gdict_window_cmd_go_previous_def) },
  { "GoNextDef", GTK_STOCK_GO_FORWARD, N_("_Next Definition"), "<control>Page_Down",
    N_("Go to the next definition"), G_CALLBACK (gdict_window_cmd_go_next_def) },
  { "GoFirstDef", GTK_STOCK_GOTO_FIRST, N_("_First Definition"), "<control>Home",
    N_("Go to the first definition"), G_CALLBACK (gdict_window_cmd_go_first_def) },
  { "GoLastDef", GTK_STOCK_GOTO_LAST, N_("_Last Definition"), "<control>End",
    N_("Go to the last definition"), G_CALLBACK (gdict_window_cmd_go_last_def) },

  /* Help menu */
  { "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", NULL,
    G_CALLBACK (gdict_window_cmd_help_contents) },
  { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL,
    G_CALLBACK (gdict_window_cmd_help_about) },
  
  /* Accellerators */
  { "Lookup", NULL, "", "<control>L", NULL, G_CALLBACK (gdict_window_cmd_lookup) },
  { "Escape", NULL, "", "Escape", "", G_CALLBACK (gdict_window_cmd_escape) },
  { "Slash", GTK_STOCK_FIND, NULL, "slash", NULL, G_CALLBACK (gdict_window_cmd_edit_find) },
};

static void
gdict_window_lookup_start_cb (GdictContext *context,
			      GdictWindow  *window)
{
  gchar *message;

  if (!window->word)
    return;

  message = g_strdup_printf (_("Searching for '%s'..."), window->word);
  
  if (window->status)
    gtk_statusbar_push (GTK_STATUSBAR (window->status), 0, message);

  g_free (message);
}

static void
gdict_window_lookup_end_cb (GdictContext *context,
			    GdictWindow  *window)
{
  gchar *message;
  gint count;

  count = gdict_defbox_count_definitions (GDICT_DEFBOX (window->defbox));

  if (count == -1)
    window->max_definition = -1;
  else
    window->max_definition = count;

  if (count == -1)
    message = g_strdup (_("No definitions found"));
  else if (count == 1)
    message = g_strdup (_("A definition found"));
  else
    message = g_strdup_printf (_("%d definitions found"), count);

  if (window->status)
    gtk_statusbar_push (GTK_STATUSBAR (window->status), 0, message);

  g_free (message);
}

static void
gdict_window_error_cb (GdictContext *context,
		       const GError *error,
		       GdictWindow  *window)
{
  if (window->word)
    g_free (window->word);

  gtk_statusbar_push (GTK_STATUSBAR (window->status), 0,
		      _("No definitions found"));
}

static GdictContext *
get_context_from_loader (GdictWindow *window)
{
  GdictSource *source;
  GdictContext *retval;

  if (!window->source_name)
    window->source_name = g_strdup (GDICT_DEFAULT_SOURCE_NAME);

  source = gdict_source_loader_get_source (window->loader,
		  			   window->source_name);
  if (!source)
    {
      show_error_dialog (GTK_WINDOW (window),
                         _("Unable to find a dictionary source"),
                         NULL);

      return NULL;
    }
  
  retval = gdict_source_get_context (source);

  /* attach our callbacks */
  window->lookup_start_id = g_signal_connect (retval, "lookup-start",
		  			      G_CALLBACK (gdict_window_lookup_start_cb),
					      window);
  window->lookup_end_id = g_signal_connect (retval, "lookup-end",
		  			    G_CALLBACK (gdict_window_lookup_end_cb),
					    window);
  window->error_id = g_signal_connect (retval, "error",
		  		       G_CALLBACK (gdict_window_error_cb),
				       window);
  
  g_object_unref (source);
  
  return retval;
}

static void
gdict_window_gconf_notify_cb (GConfClient *client,
			      guint        cnxn_id,
			      GConfEntry  *entry,
			      gpointer     user_data)
{
  GdictWindow *window = GDICT_WINDOW (user_data);

  if (strcmp (entry->key, GDICT_GCONF_PRINT_FONT_KEY) == 0)
    {
      g_free (window->print_font);

      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        window->print_font = g_strdup (gconf_value_get_string (entry->value));
      else
        window->print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
    }
  else if (strcmp (entry->key, GDICT_GCONF_SOURCE_KEY) == 0)
    {
      if (window->context)
        {
          g_signal_handler_disconnect (window->context, window->lookup_start_id);
	  g_signal_handler_disconnect (window->context, window->lookup_end_id);
	  g_signal_handler_disconnect (window->context, window->error_id);

	  g_object_unref (window->context);
	}

      g_free (window->source_name);
      
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        window->source_name = g_strdup (gconf_value_get_string (entry->value));
      else
        window->source_name = g_strdup (GDICT_DEFAULT_SOURCE_NAME);

      window->context = get_context_from_loader (window);
    }
}

static void
entry_activate_cb (GtkWidget   *widget,
		   GdictWindow *window)
{
  const gchar *word;
  gchar *title;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  word = gtk_entry_get_text (GTK_ENTRY (widget));
  if (!word)
    return;
  
  if (window->word)
    g_free (window->word);
  
  window->word = g_strdup (word);
  
  title = g_strdup_printf (_("Dictionary - %s"), window->word);
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_free (title);
  
  window->max_definition = -1;
  window->last_definition = 0;
  
  gdict_defbox_lookup (GDICT_DEFBOX (window->defbox), window->word);
}

static void
gdict_window_drag_data_received_cb (GtkWidget        *widget,
				    GdkDragContext   *context,
				    gint              x,
				    gint              y,
				    GtkSelectionData *data,
				    guint             info,
				    guint             time_,
				    gpointer          user_data)
{
  GdictWindow *window = GDICT_WINDOW (user_data);
  gchar *text;
  
  text = (gchar *) gtk_selection_data_get_text (data);
  if (text)
    {
      gchar *title;
      
      g_message ("(in %s) text := '%s'\n", G_STRFUNC, text);
      
      gtk_entry_set_text (GTK_ENTRY (window->entry), text);

      if (window->word)
        g_free (window->word);
      
      window->word = g_strdup (text);
      
      title = g_strdup_printf (_("Dictionary - %s"), window->word);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
      
      window->max_definition = -1;
      window->last_definition = 0;

      gdict_defbox_lookup (GDICT_DEFBOX (window->defbox), window->word);

      gtk_drag_finish (context, TRUE, FALSE, time_);
      
      g_free (text);
    }
  else
    gtk_drag_finish (context, FALSE, FALSE, time_);
}

static GObject *
gdict_window_constructor (GType                  type,
			  guint                  n_construct_properties,
			  GObjectConstructParam *construct_params)
{
  GObject *object;
  GdictWindow *window;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GError *error;
  
  object = G_OBJECT_CLASS (gdict_window_parent_class)->constructor (type,
  						   n_construct_properties,
  						   construct_params);
  
  window = GDICT_WINDOW (object);
  
  gtk_widget_push_composite_child ();
  
  gtk_window_set_title (GTK_WINDOW (window), _("Dictionary"));
  gtk_window_set_default_size (GTK_WINDOW (window),
  			       GDICT_WINDOW_MIN_WIDTH,
  			       GDICT_WINDOW_MIN_HEIGHT);
 
  window->main_box = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (window), window->main_box);
  gtk_widget_show (window->main_box);
  
  /* build menus */
  action_group = gtk_action_group_new ("MenuActions");
  window->action_group = action_group;
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group, entries,
  				G_N_ELEMENTS (entries),
  				window);
  
  window->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (window->ui_manager, action_group, 0);
  
  accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  
  error = NULL;
  if (!gtk_ui_manager_add_ui_from_file (window->ui_manager,
  					PKGDATADIR "/gnome-dictionary-ui.xml",
  					&error))
    {
      g_warning ("Building menus failed: %s", error->message);
      g_error_free (error);
    }
  else
    {
      window->menubar = gtk_ui_manager_get_widget (window->ui_manager, "/MainMenu");
      
      gtk_box_pack_start (GTK_BOX (window->main_box), window->menubar, FALSE, FALSE, 0);
      gtk_widget_show (window->menubar);
    }
  
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (window->main_box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  label = gtk_label_new_with_mnemonic (_("F_ind:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  window->entry = gtk_entry_new ();
  g_signal_connect (window->entry, "activate", G_CALLBACK (entry_activate_cb), window);
  gtk_box_pack_start (GTK_BOX (hbox), window->entry, TRUE, TRUE, 0);
  gtk_widget_show (window->entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), window->entry);

  window->context = get_context_from_loader (window);
  
  window->defbox = gdict_defbox_new ();
  if (window->context)
    gdict_defbox_set_context (GDICT_DEFBOX (window->defbox), window->context);

  gtk_drag_dest_set (window->defbox,
  		     GTK_DEST_DEFAULT_ALL,
  		     drop_types, n_drop_types,
  		     GDK_ACTION_COPY);
  g_signal_connect (window->defbox, "drag-data-received",
  		    G_CALLBACK (gdict_window_drag_data_received_cb),
  		    window);
  gtk_box_pack_start (GTK_BOX (window->main_box), window->defbox, TRUE, TRUE, 0);
  gtk_widget_show_all (window->defbox);

  window->status = gtk_statusbar_new ();
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (window->status), TRUE);
  gtk_box_pack_end (GTK_BOX (window->main_box), window->status, FALSE, FALSE, 0);
  gtk_widget_show (window->status);
  
  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_window_class_init (GdictWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gdict_window_finalize;
  gobject_class->set_property = gdict_window_set_property;
  gobject_class->get_property = gdict_window_get_property;
  gobject_class->constructor = gdict_window_constructor;
  
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  							_("Source Loader"),
  							_("The GdictSourceLoader to be used to load dictionary sources"),
  							GDICT_TYPE_SOURCE_LOADER,
  							(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
  g_object_class_install_property (gobject_class,
  				   PROP_WINDOW_ID,
  				   g_param_spec_uint ("window-id",
  				   		      _("Window ID"),
  				   		      _("The unique identificator for this window"),
  				   		      0,
  				   		      G_MAXUINT,
  				   		      0,
  				   		      G_PARAM_READABLE));

  gdict_window_signals[CREATED] =
    g_signal_new ("created",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictWindowClass, created),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GDICT_TYPE_WINDOW);
}

static void
gdict_window_init (GdictWindow *window)
{
  window->word = NULL;
  
  window->context = NULL;
  window->loader = NULL;
  
  window->client = gconf_client_get_default ();
  gconf_client_add_dir (window->client,
  			GDICT_GCONF_DIR,
  			GCONF_CLIENT_PRELOAD_NONE,
  			NULL);
  window->notify_id = gconf_client_notify_add (window->client,
  					       GDICT_GCONF_DIR,
  					       gdict_window_gconf_notify_cb,
  					       window,
  					       NULL,
  					       NULL);
  
  window->print_font = gconf_client_get_string (window->client,
		  				GDICT_GCONF_PRINT_FONT_KEY,
						NULL);
  window->source_name = gconf_client_get_string (window->client,
		  				 GDICT_GCONF_SOURCE_KEY,
						 NULL);
  
  window->window_id = (gulong) time (NULL);
}

GtkWidget *
gdict_window_new (GdictSourceLoader *loader)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  return g_object_new (GDICT_TYPE_WINDOW,
                       "source-loader", loader,
                       NULL);
}
