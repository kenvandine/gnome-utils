#include "session.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "mdi-color-virtual.h"
#include "view-color-generic.h"
#include "view-color-list.h"
#include "view-color-grid.h"
#include "utils.h"

#include "gnome.h"


/******************************* Save **********************************/

static char *
create_string_from_list (GList *list, char *data, int offset)
{
  char *buf;
  GString *string = g_string_new (NULL);
  char *d;

  while (list) {
    if (list->prev)
      g_string_append_c (string, ':');

    if (data) 
      d = gtk_object_get_data (GTK_OBJECT (list->data), data);
    else
      d = list->data;

    buf = g_strdup_printf ("%d", *((int *)(d + offset)));
    g_string_append (string, buf);
    g_free (buf);

    list = g_list_next (list);
  }

  buf = string->str;
  g_string_free (string, FALSE);

  return buf;
}

static void
save_views_config (GnomeMDIChild *child)
{
  GList *list = child->views;
  ViewColorGeneric *vcg;
  char *prefix;

  while (list) {    
    vcg = gtk_object_get_data (GTK_OBJECT (list->data), "view_object");

    prefix = g_strdup_printf ("/gcolorsel/%d/", vcg->key);
    gnome_config_clean_section (prefix);
    gnome_config_push_prefix (prefix);

    gnome_config_set_string ("Type", gtk_type_name (GTK_OBJECT_TYPE (vcg)));

    VIEW_COLOR_GENERIC_GET_CLASS (vcg)->save (vcg); 

    gnome_config_pop_prefix ();

    list = g_list_next (list);
  }
}

static void
save_config (GnomeMDI *mdi)
{
  GList *list = mdi->children;
  MDIColorGeneric *mcg;
  char *prefix;
  char *str;

  while (list) {
    mcg = list->data;
    
    prefix = g_strdup_printf ("/gcolorsel/%d/", mcg->key);
    gnome_config_clean_section (prefix);
    gnome_config_push_prefix (prefix);
    
    gnome_config_set_string ("Type", gtk_type_name (GTK_OBJECT_TYPE (mcg)));
    
    str = create_string_from_list (GNOME_MDI_CHILD (mcg)->views, "view_object",
				   G_STRUCT_OFFSET (ViewColorGeneric, key));
    gnome_config_set_string ("Views", str);
    g_free (str);

    str = create_string_from_list (mcg->parents, NULL,
				   G_STRUCT_OFFSET (MDIColorGeneric, key));
    gnome_config_set_string ("Parents", str);
    g_free (str);
    
    MDI_COLOR_GENERIC_GET_CLASS (mcg)->save (mcg); 

    gnome_config_pop_prefix ();
    g_free (prefix);

    save_views_config (GNOME_MDI_CHILD (mcg));
    
    list = g_list_next (list);
  }
}

void
session_save (GnomeMDI *mdi)
{
  save_config (mdi);
  gnome_mdi_save_state (mdi, "/gcolorsel/mdi");  

  gnome_config_push_prefix ("/gcolorsel/gcolorsel/");
  gnome_config_set_bool ("FirstTime", FALSE);
  gnome_config_set_int  ("Key", get_config_key_pos ());
  gnome_config_set_int  ("TabPos", mdi->tab_pos);
  gnome_config_pop_prefix ();

  gnome_config_sync ();
}

/******************************** Load ************************************/

static void
next_views (MDIColorGeneric *mcg, char *str)
{
  gchar **tab;
  char *prefix;
  char *buf;
  int i = 0;
  GtkType type;

  tab = g_strsplit (str, ":", 0);
  
  while (tab[i]) {
    prefix = g_strdup_printf ("/gcolorsel/%s/", tab[i]);
    gnome_config_push_prefix (prefix);
    g_free (prefix);

    buf = gnome_config_get_string ("Type");
    type = gtk_type_from_name (buf);
    g_free (buf);

    mdi_color_generic_append_view_type (mcg, type);

    gnome_config_pop_prefix ();

    i++;
  }
  
  g_strfreev (tab);
}

static
GnomeMDIChild *child_create (const gchar *config)
{
  char *prefix;
  char *buf;
  GtkType type;
  GnomeMDIChild *child;

  printf ("Child create : %s\n", config);
  
  prefix = g_strdup_printf ("/gcolorsel/%s/", config);
  gnome_config_push_prefix (prefix);
  g_free (prefix);

  buf = gnome_config_get_string ("Type");
  type = gtk_type_from_name (buf);
  g_free (buf);
  /* FIXME : check for error, type == 0 */

  buf = gnome_config_get_string ("Views");
  gnome_config_pop_prefix ();

  /* Create Child */

  child = GNOME_MDI_CHILD (gtk_type_new (type));
  MDI_COLOR_GENERIC (child)->key = atoi (config);

  next_views (MDI_COLOR_GENERIC (child), buf);
  g_free (buf);

  return child;
}

