/***************************************************************************
                       interface.c  -  The user interface
                             -------------------
    begin                : Sat Apr 15 2000
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

#include "config.h"

#include "interface.h"
#include "asciiselect.h"
#include "gcharmap.xpm"

/* The menus */

GnomeUIInfo file_menu[] = {
   GNOMEUIINFO_MENU_EXIT_ITEM (main_close, NULL),
   GNOMEUIINFO_END
};

GnomeUIInfo edit_menu[] = {
   GNOMEUIINFO_MENU_CUT_ITEM (CutClick, NULL),
   GNOMEUIINFO_MENU_COPY_ITEM (CopyClick, NULL),
   GNOMEUIINFO_MENU_PASTE_ITEM (PasteClick, NULL),
   GNOMEUIINFO_SEPARATOR,
   GNOMEUIINFO_MENU_SELECT_ALL_ITEM (SelectAllClick, NULL),
   GNOMEUIINFO_SEPARATOR,
   GNOMEUIINFO_ITEM_NONE("Insert ASCII character...", "Insert ASCII character",
     InsertAsciiCharacterClick),
   GNOMEUIINFO_END
};

GnomeUIInfo view_menu[] = {
   GNOMEUIINFO_TOGGLEITEM (N_("_Toolbar"), N_("View or hide the toolbar"),
     ToolbarToggle, NULL),
   GNOMEUIINFO_TOGGLEITEM (N_("_Statusbar"), N_("View or hide the statusbar"),
     StatusbarToggle, NULL),
   GNOMEUIINFO_END
};

GnomeUIInfo help_menu[] = {
   GNOMEUIINFO_HELP ("gcharmap"),
   GNOMEUIINFO_MENU_ABOUT_ITEM (AboutClick, NULL),
   GNOMEUIINFO_END
};

GnomeUIInfo menubar[] = {
   GNOMEUIINFO_MENU_FILE_TREE(file_menu),
   GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
   GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
   GNOMEUIINFO_MENU_HELP_TREE(help_menu),
   GNOMEUIINFO_END
};

GnomeUIInfo toolbar[] = {
   GNOMEUIINFO_ITEM_STOCK(N_("Cut"), N_("Cut the text to clipboard"),
     CutClick, GNOME_STOCK_PIXMAP_CUT),
   GNOMEUIINFO_ITEM_STOCK(N_("Copy"), N_("Copy the text to clipboard."),
     CopyClick, GNOME_STOCK_PIXMAP_COPY),
   GNOMEUIINFO_ITEM_STOCK(N_("Paste"), N_("Paste the text from clipboard."),
     PasteClick, GNOME_STOCK_PIXMAP_PASTE),
   GNOMEUIINFO_SEPARATOR,
   GNOMEUIINFO_ITEM_STOCK(N_("Exit"), N_("Exit the application"),
     main_close, GNOME_STOCK_PIXMAP_EXIT),
   GNOMEUIINFO_END
};

static GnomeUIInfo PopupMenu_menu[] =
{
   GNOMEUIINFO_MENU_CUT_ITEM (CutClick, NULL),
   GNOMEUIINFO_MENU_COPY_ITEM (CopyClick, NULL),
   GNOMEUIINFO_MENU_PASTE_ITEM (PasteClick, NULL),
   GNOMEUIINFO_SEPARATOR,
   GNOMEUIINFO_MENU_SELECT_ALL_ITEM (SelectAllClick, NULL),
   GNOMEUIINFO_END
};


/* The functions */

void
main_new ()
{
  GdkPixmap *Pixmap;
  GdkBitmap *Mask;
  GtkWidget *item;

  tooltips = gtk_tooltips_new ();

  mainf = gnome_app_new (N_("Gnome Character Map"), N_("Gnome Character Map"));
  gtk_widget_realize (mainf);
  gtk_signal_connect (GTK_OBJECT (mainf), "delete_event",
    GTK_SIGNAL_FUNC (main_close), NULL);
  Pixmap = gdk_pixmap_create_from_xpm_d (mainf->window, &Mask, NULL,
     (gchar **) gcharmap_xpm);
  gdk_window_set_icon (mainf->window, NULL, Pixmap, Mask);

  main_controls_new ();
  main_charmap_new ();
  gtk_widget_show_all (mainf);

  item = view_menu[0].widget;
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
  item = view_menu[1].widget;
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
}

