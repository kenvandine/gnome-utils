/***************************************************************************
                     interface.h  -  header of interface.c
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

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gnome.h>


/* The functions */

void main_new (void);
void main_controls_new (void);
void main_charmap_new (void);
gboolean ButtonClick (GtkWidget *widget, gpointer gdata);
gboolean ButtonEnter (GtkWidget *widget, gpointer gdata);
gboolean ButtonLeave (GtkWidget *widget, gpointer gdata);
gboolean EditMouseDown (GtkWidget *widget, GdkEventButton *event,
  gpointer user_data);
gboolean CutClick (GtkWidget *widget, gpointer gdata);
gboolean CopyClick (GtkWidget *widget, gpointer gdata);
gboolean PasteClick (GtkWidget *widget, gpointer gdata);
gboolean SelectAllClick (GtkWidget *widget, gpointer gdata);
gboolean AboutClick (GtkWidget *widget, gpointer gdata);
gboolean HelpClick (GtkWidget *widget, gpointer gdata);
gboolean ToolbarToggle (GtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
gboolean StatusbarToggle (GtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void PopupMenuDetach (GtkWidget *attach_widget, GtkMenu *Menu);
gboolean main_close (GtkWidget *widget, gpointer gdata);


/* The widgets */

GtkWidget *mainf;
GtkWidget *vbox0;
  GtkWidget *AppToolbar;
  GtkWidget *hbox;
    GtkWidget *vbox;
      GtkWidget *hbox1;
        GtkWidget *Label;
        GtkWidget *Edit;
      GtkWidget *table;
        GtkWidget *button;
    GtkWidget *vbox2;
      GtkWidget *btnbox;
        GtkWidget *Close;
        GtkWidget *Copy;
        GtkWidget *About;
        GtkWidget *Help;
      GtkWidget *Viewport;
        GtkWidget *Preview;
  GtkWidget *Status;
GtkWidget *PopupMenu;
GtkStyle *PreviewStyle;
GtkTooltips *tooltips;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _INTERFACE_H_ */
