/*
 *  Gnome Character Map
 *  asciiselect.h - The ASCII character selector dialog
 *
 *  Copyright (c) 2000 Hongli Lai
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _ASCII_SELECT_C_
#define _ASCII_SELECT_C_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "asciiselect.h"

#include <gnome.h>
#include "interface.h"


static gboolean updating = FALSE;


static void
cb_ascii_select_clicked (GtkDialog *dialog, gint id, gpointer user_data)
{
    gchar *text;
    
    if (id != 100) {
        gtk_widget_destroy (GTK_WIDGET (dialog));
    	return;
    }

    text = gtk_editable_get_chars (GTK_EDITABLE (user_data), 0, -1);
    if (mainapp->insert_at_end == FALSE)
    {
	int current_pos;
	
	current_pos = gtk_editable_get_position
		    (GTK_EDITABLE(mainapp->entry));
	gtk_editable_insert_text (GTK_EDITABLE (mainapp->entry), text,
              strlen (text), &current_pos);
    } else {
        gtk_entry_append_text (GTK_ENTRY (mainapp->entry), text);
    }
       
}


static void
cb_ascii_select_entry_changed (GtkEditable *edit, gpointer user_data)
{
    gchar *s;
    gint i, f;
    gunichar c;

    if (updating == TRUE) return;
    updating = TRUE;
    s = (gchar *)gtk_editable_get_chars (edit, 0, -1);
    if (!s)
    	return;
    i = g_utf8_get_char (s);
    g_free (s);
    f = (gfloat) i;
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (user_data), f);
    updating = FALSE;
}

static void
cb_ascii_select_spin_changed (GtkEditable *edit, gpointer user_data)
{
    gint i;
    gchar *text;
    gchar buf[7];
    gint n;

    if (updating == TRUE) return;
    updating = TRUE;
    text = gtk_editable_get_chars (edit, 0, -1);
    if (!text)
    	return;
    i = atoi (text);
    g_free (text);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (edit), i);

    n=g_unichar_to_utf8(i, buf);
    buf[n]=0;

    gtk_entry_set_text (GTK_ENTRY (user_data), buf);

    updating = FALSE;
}

static void
ascii_select_init (AsciiSelect *obj)
{
    GtkWidget *spin;
    GtkWidget *entry;
    GtkStyle *style;
    GtkObject *adj;
    GdkFont *font;

    obj->window = gtk_dialog_new_with_buttons (_("Select Character"), NULL,
    					       GTK_DIALOG_DESTROY_WITH_PARENT,
					       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					       _("_Insert"), 100,
					       NULL);
    
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox),
      gtk_label_new (_("Character code:")), FALSE, FALSE, 0);

    adj = gtk_adjustment_new (65, 0, 255, 1, 10, 10);
    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
      GTK_UPDATE_IF_VALID);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox),
      spin, FALSE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox),
      gtk_label_new (_("Character:")), FALSE, FALSE, 0);

    entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry), "A");
    gtk_entry_set_max_length (GTK_ENTRY (entry), 1);
    style = gtk_style_copy (gtk_widget_get_style (entry));

    font = gdk_fontset_load (
      _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*,*-r-*")
    );
    if (font != NULL)
	    gtk_style_set_font(style, font);
    gtk_widget_set_style (entry, style);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox),
      entry, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (spin), "changed",
      GTK_SIGNAL_FUNC (cb_ascii_select_spin_changed), entry);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
      GTK_SIGNAL_FUNC (cb_ascii_select_entry_changed), spin);
    g_signal_connect (G_OBJECT (obj->window), "response",
    		      G_CALLBACK (cb_ascii_select_clicked), entry);

    gtk_widget_show_all (GTK_DIALOG (obj->window)->vbox);
    gtk_widget_grab_focus (spin);
}


guint
ascii_select_get_type (void)
{
    static guint ga_type = 0;

    if (!ga_type) {
        GtkTypeInfo ga_info = {
          "AsciiSelect",
          sizeof (AsciiSelect),
          sizeof (AsciiSelectClass),
          (GtkClassInitFunc) NULL,
          (GtkObjectInitFunc) ascii_select_init,
          NULL,
          NULL,
          (GtkClassInitFunc) NULL
        };
        ga_type = gtk_type_unique (gtk_object_get_type (), &ga_info);
    }
    return ga_type;
}


AsciiSelect *
ascii_select_new (void)
{
    return ASCII_SELECT (gtk_type_new ((GtkType) ASCII_SELECT_TYPE));
}


void
ascii_select_destroy (AsciiSelect *obj)
{
    g_return_if_fail (obj != NULL);
    g_return_if_fail (ASCII_IS_SELECT (obj) == TRUE);

    if (obj->window != NULL) gtk_widget_destroy (obj->window);
    gtk_object_destroy (GTK_OBJECT (obj));
}


#endif /* _ASCII_SELECT_C_ */

