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
#include <stdio.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include "logview.h"

extern ConfigData *cfg;
extern GtkWidget *app;

static GtkWidget *about_window = NULL;

/* Prototypes */

void AboutShowWindow (GtkWidget *widget, gpointer user_data);


void
AboutShowWindow (GtkWidget *widget, gpointer user_data)
{
  GdkPixbuf *logo = NULL;
  /* Author needs some sort of dash over the 'e' in Cesar  - U-00E9 */
  const char *author[] = { N_("Cesar Miquel (miquel@df.uba.ar)"), NULL};
  char *comments = N_("A system log viewer for GNOME.");
  gchar *documenters[] = {
	  NULL
  };
  /* Translator credits */
  gchar *translator_credits = _("translator_credits");

  if (about_window != NULL) {
	  gtk_widget_show_now (about_window);
	  gdk_window_raise (about_window->window);
	  return;
  }

  logo = gdk_pixbuf_new_from_file (DATADIR G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S "gnome-log.png", NULL);
  about_window = gnome_about_new (_("GNOME System Log Viewer"), VERSION,
           			  "Copyright \xc2\xa9 1998-2003 Free Software Foundation, Inc.",
				  _(comments),
				  author,
				  (const char **)documenters,
				  strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				  logo);
  if (app != NULL)
	  gtk_window_set_transient_for (GTK_WINDOW (about_window),
				   GTK_WINDOW (app));
  if (logo != NULL)
    gdk_pixbuf_unref (logo);

  gtk_signal_connect (GTK_OBJECT (about_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      &about_window);
  gtk_widget_show (about_window);
  return;

}                           

