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

#define APPNAME "gnome-run"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

GnomeFileEntry *fentry;

static void string_callback(GnomeDialog *dlg, gint button_num, gpointer data)
{
  if (button_num == 0) {
    char *s;

    s = gtk_entry_get_text(GTK_ENTRY(gnome_file_entry_gtk_entry(fentry)));

#if 0
    gnome_entry_prepend_history(GNOME_ENTRY(gnome_file_entry_gnome_entry(fentry)),
			        1, s);
#endif

    if ( gnome_execute_shell(NULL, s) < 0 ) {
      gchar * t = g_strconcat(_("Failed to execute command:\n"), 
                                 s, "\n", g_unix_error_string(errno),
                                 NULL);
      gnome_error_dialog(t);
      g_free(t); /* well, not really needed */
    }
  }
  gtk_main_quit();
}

int main (int argc, char ** argv)
{
  GtkWidget * dialog;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, VERSION, argc, argv);

  dialog = gnome_dialog_new(_("Run Program"),
                            _("Run"), _("Cancel"), NULL);
  gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
  gnome_dialog_close_hides(GNOME_DIALOG(dialog), TRUE);
  gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);

  gtk_signal_connect(GTK_OBJECT(dialog), "clicked", 
		     GTK_SIGNAL_FUNC(string_callback), NULL);

  gtk_signal_connect(GTK_OBJECT(dialog), "close", 
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  fentry = GNOME_FILE_ENTRY(gnome_file_entry_new("gnome-run", _("Select a program to run")));

#if 0
  gnome_entry_load_history(GNOME_ENTRY(gnome_file_entry_gnome_entry(fentry)));
#endif

  gtk_window_set_focus(GTK_WINDOW(dialog), gnome_file_entry_gtk_entry(fentry));

  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fentry->gentry)->entry), "activate", 
		     GTK_SIGNAL_FUNC(string_callback), NULL);

  gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(dialog)->vbox),
                    GTK_WIDGET(fentry));
  gtk_widget_show_all(dialog);

  gtk_main();

#if 0
  gnome_entry_save_history(GNOME_ENTRY(gnome_file_entry_gnome_entry(fentry)));
#endif

  gtk_widget_destroy(dialog);

  exit(EXIT_SUCCESS);
}
