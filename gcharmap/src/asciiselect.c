/***************************************************************************
                          asciiselect.c  -  description
                             -------------------
    begin                : Mon Apr 24 2000
    copyright            : (C) 2000 by Hongli Lai
    email                : hongli@telekabel.nl
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <config.h>
#include "interface.h"
#include "asciiselect.h"

gboolean EntryDialog_OkClick (GtkWidget *widget, gpointer gdata);
gboolean EntryDialog_ApplyClick (GtkWidget *widget, gpointer gdata);
gboolean EntryDialog_CancelClick (GtkWidget *widget, gpointer gdata);
void SpinEditChanged (GtkEditable *editable, gpointer user_data);

GtkWidget *EntryDialog;
GtkWidget *SpinEdit;
GtkWidget *DLabel;

gboolean
InsertAsciiCharacterClick (GtkWidget *widget, gpointer gdata)
{
  GtkObject *Adj;
  GtkWidget *DViewport;
  GtkStyle *DStyle;
  guint8 i;
  GdkColor white = {0, 0xFFFF, 0xFFFF, 0xFFFF};
  GdkColor black = {0, 0x0000, 0x0000, 0x0000};
  gdk_color_alloc (gdk_colormap_get_system (), &white);
  gdk_color_alloc (gdk_colormap_get_system (), &black);

  EntryDialog = gnome_dialog_new (_("Insert ASCII character"),
    GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_APPLY,
    GNOME_STOCK_BUTTON_CANCEL, NULL);
  gnome_dialog_button_connect (GNOME_DIALOG (EntryDialog), 0,
    GTK_SIGNAL_FUNC (EntryDialog_OkClick), EntryDialog);
  gnome_dialog_button_connect (GNOME_DIALOG (EntryDialog), 1,
    GTK_SIGNAL_FUNC (EntryDialog_ApplyClick), NULL);
  gnome_dialog_button_connect_object (GNOME_DIALOG (EntryDialog), 2,
    (GtkSignalFunc) EntryDialog_CancelClick, GTK_OBJECT (EntryDialog));
  gnome_dialog_set_default (GNOME_DIALOG (EntryDialog), 0);

  Adj = gtk_adjustment_new (65, 0, 255, 1, 10, 10);
  SpinEdit = gtk_spin_button_new (GTK_ADJUSTMENT (Adj), 1, 0);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (EntryDialog)->vbox), SpinEdit,
    FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (SpinEdit), "changed",
    GTK_SIGNAL_FUNC (SpinEditChanged), NULL);

  DViewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (DViewport),
    GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (EntryDialog)->vbox),
    DViewport, FALSE, TRUE, 0);

  DStyle = gtk_style_copy (gtk_widget_get_style (DViewport));
  for (i = 0; i <= 5; i++) DStyle->bg[i] = black;
  for (i = 0; i <= 5; i++) DStyle->fg[i] = black;
  for (i = 0; i <= 5; i++) DStyle->text[i] = white;
  gtk_widget_set_style (DViewport, DStyle);
  gtk_widget_push_style (DStyle);
  gtk_widget_pop_style ();

  DLabel = gtk_label_new ("A");
  gtk_container_add (GTK_CONTAINER (DViewport), DLabel);
  DStyle = gtk_style_copy (gtk_widget_get_style (DLabel));
  for (i = 0; i <= 5; i++) DStyle->bg[i] = black;
  for (i = 0; i <= 5; i++) DStyle->fg[i] = white;
  DStyle->font = gdk_font_load ("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-iso8859-1");
  gtk_widget_set_style (DLabel, DStyle);
  gtk_widget_set_style (DLabel, DStyle);
  gtk_widget_push_style (DStyle);
  gtk_widget_pop_style ();

  gnome_dialog_set_parent (GNOME_DIALOG (EntryDialog), GTK_WINDOW (mainf));
  gtk_widget_show_all (EntryDialog);
  return FALSE;
}

gboolean
EntryDialog_OkClick (GtkWidget *widget, gpointer gdata)
{
  EntryDialog_ApplyClick (widget, gdata);
  EntryDialog_CancelClick (widget, gdata);
  return FALSE;
}

gboolean
EntryDialog_ApplyClick (GtkWidget *widget, gpointer gdata)
{
  gint8 i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (SpinEdit));
  gchar *text = g_strdup_printf ("%c", i);
  gtk_entry_append_text (GTK_ENTRY (Edit), text);
  return FALSE;
}

gboolean
EntryDialog_CancelClick (GtkWidget *widget, gpointer gdata)
{
  gnome_dialog_close (GNOME_DIALOG (EntryDialog));
  return FALSE;
}

void
SpinEditChanged (GtkEditable *editable, gpointer user_data)
{
  gint8 i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (SpinEdit));
  gchar *text = g_strdup_printf ("%c", i);
  gtk_label_set_text (GTK_LABEL (DLabel), text);
}

