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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "gtt.h"


/* I had some problems with the GtkStatusbar (frame and label didn't
   get shown). So I defined this here */
#define GTK_USE_STATUSBAR

/* Due to the same problems I define this, if I want to include the
   gtk_widget_show for the frame and the label of the statusbar */
#ifdef GTK_USE_STATUSBAR
#undef SB_USE_HACK
#endif


project *cur_proj = NULL;
GtkWidget *glist, *window;
#ifndef GNOME_USE_APP
GtkBox *window_vbox;
#endif

GtkWidget *status_bar;
#ifdef GTK_USE_STATUSBAR
static GtkStatusbar *status_project = NULL;
static GtkStatusbar *status_day_time = NULL;
static GtkWidget *status_timer = NULL;
static gint status_project_id = 1, status_day_time_id = 2;
#else /* GTK_USE_STATUSBAR */
static GtkLabel *status_project = NULL;
static GtkLabel *status_day_time = NULL;
#endif /* GTK_USE_STATUSBAR */

#ifdef DEBUG
int config_show_secs = 1;
#else
int config_show_secs = 0;
#endif
int config_show_statusbar = 1;
int config_show_clist_titles = 1;
int config_show_tb_icons = 1;
int config_show_tb_texts = 1;
int config_show_tb_tips = 1;
int config_show_tb_new = 1;
int config_show_tb_file = 0;
int config_show_tb_ccp = 0;
int config_show_tb_prop = 1;
int config_show_tb_timer = 1;
int config_show_tb_pref = 1;
int config_show_tb_help = 1;
int config_show_tb_exit = 1;
char *config_command = NULL;
char *config_command_null = NULL;
char *config_logfile_name = NULL;
char *config_logfile_str = NULL;
char *config_logfile_stop = NULL;
int config_logfile_use = 0;
int config_logfile_min_secs = 0;




void update_status_bar(void)
{
	static char *old_day_time = NULL;
        static char *old_project = NULL;
	char *s;

	if (!status_bar) return;
	if (status_timer) {
		if (main_timer)
			gtk_widget_show(status_timer);
		else
			gtk_widget_hide(status_timer);
	}
        if (!old_day_time) old_day_time = g_strdup("");
        if (!old_project) old_project = g_strdup("");
        s = g_strdup(project_get_timestr(NULL, config_show_secs));
        if (0 != strcmp(s, old_day_time)) {
#ifdef GTK_USE_STATUSBAR
                gtk_statusbar_remove(status_day_time, 2, status_day_time_id);
                status_day_time_id = gtk_statusbar_push(status_day_time, 2, s);
#else /* not GTK_USE_STATUSBAR */
                gtk_label_set(status_day_time, s);
#endif /* not GTK_USE_STATUSBAR */
                g_free(old_day_time);
                old_day_time = s;
        } else {
                g_free(s);
        }
        if (cur_proj) {
                s = g_strdup(cur_proj->title);
        } else {
                s = g_strdup(_("no project selected"));
        }
        if (0 != strcmp(s, old_project)) {
#ifdef GTK_USE_STATUSBAR
                gtk_statusbar_remove(status_project, 1, status_project_id);
                status_project_id = gtk_statusbar_push(status_project, 1, s);
#else /* not GTK_USE_STATUSBAR */
                gtk_label_set(status_project, s);
#endif /* not GTK_USE_STATUSBAR */
                g_free(old_project);
                old_project = s;
        } else {
                g_free(s);
        }
}



