/*   Global Preferences for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
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

#include "config.h"

#include <glade/glade.h>
#include <gnome.h>
#include <libgnome/gnome-help.h>
#include <string.h>

#include "app.h"
#include "ctree.h"
#include "cur-proj.h"
#include "gtt.h"
#include "prefs.h"
#include "timer.h"
#include "toolbar.h"
#include "util.h"


/* globals */
int config_show_secs = 0;
int config_show_statusbar = 1;
int config_show_clist_titles = 1;
int config_show_subprojects = 1;
int config_show_title_desc = 1;
int config_show_title_task = 1;

int config_show_tb_icons = 1;
int config_show_tb_texts = 1;
int config_show_tb_tips = 1;
int config_show_tb_new = 1;
int config_show_tb_file = 0;
int config_show_tb_ccp = 0;
int config_show_tb_journal = 1;
int config_show_tb_prop = 1;
int config_show_tb_timer = 1;
int config_show_tb_pref = 0;
int config_show_tb_help = 1;
int config_show_tb_exit = 1;

char *config_logfile_name = NULL;
char *config_logfile_start = NULL;
char *config_logfile_stop = NULL;
int config_logfile_use = 0;
int config_logfile_min_secs = 0;

char * config_data_url = NULL;

typedef struct _PrefsDialog 
{
	GladeXML *gtxml;
	GnomePropertyBox *dlg;
	GtkCheckButton *show_secs;
	GtkCheckButton *show_status_bar;
	GtkCheckButton *show_clist_titles;
	GtkCheckButton *show_subprojects;
	GtkCheckButton *show_desc;
	GtkCheckButton *show_task;

	GtkCheckButton *logfileuse;
	GtkWidget      *logfilename_l;
	GtkEntry       *logfilename;
	GtkWidget      *logfilestart_l;
	GtkEntry       *logfilestart;
	GtkWidget      *logfilestop_l;
	GtkEntry       *logfilestop;
	GtkWidget      *logfileminsecs_l;
	GtkEntry       *logfileminsecs;

	GtkEntry       *command;
	GtkEntry       *command_null;

	GtkCheckButton *show_tb_icons;
	GtkCheckButton *show_tb_texts;
	GtkCheckButton *show_tb_tips;
	GtkCheckButton *show_tb_new;
	GtkCheckButton *show_tb_ccp;
	GtkCheckButton *show_tb_journal;
	GtkCheckButton *show_tb_pref;
	GtkCheckButton *show_tb_timer;
	GtkCheckButton *show_tb_prop;
	GtkCheckButton *show_tb_file;
	GtkCheckButton *show_tb_help;
	GtkCheckButton *show_tb_exit;

	GtkEntry       *idle_secs;
} PrefsDialog;


/* ============================================================== */

#define ENTRY_TO_CHAR(a, b) { 			\
	char *s = gtk_entry_get_text(a); 	\
	if (s[0]) {				\
		if (b) g_free(b); 		\
		b = g_strdup(s); 		\
	} else { 				\
		if (b) g_free(b); 		\
		b = NULL; 			\
	} 					\
}

