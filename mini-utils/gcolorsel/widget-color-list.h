#ifndef WIDGET_COLOR_LIST_H
#define WIDGET_COLOR_LIST_H

#include <gdk/gdk.h>
#include <gtk/gtkclist.h>

#include <libgnome/gnome-defs.h>
  
BEGIN_GNOME_DECLS

typedef enum {
  COLOR_LIST_FORMAT_DEC_8, 
  COLOR_LIST_FORMAT_DEC_16,
  COLOR_LIST_FORMAT_HEX_8, 
  COLOR_LIST_FORMAT_HEX_16,
  COLOR_LIST_FORMAT_FLOAT
} ColorListFormat;

#define TYPE_COLOR_LIST            (color_list_get_type ())
#define COLOR_LIST(obj)            (GTK_CHECK_CAST ((obj), TYPE_COLOR_LIST, ColorList))
#define COLOR_LIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_COLOR_LIST, ColorListClass))
#define IS_COLOR_LIST(obj)         (GTK_CHECK_TYPE ((obj), TYPE_COLOR_LIST))
#define IS_COLOR_LIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_COLOR_LIST))
#define COLOR_LIST_GET_CLASS(obj)  (COLOR_LIST_CLASS (GTK_OBJECT (obj)->klass))

typedef struct _ColorList ColorList;
typedef struct _ColorListClass ColorListClass;

struct _ColorList {
  GtkCList clist;    

  GdkGC *gc;
  
  ColorListFormat format;

  int drag_row;
};

struct _ColorListClass {
  GtkCListClass parent_class;  

  GdkColor color_black;
  GdkColor color_white;

  GdkFont *pixmap_font;

  void (*data_changed) (ColorList *cl, gpointer data);
};

GtkType color_list_get_type (void);

GtkWidget *color_list_new (void);

void color_list_set_format (ColorList *cl, ColorListFormat format);

void color_list_set_sort_column (ColorList *cl, gint column, GtkSortType type);

END_GNOME_DECLS

#endif /* _COLOR_LIST_H_ */
