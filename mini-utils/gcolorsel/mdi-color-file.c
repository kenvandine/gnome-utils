#include "mdi-color-file.h"
#include "mdi-color-generic.h"

#include <gnome.h>
#include <glade/glade.h>

static void       mdi_color_file_class_init  (MDIColorFileClass *class);
static void       mdi_color_file_init        (MDIColorFile *mcf);

static gpointer   
mdi_color_generic_get_control (MDIColorGeneric *vcg, GtkVBox *box,
			       void (*changed_cb)(gpointer data), 
			       gpointer change_data);
static void mdi_color_generic_apply (MDIColorGeneric *mcg, gpointer data);
static void mdi_color_generic_close (MDIColorGeneric *mcg, gpointer data);
static void mdi_color_generic_sync  (MDIColorGeneric *mcg, gpointer data);

static void mdi_color_generic_load  (MDIColorGeneric *mcg);
static void mdi_color_generic_save  (MDIColorGeneric *mcg);

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
  GnomeMDIChildClass   *mdi_child_class;
  GtkObjectClass       *object_class;
  MDIColorGenericClass *mcg_class;
  
  object_class    = GTK_OBJECT_CLASS (class);
  mdi_child_class = GNOME_MDI_CHILD_CLASS (class);
  parent_class    = gtk_type_class(mdi_color_generic_get_type());
  mcg_class       = (MDIColorGenericClass *)class;

  mcg_class->get_control = mdi_color_generic_get_control;
  mcg_class->apply       = mdi_color_generic_apply;
  mcg_class->close       = mdi_color_generic_close;
  mcg_class->sync        = mdi_color_generic_sync;
  mcg_class->load        = mdi_color_generic_load;
  mcg_class->save        = mdi_color_generic_save;
}

static void
mdi_color_file_init (MDIColorFile *mcf)
{
  mcf->filename = NULL;

  MDI_COLOR_GENERIC (mcf)->flags = CHANGE_APPEND | CHANGE_REMOVE | 
    CHANGE_NAME | CHANGE_POS | CHANGE_RGB | CHANGE_CLEAR;
}

MDIColorFile *
mdi_color_file_new (void)
{
  MDIColorFile *mcf; 

  mcf = gtk_type_new (mdi_color_file_get_type ()); 

/*  mcf->filename = g_strdup (filename);
    GNOME_MDI_CHILD(mcf)->name = g_strdup (g_basename (filename));*/

  return mcf;
}

void
mdi_color_file_set_filename (MDIColorFile *mcf, const char *filename)
{
  if (mcf->filename)
    g_free (mcf->filename);

  mcf->filename = g_strdup (filename);  
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
				  r, g, b, name);
  }

  mdi_color_generic_thaw (MDI_COLOR_GENERIC (mcf));

  return TRUE;
}

/************************* PROPERTIES ************************************/

typedef struct prop_t {
  GladeXML *gui;

  gpointer parent_data;

  void (*changed_cb)(gpointer data);
  gpointer change_data;

  GtkWidget *entry_file;
  GtkWidget *text_comments;
} prop_t;

static gpointer
mdi_color_generic_get_control (MDIColorGeneric *mcg, GtkVBox *box,
			       void (*changed_cb)(gpointer data), 
			       gpointer change_data)
{
  GtkWidget *frame;
  prop_t *prop = g_new0 (prop_t, 1);
  
  prop->parent_data = parent_class->get_control (mcg, box, 
						 changed_cb, change_data);

  prop->changed_cb   = changed_cb;
  prop->change_data = change_data;

  prop->gui = glade_xml_new (GCOLORSEL_GLADEDIR "mdi-color-file-properties.glade", "frame");
  if (!prop->gui) {
    printf ("Could not find mdi-color-file-properties.glade\n");
    return NULL;
  }

  frame = glade_xml_get_widget (prop->gui, "frame");
  if (!frame) {
    printf ("Corrupt file mdi-color-file-properties.glade");
    return NULL;
  }

  gtk_box_pack_start_defaults (GTK_BOX (box), frame);

  prop->entry_file = glade_xml_get_widget (prop->gui, "entry-file");
  prop->text_comments = glade_xml_get_widget (prop->gui, "text-comments");

  return prop;
}

static void
mdi_color_generic_sync (MDIColorGeneric *mcg, gpointer data)
{
  prop_t *prop = data;

  printf ("MDI File :: sync\n");

  gtk_entry_set_text (GTK_ENTRY (prop->entry_file), 
		      MDI_COLOR_FILE (mcg)->filename);

  parent_class->sync (mcg, prop->parent_data);
}

static void
mdi_color_generic_apply (MDIColorGeneric *mcg, gpointer data)
{
  prop_t *prop = data;  

  printf ("MDI File :: apply\n");

  parent_class->apply (mcg, prop->parent_data);
}

static void
mdi_color_generic_close (MDIColorGeneric *mcg, gpointer data)
{
  prop_t *prop = data;

  printf ("MDI File :: close\n");

  parent_class->close (mcg, prop->parent_data);

  gtk_object_unref (GTK_OBJECT (prop->gui));
  g_free (prop);
}

/******************************** Config *********************************/

static void 
mdi_color_generic_save  (MDIColorGeneric *mcg)
{
  gnome_config_set_string ("FileName", MDI_COLOR_FILE (mcg)->filename);

  parent_class->save (mcg);
}

static void 
mdi_color_generic_load  (MDIColorGeneric *mcg)
{
  MDI_COLOR_FILE (mcg)->filename = gnome_config_get_string ("FileName");

  parent_class->load (mcg);
}

