/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   edit-menus: Gnome app for editing window manager, panel menus.
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

#include "menutree.h"

static void menu_tree_class_init (MenuTreeClass * mt_class);
static void menu_tree_init (MenuTree * mt);

static void destroy (GtkObject * mt);

guint menu_tree_get_type (void)
{
  static guint mt_type = 0;

  if (!mt_type) {
    GtkTypeInfo mt_info = 
    {
      "MenuTree",
      sizeof (MenuTree),
      sizeof (MenuTreeClass),
      (GtkClassInitFunc) menu_tree_class_init,
      (GtkObjectInitFunc) menu_tree_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    mt_type = gtk_type_unique (gtk_vbox_get_type(), 
				&mt_info);
  }

  return mt_type;
}

enum {
  SELECTION_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static gint mt_signals[LAST_SIGNAL] = { 0 };

static GtkDialogClass * parent_class = NULL;

static void menu_tree_class_init (MenuTreeClass * mt_class)
{
  GtkObjectClass * object_class;
  
  object_class = GTK_OBJECT_CLASS (mt_class);
  parent_class = gtk_type_class (gtk_vbox_get_type ());

  mt_signals[SELECTION_CHANGED_SIGNAL] = 
    gtk_signal_new ("selection_changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (MenuTreeClass, 
				       selection_changed),
		    menu_entry_marshaller,
		    GTK_TYPE_NONE,
		    3, GTK_TYPE_POINTER, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, mt_signals, 
				LAST_SIGNAL);

  object_class->destroy = destroy; 
  mt_class->selection_changed = 0;

  return;
}

static void menu_tree_init (MenuTree * mt)
{

}


static void destroy (GtkObject * o)
{
  MenuTree * mt = MENU_TREE(o);

  g_return_if_fail ( IS_MENU_TREE(mt) );

  if ( GTK_OBJECT_CLASS(parent_class)->destroy )
    GTK_OBJECT_CLASS (parent_class)->destroy (o);
}



