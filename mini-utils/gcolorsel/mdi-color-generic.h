#ifndef __MDI_COLOR_GENERIC_H__
#define __MDI_COLOR_GENERIC_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libgnomeui/gnome-mdi-child.h>

BEGIN_GNOME_DECLS

#define MDI_COLOR_GENERIC(obj)          GTK_CHECK_CAST (obj, mdi_color_generic_get_type (), MDIColorGeneric)
#define MDI_COLOR_GENERIC_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, mdi_color_generic_get_type (), MDIColorGenericClass)
#define IS_MDI_COLOR_GENERIC(obj)       GTK_CHECK_TYPE (obj, mdi_color_generic_get_type ())
#define MDI_COLOR_GENERIC_GET_CLASS(obj)  (MDI_COLOR_GENERIC_CLASS (GTK_OBJECT (obj)->klass))

typedef struct _MDIColorGenericCol    MDIColorGenericCol;
typedef struct _MDIColorGeneric       MDIColorGeneric;
typedef struct _MDIColorGenericClass  MDIColorGenericClass;

#include "widget-control-generic.h"

struct _MDIColorGenericCol {
  int ref : 8;

  MDIColorGeneric *owner; /* Never a MDIColorVirtual, ... */

  guint pos;
  guint r : 8;
  guint g : 8;
  guint b : 8;
  char *name;

  guint change : 8;

  gpointer data; /* See mdi-color-virtual */
};

typedef enum {
  /* Insert new */
  CHANGE_APPEND = 1 << 0,

  /* Edit existing */
  CHANGE_NAME   = 1 << 1,
  CHANGE_RGB    = 1 << 2,
  CHANGE_POS    = 1 << 4,

  /* Remove existing */
  CHANGE_REMOVE = 1 << 5,

  /* Remove all */
  CHANGE_CLEAR  = 1 << 6
} MDIColorGenericChangeType;

struct _MDIColorGeneric {
  GnomeMDIChild mdi_child;

  int flags; /* What the user can do. For example, it make no sense to
                move or append a color in a MDIColorVirtual.
		But he can Remove a color in a MDIColorVirtual */

  int freeze_count;
  int last;
  
  GtkType next_view_type;

  GList *other_views;
  GList *parents;

  GList *col;
  GList *changes;

  MDIColorGenericCol * (*get_owner) (MDIColorGenericCol *col);
  int (*get_append_pos)     (MDIColorGeneric *mcg, MDIColorGenericCol *col);
  GtkType (*get_control_type) (MDIColorGeneric *mcg);
};

struct _MDIColorGenericClass {
  GnomeMDIChildClass parent_class;

  void     (*document_changed) (MDIColorGeneric *mcg, gpointer data);
  gpointer (*get_control)      (MDIColorGeneric *cg, GtkVBox *box,
				void (*changed_cb)(gpointer data), 
				gpointer change_data);
  void     (*apply)            (MDIColorGeneric *mcg, gpointer data);
  void     (*close)            (MDIColorGeneric *mcg, gpointer data);
  void     (*sync)             (MDIColorGeneric *mcg, gpointer data);
};

guint mdi_color_generic_get_type (void);


void mdi_color_generic_append           (MDIColorGeneric *mcg, 
					 MDIColorGenericCol *col);
MDIColorGenericCol *
mdi_color_generic_append_new            (MDIColorGeneric *mcg,
					 int r, int g, int b, char *name, 
					 gpointer data);
void mdi_color_generic_remove           (MDIColorGeneric *mcg, 
					 MDIColorGenericCol *col);
void mdi_color_generic_change_rgb       (MDIColorGeneric *mcg, 
					 MDIColorGenericCol *col,
					 int r, int g, int b);
void mdi_color_generic_change_name      (MDIColorGeneric *mcg, 
					 MDIColorGenericCol *col,
					 char *name);
void mdi_color_generic_change_pos       (MDIColorGeneric *mcg,
					 MDIColorGenericCol *col, int new_pos);
void mdi_color_generic_clear            (MDIColorGeneric *mcg);

void mdi_color_generic_freeze           (MDIColorGeneric *mcg);
void mdi_color_generic_thaw             (MDIColorGeneric *mcg);

void mdi_color_generic_post_change      (MDIColorGeneric *mcg, 
					 MDIColorGenericCol *col,
					 MDIColorGenericChangeType type);
void mdi_color_generic_dispatch_changes (MDIColorGeneric *mcg);

gboolean mdi_color_generic_can_do       (MDIColorGeneric *mcg, 
					 MDIColorGenericChangeType what);

MDIColorGenericCol *
mdi_color_generic_search_by_data        (MDIColorGeneric *mcg,
					 gpointer data);

void mdi_color_generic_connect          (MDIColorGeneric *mcg,
					 MDIColorGeneric *to);

void mdi_color_generic_disconnect       (MDIColorGeneric *mcg,
					 MDIColorGeneric *to);

MDIColorGenericCol *
mdi_color_generic_get_owner             (MDIColorGenericCol *col);

GtkType mdi_color_generic_get_control_type (MDIColorGeneric *mcg);

void mdi_color_generic_sync_control     (MDIColorGeneric *mcg);

void mdi_color_generic_next_view_type   (MDIColorGeneric *mcg, GtkType type);
					
END_GNOME_DECLS

#endif
