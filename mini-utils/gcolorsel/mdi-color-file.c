#include "mdi-color-file.h"
#include "mdi-color-generic.h"

#include <gnome.h>

static void       mdi_color_file_class_init      (MDIColorFileClass *class);
static void       mdi_color_file_init            (MDIColorFile *mcf);

static MDIColorGenericClass *parent_class = NULL;

guint 
mdi_color_file_get_type()
{
  static guint mdi_gen_child_type = 0;
  
  if (!mdi_gen_child_type) {
    GtkTypeInfo mdi_gen_child_info = {
      "MDIColorFile",
      sizeof (MDIColorFile),
      sizeof (MDIColorFileClass),
      (GtkClassInitFunc) mdi_color_file_class_init,
      (GtkObjectInitFunc) mdi_color_file_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    mdi_gen_child_type = gtk_type_unique (mdi_color_generic_get_type (),
					  &mdi_gen_child_info);
  }
  
  return mdi_gen_child_type;
}

static void 
mdi_color_file_class_init (MDIColorFileClass *class)
{
  GnomeMDIChildClass *mdi_child_class;
  GtkObjectClass     *object_class;
  
  object_class    = GTK_OBJECT_CLASS (class);
  mdi_child_class = GNOME_MDI_CHILD_CLASS (class);
  parent_class    = gtk_type_class(mdi_color_generic_get_type());
}

static void
mdi_color_file_init (MDIColorFile *mcf)
{
  mcf->filename = NULL;

  MDI_COLOR_GENERIC (mcf)->flags = CHANGE_APPEND | CHANGE_REMOVE | 
    CHANGE_NAME | CHANGE_POS | CHANGE_RGB | CHANGE_CLEAR;
}

MDIColorFile *
mdi_color_file_new (const gchar *filename)
{
  MDIColorFile *mcf; 

  mcf = gtk_type_new (mdi_color_file_get_type ()); 

  mcf->filename = g_strdup (filename);
  GNOME_MDI_CHILD(mcf)->name = g_strdup (g_basename (filename));

  return mcf;
}

gboolean
mdi_color_file_load (MDIColorFile *mcf)
{
  FILE *fp;
  int r, g, b;
  char tmp[256], name[256];

  mdi_color_generic_freeze (MDI_COLOR_GENERIC (mcf));

  fp = fopen (mcf->filename, "r");
  if (!fp) return FALSE;

  while (1) {
    fgets(tmp, 255, fp);    

    if (feof (fp)) break;

    if ((tmp[0] == '!') || (tmp[0] == '#')) continue;

    name[0] = 0;
    if (sscanf(tmp, "%d %d %d\t\t%[a-zA-Z0-9 ]\n", &r, &g, &b, name) < 3) 
      return FALSE;

    mdi_color_generic_append_new (MDI_COLOR_GENERIC (mcf), 
				  r, g, b, name, NULL);
  }

  mdi_color_generic_thaw (MDI_COLOR_GENERIC (mcf));

  return TRUE;
}
