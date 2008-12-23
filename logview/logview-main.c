/*
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

#include "logview.h"
#include "logview-prefs.h"
#include "logview-manager.h"
#include "misc.h"

static gboolean show_version = FALSE;

static GOptionContext *
create_option_context ()
{
  GOptionContext *context;
  GOptionGroup *group;

  const GOptionEntry entries[] = {
      { "version", 'V', 0, G_OPTION_ARG_NONE, &show_version, N_("Show the application's version"), NULL },
      { NULL },
  };

  group = g_option_group_new ("gnome-system-log",
                              _("System Log Viewer"),
                              _("Show System Log Viewer options"),
                              NULL, NULL);
  g_option_group_add_entries (group, entries);

  context = g_option_context_new (_(" - Browse and monitor logs"));
  g_option_context_add_group (context, group);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  return context;
}

static void
logview_show_version_and_quit (void)
{
  g_print ("%s - Version %s\n"
           "Copyright (C) 2004-2008 Vincent Noel, Cosimo Cecchi and others\n",
           g_get_application_name (),
           VERSION);

  exit (0);
}

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GtkWidget *main_window;
  LogviewPrefs *prefs;
  LogviewManager *manager;

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  error_dialog_queue (TRUE);

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

  if (show_version) {
    logview_show_version_and_quit ();
  }

  /* open regular logs and add each log passed as a parameter */
  main_window = logview_window_new ();
  if (!main_window) {
    error_dialog_show (NULL,
                       _("Unable to create user interface."),
                       NULL);
  
    exit (1);
  }
  
  manager = logview_manager_get ();
  prefs = logview_prefs_get ();

  gtk_window_set_default_icon_name ("logviewer");

  /* show the eventual error dialogs */
  error_dialog_queue (FALSE);
  error_dialog_show_queued ();

  if (argc == 1) {
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

    for (i = 1; i < argc; i++)
      logview_manager_add_log_from_name (manager, argv[i], FALSE);
  }

  gtk_widget_show (main_window);

  gtk_main ();

  g_object_unref (prefs);
  g_object_unref (manager);

  return EXIT_SUCCESS;
}
