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
#include <libgnome/gnome-help.h>
#include <string.h>

#include "gtt.h"

/* This file is a little ugly; it originally didn't use a GnomePropertyBox,
   and now it does, but not in a very natural way. */


typedef struct _PropDlg {
	GnomePropertyBox *dlg;
	/* GtkEntry *days; */
	struct {
		GtkEntry *h, *m, *s;
	} ever, day;
	GtkEntry *title, *desc;
	project *proj;
} PropDlg;


static void help_cb(GnomePropertyBox * pb, gint page, gchar * url)
{
  gnome_help_goto(pb, url);
}

static void prop_set(GnomePropertyBox * pb, gint page, PropDlg *dlg)
{
	gchar *s;
	int secs;

	if (!dlg->proj) return;
	if (page != -1) return; /* Only do global applies */

	s = gtk_entry_get_text(dlg->title);
	if (!s) g_warning("%s:%d\n", __FILE__, __LINE__);
	if (0 != strcmp(dlg->proj->title, s)) {
		if (s[0]) {
			project_set_title(dlg->proj, s);
		} else {
			project_set_title(dlg->proj, _("empty"));
			gtk_entry_set_text(dlg->title, dlg->proj->title);
		}
	}

	s = gtk_entry_get_text(dlg->desc);
	if (!s) {
		if (dlg->proj->desc)
			project_set_desc(dlg->proj, NULL);
	} else if (!s[0]) {
		if (dlg->proj->desc)
			project_set_desc(dlg->proj, NULL);
	} else if (NULL == dlg->proj->desc) {
		project_set_desc(dlg->proj, s);
	} else if (0 != strcmp(dlg->proj->desc, s)) {
		if (s[0])
			project_set_desc(dlg->proj, s);
		else
			project_set_desc(dlg->proj, NULL);
	}

	s = gtk_entry_get_text(dlg->day.h);
	secs = atoi(s) * 3600;
	s = gtk_entry_get_text(dlg->day.m);
	secs += atoi(s) * 60;
	s = gtk_entry_get_text(dlg->day.s);
	secs += atoi(s);
	if (secs != dlg->proj->day_secs) {
		dlg->proj->day_secs = secs;
                clist_update_label(dlg->proj);
	}

	s = gtk_entry_get_text(dlg->ever.h);
	secs = atoi(s) * 3600;
	s = gtk_entry_get_text(dlg->ever.m);
	secs += atoi(s) * 60;
	s = gtk_entry_get_text(dlg->ever.s);
	secs += atoi(s);
	if (secs != dlg->proj->secs) {
		dlg->proj->secs = secs;
	}

}



static PropDlg *dlg = NULL;

void prop_dialog_set_project(project *proj)
{
	char s[128];

	if (!dlg) return;

	if (!proj) {
		dlg->proj = NULL;
		gtk_entry_set_text(dlg->title, "");
		gtk_entry_set_text(dlg->desc, "");
		gtk_entry_set_text(dlg->day.h, "");
		gtk_entry_set_text(dlg->day.m, "");
		gtk_entry_set_text(dlg->day.s, "");
		gtk_entry_set_text(dlg->ever.h, "");
		gtk_entry_set_text(dlg->ever.m, "");
		gtk_entry_set_text(dlg->ever.s, "");
		return;
	}
	dlg->proj = proj;
	if (proj->title)
		gtk_entry_set_text(dlg->title, proj->title);
	else
		gtk_entry_set_text(dlg->title, "");
	if (proj->desc)
		gtk_entry_set_text(dlg->desc, proj->desc);
	else
		gtk_entry_set_text(dlg->desc, "");
	sprintf(s, "%d", proj->day_secs / 3600);
	gtk_entry_set_text(dlg->day.h, s);
	sprintf(s, "%d", (proj->day_secs % 3600) / 60);
	gtk_entry_set_text(dlg->day.m, s);
	sprintf(s, "%d", proj->day_secs % 60);
	gtk_entry_set_text(dlg->day.s, s);
	sprintf(s, "%d", proj->secs / 3600);
	gtk_entry_set_text(dlg->ever.h, s);
	sprintf(s, "%d", (proj->secs % 3600) / 60);
	gtk_entry_set_text(dlg->ever.m, s);
	sprintf(s, "%d", proj->secs % 60);
	gtk_entry_set_text(dlg->ever.s, s);

	/* The problem here is that all the set_text's above are 
	   calling gnome_property_box_changed, and sensitizing the 
	   OK/Apply buttons. This is bad. So we just desensitize them.
	   This is lame as the PropertyBox internally thinks an apply
	   is pending, and you call the changed function all those 
	   times.

	   The real solution probably involves changing GnomePropertyBox. */

	gnome_dialog_set_sensitive(GNOME_DIALOG(dlg->dlg), 0, FALSE);
	gnome_dialog_set_sensitive(GNOME_DIALOG(dlg->dlg), 1, FALSE);
}



