#ifndef WIDGET_COLOR_GRID_H
#define WIDGET_COLOR_GRID_H

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

  gpointer data;

  gboolean selected;
};

struct _ColorGrid {
  GnomeCanvas canvas;

  GCompareFunc compare_func;

  int freeze;

  gboolean can_move;

  int count;
  int nb_col;
  int col_width;
  int col_height;

  gboolean in_drag;
  gboolean button_pressed;

  GnomeCanvasItem *drop;
  
  GList *col;
  GList *selected;

  ColorGridCol *last_clicked; /* Used for SHIFT + CLICK */
  ColorGridCol *last_focus; 
};

struct _ColorGridClass {
  GnomeCanvasClass parent_class;  

  void (*move_item) (ColorGrid *cg, int old_pos, int new_pos, gpointer data);
};

GtkType color_grid_get_type (void);

GtkWidget *color_grid_new (GCompareFunc compare_func);

int color_grid_append  (ColorGrid *cg, int r, int g, int b, gpointer data);
void color_grid_remove (ColorGrid *cg, int pos);
void color_grid_clear  (ColorGrid *cg);

void color_grid_freeze (ColorGrid *cg);
void color_grid_thaw   (ColorGrid *cg);

void color_grid_sort   (ColorGrid *cg);

void color_grid_can_move (ColorGrid *cg, gboolean value);

void color_grid_set_col_width_height (ColorGrid *cg, int width, int height);

int  color_grid_find_item_from_data  (ColorGrid *cg, gpointer data);

void color_grid_change_rgb (ColorGrid *cg, int pos, int r, int g, int b);

END_GNOME_DECLS

#endif /* _COLOR_GRID_H_ */
