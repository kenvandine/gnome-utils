/* logview-main.c
 *
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdlib.h>

#include <glib/gi18n.h>
#include <glib.h>

#include <gtk/gtk.h>

#include "logview-window.h"
#include "logview-prefs.h"
#include "logview-manager.h"
#include "logview-main.h"

static GtkWidget *main_window = NULL;

/* log files specified on the command line */
static char **log_files = NULL;

static gboolean
main_window_delete_cb (GtkWidget *widget,
                       GdkEvent *event,
                       gpointer user_data)
{
  gtk_main_quit ();

  return FALSE;
}

static void
logview_show_version_and_quit (void)
{
  g_print ("%s - Version %s\n"
           "Copyright (C) 2004-2008 Vincent Noel, Cosimo Cecchi and others.\n",
           g_get_application_name (),
           VERSION);

  exit (0);
}

static GOptionContext *
create_option_context (void)
{
  GOptionContext *context;
  GOptionGroup *group;

  const GOptionEntry entries[] = {
    { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
      logview_show_version_and_quit, N_("Show the application's version"), NULL },
    { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &log_files,
      NULL, N_("[LOGFILE...]") },
    { NULL },
  };

  context = g_option_context_new (_(" - Browse and monitor logs"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  return context;
}

void
logview_show_error (const char *primary,
                    const char *secondary)
{
  GtkWidget *message_dialog;

  message_dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           "%s", primary);
  if (secondary) {
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message_dialog),
                                              "%s", secondary);
  };

  gtk_dialog_run (GTK_DIALOG (message_dialog));
  gtk_widget_destroy (message_dialog);
}

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  LogviewPrefs *prefs;
  LogviewManager *manager;

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = create_option_context ();

  g_option_context_parse (context, &argc, &argv, &error);

  if (error) {
    g_critical ("Unable to parse arguments: %s", error->message);
    g_error_free (error);
    g_option_context_free (context);
    
    exit (1);
  }

  g_option_context_free (context);
  g_set_application_name (_("Log Viewer"));

  /* open regular logs and add each log passed as a parameter */
  main_window = logview_window_new ();
  if (!main_window) {
    g_critical ("Unable to create the user interface.");
  
    exit (1);
  }

  g_signal_connect (main_window, "delete-event",
                    G_CALLBACK (main_window_delete_cb), NULL);
  
  manager = logview_manager_get ();
  prefs = logview_prefs_get ();

  gtk_window_set_default_icon_name ("logviewer");

  if (log_files == NULL) {
    char *active_log;
    GSList *logs;

    active_log = logview_prefs_get_active_logfile (prefs);
    logs = logview_prefs_get_stored_logfiles (prefs);

    logview_manager_add_logs_from_names (manager,
                                         logs, active_log);

    g_free (active_log);
    g_slist_foreach (logs, (GFunc) g_free, NULL);
    g_slist_free (logs);
  } else {
    gint i;

    for (i = 0; log_files[i]; i++)
      logview_manager_add_log_from_name (manager, log_files[i], FALSE);
  }

  gtk_widget_show (main_window);

  gtk_main ();

  g_object_unref (prefs);
  g_object_unref (manager);

  return EXIT_SUCCESS;
}
