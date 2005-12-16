/* gtkalignedwidget.h - Popup window aligned to a widget
 *
 * Copyright (c) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License  
 * along with this program; if not, write to the Free Software  
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Ported from Seth Nickell's Python class:
 *   Copyright (c) 2003 Seth Nickell
 */

#ifndef __GTK_ALIGNED_WINDOW_H__
#define __GTK_ALIGNED_WINDOW_H__

#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

#define GTK_TYPE_ALIGNED_WINDOW			(gtk_aligned_window_get_type ())
#define GTK_ALIGNED_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ALIGNED_WINDOW, GtkAlignedWindow))
#define GTK_IS_ALIGNED_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ALIGNED_WINDOW))
#define GTK_ALIGNED_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_ALIGNED_WINDOW, GtkAlignedWindowClass))
#define GTK_IS_ALIGNED_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ALIGNED_WINDOW))
#define GTK_ALIGNED_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ALIGNED_WINDOW, GtkAlignedWindowClass))

typedef struct _GtkAlignedWindow        GtkAlignedWindow;
typedef struct _GtkAlignedWindowClass	GtkAlignedWindowClass;
typedef struct _GtkAlignedWindowPrivate GtkAlignedWindowPrivate;

struct _GtkAlignedWindow
{
  /*< private >*/
  GtkWindow parent_instance;
  
  GtkAlignedWindowPrivate *priv;
};

struct _GtkAlignedWindowClass
{
  /*< private >*/
  GtkWindowClass parent_class;
  
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType      gtk_aligned_window_get_type   (void) G_GNUC_CONST;

GtkWidget *gtk_aligned_window_new        (GtkWidget        *align_widget);
void       gtk_aligned_window_set_widget (GtkAlignedWindow *aligned_window,
					  GtkWidget        *align_widget);
GtkWidget *gtk_aligned_window_get_widget (GtkAlignedWindow *aligned_window);

G_END_DECLS

#endif
