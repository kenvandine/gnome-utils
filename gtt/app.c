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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "gtt.h"


#undef gettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


/* I had some problems with the GtkStatusbar (frame and label didn't
   get shown). So I defined this here */
#define GTK_USE_STATUSBAR

/* Due to the same problems I define this, if I want to include the
   gtk_widget_show for the frame and the label of the statusbar */
#ifdef GTK_USE_STATUSBAR
#define SB_USE_HACK
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
static gint status_project_id, status_day_time_id;
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
int config_logfile_use = 0;
int config_logfile_min_secs = 0;




void update_status_bar(void)
{
	static char *old_day_time = NULL;
        static char *old_project = NULL;
	char *s;

	if (!status_bar) return;
        if (!old_day_time) old_day_time = g_strdup("");
        if (!old_project) old_project = g_strdup("");
        s = g_strdup(project_get_timestr(NULL, 0));
        if (0 != strcmp(s, old_day_time)) {
#ifdef GTK_USE_STATUSBAR
                gtk_statusbar_pop(status_day_time, status_day_time_id);
                gtk_statusbar_push(status_day_time, s);
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
                gtk_statusbar_pop(status_project, status_day_time_id);
                gtk_statusbar_push(status_project, s);
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
	log_proj(proj);
	prop_dialog_set_project(proj);
	menu_set_states();
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



#ifndef GTK_USE_CLIST
/*
 * TODO: can this be done in a better way?
 * I don't want to create a new widget based on GtkList just to prevent the
 * 3rd mouse button to emit a select_child. So I have to tell the signal_func
 * by other means, when a real selection occures. When a popup menu is to be
 * drawn I set for_popup to non-zero, select a new child if the selection
 * state does not look like it should (I need the item, the mouse click
 * occured on), I can do select_child without GTT logging project changes and
 * such.
 */
static int for_popup = 0;

static void select_item(GtkList *glist, GtkWidget *w)
{
	GtkWidget *li;

	if (!GTK_LIST(glist)->selection) {
		if (!for_popup)
			cur_proj_set(NULL);
		return;
	}
	li = GTK_LIST(glist)->selection->data;
	if (!li) {
		cur_proj_set(NULL);
		return;
	}
	cur_proj_set((project *)gtk_object_get_data(GTK_OBJECT(li), "list_item_data"));
}



void update_title_label(project *p)
{
	if (!p) return;
	if (!p->title_label) return;
	gtk_label_set(p->title_label, p->title);
}



void update_label(project *p)
{
	char buf[20];

	if (!p) return;
	if (!p->label) return;
	update_status_bar();
	if (config_show_secs) {
		sprintf(buf, "%02d:%02d:%02d",
			p->day_secs / 3600,
			(p->day_secs / 60) % 60,
			p->day_secs % 60);
	} else {
		sprintf(buf, "%02d:%02d", p->day_secs / 3600, (p->day_secs / 60) % 60);
	}
	gtk_label_set(p->label, buf);
}



static GtkWidget *build_item(project *p)
{
	GtkWidget *item;
	GtkWidget *label;
	GtkWidget *hbox;
	char buf[20];
	
	item = gtk_list_item_new();
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(item), hbox);
	if (config_show_secs) {
		sprintf(buf, "%02d:%02d:%02d",
			p->day_secs / 3600,
			(p->day_secs / 60) % 60,
			p->day_secs % 60);
	} else {
		sprintf(buf, "%02d:%02d", p->day_secs / 3600, (p->day_secs / 60) % 60);
	}
	label = gtk_label_new(buf);
	p->label = GTK_LABEL(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	label = gtk_label_new(p->title);
	p->title_label = GTK_LABEL(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(hbox);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "list_item_data", p);
	return item;
}



void add_item(GtkWidget *glist, project *p)
{
	GtkWidget *item = build_item(p);

	gtk_container_add(GTK_CONTAINER(glist), item);
	if (cur_proj == p) {
		gtk_list_item_select(GTK_LIST_ITEM(item));
	}
}



void add_item_at(GtkWidget *glist, project *p, int pos)
{
	GList *gl;
	GtkWidget *item = build_item(p);

	gl = g_malloc(sizeof(GList));
	gl->data = item;
	gl->next = gl->prev = NULL;
	gtk_list_insert_items(GTK_LIST(glist), gl, pos);
}



void setup_list(void)
{
	project_list *pl;
	GList *gl;
	int timer_running, cp_found = 0;

	/* cur_proj = NULL; */
	timer_running = (main_timer != 0);
	stop_timer();
	for (;;) {
		gl = GTK_LIST(glist)->children;
		if (!gl) break;
		gtk_container_remove(GTK_CONTAINER(glist), gl->data);
	}
	if (!plist) {
		/* define one empty project on the first start */
		project_list_add(project_new_title(gettext("empty")));
	}
	
	for (pl = plist; pl; pl = pl->next) {
		add_item(glist, pl->proj);
		if (pl->proj == cur_proj) cp_found = 1;
	}
	if (!cp_found) cur_proj_set(NULL);
	err_init();
	gtk_widget_show(window);
	if (timer_running) start_timer();
	menu_set_states();
}
#endif /* not GTK_USE_CLIST */



static void unlock_quit(void)
{
	unlock_gtt();
	gtk_main_quit();
}



static void init_list_2(GtkWidget *w, gint butnum)
{
	if (butnum == 2) unlock_quit();
	else
#ifdef GTK_USE_CLIST
                setup_clist();
#else
                setup_list();
#endif
}

static void init_list(void)
{
	if (!project_list_load(NULL)) {
#ifdef ENOENT
                if (errno == ENOENT) {
                        errno = 0;
#ifdef GTK_USE_CLIST
                        setup_clist();
#else
                        setup_list();
#endif
                        return;
                }
#else
#warning ENOENT not defined?
#endif
		msgbox_ok_cancel(_("Error"),
				 _("An error occured while reading the "
                                   "configuration file.\n"
				   "Shall I setup a new configuration?"),
				 _("Yes"), _("No"),
				 GTK_SIGNAL_FUNC(init_list_2));
	} else {
#ifdef GTK_USE_CLIST
                setup_clist();
#else
		setup_list();
#endif
	}
}



#ifndef GTK_USE_CLIST
static gint list_button_press(GtkWidget *widget,
			      GdkEventButton *event)
{
	GtkList *list;
	GtkWidget *item;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	list = GTK_LIST (widget);
	item = gtk_get_event_widget ((GdkEvent*) event);

	while (!gtk_type_is_a (GTK_WIDGET_TYPE (item), gtk_list_item_get_type ()))
		item = item->parent;

	if (event->button == 3) {
		GtkWidget *menu;
		
		/* TODO: see declaration of for_popup above */
		for_popup = 1;

		/* make sure, the item, the mouse was clickt on, is selected */
		if (list->selection != NULL)
			if (list->selection->data == item)
				gtk_list_select_child (list, item);

		/* popup menu */
		get_menubar(&menu, NULL, MENU_POPUP);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, 0);
		for_popup = 0;
	}

	return TRUE;
}
#endif /* not GTK_USE_CLIST */



void app_new(int argc, char *argv[])
{
	GtkWidget *vbox;
#ifndef GTK_USE_CLIST
        GtkWidget *swin;
#endif /* not GTK_USE_CLIST */
	char *p, *p0, c;
	int i;
	int x, y, w, h, xy_set;
	GtkWidget *widget;
#ifndef GNOME_USE_APP
	GtkAcceleratorTable *accel;
#endif /* GNOME_USE_APP */

	w = 0; h = 0; xy_set = 0;
	x = 0; y = 0; /* keep the compiler happy */
	for (i = 1; i < argc; i++) {
		if (0 == strcmp(argv[i], "-geometry")) {
			i++;
			p = argv[i];
			if ((*p >= '0') && (*p <= '9')) {
				p0 = p;
				for (; (*p >= '0') && (*p <= '9'); p++) ;
				if (*p != 'x') {
					g_print(_("error in geometry string \"%s\"\n"), argv[i]);
					continue;
				}
				*p = 0;
				w = atoi(p0);
				*p = 'x';
				p0 = ++p;
				for (; (*p >= '0') && (*p <= '9'); p++) ;
				c = *p;
				*p = 0;
				h = atoi(p0);
				*p = c;
			}
			if (*p == 0) continue;
			if ((*p != '-') && (*p != '+')) {
				g_print(_("error in geometry string \"%s\"\n"), argv[i]);
				continue;
			}
			p0 = p;
			for (p++; (*p >= '0') && (*p <= '9'); p++) ;
			c = *p;
			*p = 0;
			x = atoi(p0);
			*p = c;
			if ((*p != '-') && (*p != '+')) {
				g_print(_("error in geometry string \"%s\"\n"), argv[i]);
				continue;
			}
			p0 = p;
			for (p++; (*p >= '0') && (*p <= '9'); p++) ;
			if (*p != 0) {
				g_print(_("error in geometry string \"%s\"\n"), argv[i]);
				continue;
			}
			y = atoi(p0);
			xy_set++;
		} else {
			/* TODO: parsing more arguments? */
			g_print(_("unknown arg: %s\n"), argv[i]);
		}
	}

#ifdef GNOME_USE_APP
	window = gnome_app_new("gtt", APP_NAME " " VERSION);
        gtk_window_set_wmclass(GTK_WINDOW(window),
                               "gtt", "GTimeTracker");
	menu_create(window);
	widget = build_toolbar();
        gtk_widget_show(widget);
        gnome_app_set_toolbar(GNOME_APP(window), GTK_TOOLBAR(widget));
	vbox = gtk_vbox_new(FALSE, 0);
#else /* not GNOME_USE_APP */ 
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_wmclass(GTK_WINDOW(window),
                               "gtt", "GTimeTracker");
	gtk_window_set_title(GTK_WINDOW(window), APP_NAME " " VERSION);
	gtk_container_border_width(GTK_CONTAINER(window), 1);

	vbox = gtk_vbox_new(FALSE, 3);
        window_vbox = GTK_BOX(vbox);

	get_menubar(&widget, &accel, MENU_MAIN);
	gtk_widget_show(widget);
	gtk_window_add_accelerator_table(GTK_WINDOW(window), accel);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	
	widget = build_toolbar();
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
#endif /* not GNOME_USE_APP */ 

	status_bar = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(status_bar);
	gtk_box_pack_end(GTK_BOX(vbox), status_bar, FALSE, FALSE, 2);
#ifdef GTK_USE_STATUSBAR
        status_day_time = GTK_STATUSBAR(gtk_statusbar_new());
#ifdef SB_USE_HACK
        gtk_widget_show(GTK_WIDGET(status_day_time->frame));
        gtk_widget_show(GTK_WIDGET(status_day_time->label));
#endif /* SB_USE_HACK */
        gtk_widget_show(GTK_WIDGET(status_day_time));
        status_day_time_id = gtk_statusbar_push(status_day_time,
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
                                               _("no project selected"));
        gtk_box_pack_start(GTK_BOX(status_bar), GTK_WIDGET(status_project),
                           TRUE, TRUE, 1);
#else /* not GTK_USE_STATUSBAR */
	status_day_time = (GtkLabel *)gtk_label_new("00:00");
	gtk_widget_show((GtkWidget *)status_day_time);
	gtk_box_pack_start(GTK_BOX(status_bar), (GtkWidget *)status_day_time,
                           FALSE, FALSE, 3);
	status_project = (GtkLabel *)gtk_label_new(_("no project selected"));
	gtk_widget_show((GtkWidget *)status_project);
	gtk_box_pack_start(GTK_BOX(status_bar), (GtkWidget *)status_project,
                           FALSE, FALSE, 3);
#endif /* not GTK_USE_STATUSBAR */

#ifdef GTK_USE_CLIST
        glist = create_clist();
	/* TODO: remove hard coded pixel values...? */
	gtk_widget_set_usize(glist, 200, 170);
	gtk_box_pack_end(GTK_BOX(vbox), glist, TRUE, TRUE, 0);
	gtk_widget_show(glist);
#else /* not GTK_USE_CLIST */
	swin = gtk_scrolled_window_new(NULL, NULL);
	/* TODO: remove hard coded pixel values...? */
	gtk_widget_set_usize(swin, 200, 170);
	gtk_box_pack_end(GTK_BOX(vbox), swin, TRUE, TRUE, 0);
	gtk_widget_show(swin);
	glist = gtk_list_new();
	gtk_list_set_selection_mode(GTK_LIST(glist), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(glist), "select_child",
			   GTK_SIGNAL_FUNC(select_item), NULL);
	gtk_signal_connect(GTK_OBJECT(glist), "button_press_event",
			   GTK_SIGNAL_FUNC(list_button_press), NULL);
	gtk_widget_set_events(glist, GDK_BUTTON_PRESS_MASK);
	gtk_container_add(GTK_CONTAINER(swin), glist);
#endif /* not GTK_USE_CLIST */
	/* start timer before the state of the menu items is set */
	start_timer();
	init_list();
	gtk_widget_show(glist);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	if ((w >= 50) || (h >= 50)) {
		if (w < 50) w = 50;
		if (h < 50) h = 50;
		gtk_widget_set_usize(window, w, h);
	} else {
		w = 50;
		h = 50;
	}
	if (xy_set) {
		int t;
		t = gdk_screen_width();
		if (x < 0) x += t - w;
		while (x < 0) x += t;
		while (x > t) x -= t;
		t = gdk_screen_height();
		if (y < 0) y += t - h;
		while (y < 0) y += t;
		while (y > t) y -= t;
		gtk_widget_set_uposition(window, x, y);
	}
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_app), NULL);
#ifdef GNOME_USE_APP
	gnome_app_set_contents(GNOME_APP(window), vbox);
#endif
}

