/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   This is a test program for the MenuEntryWiz.
 *
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

#include "iconbrowse.h"
#include "menuentrywiz.h"

void menu_entry_callback(MenuEntryWiz * mew, gchar * iconname, 
			 gchar * menuname, gchar * command, 
			 gpointer data)
{
  /* note that any of these strings can be NULL, if the user
     didn't select anything. */
  g_print("Icon: %s\n", iconname);
  g_print("Name: %s\n", menuname);
  g_print("Command: %s\n", command);

}

void quit(GtkWidget * w)
{
  gtk_widget_destroy(w);

  while ( gtk_events_pending() )
    gtk_main_iteration();

  g_mem_profile();

  gtk_main_quit();
}

int main (int argc, char ** argv)
{
  GtkWidget * mew;

  gnome_init ("testing", 0, argc, argv, 0, 0);

  mew = menu_entry_wiz_new(TRUE, TRUE);

  gtk_signal_connect ( GTK_OBJECT(mew), "destroy",
		       GTK_SIGNAL_FUNC(quit),
		       0 );

  gtk_signal_connect ( GTK_OBJECT(mew), "menu_entry",
		       GTK_SIGNAL_FUNC(menu_entry_callback),
		       0 );

  gtk_widget_show(mew);

  gtk_main();

  return 0;
}


