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

#include "tb_new.xpm"

#include "tb_open.xpm"
#include "tb_save.xpm"

#include "tb_cut.xpm"
#include "tb_copy.xpm"
#include "tb_paste.xpm"

#include "tb_properties.xpm"
#include "tb_prop_dis.xpm"
#include "tb_timer.xpm"
#include "tb_timer_stopped.xpm"

#include "tb_preferences.xpm"
#ifdef EXTENDED_TOOLBAR
# include "tb_unknown.xpm"
# include "tb_exit.xpm"
#endif /* EXTENDED_TOOLBAR */



static GtkWidget *win;
static GtkBox *hbox;
static GtkTooltips *tt;

typedef struct _ToggleData {
	GtkContainer *button;
	GtkWidget *pmap1, *pmap2, *cur_pmap;
	char *path;
} ToggleData;

static ToggleData *toggle_timer = NULL;
static ToggleData *toggle_prop = NULL;



static void sigfunc(GtkWidget *w,
		    gpointer *data)
{
	if (!data) return;
	menus_activate((char *)data);
}



static void set_toggle_state(ToggleData *data)
{
	if (!data) return;
	if (menus_get_toggle_state(data->path)) {
		if (data->cur_pmap != data->pmap1) {
			if (data->cur_pmap)
				gtk_container_remove(data->button, data->cur_pmap);
			gtk_container_add(data->button, data->pmap1);
			data->cur_pmap = data->pmap1;
		}
	} else {
		if (data->cur_pmap != data->pmap2) {
			if (data->cur_pmap)
				gtk_container_remove(data->button, data->cur_pmap);
			gtk_container_add(data->button, data->pmap2);
			data->cur_pmap = data->pmap2;
		}
	}
}



#ifndef GNOME_USE_APP
static void set_sensitive_state(ToggleData *data)
{
	if (!data) return;
	if (menus_get_sensitive_state(data->path)) {
		gtk_widget_set_sensitive(GTK_WIDGET(data->button), 1);
		if (data->cur_pmap != data->pmap1) {
			if (data->cur_pmap)
				gtk_container_remove(data->button, data->cur_pmap);
			gtk_container_add(data->button, data->pmap1);
			data->cur_pmap = data->pmap1;
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(data->button), 0);
		if (data->cur_pmap != data->pmap2) {
			if (data->cur_pmap)
				gtk_container_remove(data->button, data->cur_pmap);
			gtk_container_add(data->button, data->pmap2);
			data->cur_pmap = data->pmap2;
		}
	}
}
#endif /* GNOME_USE_APP */



static void sigfunc_toggle(GtkWidget *w,
			   ToggleData *data)
{
	if (!data) return;
	if (!data->path) return;
	menus_activate(data->path);
	set_toggle_state(data);
}



static void add_space(gint spacing)
{
	GtkWidget *w;
	
	w = gtk_label_new(" ");
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, spacing);
}



static ToggleData *add_toggle_button(gchar **xpm, gchar **xpm2,
				     char *text,
				     gchar *path)
{
	GtkWidget *w;
	GdkPixmap *pmap;
	GdkBitmap *bmap;
	GtkStyle *style;
	ToggleData *toggle_data;
	
	toggle_data = (ToggleData *)g_malloc(sizeof(ToggleData));
	toggle_data->path = path;
	toggle_data->cur_pmap = NULL;

	style = gtk_widget_get_style(win);
	pmap = gdk_pixmap_create_from_xpm_d(win->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    xpm);
	toggle_data->pmap1 = gtk_pixmap_new(pmap, bmap);
	gtk_widget_show(toggle_data->pmap1);
	pmap = gdk_pixmap_create_from_xpm_d(win->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    xpm2);
	toggle_data->pmap2 = gtk_pixmap_new(pmap, bmap);
	gtk_widget_show(toggle_data->pmap2);
	w = gtk_button_new();
	toggle_data->button = GTK_CONTAINER(w);
	set_toggle_state(toggle_data);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 2);
	if (path) {
		gtk_signal_connect(GTK_OBJECT(w), "clicked",
				   GTK_SIGNAL_FUNC(sigfunc_toggle),
				   (gpointer *)toggle_data);
	}
	if (text)
		gtk_tooltips_set_tips(tt, w, text);
	return toggle_data;
}



