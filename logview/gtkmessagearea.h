/*
 * gtkmessagearea.h
 * This file is part of GTK+
 *
 * Copyright (C) 2005 - Paolo Maggi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the gedit ChangeLog files for a list of changes.
 */

#ifndef __GTK_MESSAGE_AREA_H__
#define __GTK_MESSAGE_AREA_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GTK_TYPE_MESSAGE_AREA              (gtk_message_area_get_type())
#define GTK_MESSAGE_AREA(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_MESSAGE_AREA, GtkMessageArea))
#define GTK_MESSAGE_AREA_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_MESSAGE_AREA, GtkMessageAreaClass))
#define GTK_IS_MESSAGE_AREA(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_MESSAGE_AREA))
#define GTK_IS_MESSAGE_AREA_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MESSAGE_AREA))
#define GTK_MESSAGE_AREA_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_MESSAGE_AREA, GtkMessageAreaClass))


typedef struct _GtkMessageAreaPrivate GtkMessageAreaPrivate;
typedef struct _GtkMessageAreaClass GtkMessageAreaClass;
typedef struct _GtkMessageArea GtkMessageArea;


struct _GtkMessageArea
{
  GtkHBox parent;

  /*< private > */
  GtkMessageAreaPrivate *priv;
};


struct _GtkMessageAreaClass
{
  GtkHBoxClass parent_class;

  /* Signals */
  void (* response) (GtkMessageArea *message_area, gint response_id);

  /* Keybinding signals */
  void (* close)    (GtkMessageArea *message_area);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
  void (*_gtk_reserved5) (void);
  void (*_gtk_reserved6) (void);
};

GType            gtk_message_area_get_type              (void) G_GNUC_CONST;
GtkWidget       *gtk_message_area_new                   (void);

GtkWidget       *gtk_message_area_new_with_buttons      (const gchar    *first_button_text,
                                                         ...);

void             gtk_message_area_set_contents          (GtkMessageArea *message_area,
                                                         GtkWidget      *contents);

void             gtk_message_area_add_action_widget     (GtkMessageArea *message_area,
                                                         GtkWidget      *child,
                                                         gint            response_id);
GtkWidget       *gtk_message_area_add_button            (GtkMessageArea *message_area,
                                                         const gchar    *button_text,
                                                         gint            response_id);
GtkWidget       *gtk_message_area_add_stock_button_with_text
                                                        (GtkMessageArea *message_area,
                                                         const gchar    *text,
                                                         const gchar    *stock_id,
                                                         gint            response_id);
void             gtk_message_area_add_buttons           (GtkMessageArea *message_area,
                                                         const gchar    *first_button_text,
                                                         ...);
void             gtk_message_area_set_response_sensitive
                                                        (GtkMessageArea *message_area,
                                                         gint            response_id,
                                                         gboolean        setting);
void             gtk_message_area_set_default_response  (GtkMessageArea *message_area,
                                                         gint            response_id);

/* Emit response signal */
void             gtk_message_area_response              (GtkMessageArea *message_area,
                                                         gint            response_id);

G_END_DECLS

#endif  /* __GTK_MESSAGE_AREA_H__  */