void cur_proj_set(project *proj)
{
	pid_t fork(void);

	pid_t pid;
	char *cmd, *p;
	static char s[1024];
	int i;

	if (cur_proj == proj) return;

	cur_proj = proj;
	if (proj)
		start_timer();
	else
		stop_timer();
	log_proj(proj);
	menu_set_states();
	prop_dialog_set_project(proj);
	update_status_bar();
	cmd = (proj) ? config_command : config_command_null;
	if (!cmd) return;
	i = 0;
	for (p = cmd; *p; p++) {
		if ((p[0] == '%') && (p[1] == 's')) {
			s[i] = 0;
			if (cur_proj) strcat(s, cur_proj->title);
			i = strlen(s);
			p++;
		} else {
			s[i] = *p;
			i++;
		}
	}
	s[i] = 0;
	pid = fork();
	if (pid == 0) {
		execlp("sh", "sh", "-c", s, NULL);
		g_warning("%s: %d: cur_proj_set: couldn't exec\n", __FILE__, __LINE__);
		exit(1);
	}
	if (pid < 0) {
		g_warning("%s: %d: cur_proj_set: couldn't fork\n", __FILE__, __LINE__);
	}
}



void app_new(int argc, char *argv[], char *geometry_string)
{
	GtkWidget *vbox;
	GtkWidget *widget;
	gint x, y, w, h;

	window = gnome_app_new("gtt", APP_NAME " " VERSION);
        gtk_window_set_wmclass(GTK_WINDOW(window),
                               "gtt", "GTimeTracker");
	/* 320 x 220 seems to be a good size to default to */
	gtk_window_set_default_size(GTK_WINDOW(window), 320, 220);
	gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
	menus_create(GNOME_APP(window));
	widget = build_toolbar();
        gtk_widget_show(widget);
        gnome_app_set_toolbar(GNOME_APP(window), GTK_TOOLBAR(widget));
	vbox = gtk_vbox_new(FALSE, 0);

	status_bar = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(status_bar);
	gtk_box_pack_end(GTK_BOX(vbox), status_bar, FALSE, FALSE, 2);
        status_day_time = GTK_STATUSBAR(gtk_statusbar_new());
#ifdef SB_USE_HACK
        gtk_widget_show(GTK_WIDGET(status_day_time->frame));
        gtk_widget_show(GTK_WIDGET(status_day_time->label));
#endif /* SB_USE_HACK */
        gtk_widget_show(GTK_WIDGET(status_day_time));
        status_day_time_id = gtk_statusbar_push(status_day_time,
						2,
						_("00:00"));
        gtk_box_pack_start(GTK_BOX(status_bar), GTK_WIDGET(status_day_time),
                           FALSE, FALSE, 1);
        status_project = GTK_STATUSBAR(gtk_statusbar_new());
#ifdef SB_USE_HACK
        gtk_widget_show(GTK_WIDGET(status_project->frame));
        gtk_widget_show(GTK_WIDGET(status_project->label));
#endif /* SB_USE_HACK */
        gtk_widget_show(GTK_WIDGET(status_project));
        status_project_id = gtk_statusbar_push(status_project,
					       1,
					       _("no project selected"));
        gtk_box_pack_start(GTK_BOX(status_bar), GTK_WIDGET(status_project),
                           TRUE, TRUE, 1);
	status_timer = gnome_stock_pixmap_widget_at_size(GTK_WIDGET(status_bar),
							 GNOME_STOCK_PIXMAP_TIMER,
							 16, 16);
	gtk_widget_show(status_timer);
	gtk_box_pack_end(GTK_BOX(status_bar), GTK_WIDGET(status_timer),
			 FALSE, FALSE, 1);

        glist = create_clist();
	gtk_box_pack_end(GTK_BOX(vbox), glist->parent, TRUE, TRUE, 0);
	gtk_widget_set_usize(glist, -1, 120);
	gtk_widget_show_all(glist->parent);

	gtk_widget_show(vbox);
	gnome_app_set_contents(GNOME_APP(window), vbox);

	if (!geometry_string) {
		return;
	}
	if (gnome_parse_geometry(geometry_string, &x, &y, &w, &h)) {
		if ((x != -1) && (y != -1))
			gtk_widget_set_uposition(GTK_WIDGET(window), x, y);
		if ((w != -1) && (h != -1))
			gtk_window_set_default_size(GTK_WINDOW(window), w, h);
	} else {
		gnome_app_error(GNOME_APP(window),
			_("Couldn't understand geometry (position and size)\n"
				" specified on command line"));
	}
}