static void 
prefs_set(GnomePropertyBox * pb, gint page, PrefsDialog *odlg)
{
	int state;

	if (0 == page)
	{
		int change = 0;

		/* display options */
		state = GTK_TOGGLE_BUTTON(odlg->show_secs)->active;
		if (state != config_show_secs) {
			config_show_secs = state;
			ctree_setup (global_ptw);
			update_status_bar();
			if (status_bar)
			gtk_widget_queue_resize(status_bar);
		}
		if (GTK_TOGGLE_BUTTON(odlg->show_status_bar)->active) {
			gtk_widget_show(GTK_WIDGET(status_bar));
			config_show_statusbar = 1;
		} else {
			gtk_widget_hide(GTK_WIDGET(status_bar));
			config_show_statusbar = 0;
		}
		if (GTK_TOGGLE_BUTTON(odlg->show_clist_titles)->active) {
			config_show_clist_titles = 1;
			ctree_titles_show (global_ptw);
		} else {
			config_show_clist_titles = 0;
			ctree_titles_hide (global_ptw);
		}
	
		if (GTK_TOGGLE_BUTTON(odlg->show_subprojects)->active) {
			config_show_subprojects = 1;
			ctree_subproj_show (global_ptw);
		} else {
			config_show_subprojects = 0;
			ctree_subproj_hide (global_ptw);
		}

		if (GTK_TOGGLE_BUTTON(odlg->show_desc)->active)
		{
			if (1 != config_show_title_desc) change = 1;
			config_show_title_desc = 1;
		}
		else
		{
			if (0 != config_show_title_desc) change = 1;
			config_show_title_desc = 0;
		}

		if (GTK_TOGGLE_BUTTON(odlg->show_task)->active) 
		{
			if (1 != config_show_title_task) change = 1;
			config_show_title_task = 1;
		}
		else
		{
			if (0 != config_show_title_task) change = 1;
			config_show_title_task = 0;
		}
		if (change)
		{
			ctree_update_column_visibility (global_ptw);
		}
	
	}

	if (1 == page)
	{
		/* shell command options */
		ENTRY_TO_CHAR(odlg->command, config_command);
		ENTRY_TO_CHAR(odlg->command_null, config_command_null);
	}

	if (2 == page)
	{
		/* log file options */
		config_logfile_use = GTK_TOGGLE_BUTTON(odlg->logfileuse)->active;
		ENTRY_TO_CHAR(odlg->logfilename, config_logfile_name);
		ENTRY_TO_CHAR(odlg->logfilestart, config_logfile_start);
		ENTRY_TO_CHAR(odlg->logfilestop, config_logfile_stop);
		config_logfile_min_secs = atoi (gtk_entry_get_text(odlg->logfileminsecs));
	}

	if (3 == page)
	{
		int change = 0;

		/* toolbar */
		config_show_tb_icons = GTK_TOGGLE_BUTTON(odlg->show_tb_icons)->active;
		config_show_tb_texts = GTK_TOGGLE_BUTTON(odlg->show_tb_texts)->active;
		config_show_tb_tips = GTK_TOGGLE_BUTTON(odlg->show_tb_tips)->active;
	
		/* toolbar sections */
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
		state = GTK_TOGGLE_BUTTON(odlg->show_tb_journal)->active;
		if (config_show_tb_journal != state) {
			change = 1;
			config_show_tb_journal = state;
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

	if (4 == page)
	{
		config_idle_timeout = atoi(gtk_entry_get_text(GTK_ENTRY(odlg->idle_secs)));
	}
}

/* ============================================================== */

static void 
logfile_sensitive_cb(GtkWidget *w, PrefsDialog *odlg)
{
	int state;
	
	state = GTK_TOGGLE_BUTTON(odlg->logfileuse)->active;
	gtk_widget_set_sensitive(GTK_WIDGET(odlg->logfilename), state);
	gtk_widget_set_sensitive(odlg->logfilename_l, state);
	gtk_widget_set_sensitive(GTK_WIDGET(odlg->logfilestart), state);
	gtk_widget_set_sensitive(odlg->logfilestart_l, state);
	gtk_widget_set_sensitive(GTK_WIDGET(odlg->logfilestop), state);
	gtk_widget_set_sensitive(odlg->logfilestop_l, state);
	gtk_widget_set_sensitive(GTK_WIDGET(odlg->logfileminsecs), state);
	gtk_widget_set_sensitive(odlg->logfileminsecs_l, state);
}

static void 
options_dialog_set(PrefsDialog *odlg)
{
	char s[30];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_secs), config_show_secs);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_status_bar),
				    config_show_statusbar);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_clist_titles),
				    config_show_clist_titles);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_subprojects),
				    config_show_subprojects);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_desc),
				    config_show_title_desc);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_task),
				    config_show_title_task);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_icons),
				    config_show_tb_icons);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_texts),
				    config_show_tb_texts);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_tips),
				    config_show_tb_tips);

	if (config_command)
		gtk_entry_set_text(odlg->command, config_command);
	else
		gtk_entry_set_text(odlg->command, "");
	if (config_command_null)
		gtk_entry_set_text(odlg->command_null, config_command_null);
	else
		gtk_entry_set_text(odlg->command_null, "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->logfileuse), config_logfile_use);
	if (config_logfile_name)
		gtk_entry_set_text(odlg->logfilename, config_logfile_name);
	else
		gtk_entry_set_text(odlg->logfilename, "");
	if (config_logfile_start)
		gtk_entry_set_text(odlg->logfilestart, config_logfile_start);
	else
		gtk_entry_set_text(odlg->logfilestart, "");
	if (config_logfile_stop)
		gtk_entry_set_text(odlg->logfilestop, config_logfile_stop);
	else
		gtk_entry_set_text(odlg->logfilestop, "");
	g_snprintf(s, sizeof (s), "%d", config_logfile_min_secs);
	gtk_entry_set_text(GTK_ENTRY(odlg->logfileminsecs), s);

	/* toolbar sections */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_new),
				    config_show_tb_new);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_file),
				    config_show_tb_file);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_ccp),
				    config_show_tb_ccp);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_prop),
				    config_show_tb_prop);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_timer),
				    config_show_tb_timer);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_pref),
				    config_show_tb_pref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_help),
				    config_show_tb_help);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(odlg->show_tb_exit),
				    config_show_tb_exit);

	logfile_sensitive_cb(NULL, odlg);

	/* misc section */
	g_snprintf(s, sizeof (s), "%d", config_idle_timeout);
	gtk_entry_set_text(GTK_ENTRY(odlg->idle_secs), s);

	/* set to unmodified as it reflects the current state of the app */
	gnome_property_box_set_modified(GNOME_PROPERTY_BOX(odlg->dlg), FALSE);
}

/* ============================================================== */

