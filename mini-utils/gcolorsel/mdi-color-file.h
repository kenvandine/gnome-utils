#ifndef __MDI_COLOR_FILE_H__
#define __MDI_COLOR_FILE_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "mdi-color-generic.h"

BEGIN_GNOME_DECLS

#define MDI_COLOR_FILE(obj)          GTK_CHECK_CAST (obj, mdi_color_file_get_type (), MDIColorFile)
#define MDI_COLOR_FILE_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, mdi_color_file_get_type (), MDIColorFileClass)
#define IS_MDI_COLOR_FILE(obj)       GTK_CHECK_TYPE (obj, mdi_color_file_get_type ())

typedef struct _MDIColorFile       MDIColorFile;
typedef struct _MDIColorFileClass  MDIColorFileClass;

struct _MDIColorFile {
  MDIColorGeneric mdi_child;

  char *filename;
};

struct _MDIColorFileClass {
  MDIColorGenericClass parent_class;
};

guint         mdi_color_file_get_type (void);

MDIColorFile *mdi_color_file_new      (const gchar *file_name);

gboolean      mdi_color_file_load     (MDIColorFile *mcf);

END_GNOME_DECLS

#endif
