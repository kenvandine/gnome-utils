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

#include "gtt.h"


#undef gettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


typedef struct _OptionsDlg {
	GnomePropertyBox *dlg;
	GtkCheckButton *show_secs;
	GtkCheckButton *show_status_bar;
	GtkCheckButton *show_clist_titles;
	GtkCheckButton *show_tb_icons, *show_tb_texts;
	GtkCheckButton *show_tb_tips;
        GtkCheckButton *show_tb_new, *show_tb_ccp, *show_tb_pref;
        GtkCheckButton *show_tb_timer, *show_tb_prop, *show_tb_file;
        GtkCheckButton *show_tb_help, *show_tb_exit;

	GtkEntry *command;
	GtkEntry *command_null;
	GnomeFileEntry *logfilename;
	GtkWidget *logfilename_l;
	GtkCheckButton *logfileuse;
	GtkEntry *logfileminsecs;
	GtkWidget *logfileminsecs_l;
} OptionsDlg;



#define ENTRY_TO_CHAR(a, b) { char *s = gtk_entry_get_text(a); if (s[0]) { if (b) g_free(b); b = g_strdup(s); } else { if (b) g_free(b); b = NULL; } }

static void options_apply_cb(GnomePropertyBox *pb, gint page, OptionsDlg *odlg)
{
	int state, change;

	if ( page != -1 ) return; /* Only do something on global apply */

	/* display options */
	state = GTK_TOGGLE_BUTTON(odlg->show_secs)->active;
	if (state != config_show_secs) {
		config_show_secs = state;
                setup_clist();
	}
	if (GTK_TOGGLE_BUTTON(odlg->show_status_bar)->active) {
		gtk_widget_show(GTK_WIDGET(status_bar));
                config_show_statusbar = 1;
	} else {
		gtk_widget_hide(GTK_WIDGET(status_bar));
                config_show_statusbar = 0;
	}
        if (GTK_TOGGLE_BUTTON(odlg->show_clist_titles)->active) {
                gtk_clist_column_titles_show(GTK_CLIST(glist));
                config_show_clist_titles = 1;
        } else {
                gtk_clist_column_titles_hide(GTK_CLIST(glist));
                config_show_clist_titles = 0;
        }

	/* shell command options */
	ENTRY_TO_CHAR(odlg->command, config_command);
	ENTRY_TO_CHAR(odlg->command_null, config_command_null);

	/* log file options */
	config_logfile_use = GTK_TOGGLE_BUTTON(odlg->logfileuse)->active;
	ENTRY_TO_CHAR(GTK_ENTRY(gnome_file_entry_gtk_entry(odlg->logfilename)),
		      config_logfile_name);
	config_logfile_min_secs = atoi(gtk_entry_get_text(odlg->logfileminsecs));

        /* toolbar */
	config_show_tb_icons = GTK_TOGGLE_BUTTON(odlg->show_tb_icons)->active;
	config_show_tb_texts = GTK_TOGGLE_BUTTON(odlg->show_tb_texts)->active;
        config_show_tb_tips = GTK_TOGGLE_BUTTON(odlg->show_tb_tips)->active;

        /* toolbar sections */
        change = 0;
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_new)->active;
        if (config_show_tb_new != state) {
                change = 1;
                config_show_tb_new = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_file)->active;
        if (config_show_tb_file != state) {
                change = 1;
                config_show_tb_file = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_ccp)->active;
        if (config_show_tb_ccp != state) {
                change = 1;
                config_show_tb_ccp = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_prop)->active;
        if (config_show_tb_prop != state) {
                change = 1;
                config_show_tb_prop = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_timer)->active;
        if (config_show_tb_timer != state) {
                change = 1;
                config_show_tb_timer = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_pref)->active;
        if (config_show_tb_pref != state) {
                change = 1;
                config_show_tb_pref = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_help)->active;
        if (config_show_tb_help != state) {
                change = 1;
                config_show_tb_help = state;
        }
        state = GTK_TOGGLE_BUTTON(odlg->show_tb_exit)->active;
        if (config_show_tb_exit != state) {
                change = 1;
                config_show_tb_exit = state;
        }
        if (change) {
                update_toolbar_sections();
        }

	toolbar_set_states();
}

/* These two are somewhat lamely cut-and-pasted from prop.c, but they're 
   so short... It might be nice to add to PropertyBox to avoid
   the need (?). 
   gnome_property_box_set_help(GnomePropertyBox * pb, 
                               gchar * app, gchar * filename);
   */
static void help_cb(GnomePropertyBox * pb, gint page, gchar * url)
{
  gnome_help_goto(pb, url);
}

