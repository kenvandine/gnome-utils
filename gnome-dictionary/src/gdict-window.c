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
#include <libgnomeui/gnome-help.h>
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

#define GDICT_WINDOW_COLUMNS      56
#define GDICT_WINDOW_ROWS         33

#define GDICT_WINDOW_MIN_WIDTH	  400
#define GDICT_WINDOW_MIN_HEIGHT	  330

enum
{
  PROP_0,
  
  PROP_SOURCE_LOADER,
  PROP_SOURCE_NAME,
  PROP_PRINT_FONT,
  PROP_WORD,
  PROP_WINDOW_ID
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

  if (window->notify_id)
    gconf_client_notify_remove (window->gconf_client, window->notify_id);
  
  if (window->gconf_client)
    g_object_unref (window->gconf_client);
  
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

  if (window->icon)
    g_object_unref (window->icon);

  g_free (window->source_name);
  g_free (window->print_font);
  g_free (window->word);
  g_free (window->database);
  g_free (window->strategy);
  
  G_OBJECT_CLASS (gdict_window_parent_class)->finalize (object);
}

static void
gdict_window_lookup_start_cb (GdictContext *context,
			      GdictWindow  *window)
{
  static GdkCursor *busy_cursor = NULL;
  gchar *message;

  if (!window->word)
    return;

  if (busy_cursor == NULL)
    busy_cursor = gdk_cursor_new (GDK_WATCH);

  message = g_strdup_printf (_("Searching for '%s'..."), window->word);
  
  if (window->status)
    gtk_statusbar_push (GTK_STATUSBAR (window->status), 0, message);

  gdk_window_set_cursor (GTK_WIDGET (window)->window, busy_cursor);
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

  gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);
  g_free (message);
}

static void
gdict_window_error_cb (GdictContext *context,
		       const GError *error,
		       GdictWindow  *window)
{
  gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);

  gtk_statusbar_push (GTK_STATUSBAR (window->status), 0,
		      _("No definitions found"));
}

static void
gdict_window_set_database (GdictWindow *window,
			   const gchar *database)
{
  if (window->database)
    g_free (window->database);

  if (!database)
    database = gconf_client_get_string (window->gconf_client,
		    			GDICT_GCONF_DATABASE_KEY,
					NULL);
  
  if (!database)
    database = GDICT_DEFAULT_DATABASE;

  window->database = g_strdup (database);

  if (window->defbox)
    gdict_defbox_set_database (GDICT_DEFBOX (window->defbox), database);
}

static void
gdict_window_set_strategy (GdictWindow *window,
			   const gchar *strategy)
{
  if (window->strategy)
    g_free (window->strategy);

  window->strategy = g_strdup (strategy);
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
      gchar *detail;
      
      detail = g_strdup_printf (_("No dictionary source available with name '%s'"),
      				window->source_name);

      show_error_dialog (GTK_WINDOW (window),
                         _("Unable to find dictionary source"),
                         NULL);
      
      g_free (detail);

      return NULL;
    }
  
  gdict_window_set_database (window, gdict_source_get_database (source));
  gdict_window_set_strategy (window, gdict_source_get_strategy (source));
  
  retval = gdict_source_get_context (source);
  if (!retval)
    {
      gchar *detail;
      
      detail = g_strdup_printf (_("No context available for source '%s'"),
      				gdict_source_get_description (source));
      				
      show_error_dialog (GTK_WINDOW (window),
                         _("Unable to create a context"),
                         detail);
      
      g_free (detail);
      g_object_unref (source);
      
      return NULL;
    }
  
  g_object_unref (source);
  
  return retval;
}

static void
gdict_window_set_print_font (GdictWindow *window,
			     const gchar *print_font)
{
  if (!print_font)
    print_font = gconf_client_get_string (window->gconf_client,
    					  GDICT_GCONF_PRINT_FONT_KEY,
    					  NULL);
  
  if (!print_font)
    print_font = GDICT_DEFAULT_PRINT_FONT;
  
  if (window->print_font)
    g_free (window->print_font);
  
  window->print_font = g_strdup (print_font);
}

