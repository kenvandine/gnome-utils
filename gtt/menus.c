/*   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <gtk/gtk.h>

#include <string.h>


#include "gtt-features.h"
#include "menus.h"
#include "menucmd.h"

#undef gettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


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



#define MAX_MENU_ITEMS 28
static void set_menu_item(GtkMenuEntry me[], int i, char *path, char *accel,
			  GtkMenuCallback func, gpointer data)
{
	g_return_if_fail(i < MAX_MENU_ITEMS);
	me[i].path = path;
	me[i].accelerator = accel;
	me[i].callback = func;
	me[i].callback_data = data;
	me[i].widget = NULL;
}

static GtkMenuEntry *build_menu_items(int *ret_num)
{
	static GtkMenuEntry menu_items[MAX_MENU_ITEMS];
	static int i = 0;
	
	if (i) {
		*ret_num = i;
		return menu_items;
	}
	set_menu_item(menu_items, i, _("<Main>/File/New Project..."), _("<control>N"), new_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/File/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/File/Reload Configuration File"), _("<control>R"), init_project_list, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/File/Save Configuration File"), _("<control>S"), save_project_list, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/File/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/File/Exit"), _("<control>Q"), quit_app, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/Cut"), _("<control>X"), cut_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/Copy"), _("<control>C"), copy_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/Paste"), _("<control>V"), paste_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/Clear Daily Counter"), NULL, menu_clear_daily_counter, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/Properties..."), _("<control>E"), menu_properties, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Edit/Preferences..."), NULL, menu_options, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Timer/Start"), _("<control>A"), menu_start_timer, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Timer/Stop"), _("<control>P"), menu_stop_timer, NULL); i++;
	set_menu_item(menu_items, i, _("<Main>/Timer/<check>Timer running"), _("<control>T"), menu_toggle_timer, NULL); i++;
#ifdef USE_GTT_HELP
	set_menu_item(menu_items, i, _("<Main>/Help/Help on Help..."), NULL, menu_help_contents, "help on help"); i++;
	set_menu_item(menu_items, i, _("<Main>/Help/Contents..."), _("<alt>H"), menu_help_contents, "contents"); i++;
	set_menu_item(menu_items, i, _("<Main>/Help/<separator>"), NULL, NULL, NULL); i++;
#endif
	set_menu_item(menu_items, i, _("<Main>/Help/About..."), NULL, about_box, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Properties..."), NULL, menu_properties, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Cut"), NULL, cut_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Copy"), NULL, copy_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Paste"), NULL, paste_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Clear Daily Counter"), NULL, menu_clear_daily_counter, NULL); i++;
#ifdef DEBUG
	if (i < MAX_MENU_ITEMS) {
		g_warning("%d menu items, but MAX_MENU_ITEMS = %d\n", i, MAX_MENU_ITEMS);
	}
#endif
	if (i > MAX_MENU_ITEMS) i = MAX_MENU_ITEMS;
	*ret_num = i;
	return menu_items;
};
static int nmenu_items = 0;

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

	p = gtk_menu_factory_find(factory, _("<Main>/Help"));
	if (p) gtk_menu_item_right_justify(GTK_MENU_ITEM(p->widget));
#ifdef ALLWAYS_SHOW_TOGGLE
	menus_set_show_toggle(_("<Main>/Timer/Timer running"), 1);
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

int menus_get_sensitive_state(char *path)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    {
      if (GTK_IS_WIDGET (menu_path->widget))
	return GTK_WIDGET_SENSITIVE(menu_path->widget);
    }
  else
    g_warning ("Unable to get sensitivity from menu which doesn't exist: %s", path);
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
      GtkMenuEntry *menu_items;
      initialize = FALSE;

      factory = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU_BAR);

      subfactories[0] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU_BAR);
      gtk_menu_factory_add_subfactory (factory, subfactories[0], "<Main>");

      subfactories[1] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU);
      gtk_menu_factory_add_subfactory (factory, subfactories[1], "<Popup>");

      menu_items = build_menu_items(&nmenu_items);
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