static void signals(OptionsDlg *odlg)
{
	char *s, *t;

	t = gnome_help_file_path("gtt", "index.html");
	s = g_copy_strings("file:///", t, "#PREF", NULL);
	g_free(t);
	gtk_signal_connect(GTK_OBJECT(odlg->dlg), "help",
			   GTK_SIGNAL_FUNC(help_cb), s);
	
	gtk_signal_connect(GTK_OBJECT(odlg->dlg), "apply",
			   GTK_SIGNAL_FUNC(options_apply_cb), 
			   odlg);
}

/* should have done this in prop.c */
static void toggle_changes_property_box(OptionsDlg * odlg, GtkWidget * tb)
{
  gtk_signal_connect_object(GTK_OBJECT(tb), "toggled", 
			    GTK_SIGNAL_FUNC(gnome_property_box_changed),
			    GTK_OBJECT(odlg->dlg));
}

static void entry_changes_property_box(OptionsDlg * odlg, GtkWidget * e)
{
  gtk_signal_connect_object(GTK_OBJECT(e), "changed", 
			    GTK_SIGNAL_FUNC(gnome_property_box_changed),
			    GTK_OBJECT(odlg->dlg));
}

static void display_options(OptionsDlg *odlg, GtkBox *vbox)
{
	GtkWidget *w, *frame;
	GtkWidget *vb;

	frame = gtk_frame_new(_("Display"));
	gtk_widget_show(frame);
	gtk_box_pack_start(vbox, frame, FALSE, FALSE, 2);

	vb = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vb);
	gtk_container_add(GTK_CONTAINER(frame), vb);

	w = gtk_check_button_new_with_label(_("Show Seconds"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_secs = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show Status Bar"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_status_bar = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show Table Header"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_clist_titles = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);
}



