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



void
quit_app(GtkWidget *w, gpointer data)
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



void
about_box(GtkWidget *w, gpointer data)
{
        GtkWidget *about;
        gchar *authors[] = {
                "Eckehard Berns\n<eb@berns.prima.de>",
                NULL
        };
        about = gnome_about_new(APP_NAME,
                                VERSION,
                                "Copyright (C) 1997,98 Eckehard Berns",
                                authors,
#ifdef DEBUG
                                __DATE__ ", " __TIME__,
#else
                                NULL,
#endif
                                NULL);
        gtk_widget_show(about);
}




static void
project_name(GtkWidget *w, GtkEntry *gentry)
{
	char *s;
	project *proj;

	if (!(s = gtk_entry_get_text(gentry))) return;
	if (!s[0]) return;
	project_list_add(proj = project_new_title(s));
        clist_add(proj);
}

void
new_project(GtkWidget *widget, gpointer data)
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

	/*
	 * TODO: this is kinda hack for the default action to work. I tried
	 * to do it with just a default button, but that didn't work. So I
	 * do the default action when the entry gets `activated'.
	 */
	gtk_signal_connect(GTK_OBJECT(text), "activate",
			   GTK_SIGNAL_FUNC(project_name), (gpointer *)text);
	gtk_signal_connect_object(GTK_OBJECT(text), "activate",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(dlg));
	
	gtk_widget_show(dlg);
}



static void
init_project_list_2(GtkWidget *widget, int button)
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
                setup_clist();
	}
}



void
init_project_list(GtkWidget *widget, gpointer data)
{
	msgbox_ok_cancel(gettext("Reload Configuration File"),
			 _("This will overwrite your current set of projects.\n"
			   "Do you really want to reload the configuration file?"),
			 gettext("Yes"), gettext("No"),
			 GTK_SIGNAL_FUNC(init_project_list_2));
}



void
save_project_list(GtkWidget *widget, gpointer data)
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

void
cut_project(GtkWidget *w, gpointer data)
{
	if (!cur_proj) return;
	if (cutted_project)
		project_destroy(cutted_project);
	cutted_project = cur_proj;
	prop_dialog_set_project(NULL);
	project_list_remove(cur_proj);
	cur_proj_set(NULL);
        clist_remove(cutted_project);
}



void
paste_project(GtkWidget *w, gpointer data)
{
	int pos;
	project *p;
	
	if (!cutted_project) return;
	p = project_dup(cutted_project);
	if (!cur_proj) {
                clist_add(p);
		project_list_add(p);
		return;
	}
        pos = cur_proj->row;
	project_list_insert(p, pos);
        clist_insert(p, pos);
}



void
copy_project(GtkWidget *w, gpointer data)
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

void
menu_start_timer(GtkWidget *w, gpointer data)
{
	start_timer();
	menu_set_states();
}



void
menu_stop_timer(GtkWidget *w, gpointer data)
{
	stop_timer();
	menu_set_states();
}


void
menu_toggle_timer(GtkWidget *w, gpointer data)
{
	/* if (GTK_CHECK_MENU_ITEM(menus_get_toggle_timer())->active) { */
	if (main_timer == 0) {
		start_timer();
	} else {
		stop_timer();
	}
	menu_set_states();
}


void
menu_options(GtkWidget *w, gpointer data)
{
	options_dialog();
}



void
menu_properties(GtkWidget *w, gpointer data)
{
	if (cur_proj) {
		prop_dialog(cur_proj);
	}
}



void
menu_clear_daily_counter(GtkWidget *w, gpointer data)
{
	g_return_if_fail(cur_proj != NULL);

	cur_proj->day_secs = 0;
        clist_update_label(cur_proj);
}

