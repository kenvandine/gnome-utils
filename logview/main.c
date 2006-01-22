/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <libgnomeui/gnome-client.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomevfs/gnome-vfs.h>

#include <glib/gi18n.h>
#include <glib/goption.h>

#include <gtk/gtk.h>

#include "logview.h"
#include "misc.h"
#include "userprefs.h"

static gboolean show_version = FALSE;

static GOptionContext *
logview_init_options ()
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
	
	return context;
}

static void
logview_show_version_and_quit (void)
{
	g_print ("%s - Version %s\n"
		 "Copyright (C) 2004-2006 Vincent Noel and others\n",
		 g_get_application_name (),
		 VERSION);

	exit (0);
}

static gint
save_session_cb (GnomeClient        *gnome_client,
		 gint                phase,
		 GnomeRestartStyle   save_style,
		 gint                shutdown,
		 GnomeInteractStyle  interact_style,
		 gint                fast,
		 LogviewWindow      *logview)
{
	gchar **argv;
	gint numlogs;
	GSList *logs;
	Log *log;
	gint i;

	g_assert (LOGVIEW_IS_WINDOW (logview));
	numlogs = logview_count_logs (logview);

	argv = (gchar **) g_new0 (gchar **, numlogs + 2);
	argv[0] = g_get_prgname();

	for (logs = logview->logs; logs != NULL; logs = logs->next) {
		Log *log = (Log *) logs->data;

		g_assert (log != NULL);

		argv[i++] = g_strdup (log->name);
	}
	argv[i] = NULL;

	gnome_client_set_clone_command (gnome_client, numlogs + 1, argv);
	gnome_client_set_restart_command (gnome_client, numlogs + 1, argv);

	g_strfreev (argv);

	return TRUE;
}

static gint
die_cb (GnomeClient *gnome_client,
	gpointer     client_data)
{
    gtk_main_quit ();
    
    return 0;
}

int
main (int argc, char *argv[])
{
	GnomeClient *gnome_client;
	GError *error;
	GnomeProgram *program;
	GOptionContext *context;
	LogviewWindow *logview;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	error_dialog_queue (TRUE);
	
	gnome_vfs_init ();
	prefs_init ();
	context = logview_init_options ();
	
	program = gnome_program_init ("gnome-system-log", VERSION,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PARAM_APP_DATADIR, DATADIR,
				      GNOME_PARAM_GOPTION_CONTEXT, context,
				      NULL);
	
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

	gnome_client = gnome_master_client ();
	if (gnome_client) {
		g_signal_connect (gnome_client, "save_yourself",
				  G_CALLBACK (save_session_cb), logview);
		g_signal_connect (gnome_client, "die",
				  G_CALLBACK (die_cb), NULL);
	}

	gtk_main ();
   
	return EXIT_SUCCESS;
}
