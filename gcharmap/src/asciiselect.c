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
static gint prev_value;


static void
cb_ascii_select_clicked (GtkDialog *dialog, gint id, gpointer user_data)
{
    gchar *text;
    gint current_pos;
    
    if (id != 100) {
        gtk_widget_destroy (GTK_WIDGET (dialog));
    	return;
    }

    text = gtk_editable_get_chars (GTK_EDITABLE (user_data), 0, -1);
    gtk_editable_set_position (GTK_EDITABLE (mainapp->entry), -1);
    current_pos = gtk_editable_get_position (GTK_EDITABLE (mainapp->entry));
    gtk_editable_insert_text (GTK_EDITABLE (mainapp->entry), text, strlen(text), &current_pos);
       
}


static void
cb_ascii_select_entry_changed (GtkEditable *edit, gpointer user_data)
{
    gchar *s;
    gint i, f;

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
    gchar *text;
    gchar buf[7];
    gint n;
    gchar *err;
    gdouble res;

    if (updating == TRUE) return;
    updating = TRUE;
    text = gtk_editable_get_chars (edit, 0, -1);
    if (!text)
    	return;
    res = strtol(text, &err, 16);
    g_free (text);
    if (res > 0xffff)
    {	     
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (edit), 0xffff);
      res = prev_value;
    }	

    prev_value = res;
    n = g_unichar_to_utf8(res, buf);
    buf[n]=0;

    gtk_entry_set_text (GTK_ENTRY (user_data), buf);

    updating = FALSE;
}

static gint
spin_button_hex_input_func (GtkSpinButton *spin_button,
			    gdouble       *new_val)
{
  const gchar *buf;
  gchar *err;
  gdouble res;

  buf = gtk_entry_get_text (GTK_ENTRY (spin_button));
  res = strtol(buf, &err, 16);
  *new_val = res;
  if (*err)
    return GTK_INPUT_ERROR;
  else
    return TRUE;
}

static gint
spin_button_hex_output_func (GtkSpinButton *spin_button)
{
  static gchar buf[7];
  gint val;

  val = (gint) spin_button->adjustment->value;
  if (fabs (val) < 1e-5)
    sprintf (buf, "0x00");
  else
    sprintf (buf, "0x%.2X", val);
  if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
    gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
  return TRUE;
}

static void
ascii_select_init (AsciiSelect *obj)
{
    GtkWidget *spin;
    GtkWidget *entry;
    GtkAdjustment *adj;
    GtkWidget *label;


    obj->window = gtk_dialog_new_with_buttons (_("Select Character"), NULL,
    					       GTK_DIALOG_DESTROY_WITH_PARENT,
					       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					       _("_Insert"), 100,
					       NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (obj->window), GTK_RESPONSE_CLOSE);
    label = gtk_label_new_with_mnemonic (_("Character c_ode:"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox), 
			label, FALSE, FALSE, 0);

    adj = (GtkAdjustment *)gtk_adjustment_new (65, 0, 0xFFFF, 1, 10, 10);
    spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (spin));
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin),
      GTK_UPDATE_IF_VALID);
    gtk_signal_connect (GTK_OBJECT (spin),
			"input",
			GTK_SIGNAL_FUNC (spin_button_hex_input_func),
			NULL);
    gtk_signal_connect (GTK_OBJECT (spin),
			"output",
			GTK_SIGNAL_FUNC (spin_button_hex_output_func),
			NULL);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spin), TRUE);

    prev_value = 65;
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox),
      spin, FALSE, TRUE, 0);

    label = gtk_label_new_with_mnemonic (_("Ch_aracter:"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox), 
			label, FALSE, FALSE, 0);

    entry = gtk_entry_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
    gtk_entry_set_text (GTK_ENTRY (entry), "A");
    gtk_entry_set_max_length (GTK_ENTRY (entry), 1);

    if(check_gail(spin))
    {
       add_atk_namedesc(spin, _("Character Code"), _("Select character code"));
       add_atk_namedesc(entry, _("Character"), _("Character to insert"));
       add_atk_relation(spin, entry, ATK_RELATION_CONTROLLER_FOR);
       add_atk_relation(entry, spin, ATK_RELATION_CONTROLLED_BY);
    }

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj->window)->vbox),
      entry, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (spin), "changed",
       		      G_CALLBACK (cb_ascii_select_spin_changed), entry);
    g_signal_connect (G_OBJECT (entry), "changed",
       		      G_CALLBACK (cb_ascii_select_entry_changed), spin);
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
        GTypeInfo ga_info = {
          sizeof (AsciiSelectClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) NULL,
          (GClassFinalizeFunc) NULL,
          NULL,
          sizeof(AsciiSelect),
          0,
          (GInstanceInitFunc) ascii_select_init,
          NULL
        };
        ga_type = g_type_register_static (gtk_object_get_type (), "AsciiSelect",&ga_info, 0);
    }
    return ga_type;
}


AsciiSelect *
ascii_select_new (void)
{
    return ASCII_SELECT (g_object_new ((GType) ASCII_SELECT_TYPE, NULL));
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

