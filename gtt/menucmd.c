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
#if HAS_GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include "gtt.h"



#if 0 /* not needed */
void not_implemented(GtkWidget *w, gpointer data)
{
	msgbox_ok("Notice", "Not implemented yet", "Close", NULL);
}
#endif


void quit_app(GtkWidget *w, gpointer data)
{
	if (!project_list_save(NULL)) {
		msgbox_ok("Warning", "Could not write init file!", "Oops",
			  GTK_SIGNAL_FUNC(gtk_main_quit));
		return;
	}
	gtk_main_quit();
}



void about_box(GtkWidget *w, gpointer data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;

	new_dialog_ok(APP_NAME " - About", &dlg, &vbox,
		      "Close", NULL, NULL);

	t = gtk_label_new(APP_NAME);
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 8);

#ifdef DEBUG
	t = gtk_label_new("Version " VERSION "\n" __DATE__ " " __TIME__);
#else
	t = gtk_label_new("Version " VERSION);
#endif
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

	t = gtk_label_new("Eckehard Berns <eb@berns.prima.de>");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

	gtk_widget_show(dlg);
}




static void project_name(GtkWidget *w, GtkEntry *gentry)
{
	char *s;
	project *proj;

	if (!(s = gtk_entry_get_text(gentry))) return;
	if (!s[0]) return;
	project_list_add(proj = project_new_title(s));
	add_item(glist, proj);
}

void new_project(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg, *t, *text;
	GtkBox *vbox;

	text = gtk_entry_new();
	new_dialog_ok_cancel(APP_NAME " - MessageBox", &dlg, &vbox,
			     "OK", GTK_SIGNAL_FUNC(project_name), (gpointer *)text,
			     "Cancel", NULL, NULL);
	t = gtk_label_new("Please enter the name of the new Project:");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

	gtk_widget_show(text);
	gtk_box_pack_end(vbox, text, TRUE, TRUE, 2);
	gtk_widget_grab_focus(text);
	/* TODO: verify me
	gtk_signal_connect(GTK_OBJECT(text), "activate",
			   GTK_SIGNAL_FUNC(project_name), (gpointer *)text);
	gtk_signal_connect_object(GTK_OBJECT(text), "activate",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(dlg));
	 */
	
	gtk_widget_show(dlg);
}



static void init_project_list_2(GtkWidget *widget, gpointer *data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	if (!project_list_load(NULL)) {
		new_dialog_ok("Warning", &dlg, &vbox,
			      "Oops", NULL, NULL);
		t = gtk_label_new("I could not read the init file");
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
		gtk_widget_show(dlg);
	} else {
		setup_list();
	}
}



void init_project_list(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	new_dialog_ok_cancel("Configmation Request", &dlg, &vbox,
			     "OK", GTK_SIGNAL_FUNC(init_project_list_2), NULL,
			     "Cancel", NULL, NULL);
	t = gtk_label_new("This will overwrite your current set of projects.");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
	t = gtk_label_new("Do you really want to reload the init file?");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
	gtk_widget_show(dlg);
}



void save_project_list(GtkWidget *widget, gpointer data)
{
	if (!project_list_save(NULL)) {
		GtkWidget *dlg, *t;
		GtkBox *vbox;
		
		new_dialog_ok("Warning", &dlg, &vbox,
			      "Oops", NULL, NULL);
		t = gtk_label_new("Could not write init file!");
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);
		gtk_widget_show(dlg);
		return;
	}
}



static project *cutted_project = NULL;

void cut_project(GtkWidget *w, gpointer data)
{
	if (!cur_proj) return;
	if (cutted_project)
		project_destroy(cutted_project);
	cutted_project = cur_proj;
	prop_dialog_set_project(NULL);
	project_list_remove(cur_proj);
	cur_proj_set(NULL);
	gtk_container_remove(GTK_CONTAINER(glist), GTK_LIST(glist)->selection->data);
}