static GtkButton *add_button(gchar **xpm, char *text, gchar *path)
{
	GtkWidget *w, *pixmap;
	GdkPixmap *pmap;
	GdkBitmap *bmap;
	GtkStyle *style;

	style = gtk_widget_get_style(win);
	pmap = gdk_pixmap_create_from_xpm_d(win->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    xpm);
	pixmap = gtk_pixmap_new(pmap, bmap);
	gtk_widget_show(pixmap);
	w = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(w), pixmap);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 2);
	if (path) {
		gtk_signal_connect(GTK_OBJECT(w), "clicked",
				   GTK_SIGNAL_FUNC(sigfunc),
				   (gpointer *)path);
	}
	if (text)
		gtk_tooltips_set_tips(tt, w, text);
	return GTK_BUTTON(w);
}



GtkWidget *build_toolbar(GtkWidget *window, GtkTooltips **tips)
{
	win = window;

	tt = gtk_tooltips_new();

	hbox = GTK_BOX(gtk_hbox_new(FALSE, 0));
	gtk_widget_show(GTK_WIDGET(hbox));

	add_button(tb_new_xpm, "New Project...", "<Main>/File/New Project...");
	add_space(2);
	add_button(tb_open_xpm, "Reload", "<Main>/File/Reload rc");
	add_button(tb_save_xpm, "Save", "<Main>/File/Save rc");
	add_space(2);
	add_button(tb_cut_xpm, "Cut", "<Main>/Edit/Cut");
	add_button(tb_copy_xpm, "Copy", "<Main>/Edit/Copy");
	add_button(tb_paste_xpm, "Paste", "<Main>/Edit/Paste");
	add_space(2);
	toggle_prop = add_toggle_button(tb_properties_xpm, tb_prop_dis_xpm,
					"Edit Properties",
					"<Main>/Edit/Properties...");
	toggle_timer = add_toggle_button(tb_timer_xpm, tb_timer_stopped_xpm,
					 "start/stop Timer",
					 "<Main>/Timer/Timer running");
	add_space(2);
	add_button(tb_preferences_xpm, "Edit Preferences...", "<Main>/Edit/Preferences...");
#ifdef EXTENDED_TOOLBAR
	add_button(tb_unknown_xpm, "About...", "<Main>/Help/About...");
	add_button(tb_exit_xpm, "Quit", "<Main>/File/Quit");
#endif /* EXTENDED_TOOLBAR */

	if (tips) *tips = tt;
	return GTK_WIDGET(hbox);
}



#ifdef GNOME_USE_APP
#include "menucmd.h"



static void toolbar_toggle_timer(GtkWidget *w, gpointer *data)
{
	if (main_timer == 0) {
		start_timer();
	} else {
		stop_timer();
	}
	menu_set_states();
}



