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
#include <gnome.h>
#include <string.h>

#include "gtt.h"


static GnomeUIInfo menu_main_file[] = {
	{GNOME_APP_UI_ITEM, N_("New Project..."), NULL, new_project, NULL,
		NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
		'N', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Reload Configuration File"), NULL,
		init_project_list, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
		'R', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ITEM, N_("Save Configuration File"), NULL,
		save_project_list, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
		'S', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Exit"), NULL,
		quit_app, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT,
		'Q', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ENDOFINFO}
};


static GnomeUIInfo menu_main_edit[] = {
#define MENU_EDIT_CUT_POS 0
	{GNOME_APP_UI_ITEM, N_("Cut"), NULL, cut_project, NULL,
		NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
		'X', GDK_CONTROL_MASK, NULL},
#define MENU_EDIT_COPY_POS 1
	{GNOME_APP_UI_ITEM, N_("Copy"), NULL,
		copy_project, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
		'C', GDK_CONTROL_MASK, NULL},
#define MENU_EDIT_PASTE_POS 2
	{GNOME_APP_UI_ITEM, N_("Paste"), NULL,
		paste_project, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
		'V', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_SEPARATOR},
#define MENU_EDIT_CDC_POS 4
	{GNOME_APP_UI_ITEM, N_("Clear Daily Counter"), NULL,
		menu_clear_daily_counter, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
		0, 0, NULL},
#define MENU_EDIT_PROP_POS 5
	{GNOME_APP_UI_ITEM, N_("Properties..."), NULL,
		menu_properties, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
		'E', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Preferences..."), NULL,
		menu_options, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP,
		0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO}
};


static GnomeUIInfo menu_main_timer[] = {
#define MENU_TIMER_START_POS 0
	{GNOME_APP_UI_ITEM, N_("Start"), NULL, menu_start_timer, NULL,
		NULL, GNOME_APP_PIXMAP_NONE, NULL,
		'A', GDK_CONTROL_MASK, NULL},
#define MENU_TIMER_STOP_POS 1
	{GNOME_APP_UI_ITEM, N_("Stop"), NULL,
		menu_stop_timer, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		'P', GDK_CONTROL_MASK, NULL},
#define MENU_TIMER_TOGGLE_POS 2
	{GNOME_APP_UI_TOGGLEITEM, N_("Timer Running"), NULL,
		menu_toggle_timer, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		'T', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ENDOFINFO}
};


static GnomeUIInfo menu_main_help[] = {
	{GNOME_APP_UI_ITEM, N_("About..."), NULL, about_box, NULL,
		NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
		0, 0, NULL},
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_HELP, NULL, NULL, "gtt"},
	{GNOME_APP_UI_ENDOFINFO}
};


static GnomeUIInfo menu_main[] = {
	{GNOME_APP_UI_SUBTREE, N_("File"), NULL, menu_main_file, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SUBTREE, N_("Edit"), NULL, menu_main_edit, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SUBTREE, N_("Timer"), NULL, menu_main_timer, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
#define MENU_HELP_POS 3
	{GNOME_APP_UI_SUBTREE, N_("Help"), NULL, menu_main_help, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO}
};



static GnomeUIInfo menu_popup[] = {
	{GNOME_APP_UI_ITEM, N_("Properties..."), NULL,
		menu_properties, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
		0, 0, NULL},
	{GNOME_APP_UI_SEPARATOR},
#define MENU_POPUP_CUT_POS 2
	{GNOME_APP_UI_ITEM, N_("Cut"), NULL,
		cut_project, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
		0, 0, NULL},
#define MENU_POPUP_COPY_POS 3
	{GNOME_APP_UI_ITEM, N_("Copy"), NULL,
		copy_project, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
		0, 0, NULL},
#define MENU_POPUP_PASTE_POS 4
	{GNOME_APP_UI_ITEM, N_("Paste"), NULL,
		paste_project, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
		0, 0, NULL},
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Clear Daily Counter"), NULL,
		menu_clear_daily_counter, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
		0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO}
};




GtkWidget *
menus_get_popup(void)
{
	static GtkWidget *menu = NULL;
	GtkWidget *w;
	GnomeUIInfo *p;

	if (menu) return menu;
	menu = gtk_menu_new();
	for (p = menu_popup; p->type != GNOME_APP_UI_ENDOFINFO; p++) {
		if (p->type == GNOME_APP_UI_SEPARATOR) {
			w = gtk_menu_item_new();
			gtk_widget_show(w);
			gtk_menu_append(GTK_MENU(menu), w);
		} else {
			w = gtk_menu_item_new_with_label(p->label);
			p->widget = w;
			gtk_widget_show(w);
			if (p->moreinfo)
				gtk_signal_connect(GTK_OBJECT(w), "activate",
						   (GtkSignalFunc)p->moreinfo,
						   NULL);
			gtk_menu_append(GTK_MENU(menu), w);
		}
	}
	return menu;
}



