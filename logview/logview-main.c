/*
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
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

#include <libgnomevfs/gnome-vfs.h>

#include <glib/gi18n.h>
#include <glib.h>

#include <gtk/gtk.h>

#include "logview.h"
#include "misc.h"
#include "userprefs.h"

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
  LogviewWindow *logview;

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  error_dialog_queue (TRUE);

  gnome_vfs_init ();
  prefs_init ();
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

  if (show_version)
    logview_show_version_and_quit ();

  /* Open regular logs and add each log passed as a parameter */
  logview = LOGVIEW_WINDOW (logview_window_new ());
  if (!logview) {
    error_dialog_show (NULL,
                       _("Unable to create user interface."),
                       NULL);
  
    exit (0);
  }

  gtk_window_set_default_icon_name ("logviewer");

  prefs_connect (logview);
  logview_menus_set_state (logview);

  gtk_widget_show (GTK_WIDGET (logview));

  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (argc == 1) {
    logview_add_logs_from_names (logview,
                                 prefs_get_logs (),
                                 prefs_get_active_log ());
  } else {
    gint i;

    for (i = 1; i < argc; i++)
      logview_add_log_from_name (logview, argv[i]);
  }

  logview_show_main_content (logview);

  /* show the eventual error dialogs */
  error_dialog_queue (FALSE);
  error_dialog_show_queued ();

  gtk_main ();

  prefs_shutdown ();

  return EXIT_SUCCESS;
}