static void
load_views_config (GnomeMDIChild *child, char *str)
{
  GList *list = child->views;
  ViewColorGeneric *vcg;
  char *prefix;
  char **tab;
  int i = 0;

  tab = g_strsplit (str, ":", 0);

  while (list) {    
    vcg = gtk_object_get_data (GTK_OBJECT (list->data), "view_object");

    vcg->key = atoi (tab[i]);

    /* Load properties */
    prefix = g_strdup_printf ("/gcolorsel/%d/", vcg->key);
    gnome_config_push_prefix (prefix);
    VIEW_COLOR_GENERIC_GET_CLASS (vcg)->load (vcg); 
    gnome_config_pop_prefix ();

    list = g_list_next (list); i++;
  }

  g_strfreev (tab);
}

static MDIColorGeneric *
search_from_key (GnomeMDI *mdi, int key)
{
  GList *list = mdi->children;
  MDIColorGeneric *mcg;

  while (list) {
    mcg = list->data;

    if (mcg->key == key) return mcg;

    list = g_list_next (list);
  }

  return NULL;
}

static void
connect_to_parents (GnomeMDI *mdi, MDIColorGeneric *mcg, char *str)
{
  char **tab;
  int i = 0;
  MDIColorGeneric *parent;

  tab = g_strsplit (str, ":", 0);

  while (tab[i]) {
    parent = search_from_key (mdi, atoi (tab[i]));
    
    mdi_color_generic_connect (parent, mcg);

    i++;
  }

  g_strfreev (tab);
}

static void
load_config (GnomeMDI *mdi)
{
  GList *list = mdi->children;
  MDIColorGeneric *mcg;
  char *prefix;
  char *str;

  while (list) {
    mcg = list->data;
    
    prefix = g_strdup_printf ("/gcolorsel/%d/", mcg->key);
    gnome_config_push_prefix (prefix);
    g_free (prefix);

    /* Load child properties */
    MDI_COLOR_GENERIC_GET_CLASS (mcg)->load (mcg); 

    /* Configure views */
    str = gnome_config_get_string ("Views");    
    load_views_config (GNOME_MDI_CHILD (mcg), str);
    g_free (str);

    /* Connect to parent */
    str = gnome_config_get_string ("Parents");
    if (str) {
      connect_to_parents (mdi, mcg, str);
      g_free (str);
    }

    gnome_config_pop_prefix ();
    
    list = g_list_next (list);
  }
}

gboolean 
session_load (GnomeMDI *mdi)
{
  if (! gnome_config_get_bool ("/gcolorsel/gcolorsel/FirstTime=TRUE")) {
    mdi->tab_pos = gnome_config_get_int ("/gcolorsel/gcolorsel/TabPos");

    gnome_mdi_restore_state (mdi, "/gcolorsel/mdi", child_create);

    load_config (mdi);

    set_config_key_pos (gnome_config_get_int ("/gcolorsel/gcolorsel/Key"));

    return TRUE;
  } 

  return FALSE;
}

void
session_load_data (GnomeMDI *mdi)
{
  GList *list = mdi->children;

  while (list) {
    if (IS_MDI_COLOR_FILE (list->data)) 
      mdi_color_file_load (MDI_COLOR_FILE (list->data));

    list = g_list_next (list);
  }
}

/******************************* Create ***********************************/

void
session_create (GnomeMDI *mdi)
{
  GtkWidget *first;
  MDIColorFile *file;
  MDIColorVirtual *virtual;

  /* Configure MDI */
  mdi->tab_pos = GTK_POS_BOTTOM;
  gnome_mdi_set_mode (mdi, GNOME_MDI_NOTEBOOK);

  /* Create a file document */
  file = mdi_color_file_new ();
  gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (file));
  
  mdi_color_file_set_filename (file, "/usr/X11R6/lib/X11/rgb.txt");
  gnome_mdi_child_set_name (GNOME_MDI_CHILD (file), "System");
  
  /* Add a ColorList view for the file document */
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (file));
  first = gnome_mdi_get_active_view (mdi);

  /* Add a ColorGrid view for the file document */
  mdi_color_generic_append_view_type (MDI_COLOR_GENERIC (file), 
				      TYPE_VIEW_COLOR_GRID);
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (file));
  
  /* Create a search document */
  virtual = mdi_color_virtual_new ();
  gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (virtual));

  /* Configure search */
  mdi_color_virtual_set (virtual, 255, 255, 255, 100);
  
  /* Add a ColorList view for the search document  */  
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (virtual)); 

  /* Add a ColorSearch view for the search document */
  mdi_color_generic_append_view_type (MDI_COLOR_GENERIC (virtual), 
				      TYPE_VIEW_COLOR_GRID);
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (virtual)); 

  /* Connect Search document to file document */
  mdi_color_generic_connect (MDI_COLOR_GENERIC (file),
			     MDI_COLOR_GENERIC (virtual)); 
 
  /* Tell the MDI to display ColorList for the file document */
  gnome_mdi_set_active_view (mdi, first);
}
