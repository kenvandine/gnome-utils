#include <config.h>
#include <gtk/gtk.h>

#include "menus.h"
#include "menucmd.h"
#include "gtt-features.h"


static void menus_init(void);
static gint
menus_install_accel (GtkWidget *widget,
		     gchar     *signal_name,
		     gchar      key,
		     gchar      modifiers,
		     gchar     *path);
static void
menus_remove_accel (GtkWidget *widget,
		    gchar     *signal_name,
		    gchar     *path);



static GtkMenuEntry menu_items[] =
{
	{"<Main>/File/New Project...", "<control>N", new_project, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
	{"<Main>/File/Reload rc", "<control>R", init_project_list, NULL},
	{"<Main>/File/Save rc", "<control>S", save_project_list, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
	{"<Main>/File/Quit", "<control>Q", quit_app, NULL},
	{"<Main>/Edit/Cut", "<control>X", cut_project, NULL},
	{"<Main>/Edit/Copy", "<control>C", copy_project, NULL},
	{"<Main>/Edit/Paste", "<control>V", paste_project, NULL},
	{"<Main>/Edit/<separator>", NULL, NULL, NULL},
	{"<Main>/Edit/Properties...", "<control>E", menu_properties, NULL},
	{"<Main>/Edit/Preferences...", NULL, menu_options, NULL},
	{"<Main>/Timer/Start", "<control>A", menu_start_timer, NULL},
	{"<Main>/Timer/Stop", "<control>P", menu_stop_timer, NULL},
	{"<Main>/Timer/<check>Timer running", "<control>T", menu_toggle_timer, NULL},
	{"<Main>/Help/About...", NULL, about_box, NULL},
};
static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static int initialize = TRUE;
static GtkMenuFactory *factory = NULL;
static GtkMenuFactory *subfactories[MENU_NUM];
static GHashTable *entry_ht = NULL;


void get_menubar(GtkWidget **menubar,
		 GtkAcceleratorTable **table,
		 int subfact)
{
	GtkMenuPath *p;

	if (initialize)
		menus_init ();

	p = gtk_menu_factory_find(factory, "<Main>/Help");
	if (p) gtk_menu_item_right_justify(GTK_MENU_ITEM(p->widget));
#ifdef ALLWAYS_SHOW_TOGGLE
	menus_set_show_toggle("<Main>/Timer/Timer running", 1);
#endif

	if (menubar)
		*menubar = subfactories[subfact]->widget;
	if (table)
		*table = subfactories[subfact]->table;
}

void
menus_create (GtkMenuEntry *entries,
	      int           nmenu_entries)
{
  char *accelerator;
  int i;

  if (initialize)
    menus_init ();

  if (entry_ht)
    for (i = 0; i < nmenu_entries; i++)
      {
	accelerator = g_hash_table_lookup (entry_ht, entries[i].path);
	if (accelerator)
	  {
	    if (accelerator[0] == '\0')
	      entries[i].accelerator = NULL;
	    else
	      entries[i].accelerator = accelerator;
	  }
      }

  gtk_menu_factory_add_entries (factory, entries, nmenu_entries);

  for (i = 0; i < nmenu_entries; i++)
    if (entries[i].widget && GTK_BIN (entries[i].widget)->child)
      {
	gtk_signal_connect (GTK_OBJECT (entries[i].widget), "install_accelerator",
			    (GtkSignalFunc) menus_install_accel,
			    entries[i].path);
	gtk_signal_connect (GTK_OBJECT (entries[i].widget), "remove_accelerator",
			    (GtkSignalFunc) menus_remove_accel,
			    entries[i].path);
      }
}

void
menus_set_sensitive (char *path,
		     int   sensitive)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    gtk_widget_set_sensitive (menu_path->widget, sensitive);
  else
    g_warning ("Unable to set sensitivity for menu which doesn't exist: %s", path);
}

void
menus_set_state (char *path,
		 int   state)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    {
      if (GTK_IS_CHECK_MENU_ITEM (menu_path->widget))
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (menu_path->widget), state);
    }
  else
    g_warning ("Unable to set state for menu which doesn't exist: %s", path);
}

int menus_get_toggle_state(char *path)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    {
      if (GTK_IS_CHECK_MENU_ITEM (menu_path->widget))
	return GTK_CHECK_MENU_ITEM (menu_path->widget)->active;
    }
  else
    g_warning ("Unable to get state from menu which doesn't exist: %s", path);
  return 0;
}

void
menus_set_show_toggle (char *path,
		       int   state)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    {
      if (GTK_IS_CHECK_MENU_ITEM (menu_path->widget))
	gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_path->widget), state);
    }
  else
    g_warning ("Unable to set state for menu which doesn't exist: %s", path);
}

void
menus_activate (char *path)
{
	GtkMenuPath *menu_path;

	if (initialize)
		menus_init ();

	menu_path = gtk_menu_factory_find (factory, path);
	if (menu_path)
	{
		if (GTK_IS_MENU_ITEM (menu_path->widget))
			gtk_menu_item_activate (GTK_MENU_ITEM (menu_path->widget));
	}
	else
		g_warning ("Unable to activate menu which doesn't exist: %s", path);
}

void
menus_add_path (char *path,
		char *accelerator)
{
  if (!entry_ht)
    entry_ht = g_hash_table_new (g_string_hash, g_string_equal);

  g_hash_table_insert (entry_ht, path, accelerator);
}

void
menus_destroy (char *path)
{
  if (initialize)
    menus_init ();

  gtk_menu_factory_remove_paths (factory, &path, 1);
}

static void
menus_init ()
{
  if (initialize)
    {
      initialize = FALSE;

      factory = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU_BAR);

      subfactories[0] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU_BAR);
      gtk_menu_factory_add_subfactory (factory, subfactories[0], "<Main>");

      menus_create (menu_items, nmenu_items);
    }
}

static gint
menus_install_accel (GtkWidget *widget,
		     gchar     *signal_name,
		     gchar      key,
		     gchar      modifiers,
		     gchar     *path)
{
  char accel[64];
  char *t1, t2[2];
  void strcat(char *, char*);

  accel[0] = '\0';
  if (modifiers & GDK_CONTROL_MASK)
    strcat (accel, "<control>");
  if (modifiers & GDK_SHIFT_MASK)
    strcat (accel, "<shift>");
  if (modifiers & GDK_MOD1_MASK)
    strcat (accel, "<alt>");

  t2[0] = key;
  t2[1] = '\0';
  strcat (accel, t2);

  if (entry_ht)
    {
      t1 = g_hash_table_lookup (entry_ht, path);
      g_free (t1);
    }
  else
    entry_ht = g_hash_table_new (g_string_hash, g_string_equal);

  g_hash_table_insert (entry_ht, path, g_strdup (accel));

  return TRUE;
}

static void
menus_remove_accel (GtkWidget *widget,
		    gchar     *signal_name,
		    gchar     *path)
{
  char *t;

  if (entry_ht)
    {
      t = g_hash_table_lookup (entry_ht, path);
      g_free (t);

      g_hash_table_insert (entry_ht, path, g_strdup (""));
    }
}

