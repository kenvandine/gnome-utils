#ifndef WIDGET_CONTROL_VIRTUAL_H
#define WIDGET_CONTROL_VIRTUAL_H

#include "widget-control-generic.h"

#include <gnome.h>
  
BEGIN_GNOME_DECLS

#define TYPE_CONTROL_VIRTUAL            (control_virtual_get_type ())
#define CONTROL_VIRTUAL(obj)            (GTK_CHECK_CAST ((obj), TYPE_CONTROL_VIRTUAL, ControlVirtual))
#define CONTROL_VIRTUAL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_CONTROL_VIRTUAL, ControlVirtualClass))
#define IS_CONTROL_VIRTUAL(obj)         (GTK_CHECK_TYPE ((obj), TYPE_CONTROL_VIRTUAL))
#define IS_CONTROL_VIRTUAL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_CONTROL_VIRTUAL))
#define CONTROL_VIRTUAL_GET_CLASS(obj)  (CONTROL_VIRTUAL_CLASS (GTK_OBJECT (obj)->klass))

typedef struct _ControlVirtual ControlVirtual;
typedef struct _ControlVirtualClass ControlVirtualClass;

struct _ControlVirtual {
  ControlGeneric cg;

  GtkWidget *preview;

  GtkWidget *range_red;  
  GtkWidget *range_green;
  GtkWidget *range_blue;
  GtkWidget *range_tolerance;

  float r;
  float g;
  float b;
  float t;
};

struct _ControlVirtualClass {
  ControlGenericClass parent_class; 
};

GtkType    control_virtual_get_type (void);
GtkWidget *control_virtual_new      (void);

END_GNOME_DECLS

#endif /* _CONTROL_VIRTUAL_H_ */
