/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "logview.h"

void
AboutShowWindow (GtkWidget *widget, gpointer user_data)
{
  /* Author needs some sort of dash over the 'e' in Cesar  - U-00E9 */
  static const gchar *author[] = { "Cesar Miquel <miquel@df.uba.ar>",
				   "Glynn Foster <fherrera@onirica.com>)",
				   "Fernando Herrera  <fherrera@onirica.com>",
				   "Shakti Sen  <shprasad@novell.com>",
				   "Julio M Merino Vidal <jmmv@menta.net>",
				   "Jason Leach  <leach@wam.umd.edu>",
				   "Christian Neumair  <chris@gnome-de.org>",
				   "Jan Arne Petersen  <jpetersen@uni-bonn.de>",
				   "Jason Long  <jlong@messiah.edu>",
				   "Kjartan Maraas  <kmaraas@gnome.org>",
				   "Vincent Noel  <vnoel@cox.net>",
				   NULL};
  gchar *documenters[] = {NULL};
  /* Translator credits */
  const gchar *translator_credits = _("translator-credits");
  LogviewWindow *window = user_data;

  gtk_show_about_dialog (GTK_WINDOW (window),
		"name",  _("System Log Viewer"),
		"version", VERSION,
		"copyright", "Copyright \xc2\xa9 1998-2004 Free Software Foundation, Inc.",
		"comments", _("A system log viewer for GNOME."),
		"authors", author,
		"documenters", documenters,
		"translator_credits", strcmp (translator_credits, "translator-credits") != 0 ? translator_credits : NULL,
		"logo_icon_name", "logviewer",
		NULL);
  return;
}