void
menus_create(GnomeApp *app)
{
	menus_get_popup(); /* initialize it */
	gnome_app_create_menus(app, menu_main);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_main[MENU_HELP_POS].widget));
}



GtkCheckMenuItem *
menus_get_toggle_timer(void)
{
	return GTK_CHECK_MENU_ITEM(menu_main_timer[MENU_TIMER_TOGGLE_POS].widget);
}



void
menu_set_states(void)
{
	GtkCheckMenuItem *mi;

	if (!menu_main_timer[MENU_TIMER_START_POS].widget) return;
	mi = GTK_CHECK_MENU_ITEM(menu_main_timer[MENU_TIMER_TOGGLE_POS].widget);
	mi->active = (main_timer != 0);
	gtk_widget_set_sensitive(menu_main_timer[MENU_TIMER_START_POS].widget,
				 (main_timer == 0));
	gtk_widget_set_sensitive(menu_main_timer[MENU_TIMER_STOP_POS].widget,
				 (main_timer != 0));
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_CUT_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_COPY_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_PASTE_POS].widget,
				 (cutted_project) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_CDC_POS].widget,
				 (cur_proj) ? 1 : 0);

	if (!menu_popup[MENU_POPUP_CUT_POS].widget) return;
	gtk_widget_set_sensitive(menu_popup[MENU_POPUP_CUT_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_popup[MENU_POPUP_COPY_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_popup[MENU_POPUP_PASTE_POS].widget,
				 (cutted_project) ? 1 : 0);

	toolbar_set_states();

#if 0
	menus_set_state(_("<Main>/Timer/Timer running"), (main_timer == 0) ? 0 : 1);
	menus_set_sensitive(_("<Main>/Timer/Start"), (main_timer == 0) ? 1 : 0);
	menus_set_sensitive(_("<Main>/Timer/Stop"), (main_timer == 0) ? 0 : 1);
	menus_set_sensitive(_("<Main>/Edit/Cut"), (cur_proj) ? 1 : 0);
	menus_set_sensitive(_("<Main>/Edit/Copy"), (cur_proj) ? 1 : 0);
	menus_set_sensitive(_("<Main>/Edit/Paste"), (cutted_project) ? 1 : 0);
	menus_set_sensitive(_("<Popup>/Paste"), (cutted_project) ? 1 : 0);
	menus_set_sensitive(_("<Main>/Edit/Clear Daily Counter"), (cur_proj) ? 1 : 0);
	menus_set_sensitive(_("<Popup>/Clear Daily Counter"), (cur_proj) ? 1 : 0);
	menus_set_sensitive(_("<Main>/Edit/Properties..."), (cur_proj) ? 1 : 0);
#endif
}







#if 0
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
#ifdef DEBUG
	set_menu_item(menu_items, i, "<Main>/Test/Test", NULL, menu_test, NULL); i++;
#endif /* DEBUG */
	set_menu_item(menu_items, i, _("<Main>/Help/About..."), NULL, about_box, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Properties..."), NULL, menu_properties, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Cut"), NULL, cut_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Copy"), NULL, copy_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Paste"), NULL, paste_project, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/<separator>"), NULL, NULL, NULL); i++;
	set_menu_item(menu_items, i, _("<Popup>/Clear Daily Counter"), NULL, menu_clear_daily_counter, NULL); i++;



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
		for (i = 0; i < nmenu_entries; i++) {
			accelerator = g_hash_table_lookup (entry_ht, entries[i].path);
			if (accelerator) {
				if (accelerator[0] == '\0')
					entries[i].accelerator = NULL;
				else
					entries[i].accelerator = accelerator;
			}
		}

	gtk_menu_factory_add_entries (factory, entries, nmenu_entries);

	for (i = 0; i < nmenu_entries; i++)
		if (entries[i].widget && GTK_BIN (entries[i].widget)->child) {
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
    entry_ht = g_hash_table_new (g_str_hash, g_str_equal);

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
    entry_ht = g_hash_table_new (g_str_hash, g_str_equal);

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

#endif