void
main_controls_new ()
{
  guint8 i;
  GdkColor white = {0, 0xffff, 0xffff, 0xffff};
  GdkColor black = {0, 0x0000, 0x0000, 0x0000};
  gdk_color_alloc (gdk_colormap_get_system (), &white);
  gdk_color_alloc (gdk_colormap_get_system (), &black);

  vbox0 = gtk_vbox_new (FALSE, 8);
  gnome_app_set_contents (GNOME_APP (mainf), vbox0);
  gnome_app_create_menus (GNOME_APP (mainf), menubar);

  AppToolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gnome_app_fill_toolbar (GTK_TOOLBAR (AppToolbar), toolbar, NULL);
  gnome_app_add_toolbar (GNOME_APP (mainf), GTK_TOOLBAR (AppToolbar),
    "Toolbar", GNOME_DOCK_ITEM_BEH_EXCLUSIVE, GNOME_DOCK_TOP, 1, 0, 22);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox0), hbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);

  vbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  hbox1 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, TRUE, 0);

  Label = gtk_label_new (N_("Text to Copy:"));
  gtk_box_pack_start (GTK_BOX (hbox1), Label, FALSE, TRUE, 0);

  Edit = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox1), Edit, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (Edit), "button-press-event",
    GTK_SIGNAL_FUNC (EditMouseDown), NULL);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  btnbox = gtk_vbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (btnbox), GTK_BUTTONBOX_START);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (btnbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox2), btnbox, TRUE, TRUE, 0);

  Viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (Viewport), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), Viewport, FALSE, TRUE, 0);

  PreviewStyle = gtk_style_copy (gtk_widget_get_style (Viewport));
  for (i = 0; i <= 5; i++) PreviewStyle->bg[i] = black;
  for (i = 0; i <= 5; i++) PreviewStyle->fg[i] = black;
  for (i = 0; i <= 5; i++) PreviewStyle->text[i] = white;
  gtk_widget_set_style (Viewport, PreviewStyle);
  gtk_widget_push_style (PreviewStyle);
  gtk_widget_pop_style ();

  Preview = gtk_label_new ("");
  gtk_container_add (GTK_CONTAINER (Viewport), Preview);
  PreviewStyle = gtk_style_copy (gtk_widget_get_style (Preview));
  for (i = 0; i <= 5; i++) PreviewStyle->bg[i] = black;
  for (i = 0; i <= 5; i++) PreviewStyle->fg[i] = white;
  PreviewStyle->font = gdk_font_load ("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-iso8859-1");
  gtk_widget_set_style (Preview, PreviewStyle);
  gtk_widget_set_style (Preview, PreviewStyle);
  gtk_widget_push_style (PreviewStyle);
  gtk_widget_pop_style ();

  Close = gtk_button_new_with_label ("Close");
  GTK_WIDGET_SET_FLAGS (Close, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (Close), "clicked",
    GTK_SIGNAL_FUNC (main_close), NULL);
  gtk_container_add (GTK_CONTAINER (btnbox), Close);

  Copy = gtk_button_new_with_label ("Copy");
  GTK_WIDGET_SET_FLAGS (Copy, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (Copy), "clicked",
    GTK_SIGNAL_FUNC (CopyClick), NULL);
  gtk_container_add (GTK_CONTAINER (btnbox), Copy);

  About = gtk_button_new_with_label ("About");
  GTK_WIDGET_SET_FLAGS (About, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (About), "clicked",
    GTK_SIGNAL_FUNC (AboutClick), NULL);
  gtk_container_add (GTK_CONTAINER (btnbox), About);

  Help = gtk_button_new_with_label ("Help");
  GTK_WIDGET_SET_FLAGS (Help, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (Help), "clicked",
    GTK_SIGNAL_FUNC (HelpClick), NULL);
  gtk_container_add (GTK_CONTAINER (btnbox), Help);

  Status = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
  gnome_app_set_statusbar (GNOME_APP (mainf), Status);

  PopupMenu = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (PopupMenu), PopupMenu_menu, NULL,
    FALSE, 0);

  gtk_widget_grab_default (Copy);
  gtk_widget_grab_focus (Edit);
}

