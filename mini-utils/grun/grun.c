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

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#define APPNAME "grun"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

static void run_command(gchar * command);

static void string_callback(gchar * s, gpointer data)
{
  if (s) {
    run_command(s);
  }
  gtk_main_quit(); /* When the close signal works this line comes out */
}

int main (int argc, char ** argv)
{
  GtkWidget * dialog;
  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, 0, argc, argv, 0, 0);

  dialog = gnome_request_string_dialog(_("Enter a command to execute:"),
				       string_callback, NULL);

#if 0 /* Damn gnome-dialog is still broken */
  gtk_signal_connect(GTK_OBJECT(dialog), "close", 
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
#else /* Temporary hack */
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 1,
			      GTK_SIGNAL_FUNC(gtk_main_quit),
			      NULL); 
  /* Still doesn't work if you press Escape */
#endif

  gtk_main();

  exit(EXIT_SUCCESS);
}


/********************************
  Stuff that should be in gnome-util and be an exec and be written better
  and is in about 4 different apps now. :)
  *******************************/

static void run_command(gchar * command)
{
  pid_t new_pid;
  
  new_pid = fork();

  switch (new_pid) {
  case -1 :
    g_warning("Command execution failed: fork failed\n");
    break;
  case 0 : 
    _exit(system(command));
    break;
  default:
    break;
  }
}