static void
gdict_window_set_word (GdictWindow *window,
		       const gchar *word)
{
  g_free (window->word);
  window->word = NULL;

  if (word && word[0] != '\0')
    window->word = g_strdup (word);
}

static void
gdict_window_set_context (GdictWindow  *window,
			  GdictContext *context)
{
  if (window->context)
    {
      g_signal_handler_disconnect (window->context, window->lookup_start_id);
      g_signal_handler_disconnect (window->context, window->lookup_end_id);
      g_signal_handler_disconnect (window->context, window->error_id);
      
      window->lookup_start_id = 0;
      window->lookup_end_id = 0;
      window->error_id = 0;
      
      g_object_unref (window->context);
      window->context = NULL;
    }

  if (window->defbox)
    gdict_defbox_set_context (GDICT_DEFBOX (window->defbox), context);

  if (!context)
    return;
  
  /* attach our callbacks */
  window->lookup_start_id = g_signal_connect (context, "lookup-start",
		  			      G_CALLBACK (gdict_window_lookup_start_cb),
					      window);
  window->lookup_end_id   = g_signal_connect (context, "lookup-end",
		  			      G_CALLBACK (gdict_window_lookup_end_cb),
					      window);
  window->error_id        = g_signal_connect (context, "error",
		  			      G_CALLBACK (gdict_window_error_cb),
					      window);
  
  window->context = context;
}

