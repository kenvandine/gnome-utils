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

/* XXX: this is our main window, perhaps it is a bit ugly this way and
 * should be passed around in the data fields */
extern GtkWidget *window;

void
quit_app(GtkWidget *w, gpointer data)
{
	if (!project_list_save(NULL)) {
		msgbox_ok(_("Warning"),
			  _("I could not write the configuration file!"),
			  GNOME_STOCK_BUTTON_OK,
			  GTK_SIGNAL_FUNC(gtk_main_quit));
		return;
	}
	gtk_main_quit();
}



void
about_box(GtkWidget *w, gpointer data)
{
        static GtkWidget *about = NULL;
        gchar *authors[] = {
                "Eckehard Berns\n<eb@berns.i-s-o.net>",
                NULL
        };
	if (about != NULL)
	{
		gdk_window_show(about->window);
		gdk_window_raise(about->window);
		return;
	}
        about = gnome_about_new(APP_NAME,
                                VERSION,
                                "Copyright (C) 1997,98 Eckehard Berns",
                                (const gchar **)authors,
#ifdef DEBUG
                                __DATE__ ", " __TIME__,
#else
				_("Time tracking tool for GNOME"),
#endif
                                NULL);
	gtk_signal_connect(GTK_OBJECT(about), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about);
	gnome_dialog_set_parent(GNOME_DIALOG(about), GTK_WINDOW(window));
        gtk_widget_show(about);
}




static void
project_name_desc(GtkWidget *w, GtkEntry **entries)
{
	char *name, *desc;
	project *proj;

	if (!(name = gtk_entry_get_text(entries[0]))) return;
        if (!(desc = gtk_entry_get_text(entries[1]))) return;
	if (!name[0]) return;

	project_list_add(proj = project_new_title_desc(name, desc));
        clist_add(proj);
}

static void
free_data(GtkWidget *dlg, gpointer data)
{
	g_free(data);
}


void
new_project(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg, *t, *title, *d, *desc;
	GtkBox *vbox;
        GtkWidget **entries = g_new0(GtkWidget *, 2);
        GtkWidget *table;

	title = gnome_entry_new("project_title");
        desc = gnome_entry_new("project_description");
        entries[0] = gnome_entry_gtk_entry(GNOME_ENTRY(title));
        entries[1] = gnome_entry_gtk_entry(GNOME_ENTRY(desc));

	new_dialog_ok_cancel(_("New Project..."), &dlg, &vbox,
			     GNOME_STOCK_BUTTON_OK,
			     GTK_SIGNAL_FUNC(project_name_desc),
                             entries,
			     GNOME_STOCK_BUTTON_CANCEL, NULL, NULL);

	t = gtk_label_new(_("Project Title"));
        d = gtk_label_new(_("Description"));

        table = gtk_table_new(2,2, FALSE);
        gtk_table_attach(GTK_TABLE(table), t,     0,1, 0,1,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 2, 1);
        gtk_table_attach(GTK_TABLE(table), title, 1,2, 0,1,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 2, 1);
        gtk_table_attach(GTK_TABLE(table), d,     0,1, 1,2,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 2, 1);
        gtk_table_attach(GTK_TABLE(table), desc,  1,2, 1,2,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 2, 1);

        gtk_box_pack_start(vbox, table, FALSE, FALSE, 2);
	gtk_widget_show(t);
	gtk_widget_show(title);
        gtk_widget_show(d);
        gtk_widget_show(desc);
        gtk_widget_show(table);

	gtk_widget_grab_focus(title);

	gnome_dialog_editable_enters(GNOME_DIALOG(dlg),
				     GTK_EDITABLE(entries[0]));
	gnome_dialog_editable_enters(GNOME_DIALOG(dlg),
				     GTK_EDITABLE(entries[1]));

	gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
			   GTK_SIGNAL_FUNC(free_data),
			   entries);
	
	gtk_widget_show(dlg);
}



static void
init_project_list_2(GtkWidget *widget, int button)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	if (button != 0) return;
	if (!project_list_load(NULL)) {
		new_dialog_ok(_("Warning"), &dlg, &vbox,
			      GNOME_STOCK_BUTTON_OK, NULL, NULL);
		t = gtk_label_new(_("I could not read the configuration file"));
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
	msgbox_ok_cancel(_("Reload Configuration File"),
			 _("This will overwrite your current set of projects.\n"
			   "Do you really want to reload the configuration file?"),
			 GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			 GTK_SIGNAL_FUNC(init_project_list_2));
}



void
save_project_list(GtkWidget *widget, gpointer data)
{
	if (!project_list_save(NULL)) {
		GtkWidget *dlg, *t;
		GtkBox *vbox;
		
		new_dialog_ok(_("Warning"), &dlg, &vbox,
			      GNOME_STOCK_BUTTON_OK, NULL, NULL);
		t = gtk_label_new(_("I could not write the configuration file!"));
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