void paste_project(GtkWidget *w, gpointer data)
{
	int pos;
	project *p;
	
	if (!cutted_project) return;
	p = project_dup(cutted_project);
	if (!cur_proj) {
		add_item(glist, p);
		project_list_add(p);
		return;
	}
	pos = gtk_list_child_position(GTK_LIST(glist), GTK_LIST(glist)->selection->data);
	project_list_insert(p, pos);
	add_item_at(glist, p, pos);
}



void copy_project(GtkWidget *w, gpointer data)
{
	if (!cur_proj) return;
	if (cutted_project)
		project_destroy(cutted_project);
	cutted_project = project_dup(cur_proj);
}




/*
 * timer related menu functions
 */

void menu_start_timer(GtkWidget *w, gpointer data)
{
	start_timer();
	menu_set_states();
}



void menu_stop_timer(GtkWidget *w, gpointer data)
{
	stop_timer();
	menu_set_states();
}


void menu_toggle_timer(GtkWidget *w, gpointer data)
{
	g_return_if_fail(GTK_IS_CHECK_MENU_ITEM(w));

	if (GTK_CHECK_MENU_ITEM(w)->active) {
		start_timer();
	} else {
		stop_timer();
	}
	menu_set_states();
}


void menu_options(GtkWidget *w, gpointer data)
{
	options_dialog();
}



void menu_properties(GtkWidget *w, gpointer data)
{
	if (cur_proj) {
		prop_dialog(cur_proj);
	}
}



#ifdef GNOME_USE_MENU_INFO

#define SET_ARRAY_SIZE(a, b) static int a = sizeof(b) / sizeof(b[0]);
static char fileaccel[] = "NRSQ";
/*
{
	'N',
	'R',
	'S',
	'Q'
};
SET_ARRAY_SIZE(fileaccel_num, fileaccel);
 */
static GnomeMenuInfo filemenu[] = {
	{GNOME_APP_MENU_ITEM, "New Project...", new_project, NULL},
	{GNOME_APP_MENU_ITEM, "Reload init file", init_project_list, NULL},
	{GNOME_APP_MENU_ITEM, "Save init file", save_project_list, NULL},
	{GNOME_APP_MENU_ITEM, "Quit", quit_app, NULL},
	{GNOME_APP_MENU_ENDOFINFO, NULL, NULL, NULL}
};

static char editaccel[] = {
	'X',
	'C',
	'V',
	'E'
};
SET_ARRAY_SIZE(editaccel_num, editaccel);
#define EDIT_CUT_POS 0
#define EDIT_COPY_POS 1
#define EDIT_PASTE_POS 2
#define EDIT_PROP_POS 3
static GnomeMenuInfo editmenu[] = {
	{GNOME_APP_MENU_ITEM, "Cut", cut_project, NULL},
	{GNOME_APP_MENU_ITEM, "Copy", copy_project, NULL},
	{GNOME_APP_MENU_ITEM, "Paste", paste_project, NULL},
	{GNOME_APP_MENU_ITEM, "Properties...", menu_properties, NULL},
	{GNOME_APP_MENU_ITEM, "Preferences...", menu_options, NULL},
	{GNOME_APP_MENU_ENDOFINFO, NULL, NULL, NULL}
};

static char timeraccel[] = {
	'T',
	'P'
};
SET_ARRAY_SIZE(timeraccel_num, timeraccel);
#define TIMER_START_POS 0
#define TIMER_STOP_POS 1
static GnomeMenuInfo timermenu[] = {
	{GNOME_APP_MENU_ITEM, "Start", menu_start_timer, NULL},
	{GNOME_APP_MENU_ITEM, "Stop", menu_stop_timer, NULL},
	/* {GNOME_APP_MENU_ITEM, "Toggle", menu_toggle_timer, NULL}, */
	{GNOME_APP_MENU_ENDOFINFO, NULL, NULL, NULL}
};

static GnomeMenuInfo helpmenu[] = {
	{GNOME_APP_MENU_ITEM, "About...", about_box, NULL},
	{GNOME_APP_MENU_ENDOFINFO, NULL, NULL, NULL}
};

