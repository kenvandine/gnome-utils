/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   grun: Popup a command dialog. Original version by Elliot Lee, 
 *    bloatware edition by Havoc Pennington. Both versions written in 10
 *    minutes or less. :-)
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <config.h>
#include <gnome.h>
#include <errno.h>

#define APPNAME "grun"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

static void string_callback(gchar * s, gpointer data)
{
  if (s) {
    if ( gnome_execute_shell(NULL, s) != 0 ) {
      gchar * t = g_copy_strings(_("Failed to execute command:\n"), 
                                 s, "\n", g_unix_error_string(errno),
                                 NULL);
      gnome_error_dialog(t);
      g_free(t); /* well, not really needed */
    }
  }
}

int main (int argc, char ** argv)
{
  GtkWidget * dialog;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, VERSION, argc, argv);

  dialog = gnome_request_string_dialog(_("Enter a command to execute:"),
                                       string_callback, NULL);

  gtk_signal_connect(GTK_OBJECT(dialog), "close", 
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  gtk_main();

  exit(EXIT_SUCCESS);
}


