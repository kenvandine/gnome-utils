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

#include "gtt.h"


#undef gettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


typedef struct _OptionsDlg {
	GtkDialog *dlg;
	GtkCheckButton *show_secs;
	GtkCheckButton *show_status_bar;
	GtkCheckButton *show_tb_icons, *show_tb_texts;

	GtkEntry *command;
	GtkEntry *command_null;
	GtkEntry *logfilename;
	GtkWidget *logfilename_l;
	GtkCheckButton *logfileuse;
	GtkEntry *logfileminsecs;
	GtkWidget *logfileminsecs_l;
	GtkWidget *ok;
} OptionsDlg;



#define ENTRY_TO_CHAR(a, b) { char *s = gtk_entry_get_text(a); if (s[0]) { if (b) g_free(b); b = g_strdup(s); } else { if (b) g_free(b); b = NULL; } }

static void options_ok(GtkWidget *w, OptionsDlg *odlg)
{
	int state;

	/* display options */
	state = GTK_TOGGLE_BUTTON(odlg->show_secs)->active;
	if (state != config_show_secs) {
		config_show_secs = state;
		setup_list();
	}
	if (GTK_TOGGLE_BUTTON(odlg->show_status_bar)->active) {
		gtk_widget_show(status_bar);
	} else {
		gtk_widget_hide(status_bar);
	}
	config_show_tb_icons = GTK_TOGGLE_BUTTON(odlg->show_tb_icons)->active;
	config_show_tb_texts = GTK_TOGGLE_BUTTON(odlg->show_tb_texts)->active;
	toolbar_set_states();

	/* shell command options */
	ENTRY_TO_CHAR(odlg->command, config_command);
	ENTRY_TO_CHAR(odlg->command_null, config_command_null);

	/* log file options */
	config_logfile_use = GTK_TOGGLE_BUTTON(odlg->logfileuse)->active;
	ENTRY_TO_CHAR(odlg->logfilename, config_logfile_name);
	config_logfile_min_secs = atoi(gtk_entry_get_text(odlg->logfileminsecs));

	/* OK or Apply pressed? */
	if (w == odlg->ok) {
		gtk_widget_hide(GTK_WIDGET(odlg->dlg));
	}
}



static void options_help(GtkWidget *w, gpointer *data)
{
#ifdef USE_GTT_HELP
        help_goto("ch-dialogs.html#s-pref");
#endif /* USE_GTT_HELP */
}



static void buttons(OptionsDlg *odlg, GtkBox *aa)
{
	GtkWidget *w;

	w = gtk_button_new_with_label(_("OK"));
	gtk_widget_show(w);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   GTK_SIGNAL_FUNC(options_ok),
			   (gpointer *)odlg);
	gtk_box_pack_start(aa, w, FALSE, FALSE, 2);
	odlg->ok = w;

	w = gtk_button_new_with_label(_("Apply"));
	gtk_widget_show(w);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   GTK_SIGNAL_FUNC(options_ok),
			   (gpointer *)odlg);
	gtk_box_pack_start(aa, w, FALSE, FALSE, 2);

	w = gtk_button_new_with_label(_("Cancel"));
	gtk_widget_show(w);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_hide),
				  GTK_OBJECT(odlg->dlg));
	gtk_box_pack_start(aa, w, FALSE, FALSE, 2);

	w = gtk_button_new_with_label(_("Help"));
	gtk_widget_show(w);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  GTK_SIGNAL_FUNC(options_help),
				  NULL);
	gtk_box_pack_start(aa, w, FALSE, FALSE, 2);
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
	
	w = gtk_check_button_new_with_label(_("Show Toolbar Icons"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_icons = GTK_CHECK_BUTTON(w);

	w = gtk_check_button_new_with_label(_("Show Toolbar Texts"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_tb_texts = GTK_CHECK_BUTTON(w);

	w = gtk_check_button_new_with_label(_("Show Status Bar"));
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vb), w, FALSE, FALSE, 0);
	odlg->show_status_bar = GTK_CHECK_BUTTON(w);
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
	w = gtk_label_new(_("No Project Command:"));
	gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 1, 2);
	gtk_table_set_col_spacing(table, 0, 5);
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 1, 2);
	odlg->command_null = GTK_ENTRY(w);
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

	hbox = GTK_BOX(gtk_hbox_new(FALSE, 5));
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_box_pack_start(vbox2, GTK_WIDGET(hbox), FALSE, FALSE, 0);

	w = gtk_label_new(_("Filename:"));
	gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 0);
	odlg->logfilename_l = w;
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
	odlg->logfilename = GTK_ENTRY(w);

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
}



static void options_dialog_set(OptionsDlg *odlg)
{
	char s[20];
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_secs), config_show_secs);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_status_bar),
				    GTK_WIDGET_VISIBLE(status_bar));

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_icons),
				    config_show_tb_icons);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_tb_texts),
				    config_show_tb_texts);

	if (config_command) gtk_entry_set_text(odlg->command, config_command);
	if (config_command_null) gtk_entry_set_text(odlg->command_null, config_command_null);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->logfileuse), config_logfile_use);
	if (config_logfile_name) gtk_entry_set_text(odlg->logfilename, config_logfile_name);
	sprintf(s, "%d", config_logfile_min_secs);
	gtk_entry_set_text(odlg->logfileminsecs, s);

	logfile_sigfunc(NULL, odlg);
}



void options_dialog(void)
{
	GtkBox *vbox, *aa;
	static OptionsDlg *odlg = NULL;
	
	if (!odlg) {
		char s[64];
		odlg = g_malloc(sizeof(OptionsDlg));

		odlg->dlg = GTK_DIALOG(gtk_dialog_new());
		sprintf(s, APP_NAME " - %s", _("Preferences"));
		gtk_window_set_title(GTK_WINDOW(odlg->dlg), s);
                gtk_signal_connect(GTK_OBJECT(odlg->dlg), "delete_event",
                                   GTK_SIGNAL_FUNC(gtt_delete_event),
                                   NULL);

		vbox = (GtkBox *)gtk_vbox_new(FALSE, 0);
		gtk_widget_show(GTK_WIDGET(vbox));
		gtk_container_border_width(GTK_CONTAINER(vbox), 10);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(odlg->dlg)->vbox), GTK_WIDGET(vbox),
				   FALSE, FALSE, 0);
		aa = GTK_BOX(GTK_DIALOG(odlg->dlg)->action_area);

		buttons(odlg, aa);
		display_options(odlg, vbox);
		command_options(odlg, vbox);
		logfile_options(odlg, vbox);

	}
	options_dialog_set(odlg);
	gtk_widget_show(GTK_WIDGET(odlg->dlg));
}

