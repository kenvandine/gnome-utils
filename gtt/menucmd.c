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
#include <string.h>


#include "gtt.h"

#undef gettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


#if 0 /* not needed */
void not_implemented(GtkWidget *w, gpointer data)
{
	msgbox_ok(gettext("Notice"),
		  gettext("Not implemented yet"),
		  gettext("Close"), NULL);
}
#endif


void quit_app(GtkWidget *w, gpointer data)
{
	if (!project_list_save(NULL)) {
		msgbox_ok(gettext("Warning"),
			  gettext("I could not write the configuration file!"),
			  gettext("Oops"),
			  GTK_SIGNAL_FUNC(gtk_main_quit));
		return;
	}
	gtk_main_quit();
}



void about_box(GtkWidget *w, gpointer data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	char s[128];

	new_dialog_ok(gettext("About"), &dlg, &vbox,
                      gettext("Close"), NULL, NULL);

	t = gtk_label_new(APP_NAME);
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 8);

#ifdef DEBUG
	sprintf(s, "%s " VERSION "\n" __DATE__ " " __TIME__, gettext("version"));
	t = gtk_label_new(s);
#else
	sprintf(s, "%s " VERSION, gettext("version"));
	t = gtk_label_new(s);
#endif
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

#ifdef STANDALONE
	t = gtk_label_new("Eckehard Berns <eb@berns.prima.de>\n"
			  "http://www.i-s-o.net/~ecki/gtt/");
#else
	t = gtk_label_new("Eckehard Berns <eb@berns.prima.de>\n"
			  "http://www.gnome.org/");
#endif
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
	new_dialog_ok_cancel(gettext("New Project..."), &dlg, &vbox,
			     gettext("OK"),
			     GTK_SIGNAL_FUNC(project_name), (gpointer *)text,
			     gettext("Cancel"), NULL, NULL);
	t = gtk_label_new(gettext("Please enter the name of the new project:"));
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



static void init_project_list_2(GtkWidget *widget, int button)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	if (button != 1) return;
	if (!project_list_load(NULL)) {
		new_dialog_ok(gettext("Warning"), &dlg, &vbox,
			      gettext("Oops"), NULL, NULL);
		t = gtk_label_new(gettext("I could not read the configuration file"));
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
		gtk_widget_show(dlg);
	} else {
		setup_list();
	}
}



void init_project_list(GtkWidget *widget, gpointer data)
{
	msgbox_ok_cancel(gettext("Reload Configuration File"),
			 _("This will overwrite your current set of projects.\n"
			   "Do you really want to reload the configuration file?"),
			 gettext("Yes"), gettext("No"),
			 GTK_SIGNAL_FUNC(init_project_list_2));
}



void save_project_list(GtkWidget *widget, gpointer data)
{
	if (!project_list_save(NULL)) {
		GtkWidget *dlg, *t;
		GtkBox *vbox;
		
		new_dialog_ok(gettext("Warning"), &dlg, &vbox,
			      gettext("Oops"), NULL, NULL);
		t = gtk_label_new(gettext("I could not write the configuration file!"));
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);
		gtk_widget_show(dlg);
		return;
	}
}



project *cutted_project = NULL;

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
	menu_set_states(); /* to enable paste */
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



void menu_clear_daily_counter(GtkWidget *w, gpointer data)
{
	g_return_if_fail(cur_proj != NULL);

	cur_proj->day_secs = 0;
	update_label(cur_proj);
}



#ifdef USE_GTT_HELP
#include "gtthelp.h"
static GtkWidget *help = NULL;

static void help_destroy(GtkWidget *w, gpointer *data)
{
	help = NULL;
}


void help_goto(char *helppos)
{
        char s[1024];

	if (!help) {
		help = gtt_help_new(APP_NAME " - Help", HELP_PATH "/index.html");
		gtk_signal_connect(GTK_OBJECT(help), "destroy",
				   GTK_SIGNAL_FUNC(help_destroy), NULL);
	}
        sprintf(s, HELP_PATH "/%s", helppos);
        gtt_help_goto(GTT_HELP(help), s);
	gtk_widget_show(help);
}



void menu_help_contents(GtkWidget *w, gpointer data)
{
	if (!help) {
		help = gtt_help_new(APP_NAME " - Help", HELP_PATH "/index.html");
		gtk_signal_connect(GTK_OBJECT(help), "destroy",
				   GTK_SIGNAL_FUNC(help_destroy), NULL);
	}
	if (0 == strcmp(data, "help on help")) {
		gtt_help_on_help(GTT_HELP(help));
	} else {
		gtt_help_goto(GTT_HELP(help), HELP_PATH "/index.html");
	}
	gtk_widget_show(help);
}
#endif /* USE_GTT_HELP */



#ifdef GNOME_USE_APP
void menu_create(GtkWidget *gnome_app)
{
	GtkAcceleratorTable *accel;
	GtkWidget *w;

	get_menubar(&w, &accel, MENU_MAIN);
	gtk_widget_show(w);
	gtk_window_add_accelerator_table(GTK_WINDOW(gnome_app), accel);
	gnome_app_set_menus(GNOME_APP(gnome_app), GTK_MENU_BAR(w));
}
#endif /* GNOME_USE_APP */



void menu_set_states(void)
{
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
	toolbar_set_states();
}