static void
gdict_window_set_source_name (GdictWindow *window,
			      const gchar *source_name)
{
  GdictContext *context;
  
  /* some sanity checks first */
  if (!source_name)
    source_name = gconf_client_get_string (window->gconf_client,
      					   GDICT_GCONF_SOURCE_KEY,
      					   NULL);
  
  if (!source_name)
    source_name = GDICT_DEFAULT_SOURCE_NAME;
  
  if (window->source_name)
    g_free (window->source_name);
  
  window->source_name = g_strdup (source_name);
  
  context = get_context_from_loader (window);
  
  gdict_window_set_context (window, context);
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
    case PROP_SOURCE_NAME:
      gdict_window_set_source_name (window, g_value_get_string (value));
      break;
    case PROP_WORD:
      gdict_window_set_word (window, g_value_get_string (value));
      break;
    case PROP_PRINT_FONT:
      gdict_window_set_print_font (window, g_value_get_string (value));
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
    case PROP_SOURCE_NAME:
      g_value_set_string (value, window->source_name);
      break;
    case PROP_WORD:
      g_value_set_string (value, window->word);
      break;
    case PROP_PRINT_FONT:
      g_value_set_string (value, window->print_font);
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
  
  /* store the default size of the window and its state, so that
   * it's picked up by the newly created window
   */
  gconf_client_set_int (window->gconf_client,
		        GDICT_GCONF_WINDOW_WIDTH_KEY,
		  	window->default_width,
			NULL);
  gconf_client_set_int (window->gconf_client,
		  	GDICT_GCONF_WINDOW_HEIGHT_KEY,
			window->default_height,
			NULL);
  gconf_client_set_bool (window->gconf_client,
		  	 GDICT_GCONF_WINDOW_IS_MAXIMIZED_KEY,
			 window->is_maximized,
			 NULL);
 
  new_window = gdict_window_new (window->loader, NULL, NULL);
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

  /* store the default size of the window and its state */
  gconf_client_set_int (window->gconf_client,
		        GDICT_GCONF_WINDOW_WIDTH_KEY,
		  	window->default_width,
			NULL);
  gconf_client_set_int (window->gconf_client,
		  	GDICT_GCONF_WINDOW_HEIGHT_KEY,
			window->default_height,
			NULL);
  gconf_client_set_bool (window->gconf_client,
		  	 GDICT_GCONF_WINDOW_IS_MAXIMIZED_KEY,
			 window->is_maximized,
			 NULL);
  
  /* if this was called from the uimanager, destroy the widget;
   * otherwise, if it was called from the delete_event, it will
   * destroy the widget itself.
   */
  if (action)
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
  				       _("Dictionary Preferences"),
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
					NULL,
  					gtk_widget_get_screen (GTK_WIDGET (window)),
  					&err);
  if (err)
    {
      show_error_dialog (GTK_WINDOW (window),
      			 _("There was an error while displaying help"),
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
  { "EditPreferences", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL, NULL,
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

static gboolean
gdict_window_delete_event_cb (GtkWidget *widget,
			      GdkEvent  *event,
			      gpointer   user_data)
{
  gdict_window_cmd_file_close_window (NULL, GDICT_WINDOW (widget));

  return FALSE;
}

static gboolean
gdict_window_state_event_cb (GtkWidget           *widget,
			     GdkEventWindowState *event,
			     gpointer             user_data)
{
  GdictWindow *window = GDICT_WINDOW (widget);
  
  if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
    window->is_maximized = TRUE;
  else
    window->is_maximized = FALSE;
  
  return FALSE;
}

static void
gdict_window_gconf_notify_cb (GConfClient *client,
			      guint        cnxn_id,
			      GConfEntry  *entry,
			      gpointer     user_data)
{
  GdictWindow *window;

  window = GDICT_WINDOW (user_data);

  if (strcmp (entry->key, GDICT_GCONF_PRINT_FONT_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_window_set_print_font (window, gconf_value_get_string (entry->value));
      else
        gdict_window_set_print_font (window, GDICT_DEFAULT_PRINT_FONT);
    }
  else if (strcmp (entry->key, GDICT_GCONF_SOURCE_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_window_set_source_name (window, gconf_value_get_string (entry->value));
      else
        gdict_window_set_source_name (window, GDICT_DEFAULT_SOURCE_NAME);
    }
  else if (strcmp (entry->key, GDICT_GCONF_DATABASE_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_window_set_database (window, gconf_value_get_string (entry->value));
      else
        gdict_window_set_database (window, GDICT_DEFAULT_DATABASE);
    }
}

static void
entry_activate_cb (GtkWidget   *widget,
		   GdictWindow *window)
{
  const gchar *word;
  gchar *title;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  if (!window->context)
    return;
  
  word = gtk_entry_get_text (GTK_ENTRY (widget));
  if (!word || *word == '\0')
    return;

  gdict_window_set_word (window, word);
  
  title = g_strdup_printf (_("%s - Dictionary"), window->word);
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
      
      gtk_entry_set_text (GTK_ENTRY (window->entry), text);

      gdict_window_set_word (window, text);
      
      title = g_strdup_printf (_("%s - Dictionary"), window->word);
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

static void
gdict_window_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GdictWindow *window = GDICT_WINDOW (widget);

  if (!window->is_maximized)
    {
      window->default_width = allocation->width;
      window->default_height = allocation->height;
    }

  if (GTK_WIDGET_CLASS (gdict_window_parent_class)->size_allocate)
    GTK_WIDGET_CLASS (gdict_window_parent_class)->size_allocate (widget,
		    						 allocation);
}

static void
set_window_default_size (GdictWindow *window)
{
  GtkWidget *widget;
  gboolean is_maximized;
  gint width, height;
  gint font_size;
  GdkScreen *screen;
  gint monitor_num;
  GtkRequisition req;
  GdkRectangle monitor;

  g_assert (GDICT_IS_WINDOW (window));

  widget = GTK_WIDGET (window);
  
  /* recover the state from GConf */
  width = gconf_client_get_int (window->gconf_client,
		  		GDICT_GCONF_WINDOW_WIDTH_KEY,
				NULL);
  height = gconf_client_get_int (window->gconf_client,
		  		 GDICT_GCONF_WINDOW_HEIGHT_KEY,
				 NULL);
  is_maximized = gconf_client_get_bool (window->gconf_client,
		  			GDICT_GCONF_WINDOW_IS_MAXIMIZED_KEY,
					NULL);
  
  /* XXX - the user wants gnome-dictionary to resize itself, so
   * we compute the minimum safe geometry needed for displaying
   * the text returned by a dictionary server, which is based
   * on the font size and the ANSI terminal.  this is dumb,
   * I know, but dictionary servers return pre-formatted text
   * and we can't reformat it ourselves.
   */
  if (width == -1 || height == -1)
    {
      /* Size based on the font size */
      font_size = pango_font_description_get_size (widget->style->font_desc);
      font_size = PANGO_PIXELS (font_size);

      width = font_size * GDICT_WINDOW_COLUMNS;
      height = font_size * GDICT_WINDOW_ROWS;

      /* Use at least the requisition size of the window... */
      gtk_widget_size_request (widget, &req);
      width = MAX (width, req.width);
      height = MAX (height, req.height);

      /* ... but make it no larger than the monitor */
      screen = gtk_widget_get_screen (widget);
      monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);

      gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
      width = MIN (width, monitor.width * 3 / 4);
      height = MIN (height, monitor.height * 3 / 4);
    }

  g_print ("(in %s) window size: <%d, %d>\n", G_STRFUNC, width, height);
  
  /* Set default size */
  gtk_window_set_default_size (GTK_WINDOW (widget),
  			       width,
  			       height);

  if (is_maximized)
    gtk_window_maximize (GTK_WINDOW (widget));
}

static void
gdict_window_style_set (GtkWidget *widget,
			GtkStyle  *old_style)
{
  GdictWindow *window;

  if (GTK_WIDGET_CLASS (gdict_window_parent_class)->style_set)
    GTK_WIDGET_CLASS (gdict_window_parent_class)->style_set (widget, old_style);

  set_window_default_size (GDICT_WINDOW (widget));
}

static GObject *
gdict_window_constructor (GType                  type,
			  guint                  n_construct_properties,
			  GObjectConstructParam *construct_params)
{
  GObject *object;
  GdictWindow *window;
  gint width, height;
  gboolean is_maximized;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GError *error;
  
  object = G_OBJECT_CLASS (gdict_window_parent_class)->constructor (type,
  						   n_construct_properties,
  						   construct_params);
  window = GDICT_WINDOW (object);
  
  gtk_widget_push_composite_child ();

  /* retrieve the window state from gconf */
  is_maximized = gconf_client_get_bool (window->gconf_client,
		  			GDICT_GCONF_WINDOW_IS_MAXIMIZED_KEY,
					NULL);

  width = gconf_client_get_int (window->gconf_client,
		  		GDICT_GCONF_WINDOW_WIDTH_KEY,
				NULL);
  height = gconf_client_get_int (window->gconf_client,
		  		 GDICT_GCONF_WINDOW_HEIGHT_KEY,
				 NULL);
  
  if (width == -1 || height == -1)
    {
      PangoContext *pango_context;
      PangoFontDescription *font_desc;
      gint font_size;
  
      pango_context = gtk_widget_get_pango_context (GTK_WIDGET (window));
      font_desc = pango_context_get_font_description (pango_context);

      font_size = pango_font_description_get_size (font_desc) / PANGO_SCALE;

      width = MAX (GDICT_WINDOW_COLUMNS * font_size, GDICT_WINDOW_MIN_WIDTH);
      height = MAX (GDICT_WINDOW_ROWS * font_size, GDICT_WINDOW_MIN_HEIGHT);
    }
  else
    {
      window->default_width = width;
      window->default_height = height;
    }

  window->is_maximized = is_maximized;
  
  gtk_window_set_title (GTK_WINDOW (window), _("Dictionary"));
  gtk_window_set_default_size (GTK_WINDOW (window),
  			       width,
  			       height);
  if (is_maximized)
    gtk_window_maximize (GTK_WINDOW (window));
 
  window->main_box = gtk_vbox_new (FALSE, 0);
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
  
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (window->main_box), vbox);
  gtk_widget_show (vbox);
  
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  label = gtk_label_new_with_mnemonic (_("Look _up:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  window->entry = gtk_entry_new ();
  if (window->word)
    gtk_entry_set_text (GTK_ENTRY (window->entry), window->word);
  
  g_signal_connect (window->entry, "activate", G_CALLBACK (entry_activate_cb), window);
  gtk_box_pack_start (GTK_BOX (hbox), window->entry, TRUE, TRUE, 0);
  gtk_widget_show (window->entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), window->entry);

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
  gtk_box_pack_start (GTK_BOX (vbox), window->defbox, TRUE, TRUE, 0);
  gtk_widget_show_all (window->defbox);

  window->status = gtk_statusbar_new ();
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (window->status), TRUE);
  gtk_box_pack_end (GTK_BOX (window->main_box), window->status, FALSE, FALSE, 0);
  gtk_widget_show (window->status);

  g_signal_connect (window, "delete-event",
		    G_CALLBACK (gdict_window_delete_event_cb),
		    NULL);
  g_signal_connect (window, "window-state-event",
		    G_CALLBACK (gdict_window_state_event_cb),
		    NULL);

  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_window_class_init (GdictWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = gdict_window_finalize;
  gobject_class->set_property = gdict_window_set_property;
  gobject_class->get_property = gdict_window_get_property;
  gobject_class->constructor = gdict_window_constructor;

  widget_class->style_set = gdict_window_style_set;
  widget_class->size_allocate = gdict_window_size_allocate;
  
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  							"Source Loader",
  							"The GdictSourceLoader to be used to load dictionary sources",
  							GDICT_TYPE_SOURCE_LOADER,
  							(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
  g_object_class_install_property (gobject_class,
		  		   PROP_SOURCE_NAME,
				   g_param_spec_string ("source-name",
					   		"Source Name",
							"The name of the GdictSource to be used",
							GDICT_DEFAULT_SOURCE_NAME,
							(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
  g_object_class_install_property (gobject_class,
  				   PROP_PRINT_FONT,
  				   g_param_spec_string ("print-font",
  				   			"Print Font",
  				   			"The font name to be used when printing",
  				   			GDICT_DEFAULT_PRINT_FONT,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
  g_object_class_install_property (gobject_class,
		  		   PROP_WORD,
				   g_param_spec_string ("word",
					   		"Word",
							"The word to search",
							NULL,
							(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
  g_object_class_install_property (gobject_class,
  				   PROP_WINDOW_ID,
  				   g_param_spec_uint ("window-id",
  				   		      "Window ID",
  				   		      "The unique identifier for this window",
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
  gchar *icon_file;
  GdkPixbuf *icon;
  GError *icon_error;
  
  window->loader = NULL;
  window->context = NULL;
  
  window->gconf_client = gconf_client_get_default ();
  gconf_client_add_dir (window->gconf_client,
  			GDICT_GCONF_DIR,
  			GCONF_CLIENT_PRELOAD_NONE,
  			NULL);
  window->notify_id = gconf_client_notify_add (window->gconf_client,
  					       GDICT_GCONF_DIR,
  					       gdict_window_gconf_notify_cb,
  					       window,
  					       NULL,
  					       NULL);
  
  window->word = NULL;
  window->source_name = NULL;
  window->print_font = NULL;

  window->database = NULL;
  window->strategy = NULL;

  icon_file = g_build_filename (DATADIR,
		  		"pixmaps",
				"gnome-dictionary.png",
				NULL);
  icon_error = NULL;
  icon = gdk_pixbuf_new_from_file (icon_file, &icon_error);
  if (icon_error)
    {
      show_error_dialog (GTK_WINDOW (window),
		         _("Unable to load the application icon"),
		         icon_error->message);

      g_error_free (icon_error);
    }
  else
    {
      gtk_window_set_default_icon (icon);
      window->icon = icon;
    }

  g_free (icon_file);

  window->default_width = -1;
  window->default_height = -1;
  window->is_maximized = FALSE;
  
  window->window_id = (gulong) time (NULL);
}

GtkWidget *
gdict_window_new (GdictSourceLoader *loader,
		  const gchar       *source_name,
		  const gchar       *word)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  return g_object_new (GDICT_TYPE_WINDOW,
                       "source-loader", loader,
		       "source-name", source_name,
		       "word", word,
                       NULL);
}
