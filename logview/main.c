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

#include <config.h>
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

static gchar *config_prefix = NULL;
static gchar *sm_client_id = NULL;
static int screen = 0;

GOptionContext *context;
GOptionEntry options[] = {
    { "sm-config-prefix", 0, 0, G_OPTION_ARG_STRING, &config_prefix, "", NULL},
    { "sm-client-id", 0, 0, G_OPTION_ARG_STRING, &sm_client_id, "", NULL},
    { "screen", 0, 0, G_OPTION_ARG_INT, &screen, "", NULL},
	{ NULL }
};

GOptionContext *
logview_init_options ()
{
	GOptionContext *context;
	
	context = g_option_context_new (" - Browse and monitor logs");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_set_help_enabled (context, TRUE);
	g_option_context_set_ignore_unknown_options (context, TRUE);
	return context;
}
	
static gint
save_session (GnomeClient *gnome_client, gint phase,
              GnomeRestartStyle save_style, gint shutdown,
              GnomeInteractStyle interact_style, gint fast,
              LogviewWindow *logview)
{
   gchar **argv;
   gint numlogs;
   GList *logs;
   Log *log;
   int i;

   g_assert (LOGVIEW_IS_WINDOW (logview));
   numlogs = logview_count_logs (logview);

   argv = g_malloc0 (sizeof (gchar *) * numlogs+1);
   argv[0] = g_get_prgname();

   for (logs = logview->logs; logs != NULL; logs = g_list_next (logs)) {
       log = logs->data;
       argv[i++] = g_strdup_printf ("%s", log->name);
   }
   
   gnome_client_set_clone_command (gnome_client, numlogs+1, argv);
   gnome_client_set_restart_command (gnome_client, numlogs+1, argv);

   g_free (argv);
   return TRUE;

}

static gint
die (GnomeClient *gnome_client, gpointer client_data)
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
   LogviewWindow *logview;
   int i;
   GdkCursor *cursor;

   bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
   bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
   textdomain(GETTEXT_PACKAGE);

   error_dialog_queue (TRUE);

   program = gnome_program_init ("gnome-system-log",VERSION, LIBGNOMEUI_MODULE, argc, argv,
				 GNOME_PARAM_APP_DATADIR, DATADIR, 
				 NULL);

   g_set_application_name (_("Log Viewer"));
   gtk_window_set_default_icon_name ("logviewer");

   gnome_vfs_init ();
   prefs_init (argc, argv);
   
   context = logview_init_options ();
   g_option_context_parse (context, &argc, &argv, &error);

   /* Open regular logs and add each log passed as a parameter */

   logview = LOGVIEW_WINDOW(logview_window_new ());
   prefs_connect (logview);
   logview_menus_set_state (logview);
   gtk_widget_show (GTK_WIDGET (logview));

   cursor = gdk_cursor_new (GDK_WATCH);
   gdk_window_set_cursor (GTK_WIDGET (logview)->window, cursor);
   gdk_cursor_unref (cursor);
   gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));

   while (gtk_events_pending ())
       gtk_main_iteration ();
   if (argc == 1) {
       logview_add_logs_from_names (logview, prefs_get_logs (), prefs_get_active_log ());
   } else {
	   for (i=1; i<argc; i++)
		   logview_add_log_from_name (logview, argv[i]);
   }

   gdk_window_set_cursor (GTK_WIDGET (logview)->window, NULL);    
   gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));
   
   logview_show_main_content (logview);

   gnome_client = gnome_master_client ();

   error_dialog_queue (FALSE);
   error_dialog_show_queued ();
   
   g_signal_connect (gnome_client, "save_yourself",
		     G_CALLBACK (save_session), logview);
   g_signal_connect (gnome_client, "die", G_CALLBACK (die), NULL);

   gtk_main ();

   return 0;
}

