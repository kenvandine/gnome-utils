#ifndef _COLOR_SEARCH_H_
#define _COLOR_SEARCH_H_

#include <gdk/gdk.h>
#include <gtk/gtkhbox.h>

#include <libgnome/gnome-defs.h>

#include "widget-color-list.h"
  
BEGIN_GNOME_DECLS

#define TYPE_COLOR_SEARCH            (color_search_get_type ())
#define COLOR_SEARCH(obj)            (GTK_CHECK_CAST ((obj), TYPE_COLOR_SEARCH, ColorSearch))
#define COLOR_SEARCH_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_COLOR_SEARCH, ColorSearchClass))
#define IS_COLOR_SEARCH(obj)         (GTK_CHECK_TYPE ((obj), TYPE_COLOR_SEARCH))
#define IS_COLOR_SEARCH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_COLOR_SEARCH))
#define COLOR_SEARCH_GET_CLASS(obj)  (COLOR_SEARCH_CLASS (GTK_OBJECT (obj)->klass))

typedef struct _ColorSearch ColorSearch;
typedef struct _ColorSearchClass ColorSearchClass;

struct _ColorSearch {
  GtkVBox vbox;

  GtkWidget *cl;
  GtkWidget *sw;
  GtkWidget *preview;

  GtkWidget *range_red;  
  GtkWidget *range_green;
  GtkWidget *range_blue;
  GtkWidget *range_tolerance;
};

struct _ColorSearchClass {
  GtkVBoxClass parent_class; 

  void (* search_update) (ColorSearch *cs);
};

GtkType color_search_get_type (void);

GtkWidget *color_search_new (void);

void color_search_update_preview (ColorSearch *cs);
void color_search_update_result (ColorSearch *cs);
void color_search_update (ColorSearch *cs);

void color_search_search (ColorSearch *cs, gint r, gint g, gint b);

void color_search_get_rgb (ColorSearch *cs, int *r, int *g, int *b);
gint color_search_get_tolerance (ColorSearch *cs);

void color_search_search_in (ColorSearch *cs, GtkCList *clist);

END_GNOME_DECLS

#endif /* _COLOR_SEARCH_H_ */
