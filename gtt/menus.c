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

#include "cur-proj.h"
#include "gtt.h"
#include "journal.h"
#include "menucmd.h"
#include "toolbar.h"


static GnomeUIInfo menu_main_file[] = {
	GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Project..."), NULL,
				  new_project, NULL),
	GNOMEUIINFO_SEPARATOR,
	{GNOME_APP_UI_ITEM, N_("_Reload Configuration File"), NULL,
		init_project_list, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
		'L', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ITEM, N_("_Save Configuration File"), NULL,
		save_project_list, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
		'S', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ITEM, N_("_Export Current State"), NULL,
		export_current_state, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
		'E', GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM(quit_app,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo menu_main_settings[] = {
	GNOMEUIINFO_MENU_PREFERENCES_ITEM(menu_options,NULL),
	GNOMEUIINFO_END
};



static GnomeUIInfo menu_main_edit[] = {
#define MENU_EDIT_CUT_POS 0
	GNOMEUIINFO_MENU_CUT_ITEM(cut_project,NULL),
#define MENU_EDIT_COPY_POS 1
	GNOMEUIINFO_MENU_COPY_ITEM(copy_project,NULL),
#define MENU_EDIT_PASTE_POS 2
	GNOMEUIINFO_MENU_PASTE_ITEM(paste_project,NULL),
	GNOMEUIINFO_SEPARATOR,
#define MENU_EDIT_CDC_POS 4
	GNOMEUIINFO_ITEM_STOCK(N_("Clear _Daily Counter"), NULL,
			       menu_clear_daily_counter,
			       GNOME_STOCK_MENU_BLANK),
	GNOMEUIINFO_SEPARATOR,
#define MENU_EDIT_JNL_POS 6
	GNOMEUIINFO_ITEM_STOCK(N_("_Journal..."), NULL,
			       edit_journal,
			       GNOME_STOCK_MENU_BLANK),
	GNOMEUIINFO_SEPARATOR,
#define MENU_EDIT_PROP_POS 8
	GNOMEUIINFO_MENU_PROPERTIES_ITEM(menu_properties,NULL),
	GNOMEUIINFO_END
};


static GnomeUIInfo menu_main_timer[] = {
#define MENU_TIMER_START_POS 0
	{GNOME_APP_UI_ITEM, N_("St_art"), NULL, menu_start_timer, NULL,
		NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TIMER,
		'A', GDK_CONTROL_MASK, NULL},
#define MENU_TIMER_STOP_POS 1
	{GNOME_APP_UI_ITEM, N_("Sto_p"), NULL,
		menu_stop_timer, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TIMER_STOP,
		'Z', GDK_CONTROL_MASK, NULL},
#define MENU_TIMER_TOGGLE_POS 2
	{GNOME_APP_UI_TOGGLEITEM, N_("_Timer Running"), NULL,
		menu_toggle_timer, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		'T', GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_END
};


static GnomeUIInfo menu_main_help[] = {
	GNOMEUIINFO_HELP("gtt"),
	GNOMEUIINFO_MENU_ABOUT_ITEM(about_box,NULL),
	GNOMEUIINFO_END
};


static GnomeUIInfo menu_main[] = {
	GNOMEUIINFO_MENU_FILE_TREE(menu_main_file),
	GNOMEUIINFO_MENU_EDIT_TREE(menu_main_edit),
	GNOMEUIINFO_MENU_SETTINGS_TREE(menu_main_settings),
	GNOMEUIINFO_SUBTREE(N_("_Timer"), menu_main_timer),
	GNOMEUIINFO_MENU_HELP_TREE(menu_main_help),
	GNOMEUIINFO_END
};



static GnomeUIInfo menu_popup[] = {
#define MENU_POPUP_CUT_POS 0
	GNOMEUIINFO_MENU_CUT_ITEM(cut_project,NULL),
#define MENU_POPUP_COPY_POS 1
	GNOMEUIINFO_MENU_COPY_ITEM(copy_project,NULL),
#define MENU_POPUP_PASTE_POS 2
	GNOMEUIINFO_MENU_PASTE_ITEM(paste_project,NULL),
	GNOMEUIINFO_SEPARATOR,
#define MENU_POPUP_CDC_POS 4
	GNOMEUIINFO_ITEM_STOCK(N_("Clear _Daily Counter"), NULL,
			       menu_clear_daily_counter,
			       GNOME_STOCK_MENU_BLANK),
#define MENU_EDIT_JNL_POS 6
	GNOMEUIINFO_ITEM_STOCK(N_("_Journal..."), NULL,
			       edit_journal,
			       GNOME_STOCK_MENU_BLANK),
	GNOMEUIINFO_SEPARATOR,
#define MENU_POPUP_PROP_POS 8
	GNOMEUIINFO_MENU_PROPERTIES_ITEM(menu_properties,NULL),
	GNOMEUIINFO_END
};




GtkWidget *
menus_get_popup(void)
{
	static GtkWidget *menu = NULL;

	if (menu) return menu;

	menu = gnome_popup_menu_new(menu_popup);
	return menu;
}



void
menus_create(GnomeApp *app)
{
	menus_get_popup(); /* initialize it */
	gnome_app_create_menus(app, menu_main);
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
	gtk_widget_set_sensitive(menu_main_timer[MENU_TIMER_TOGGLE_POS].widget,
				 1);
	mi = GTK_CHECK_MENU_ITEM(menu_main_timer[MENU_TIMER_TOGGLE_POS].widget);
	mi->active = timer_is_running();
	gtk_widget_set_sensitive(menu_main_timer[MENU_TIMER_TOGGLE_POS].widget,
				 (cur_proj != NULL));
	gtk_widget_set_sensitive(menu_main_timer[MENU_TIMER_START_POS].widget,
				 (FALSE == timer_is_running()) && (cur_proj));
	gtk_widget_set_sensitive(menu_main_timer[MENU_TIMER_STOP_POS].widget,
				 (timer_is_running()) && (cur_proj));
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_CUT_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_COPY_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_PASTE_POS].widget,
				 (cutted_project) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_CDC_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_JNL_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_main_edit[MENU_EDIT_PROP_POS].widget,
				 (cur_proj) ? 1 : 0);

	if (!menu_popup[MENU_POPUP_CUT_POS].widget) return;
	gtk_widget_set_sensitive(menu_popup[MENU_POPUP_CUT_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_popup[MENU_POPUP_COPY_POS].widget,
				 (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(menu_popup[MENU_POPUP_PASTE_POS].widget,
				 (cutted_project) ? 1 : 0);

	toolbar_set_states();
}


