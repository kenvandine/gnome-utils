#ifndef __MDI_COLOR_VIRTUAL_H__
#define __MDI_COLOR_VIRTUAL_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "mdi-color-generic.h"

BEGIN_GNOME_DECLS

#define MDI_COLOR_VIRTUAL(obj)          GTK_CHECK_CAST (obj, mdi_color_virtual_get_type (), MDIColorVirtual)
#define MDI_COLOR_VIRTUAL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, mdi_color_virtual_get_type (), MDIColorVirtualClass)
#define IS_MDI_COLOR_VIRTUAL(obj)       GTK_CHECK_TYPE (obj, mdi_color_virtual_get_type ())

typedef struct _MDIColorVirtual       MDIColorVirtual;
typedef struct _MDIColorVirtualClass  MDIColorVirtualClass;

struct _MDIColorVirtual {
  MDIColorGeneric mdi_child;

  float r;
  float g;
  float b;
  float t; /* Tolerance */
};

struct _MDIColorVirtualClass {
  MDIColorGenericClass parent_class;
};

guint            mdi_color_virtual_get_type (void);
MDIColorVirtual *mdi_color_virtual_new      (void);

void             mdi_color_virtual_set      (MDIColorVirtual *mcv, 
					     float r, float g, 
					     float b, float t);

void             mdi_color_virtual_get      (MDIColorVirtual *mcv,
					     float *r, float *g, 
					     float *b, float *t);

END_GNOME_DECLS

#endif
