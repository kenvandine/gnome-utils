
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

#include <gnome.h>

#include "logview.h"

extern ConfigData *cfg;

/* ----------------------------------------------------------------------
   NAME:	ButtonWithPixmap
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

GtkWidget
*ButtonWithPixmap (char **xpmdata, int w, int h)
{
   GtkWidget *button, *pixmapwid, *alignment;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkStyle *style;

   button = gtk_button_new ();
   gtk_container_border_width (GTK_CONTAINER (button), 0);
   gtk_widget_set_style (button, cfg->main_style);
   gtk_widget_show (button);

   style = gtk_widget_get_style (button);

   pixmap = gdk_pixmap_create_from_xpm_d (button->window, &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  xpmdata);
   pixmapwid = gtk_pixmap_new (pixmap, mask);
   alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
   gtk_container_border_width (GTK_CONTAINER (alignment), 0);
   if (w > 0 && h > 0)
     gtk_widget_set_usize (alignment, w, h);
   gtk_container_add (GTK_CONTAINER (button), alignment);
   gtk_container_add (GTK_CONTAINER (alignment), pixmapwid);
   gtk_widget_show (pixmapwid);
   gtk_widget_show (alignment);

   return button;
}

/* ----------------------------------------------------------------------
   NAME:        ShowErrMessage
   DESCRIPTION: Print an error message. It will eventually open a 
   window and display the message.
   ---------------------------------------------------------------------- */

void
ShowErrMessage (char *msg)
{
  GtkWidget *msgbox;

  printf(_("Error: [%s]\n"), msg);
  msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_ERROR,
				  GNOME_STOCK_BUTTON_OK, NULL);
  gtk_window_set_modal (GTK_WINDOW(msgbox),TRUE); 
  gtk_widget_show (msgbox);
}

