#ifndef _COLOR_LIST_H_
#define _COLOR_LIST_H_

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
typedef struct _ColorListData ColorListData;

struct _ColorList {
  GtkCList clist;    

  GdkGC *gc;
  
  ColorListFormat format;

  gboolean modified;

  int drag_row;
};

struct _ColorListClass {
  GtkCListClass parent_class;  

  GdkColor color_black;
  GdkColor color_white;

  GdkFont *pixmap_font;
};

struct _ColorListData {
  char *name;
  int r;
  int g;
  int b;
  int num;
  GdkColor *col;
  gulong pixel;
};

GtkType color_list_get_type (void);

GtkWidget *color_list_new (void);

void color_list_set_format (ColorList *cl, ColorListFormat format);

void color_list_set_sort_column (ColorList *cl, gint column, GtkSortType type);

gint color_list_append (ColorList *cl, int r, int g, int b, int num, char *name);
gint color_list_load (ColorList *cl, gchar *filename, GnomeApp *app);
gint color_list_save (ColorList *cl, gchar *filename, GnomeApp *app);

void color_list_set_modified (ColorList *cl, gboolean val);
void color_list_set_editable (ColorList *cl, gboolean val);

END_GNOME_DECLS

#endif /* _COLOR_LIST_H_ */