static GnomeToolbarInfo tbar[] = {
	{GNOME_APP_TOOLBAR_ITEM, "New", "New Project...",
		GNOME_APP_PIXMAP_DATA, tb_new_xpm, new_project},
	{GNOME_APP_TOOLBAR_SPACE, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, NULL},
	{GNOME_APP_TOOLBAR_ITEM, "Reload", "Reload init file",
		GNOME_APP_PIXMAP_DATA, tb_open_xpm, init_project_list},
	{GNOME_APP_TOOLBAR_ITEM, "Save", "Save init file",
		GNOME_APP_PIXMAP_DATA, tb_save_xpm, save_project_list},
	{GNOME_APP_TOOLBAR_SPACE, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, NULL},
	{GNOME_APP_TOOLBAR_ITEM, "Cut", "Cut selected project",
		GNOME_APP_PIXMAP_DATA, tb_cut_xpm, cut_project},
	{GNOME_APP_TOOLBAR_ITEM, "Copy", "Copy selected project",
		GNOME_APP_PIXMAP_DATA, tb_copy_xpm, copy_project},
	{GNOME_APP_TOOLBAR_ITEM, "Paste", "Paste selected project",
		GNOME_APP_PIXMAP_DATA, tb_paste_xpm, paste_project},
	{GNOME_APP_TOOLBAR_SPACE, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, NULL},
	{GNOME_APP_TOOLBAR_ITEM, "Props", "Edit properties...",
		GNOME_APP_PIXMAP_DATA, tb_properties_xpm, menu_properties},
	{GNOME_APP_TOOLBAR_ITEM, "Timer", "Start/Stop timer",
		GNOME_APP_PIXMAP_DATA, tb_timer_xpm, toolbar_toggle_timer},
	{GNOME_APP_TOOLBAR_SPACE, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, NULL},
	{GNOME_APP_TOOLBAR_ITEM, "Prefs", "Edit preferences...",
		GNOME_APP_PIXMAP_DATA, tb_preferences_xpm, menu_options},
#ifdef EXTENDED_TOOLBAR
	{GNOME_APP_TOOLBAR_SPACE, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, NULL},
	{GNOME_APP_TOOLBAR_ITEM, "About", "About...",
		GNOME_APP_PIXMAP_DATA, tb_unknown_xpm, about_box},
	{GNOME_APP_TOOLBAR_ITEM, "Quit", "Quit " APP_NAME,
		GNOME_APP_PIXMAP_DATA, tb_exit_xpm, quit_app},
#endif /* EXTENDED_TOOLBAR */ 
	{GNOME_APP_TOOLBAR_ENDOFINFO, NULL, NULL, 0, NULL, NULL}
};



#ifndef GNOME_USE_APP
static GnomeToolbarInfo *toolbar_find(GnomeToolbarInfo *tinfo, const char *text)
{
	int i;

	g_return_val_if_fail(tinfo != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	for (i = 0; tinfo[i].type != GNOME_APP_TOOLBAR_ENDOFINFO; i++) {
		if (!tinfo[i].text) continue;
		if (0 == strcmp(tinfo[i].text, text)) return &tinfo[i];
	}
	return NULL;
}
#endif /* GNOME_USE_APP */



/* TODO: I should pass toolbar_set_states the gnome_app, too */
static GnomeApp *app = NULL;

void toolbar_create(GtkWidget *gnome_app)
{
	app = GNOME_APP(gnome_app);
	gnome_app_create_toolbar(GNOME_APP(gnome_app), tbar);
	/* gtk_toolbar_set_style(GTK_TOOLBAR(GNOME_APP(gnome_app)->toolbar), GTK_TOOLBAR_ICONS); */
	/* 
	 * TODO: GnomeApp has problems with positioning right now. So I put
	 * the toolbar at the bottom
	 */
	/* gnome_app_toolbar_set_position(GNOME_APP(gnome_app), GNOME_APP_POS_BOTTOM); */
}

#endif /* GNOME_USE_APP */



void toolbar_set_states(void)
{
#ifdef GNOME_USE_APP
	/* TODO: hmm - that doesn't work */
# if 0
	if (!app) return;
	if (app->toolbar)
		gtk_widget_destroy(app->toolbar);
	app->toolbar = NULL;
	toolbar_find(tbar, "Timer")->pixmap_info =
		(main_timer != 0) ? tb_timer_xpm : tb_timer_stopped_xpm;
	gnome_app_create_toolbar(app, tbar);
# endif
#else /* GNOME_USE_APP */ 
	if (toggle_timer) set_toggle_state(toggle_timer);
	/* TODO: remove me
	if (prop_button)
		gtk_widget_set_sensitive(GTK_WIDGET(prop_button), (cur_proj) ? 1 : 0);
	 */
	if (toggle_prop) set_sensitive_state(toggle_prop);
#endif /* GNOME_USE_APP */
}

