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


#include "tb_timer.xpm"
#include "tb_timer_stopped.xpm"

#include "tb_preferences.xpm"
#include "tb_unknown.xpm"
#include "tb_exit.xpm"



typedef struct _MyToggle MyToggle;
typedef struct _MyToolbar MyToolbar;

struct _MyToggle {
        GtkWidget *button;
        GtkPixmap *pmap1, *pmap2, *cur_pmap;
        GtkBox *vbox;
};

struct _MyToolbar {
        GtkToolbar *tbar;
        GtkTooltips *tt;
        GtkWidget *cut, *copy, *paste; /* to make them sensible
                                          as needed */
        GtkWidget *prop_w;
        MyToggle *timer;
};

MyToolbar *mytbar = NULL;



static GtkWidget *
add_button(GtkToolbar *tbar, char *text, char *tt_text,
           gchar **pmap_data, GtkSignalFunc sigfunc)
{
	GtkWidget *w, *pixmap;

        /*
	 * TODO: hmmm, I should rename some global variables some time.
	 * `window' is the main window of the app. I will be doing this at
	 * least when I have to support multiple app window (e.g. for the
	 * networked version)
	 */
        pixmap = gnome_create_pixmap_widget_d((GtkWidget *)window,
                                              (GtkWidget *)tbar,
                                              pmap_data);

        w = gtk_toolbar_append_item(tbar, text, tt_text, NULL, pixmap,
                                    sigfunc, NULL);

	return w;
}



static GtkWidget *
add_stock_button(GtkToolbar *tbar, char *text, char *tt_text,
                 char *icon, GtkSignalFunc sigfunc)
{
	GtkWidget *w, *pixmap;
        /* TODO: see notes above (window) */
	pixmap = gnome_stock_pixmap_widget((GtkWidget *)window, icon);
	w = gtk_toolbar_append_item(tbar, text, tt_text, NULL, pixmap,
				    sigfunc, NULL);
	return w;
}


static MyToggle *
add_toggle_button(GtkToolbar *tbar, char *text, char *tt_text,
                  gchar **pmap1, gchar **pmap2, GtkSignalFunc sigfunc)
{
	MyToggle *w;
	GdkPixmap *pmap;
	GdkBitmap *bmap;
	static GtkStyle *style = NULL;

        w = g_malloc(sizeof(MyToggle));

        /* TODO: see notes above (window) */
	if (!style) style = gtk_widget_get_style(window);
	pmap = gdk_pixmap_create_from_xpm_d(window->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    pmap1);
	w->pmap1 = (GtkPixmap *)gtk_pixmap_new(pmap, bmap);
	gtk_widget_ref(GTK_WIDGET(w->pmap1));
	pmap = gdk_pixmap_create_from_xpm_d(window->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    pmap2);
	w->pmap2 = (GtkPixmap *)gtk_pixmap_new(pmap, bmap);
	gtk_widget_ref(GTK_WIDGET(w->pmap2));

        gtk_widget_show(GTK_WIDGET(w->pmap1));
        gtk_widget_show(GTK_WIDGET(w->pmap2));
        w->vbox = (GtkBox *)gtk_vbox_new(FALSE, 0);
        gtk_widget_show((GtkWidget *)(w->vbox));
        gtk_box_pack_start(w->vbox, GTK_WIDGET(w->pmap1), FALSE, FALSE, 0);
        w->button = gtk_toolbar_append_item(tbar, text, tt_text, NULL,
                                            GTK_WIDGET(w->vbox),
                                            sigfunc, NULL);
        w->cur_pmap = w->pmap1;

	return w;
}



static void
my_set_icon(GtkToolbar *toolbar, MyToggle *toggle, GtkPixmap *pmap)
{
        if (toggle->cur_pmap == pmap) return;

        gtk_container_remove(GTK_CONTAINER(toggle->vbox), GTK_WIDGET(toggle->cur_pmap));
        gtk_box_pack_start(toggle->vbox, GTK_WIDGET(pmap), FALSE, FALSE, 0);
        toggle->cur_pmap = pmap;
}



void
toolbar_set_states(void)
{
        extern project *cutted_project;
	GtkToolbarStyle tb_style;

        g_return_if_fail(mytbar != NULL);
        g_return_if_fail(mytbar->tbar != NULL);
        g_return_if_fail(GTK_IS_TOOLBAR(mytbar->tbar));

        if (mytbar->tt) {
                if (config_show_tb_tips)
                        gtk_tooltips_enable(mytbar->tt);
                else
                        gtk_tooltips_disable(mytbar->tt);
        }
        if (mytbar->cut)
                gtk_widget_set_sensitive(mytbar->cut, (cur_proj != NULL));
        if (mytbar->copy)
                gtk_widget_set_sensitive(mytbar->copy, (cur_proj != NULL));
        if (mytbar->paste)
                gtk_widget_set_sensitive(mytbar->paste,
                                         (cutted_project != NULL));
        if (mytbar->prop_w)
                gtk_widget_set_sensitive(mytbar->prop_w, (cur_proj != NULL));
        if (mytbar->timer)
                my_set_icon(mytbar->tbar, mytbar->timer,
                            (main_timer != 0) ?
                            mytbar->timer->pmap1 :
                            mytbar->timer->pmap2);

	if ((config_show_tb_icons) && (config_show_tb_texts)) {
		tb_style = GTK_TOOLBAR_BOTH;
	} else if ((!config_show_tb_icons) && (config_show_tb_texts)) {
		tb_style = GTK_TOOLBAR_TEXT;
        } else {
		tb_style = GTK_TOOLBAR_ICONS;
        }
	gtk_toolbar_set_style(mytbar->tbar, tb_style);
}