static void toolbar_options(OptionsDlg *odlg, GtkBox *vbox)
{
	GtkWidget *w, *frame;
	GtkWidget *vb;

	frame = gtk_frame_new(_("Toolbar"));
	gtk_widget_show(frame);
	gtk_box_pack_start(vbox, frame, FALSE, FALSE, 2);

	vb = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vb);
	gtk_container_add(GTK_CONTAINER(frame), vb);

	w = gtk_check_button_new_with_label(_("Show Toolbar Icons"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_icons = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show Toolbar Texts"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_texts = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show Tooltips"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_tips = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	frame = gtk_frame_new(_("Toolbar Segments"));
	gtk_widget_show(frame);
	gtk_box_pack_start(vbox, frame, FALSE, FALSE, 2);

	vb = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vb);
	gtk_container_add(GTK_CONTAINER(frame), vb);

	w = gtk_check_button_new_with_label(_("Show `New'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_new = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Save', `Reload'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_file = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Cut', `Copy', `Paste'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_ccp = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Properties'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_prop = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Timer'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_timer = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Preferences'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_pref = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Help'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_help = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	w = gtk_check_button_new_with_label(_("Show `Quit'"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_exit = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);
}



static void command_options(OptionsDlg *odlg, GtkBox *vbox)
{
	GtkWidget *w, *frame;
	GtkTable *table;

	frame = gtk_frame_new(_("Shell Commands"));
	gtk_widget_show(frame);
	gtk_box_pack_start(vbox, frame, FALSE, FALSE, 2);

	table = GTK_TABLE(gtk_table_new(2,2, FALSE));
	gtk_widget_show(GTK_WIDGET(table));
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(table));

	w = gtk_label_new(_("Switch Project Command:"));
	gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 0, 1);
	gtk_table_set_col_spacing(table, 0, 5);
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 0, 1);
	odlg->command = GTK_ENTRY(w);
	entry_changes_property_box(odlg, w);

	w = gtk_label_new(_("No Project Command:"));
	gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 1, 2);
	gtk_table_set_col_spacing(table, 0, 5);
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 1, 2);
	odlg->command_null = GTK_ENTRY(w);
	entry_changes_property_box(odlg, w);
}



static void logfile_sigfunc(GtkWidget *w, OptionsDlg *odlg)
{
	int state;
	
	state = GTK_TOGGLE_BUTTON(odlg->logfileuse)->active;
	gtk_widget_set_sensitive(GTK_WIDGET(odlg->logfilename), state);
	gtk_widget_set_sensitive(odlg->logfilename_l, state);
	gtk_widget_set_sensitive(GTK_WIDGET(odlg->logfileminsecs), state);
	gtk_widget_set_sensitive(odlg->logfileminsecs_l, state);
}

static void logfile_options(OptionsDlg *odlg, GtkBox *vbox)
{
	GtkWidget *w, *frame;
	GtkBox *vbox2, *hbox;

	frame = gtk_frame_new(_("Logfile"));
	gtk_widget_show(frame);
	gtk_box_pack_start(vbox, frame, FALSE, FALSE, 2);
	
	vbox2 = GTK_BOX(gtk_vbox_new(FALSE, 2));
	gtk_widget_show(GTK_WIDGET(vbox2));
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vbox2));

	w = gtk_check_button_new_with_label(_("Use Logfile"));
	gtk_widget_show(w);
	gtk_box_pack_start(vbox2, w, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   GTK_SIGNAL_FUNC(logfile_sigfunc),
			   (gpointer *)odlg);
	odlg->logfileuse = GTK_CHECK_BUTTON(w);
	toggle_changes_property_box(odlg, w);

	hbox = GTK_BOX(gtk_hbox_new(FALSE, 5));
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_box_pack_start(vbox2, GTK_WIDGET(hbox), FALSE, FALSE, 0);

	w = gtk_label_new(_("Filename:"));
	gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 0);
	odlg->logfilename_l = w;
	w = gnome_file_entry_new("logfilename", "Logfile");
	gtk_widget_show(w);
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
	odlg->logfilename = GNOME_FILE_ENTRY(w);
	entry_changes_property_box(odlg, 
				   gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(w)));

	hbox = GTK_BOX(gtk_hbox_new(FALSE, 5));
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_box_pack_start(vbox2, GTK_WIDGET(hbox), FALSE, FALSE, 0);

	w = gtk_label_new(_("Timeout in secs:"));
	gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 0);
	odlg->logfileminsecs_l = w;
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
	odlg->logfileminsecs = GTK_ENTRY(w);
	entry_changes_property_box(odlg, w);
}



static void options_dialog_set(OptionsDlg *odlg)
{
	char s[20];
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_secs), config_show_secs);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_status_bar),
				    config_show_statusbar);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_clist_titles),
				    config_show_clist_titles);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_icons),
				    config_show_tb_icons);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_texts),
				    config_show_tb_texts);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_tips),
				    config_show_tb_tips);

	if (config_command) gtk_entry_set_text(odlg->command, config_command);
	if (config_command_null) gtk_entry_set_text(odlg->command_null, config_command_null);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->logfileuse), config_logfile_use);
	if (config_logfile_name) gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(odlg->logfilename)), config_logfile_name);
	sprintf(s, "%d", config_logfile_min_secs);
	gtk_entry_set_text(odlg->logfileminsecs, s);

        /* toolbar sections */
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_new),
                                    config_show_tb_new);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_file),
                                    config_show_tb_file);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_ccp),
                                    config_show_tb_ccp);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_prop),
                                    config_show_tb_prop);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_timer),
                                    config_show_tb_timer);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_pref),
                                    config_show_tb_pref);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_help),
                                    config_show_tb_help);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_exit),
                                    config_show_tb_exit);

	logfile_sigfunc(NULL, odlg);

	/* Full explanation of this is in prop.c */
	gnome_dialog_set_sensitive(GNOME_DIALOG(odlg->dlg), 0, FALSE);
	gnome_dialog_set_sensitive(GNOME_DIALOG(odlg->dlg), 1, FALSE);
}



void options_dialog(void)
{
	GtkBox *vbox;
        GtkWidget *w;
	static OptionsDlg *odlg = NULL;
	
	if (!odlg) {
		char s[64];
		odlg = g_malloc(sizeof(OptionsDlg));

		odlg->dlg = GNOME_PROPERTY_BOX(gnome_property_box_new());
		sprintf(s, APP_NAME " - %s", _("Preferences"));
		gtk_window_set_title(GTK_WINDOW(odlg->dlg), s);

		vbox = (GtkBox *)gtk_vbox_new(FALSE, 0);
		gtk_widget_show(GTK_WIDGET(vbox));
		gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD);
                w = gtk_label_new(_("Misc"));
                gtk_widget_show(w);
                gnome_property_box_append_page(odlg->dlg, GTK_WIDGET(vbox), w);

		signals(odlg);
		display_options(odlg, vbox);
		command_options(odlg, vbox);
		logfile_options(odlg, vbox);

		vbox = (GtkBox *)gtk_vbox_new(FALSE, 0);
		gtk_widget_show(GTK_WIDGET(vbox));
		gtk_container_border_width(GTK_CONTAINER(vbox), 10);
                w = gtk_label_new(_("Toolbar"));
                gtk_widget_show(w);
                gnome_property_box_append_page(odlg->dlg, GTK_WIDGET(vbox), w);
                toolbar_options(odlg, vbox);

		gnome_dialog_close_hides(GNOME_DIALOG(odlg->dlg), TRUE);
	}
	options_dialog_set(odlg);
	gtk_widget_show(GTK_WIDGET(odlg->dlg));
}

