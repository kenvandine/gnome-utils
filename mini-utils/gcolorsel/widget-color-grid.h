#ifndef WIDGET_COLOR_GRID_H
#define WIDGET_COLOR_GRID_H

#include "mdi-color-generic.h"

#include <gdk/gdk.h>
#include <libgnomeui/gnome-canvas.h>
#include <gtk/gtktooltips.h>

#include <libgnome/gnome-defs.h>
  
BEGIN_GNOME_DECLS

#define TYPE_COLOR_GRID            (color_grid_get_type ())
#define COLOR_GRID(obj)            (GTK_CHECK_CAST ((obj), TYPE_COLOR_GRID, ColorGrid))
#define COLOR_GRID_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_COLOR_GRID, ColorGridClass))
#define IS_COLOR_GRID(obj)         (GTK_CHECK_TYPE ((obj), TYPE_COLOR_GRID))
#define IS_COLOR_GRID_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_COLOR_GRID))
#define COLOR_GRID_GET_CLASS(obj)  (COLOR_GRID_CLASS (GTK_OBJECT (obj)->klass))

typedef struct _ColorGrid ColorGrid;
typedef struct _ColorGridClass ColorGridClass;
typedef struct _ColorGridCol ColorGridCol;

struct _ColorGridCol {
  ColorGrid *cg;
  GnomeCanvasItem *item;

  MDIColorGenericCol *col;

  gboolean selected;
};

struct _ColorGrid {
  GnomeCanvas canvas;

  int nb_col;
  int col_width;
  int col_height;

  gboolean in_drag;
  gboolean button_pressed;

  GnomeCanvasItem *drop;
  
  GList *col;
  GList *selected;

  ColorGridCol *last_clicked;
};

struct _ColorGridClass {
  GnomeCanvasClass parent_class;  

  void (*data_changed) (ColorGrid *cg, gpointer data);
};

GtkType color_grid_get_type (void);

GtkWidget *color_grid_new (void);

END_GNOME_DECLS

#endif /* _COLOR_GRID_H_ */