#define GETWID(strname) 						\
({									\
	GtkWidget *e;							\
	e = glade_xml_get_widget (gtxml, strname);			\
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",		\
			  GTK_SIGNAL_FUNC(gnome_property_box_changed), 	\
			  GTK_OBJECT(dlg->dlg));			\
	e;								\
})

#define GETCHWID(strname) 						\
({									\
	GtkWidget *e;							\
	e = glade_xml_get_widget (gtxml, strname);			\
	gtk_signal_connect_object(GTK_OBJECT(e), "toggled",		\
			  GTK_SIGNAL_FUNC(gnome_property_box_changed), 	\
			  GTK_OBJECT(dlg->dlg));			\
	e;								\
})

static void 
display_options(PrefsDialog *dlg)
{
	GtkWidget *w;
	GladeXML *gtxml = dlg->gtxml;

	w = GETCHWID ("show secs");
	dlg->show_secs = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show status");
	dlg->show_status_bar = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show head");
	dlg->show_clist_titles = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show sub");
	dlg->show_subprojects = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show desc");
	dlg->show_desc = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show task");
	dlg->show_task = GTK_CHECK_BUTTON(w);
}


static void 
command_options (PrefsDialog *dlg)
{
	GtkWidget *e;
	GladeXML *gtxml = dlg->gtxml;

	e = GETWID ("switch project");
	dlg->command = GTK_ENTRY(e);

	e = GETWID ("no project");
	dlg->command_null = GTK_ENTRY(e);
}

static void 
logfile_options(PrefsDialog *dlg)
{
	GtkWidget *w;
	GladeXML *gtxml = dlg->gtxml;

	w = GETCHWID ("use logfile");
	dlg->logfileuse = GTK_CHECK_BUTTON(w);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
		   GTK_SIGNAL_FUNC(logfile_sensitive_cb),
		   (gpointer *)dlg);

	w = glade_xml_get_widget (gtxml, "filename label");
	dlg->logfilename_l = w;

	w = GETWID ("filename combo");
	dlg->logfilename = GTK_ENTRY(w);

	w = glade_xml_get_widget (gtxml, "fstart label");
	dlg->logfilestart_l = w;

	w = GETWID ("fstart combo");
	dlg->logfilestart = GTK_ENTRY(w);

	w = glade_xml_get_widget (gtxml, "fstop label");
	dlg->logfilestop_l = w;

	w = GETWID ("fstop combo");
	dlg->logfilestop = GTK_ENTRY(w);

	w = glade_xml_get_widget (gtxml, "fmin label");
	dlg->logfileminsecs_l = w;

	w = GETWID ("fmin combo");
	dlg->logfileminsecs = GTK_ENTRY(w);
}

static void 
toolbar_options(PrefsDialog *dlg)
{
	GtkWidget *w;
	GladeXML *gtxml = dlg->gtxml;

	w = GETCHWID ("show icons");
	dlg->show_tb_icons = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show texts");
	dlg->show_tb_texts = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show tooltips");
	dlg->show_tb_tips = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show new");
	dlg->show_tb_new = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show save");
	dlg->show_tb_file = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show cut");
	dlg->show_tb_ccp = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show journal");
	dlg->show_tb_journal = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show props");
	dlg->show_tb_prop = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show timer");
	dlg->show_tb_timer = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show prefs");
	dlg->show_tb_pref = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show help");
	dlg->show_tb_help = GTK_CHECK_BUTTON(w);

	w = GETCHWID ("show quit");
	dlg->show_tb_exit = GTK_CHECK_BUTTON(w);
}

static void 
misc_options(PrefsDialog *dlg)
{
	GtkWidget *w;
	GladeXML *gtxml = dlg->gtxml;

	w = GETWID ("idle secs");
	dlg->idle_secs = GTK_ENTRY(w);
}

/* ============================================================== */

static PrefsDialog *
prefs_dialog_new (void)
{
	PrefsDialog *dlg;
	GladeXML *gtxml;
	static GnomeHelpMenuEntry help_entry = { NULL, "preferences.html#PREF" };

	dlg = g_malloc(sizeof(PrefsDialog));

	gtxml = glade_xml_new ("glade/prefs.glade", "Global Preferences");
	dlg->gtxml = gtxml;

	dlg->dlg = GNOME_PROPERTY_BOX (glade_xml_get_widget (gtxml,  "Global Preferences"));

	help_entry.name = gnome_app_id;
	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "help",
			   GTK_SIGNAL_FUNC(gnome_help_pbox_display),
			   &help_entry);

	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "apply",
			   GTK_SIGNAL_FUNC(prefs_set), dlg);

	/* ------------------------------------------------------ */
	/* grab the various entry boxes and hook them up */
	display_options (dlg);
	command_options (dlg);
	logfile_options (dlg);
	toolbar_options (dlg);
	misc_options (dlg);

	gnome_dialog_close_hides(GNOME_DIALOG(dlg->dlg), TRUE);
/*
	gnome_dialog_set_parent(GNOME_DIALOG(dlg->dlg), GTK_WINDOW(window));

*/
	return dlg;
}


/* ============================================================== */

static PrefsDialog *dlog = NULL;

void 
prefs_dialog_show(void)
{
	if (!dlog) dlog = prefs_dialog_new();
 
	options_dialog_set (dlog);
	gtk_widget_show(GTK_WIDGET(dlog->dlg));
}

/* ==================== END OF FILE ============================= */