void prop_dialog(project *proj)
{
	GtkWidget *w;
	GtkBox *vbox;
	GtkTable *table;
	char *s1, *t;

	if (!proj) return;
	if (!dlg) {
		char s[64];
		dlg = g_malloc(sizeof(PropDlg));
		dlg->dlg = GNOME_PROPERTY_BOX(gnome_property_box_new());
		sprintf(s, APP_NAME " - %s", _("Properties"));
		gtk_window_set_title(GTK_WINDOW(dlg->dlg), s);

		vbox = GTK_BOX(gtk_vbox_new(FALSE, 2));
		gtk_widget_show(GTK_WIDGET(vbox));
		gnome_property_box_append_page(dlg->dlg, GTK_WIDGET(vbox),
					       gtk_label_new(_("Project")));

		gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD);

		gtk_signal_connect(GTK_OBJECT(dlg->dlg), "apply",
				   GTK_SIGNAL_FUNC(prop_set), dlg);

		t = gnome_help_file_path("gtt", "index.html");
		s1 = g_copy_strings("file:///", t, "#PROP", NULL);
		g_free(t);
		gtk_signal_connect(GTK_OBJECT(dlg->dlg), "help",
				   GTK_SIGNAL_FUNC(help_cb), s1);

		table = GTK_TABLE(gtk_table_new(4, 7, FALSE));
		gtk_widget_show(GTK_WIDGET(table));
		gtk_box_pack_start(vbox, GTK_WIDGET(table), FALSE, FALSE, 2);
		gtk_table_set_col_spacing(table, 0, 5);
		gtk_table_set_col_spacing(table, 2, 5);
		gtk_table_set_col_spacing(table, 4, 5);

		w = gtk_label_new(_("Project Title:"));
		gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 0, 1, 0, 1);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 1, 7, 0, 1);
		dlg->title = GTK_ENTRY(w);

		w = gtk_label_new(_("Project Description:"));
		gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 0, 1, 1, 2);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 1, 7, 1, 2);
		dlg->desc = GTK_ENTRY(w);

		w = gtk_label_new(_("Project Time today:"));
		gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 0, 1, 2, 3);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		/*
		 * Does anybody know a better way to make entries smaller?
		 * I hate hard coded pixel widths/heights
		 */
		gtk_widget_set_usize(w, 30, -1);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 1, 2, 2, 3);
		dlg->day.h = GTK_ENTRY(w);
		w = gtk_label_new(_("hours"));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 2, 3, 2, 3);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_set_usize(w, 30, -1);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 3, 4, 2, 3);
		dlg->day.m = GTK_ENTRY(w);
		w = gtk_label_new(_("mins"));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 4, 5, 2, 3);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_set_usize(w, 30, -1);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 5, 6, 2, 3);
		dlg->day.s = GTK_ENTRY(w);
		w = gtk_label_new(_("secs"));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 6, 7, 2, 3);

		w = gtk_label_new(_("Project Time ever:"));
		gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 0, 1, 3, 4);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_set_usize(w, 30, -1);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 1, 2, 3, 4);
		dlg->ever.h = GTK_ENTRY(w);
		w = gtk_label_new(_("hours"));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 2, 3, 3, 4);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_set_usize(w, 30, -1);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 3, 4, 3, 4);
		dlg->ever.m = GTK_ENTRY(w);
		w = gtk_label_new(_("mins"));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 4, 5, 3, 4);
		w = gtk_entry_new();
		gtk_signal_connect_object(GTK_OBJECT(w), "changed",
					  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
					  GTK_OBJECT(dlg->dlg));
		gtk_widget_set_usize(w, 30, -1);
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 5, 6, 3, 4);
		dlg->ever.s = GTK_ENTRY(w);
		w = gtk_label_new(_("secs"));
		gtk_widget_show(w);
		gtk_table_attach_defaults(table, w, 6, 7, 3, 4);

		gnome_dialog_close_hides(GNOME_DIALOG(dlg->dlg), TRUE);
	}
	prop_dialog_set_project(proj);
	gtk_widget_show(GTK_WIDGET(dlg->dlg));
}