void
main_charmap_new ()
{
  guint8 v; /* Columns */
  guint8 h; /* Rows */
  GdkColor white = {0, 0xFFFF, 0xFFFF, 0xFFFF};
  GdkColor blue = {0, 0x7777, 0x7777, 0xFFFF};
  GdkColor lightblue = {0, 0x9999, 0x9999, 0xFFFF};
  gdk_color_alloc (gdk_colormap_get_system (), &white);
  gdk_color_alloc (gdk_colormap_get_system (), &blue);
  gdk_color_alloc (gdk_colormap_get_system (), &lightblue);

  table = gtk_table_new (8, 24, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  for (v = 0; v <= 3; v++) {
     for (h = 0; h <= 23; h++) {
        gchar *s = g_new0 (gchar, 20);
        s = g_strdup_printf ("%c", v * 24 + h + 32);

        button = gtk_button_new_with_label (s);
        gtk_table_attach (GTK_TABLE (table), button, h, h + 1, v, v + 1,
          (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
          (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
        gtk_tooltips_set_tip (tooltips, button, s, s);
        gtk_signal_connect (GTK_OBJECT (button), "clicked",
          GTK_SIGNAL_FUNC (ButtonClick), NULL);
        gtk_signal_connect (GTK_OBJECT (button), "enter",
          GTK_SIGNAL_FUNC (ButtonEnter), NULL);
        gtk_signal_connect (GTK_OBJECT (button), "leave",
          GTK_SIGNAL_FUNC (ButtonLeave), NULL);

        g_free (s);
     }
  }
  for (v = 0; v <= 3; v++) {
     for (h = 0; h <= 23; h++) {
        gchar *s = g_new0 (gchar, 20);
        s = g_strdup_printf ("%c", v * 24 + h + 161);

        button = gtk_button_new_with_label (s);
        gtk_table_attach (GTK_TABLE (table), button, h, h + 1, v + 4, v + 5,
          (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
          (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
        gtk_tooltips_set_tip (tooltips, button, s, s);
        gtk_signal_connect (GTK_OBJECT (button), "clicked",
          GTK_SIGNAL_FUNC (ButtonClick), NULL);
        gtk_signal_connect (GTK_OBJECT (button), "enter",
          GTK_SIGNAL_FUNC (ButtonEnter), NULL);
        gtk_signal_connect (GTK_OBJECT (button), "leave",
          GTK_SIGNAL_FUNC (ButtonLeave), NULL);

        g_free (s);
     }
  }
}

gboolean
EditMouseDown (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  if (event->button == 3)
     gtk_menu_popup (GTK_MENU (PopupMenu), NULL, NULL, NULL, NULL, TRUE, 0);
  return FALSE;
}

gboolean
ButtonClick (GtkWidget *widget, gpointer gdata)
{
  gchar *text;
  GtkLabel *label = GTK_LABEL (GTK_BIN (widget)->child);
  gtk_label_get (label, &text);
  gtk_entry_append_text (GTK_ENTRY (Edit), text);
  return FALSE;
}

gboolean
ButtonEnter (GtkWidget *widget, gpointer gdata)
{
  gchar *text;
  GtkLabel *label = GTK_LABEL (GTK_BIN (widget)->child);
  gtk_label_get (label, &text);
  gtk_label_set_text (GTK_LABEL (Preview), text);
  gnome_appbar_set_status (GNOME_APPBAR (Status),
    g_strdup_printf (N_("%s: ASCII character %d"), text, (unsigned char) text[0]));
  return FALSE;
}

gboolean
ButtonLeave (GtkWidget *widget, gpointer gdata)
{
  gtk_label_set_text (GTK_LABEL (Preview), "");
  gnome_appbar_set_status (GNOME_APPBAR (Status), "");
  return FALSE;
}

gboolean
CutClick (GtkWidget *widget, gpointer gdata)
{
  SelectAllClick (Edit, NULL);
  gtk_editable_cut_clipboard (GTK_EDITABLE (Edit));
  gnome_app_flash (GNOME_APP (mainf), N_("Text cut to clipboard..."));
  return FALSE;
}

gboolean
CopyClick (GtkWidget *widget, gpointer gdata)
{
  SelectAllClick (Edit, NULL);
  gtk_editable_copy_clipboard (GTK_EDITABLE (Edit));
  gnome_app_flash (GNOME_APP (mainf), N_("Text copied to clipboard..."));
  return FALSE;
}

gboolean
PasteClick (GtkWidget *widget, gpointer gdata)
{
  gtk_editable_paste_clipboard (GTK_EDITABLE (Edit));
  gnome_appbar_set_status (GNOME_APPBAR (Status),
    N_("Text pasted from clipboard..."));
  return FALSE;
}

gboolean
SelectAllClick (GtkWidget *widget, gpointer gdata)
{
  gint length = strlen (gtk_entry_get_text (GTK_ENTRY (Edit)));
  gtk_entry_select_region (GTK_ENTRY (Edit), 0, length);
  return FALSE;
}

gboolean
AboutClick (GtkWidget *widget, gpointer gdata)
{
/*  #include "pixmaps/logo.xpm" */
  const gchar *Authors[] = {"Hongli Lai (hongli@telekabel.nl)", NULL};
  GtkWidget *dialog = gnome_about_new (N_("Gnome Character Map"), VERSION,
    "Copyright (c) 2000 Hongli Lai", Authors, N_("The Gnome equalivant of the "
    "Microsoft Windows' Character Map. Warning: might contain bad English.\n"
    "http://users.telekabel.nl/hongli/"), NULL
    /* (gchar *) logo_xpm */);

  gnome_dialog_set_parent(GNOME_DIALOG (dialog), GTK_WINDOW (mainf));
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_widget_show (dialog);
  return FALSE;
}

gboolean
HelpClick (GtkWidget *widget, gpointer gdata)
{
  gnome_app_message (GNOME_APP (mainf),
    N_("There's no documentation available yet."));
  return FALSE;
}

gboolean
ToolbarToggle (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
  if (checkmenuitem->active == TRUE)
     gtk_widget_show (AppToolbar->parent);
  else
     gtk_widget_hide (AppToolbar->parent);
  return FALSE;
}

gboolean
StatusbarToggle (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
  if (checkmenuitem->active == TRUE)
     gtk_widget_show (GNOME_APP (mainf)->statusbar->parent);
  else
     gtk_widget_hide (GNOME_APP (mainf)->statusbar->parent);
  return FALSE;
}

void
PopupMenuDetach (GtkWidget *attach_widget, GtkMenu *Menu)
{
  /* Do nothing */
}

gboolean
main_close (GtkWidget *widget, gpointer gdata)
{
  gtk_main_quit ();
  return FALSE;
}