static void
toolbar_help(GtkWidget *widget, gpointer data)
{
	char *s, *t;
	
	t = gnome_help_file_path("gtt", "index.html");
	s = g_copy_strings("file:///", t, NULL);
	g_free(t);
	gnome_help_goto(NULL, s);
	g_free(s);
}



/* returns a pointer to the (still hidden) GtkToolbar */
GtkWidget *
build_toolbar(void)
{
        if (mytbar) return GTK_WIDGET(mytbar->tbar);
        mytbar = g_malloc0(sizeof(MyToolbar));
        mytbar->tbar = GTK_TOOLBAR(gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
                                                   GTK_TOOLBAR_ICONS));
        mytbar->tt = mytbar->tbar->tooltips;

        if (config_show_tb_new) {
                /* Note to translators: just skip the `[GTT]' */
                add_stock_button(mytbar->tbar, gtt_gettext(_("[GTT]New")),
                                 _("New Project..."),
                                 GNOME_STOCK_PIXMAP_NEW,
                                 (GtkSignalFunc)new_project);
                gtk_toolbar_append_space(mytbar->tbar);
        }
        if (config_show_tb_file) {
                add_stock_button(mytbar->tbar, _("Reload"),
                                 _("Reload Configuration File"),
                                 GNOME_STOCK_PIXMAP_OPEN,
                                 (GtkSignalFunc)init_project_list);
                add_stock_button(mytbar->tbar, _("Save"),
                                 _("Save Configuration File"),
                                 GNOME_STOCK_PIXMAP_SAVE,
                                 (GtkSignalFunc)save_project_list);
                gtk_toolbar_append_space(mytbar->tbar);
        }
        if (config_show_tb_ccp) {
                mytbar->cut = add_stock_button(mytbar->tbar, _("Cut"),
                                               _("Cut Selected Project"),
                                               GNOME_STOCK_PIXMAP_CUT,
                                               (GtkSignalFunc)cut_project);
                mytbar->copy = add_stock_button(mytbar->tbar, _("Copy"),
                                                _("Copy Selected Project"),
                                                GNOME_STOCK_PIXMAP_COPY,
                                                (GtkSignalFunc)copy_project);
                mytbar->paste = add_stock_button(mytbar->tbar, _("Paste"),
                                                 _("Paste Project"),
                                                 GNOME_STOCK_PIXMAP_PASTE,
                                                 (GtkSignalFunc)paste_project);
                gtk_toolbar_append_space(mytbar->tbar);
        }
        if (config_show_tb_prop)
                mytbar->prop_w = add_stock_button(mytbar->tbar, _("Props"),
                                                  _("Edit Properties..."),
                                                  GNOME_STOCK_PIXMAP_PROPERTIES,
						  (GtkSignalFunc)menu_properties);
        if (config_show_tb_timer)
                mytbar->timer = add_toggle_button(mytbar->tbar, _("Timer"),
                                                  _("Start/Stop Timer"),
                                                  tb_timer_xpm,
                                                  tb_timer_stopped_xpm,
                                                  (GtkSignalFunc)menu_toggle_timer);
        if (((config_show_tb_timer) || (config_show_tb_prop)) &&
            ((config_show_tb_pref) || (config_show_tb_help) ||
             (config_show_tb_exit)))
                gtk_toolbar_append_space(mytbar->tbar);
        if (config_show_tb_pref)
                add_button(mytbar->tbar, _("Prefs"), _("Edit Preferences..."),
                           tb_preferences_xpm,
                           (GtkSignalFunc)menu_options);
        if (config_show_tb_help) {
		add_button(mytbar->tbar, _("Manual"), _("Manual..."),
			   tb_unknown_xpm,
			   (GtkSignalFunc)toolbar_help);
        }
        if (config_show_tb_exit) {
                add_button(mytbar->tbar, _("Exit"), _("Exit GTimeTracker"),
                           tb_exit_xpm,
                           (GtkSignalFunc)quit_app);
        }
	return GTK_WIDGET(mytbar->tbar);
}



/* TODO: I have to completely rebuild the toolbar, when I want to add or
   remove items. There should be a better way now */
void
update_toolbar_sections(void)
{
        GtkWidget *tb;
        GtkWidget *w;

        if (!window) return;
        if (!mytbar) return;

        /* TODO: I still get the segfault after destroying a widget */
        w = GTK_WIDGET(mytbar->tbar)->parent;
	if (w) {
		gtk_container_remove(GTK_CONTAINER(w), GTK_WIDGET(mytbar->tbar));
		gtk_widget_hide(GTK_WIDGET(mytbar->tbar));
	}

        g_free(mytbar);
        mytbar = NULL;
        tb = build_toolbar();
        gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(mytbar->tbar));
        gtk_widget_show(GTK_WIDGET(tb));
}



#ifdef DEBUG
void
toolbar_test(void)
{
#ifdef WANT_STOCK
        static shown = 1;

        if (!mytbar) return;
        if (!mytbar->prop_w) return;
        if (shown) {
                gtk_widget_hide(mytbar->prop_w);
                shown = 0;
        } else {
                gtk_widget_show(mytbar->prop_w);
                shown = 1;
        }
#endif
}
#endif /* DEBUG */