#define HELP_POS 3
static GnomeMenuInfo mainmenu[] = {
	{GNOME_APP_MENU_SUBMENU, "File", filemenu, NULL},
	{GNOME_APP_MENU_SUBMENU, "Edit", editmenu, NULL},
	{GNOME_APP_MENU_SUBMENU, "Timer", timermenu, NULL},
	{GNOME_APP_MENU_SUBMENU, "Help", helpmenu, NULL},
	{GNOME_APP_MENU_ENDOFINFO, NULL, NULL, NULL}
};
#endif /* GNOME_USE_MENU_INFO */

#ifdef GNOME_USE_APP
void menu_create(GtkWidget *gnome_app)
{
	GtkAcceleratorTable *accel;
#ifdef GNOME_USE_MENU_INFO 
	int i;

	gnome_app_create_menus(GNOME_APP(gnome_app), mainmenu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(mainmenu[HELP_POS].widget));
	accel = gtk_accelerator_table_new();
	for (i = 0; fileaccel[i]; i++) {
		if (fileaccel[i] != ' ')
			gtk_widget_install_accelerator(filemenu[i].widget, accel,
						       "activate",
						       fileaccel[i], GDK_CONTROL_MASK);
	}
	for (i = 0; i < editaccel_num; i++) {
		if (editaccel[i])
			gtk_widget_install_accelerator(editmenu[i].widget, accel,
						       "activate",
						       editaccel[i], GDK_CONTROL_MASK);
	}
	for (i = 0; i < timeraccel_num; i++) {
		if (timeraccel[i])
			gtk_widget_install_accelerator(timermenu[i].widget, accel,
						       "activate",
						       timeraccel[i], GDK_CONTROL_MASK);
	}
	gtk_widget_install_accelerator(helpmenu[0].widget, accel,
				       "activate",
				       'H', GDK_MOD1_MASK);
	gtk_window_add_accelerator_table(GTK_WINDOW(gnome_app), accel);
#else /* GNOME_USE_MENU_INFO */
	GtkWidget *w;

	get_menubar(&w, &accel, MENU_MAIN);
	gtk_widget_show(w);
	gtk_window_add_accelerator_table(GTK_WINDOW(gnome_app), accel);
	gnome_app_set_menus(GNOME_APP(gnome_app), GTK_MENU_BAR(w));
#endif /* GNOME_USE_MENU_INFO */
}
#endif /* GNOME_USE_APP */



void menu_set_states(void)
{
#ifdef GNOME_USE_MENU_INFO
	gtk_widget_set_sensitive(editmenu[EDIT_CUT_POS].widget, (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(editmenu[EDIT_COPY_POS].widget, (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(editmenu[EDIT_PASTE_POS].widget, (cutted_project) ? 1 : 0);
	gtk_widget_set_sensitive(editmenu[EDIT_PROP_POS].widget, (cur_proj) ? 1 : 0);
	gtk_widget_set_sensitive(timermenu[TIMER_START_POS].widget, (main_timer == 0) ? 1 : 0);
	gtk_widget_set_sensitive(timermenu[TIMER_STOP_POS].widget, (main_timer != 0) ? 1 : 0);
#else /* GNOME_USE_MENU_INFO */
	menus_set_state("<Main>/Timer/Timer running", (main_timer == 0) ? 0 : 1);
	menus_set_sensitive("<Main>/Timer/Start", (main_timer == 0) ? 1 : 0);
	menus_set_sensitive("<Main>/Timer/Stop", (main_timer == 0) ? 0 : 1);
	menus_set_sensitive("<Main>/Edit/Cut", (cur_proj) ? 1 : 0);
	menus_set_sensitive("<Main>/Edit/Copy", (cur_proj) ? 1 : 0);
	menus_set_sensitive("<Main>/Edit/Paste", (cutted_project) ? 1 : 0);
	menus_set_sensitive("<Main>/Edit/Properties...", (cur_proj) ? 1 : 0);
#endif /* GNOME_USE_MENU_INFO */
	toolbar_set_states();
}

