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
#include <libgnomeui/gnome-stock.h>
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif
#include <string.h>

#include "gtt.h"


#undef gettext
#undef _
#include <libintl.h>
#define gettext_noop(String) (String)
#define _(String) gettext(String)



#if HAS_GNOME && 1
#define WANT_STOCK
#endif

#if HAS_GNOME && 0
#define USE_HACK
#endif



#ifndef WANT_STOCK
#include "tb_new.xpm"

#include "tb_open.xpm"
#include "tb_save.xpm"

#include "tb_cut.xpm"
#include "tb_copy.xpm"
#include "tb_paste.xpm"

#include "tb_properties.xpm"
#include "tb_prop_dis.xpm"
#endif /* not WANT_STOCK */

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
#ifndef USE_HACK
        GtkBox *vbox;
#endif
};

struct _MyToolbar {
        GtkToolbar *tbar;

        GtkWidget *cut, *copy, *paste; /* to make them sensible
                                          as needed */
#ifdef WANT_STOCK
        GtkWidget *prop_w;
#else /* not WANT_STOCK */
        MyToggle *prop;
#endif /* not WANT_STOCK */
        MyToggle *timer;
};

MyToolbar *mytbar = NULL;



static void
sigfunc(GtkWidget *w,
        gpointer *data)
{
	if (!data) return;
	menus_activate((char *)data);
}



static GtkWidget *
add_button(GtkToolbar *tbar, char *text, char *tt_text,
           gchar **pmap_data, char *menu_path)
{
	GtkWidget *w, *pixmap;
	GdkPixmap *pmap;
	GdkBitmap *bmap;
	static GtkStyle *style = NULL;

        /* TODO: hmmm, I should rename some global variables some
           time. `window' is the main window of the app.
           I will be doing this at least when I have to support
           multiple app window (e.g. for the networked version) */
	if (!style) style = gtk_widget_get_style(window);
	pmap = gdk_pixmap_create_from_xpm_d(window->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    pmap_data);
	pixmap = gtk_pixmap_new(pmap, bmap);

        w = gtk_toolbar_append_item(tbar, text, tt_text, pixmap,
                                    (GtkSignalFunc)sigfunc,
                                    (gpointer *)menu_path);

	return w;
}



#ifdef WANT_STOCK
static GtkWidget *
add_stock_button(GtkToolbar *tbar, char *text, char *tt_text,
                 char *icon, char *menu_path)
{
	GtkWidget *w, *pixmap;
#if 0
        GtkWidget *button, *vbox, *label;

        button = gtk_button_new();
        gtk_widget_show(button);
        vbox = gtk_vbox_new(FALSE, 0);
        gtk_widget_show(vbox);
        gtk_container_add(GTK_CONTAINER(button), vbox);

        /* TODO: see notes above (window) */
        pixmap = gnome_stock_pixmap_widget((GtkWidget *)window, icon);
        gtk_widget_show(pixmap);
        gtk_box_pack_start(GTK_BOX(vbox), pixmap, FALSE, FALSE, 0);
        label = gtk_label_new(text);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                           (GtkSignalFunc)sigfunc,
                           (gpointer *)menu_path);
        gtk_toolbar_append_widget(tbar, tt_text, button);
        w = button;
#elif 1
        /* TODO: see notes above (window) */
	pixmap = gnome_stock_pixmap_widget((GtkWidget *)window, icon);
        w = gtk_toolbar_append_item(tbar, text, tt_text, pixmap,
                                    (GtkSignalFunc)sigfunc,
                                    (gpointer *)menu_path);
#else
        /* TODO: see notes above (window) */
	pixmap = (GtkWidget *)gnome_stock_pixmap((GtkWidget *)window, icon,
                                                 GNOME_STOCK_PIXMAP_REGULAR);
        w = gtk_toolbar_append_item(tbar, text, tt_text, pixmap,
                                    (GtkSignalFunc)sigfunc,
                                    (gpointer *)menu_path);
#endif

	return w;
}
#endif /* WANT_STOCK */


static MyToggle *
add_toggle_button(GtkToolbar *tbar, char *text, char *tt_text,
                  gchar **pmap1, gchar **pmap2, char *menu_path)
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
	pmap = gdk_pixmap_create_from_xpm_d(window->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    pmap2);
	w->pmap2 = (GtkPixmap *)gtk_pixmap_new(pmap, bmap);

#ifdef USE_HACK
        w->button = gtk_toolbar_append_item(tbar, text, tt_text,
                                            GTK_WIDGET(w->pmap1),
                                            (GtkSignalFunc)sigfunc,
                                            (gpointer *)menu_path);
#else /* not USE_HACK */
        gtk_widget_show(GTK_WIDGET(w->pmap1));
        gtk_widget_show(GTK_WIDGET(w->pmap2));
        w->vbox = (GtkBox *)gtk_vbox_new(FALSE, 0);
        gtk_widget_show((GtkWidget *)(w->vbox));
        gtk_box_pack_start(w->vbox, GTK_WIDGET(w->pmap1), FALSE, FALSE, 0);
        w->button = gtk_toolbar_append_item(tbar, text, tt_text,
                                            GTK_WIDGET(w->vbox),
                                            (GtkSignalFunc)sigfunc,
                                            (gpointer *)menu_path);
#endif /* not USE_HACK */
        w->cur_pmap = w->pmap1;

	return w;
}



#ifdef USE_HACK
/* Okay, since nobody made a GtkToolbarItem, I have to hack a lot */

typedef enum
{
  CHILD_SPACE,
  CHILD_BUTTON,
  CHILD_WIDGET
} ChildType;

typedef struct
{
  ChildType type;
  GtkWidget *widget;
  GtkWidget *icon;
  GtkWidget *label;
} Child;

static void
my_set_icon(GtkToolbar *toolbar, MyToggle *toggle, GtkPixmap *pmap)
{
        GList *gl;
        Child *child;

        if (toggle->cur_pmap == pmap) return;

        for (gl = toolbar->children; gl; gl = gl->next)
                if (((Child *)(gl->data))->widget == toggle->button) break;
        if (!gl) return;
        child = gl->data;
        if (GTK_WIDGET_VISIBLE(child->icon)) {
                gtk_widget_show(GTK_WIDGET(pmap));
        } else {
                gtk_widget_hide(GTK_WIDGET(pmap));
        }
        gtk_container_remove(GTK_CONTAINER(((GtkButton *)(child->widget))->child),
                             child->icon);
        gtk_container_remove(GTK_CONTAINER(((GtkButton *)(child->widget))->child),
                             child->label);
        child->icon = GTK_WIDGET(pmap);
        gtk_box_pack_start(GTK_BOX(((GtkButton *)(child->widget))->child),
                           child->icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(((GtkButton *)(child->widget))->child),
                           child->label, FALSE, FALSE, 0);
        toggle->cur_pmap = pmap;
}

#else /* not USE_HACK */

static void
my_set_icon(GtkToolbar *toolbar, MyToggle *toggle, GtkPixmap *pmap)
{
        if (toggle->cur_pmap == pmap) return;

        gtk_container_remove(GTK_CONTAINER(toggle->vbox), GTK_WIDGET(toggle->cur_pmap));
        gtk_box_pack_start(toggle->vbox, GTK_WIDGET(pmap), FALSE, FALSE, 0);
        toggle->cur_pmap = pmap;
}
#endif /* USE_HACK */



void
toolbar_set_states(void)
{
        extern project *cutted_project;
	GtkToolbarStyle tb_style;

        g_return_if_fail(mytbar != NULL);
        g_return_if_fail(mytbar->tbar != NULL);
        g_return_if_fail(GTK_IS_TOOLBAR(mytbar->tbar));

        if (mytbar->cut)
                gtk_widget_set_sensitive(mytbar->cut, (cur_proj != NULL));
        if (mytbar->copy)
                gtk_widget_set_sensitive(mytbar->copy, (cur_proj != NULL));
        if (mytbar->paste)
                gtk_widget_set_sensitive(mytbar->paste,
                                         (cutted_project != NULL));
#ifdef WANT_STOCK
        if (mytbar->prop_w)
                gtk_widget_set_sensitive(mytbar->prop_w, (cur_proj != NULL));
#else /* not WANT_STOCK */
        if (mytbar->prop) {
                if (cur_proj) {
                        gtk_widget_set_sensitive(GTK_WIDGET(mytbar->prop->button), 1);
                        my_set_icon(mytbar->tbar, mytbar->prop,
                                    mytbar->prop->pmap1);
                } else {
                        gtk_widget_set_sensitive(GTK_WIDGET(mytbar->prop->button), 0);
                        my_set_icon(mytbar->tbar, mytbar->prop,
                                    mytbar->prop->pmap2);
                }
        }
#endif /* not WANT_STOCK */
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



/* returns a pointer to the (still hidden) GtkToolbar */
GtkWidget *
build_toolbar(void)
{
        if (mytbar) return GTK_WIDGET(mytbar->tbar);
        mytbar = g_malloc0(sizeof(MyToolbar));
        mytbar->tbar = GTK_TOOLBAR(gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
                                                   GTK_TOOLBAR_ICONS));

#ifdef WANT_STOCK
        if (config_show_tb_new) {
                add_stock_button(mytbar->tbar, dgettext("gtt", "New"),
                                 _("New Project..."),
                                 GNOME_STOCK_PIXMAP_NEW,
                                 _("<Main>/File/New Project..."));
                gtk_toolbar_append_space(mytbar->tbar);
        }
        if (config_show_tb_file) {
                add_stock_button(mytbar->tbar, _("Reload"),
                                 _("Reload Configuration File"),
                                 GNOME_STOCK_PIXMAP_OPEN,
                                 _("<Main>/File/Reload Configuration File"));
                add_stock_button(mytbar->tbar, _("Save"),
                                 _("Save Configuration File"),
                                 GNOME_STOCK_PIXMAP_SAVE,
                                 _("<Main>/File/Save Configuration File"));
                gtk_toolbar_append_space(mytbar->tbar);
        }
        if (config_show_tb_ccp) {
                mytbar->cut = add_stock_button(mytbar->tbar, _("Cut"),
                                               _("Cut Selected Project"),
                                               GNOME_STOCK_PIXMAP_CUT,
                                               _("<Main>/Edit/Cut"));
                mytbar->copy = add_stock_button(mytbar->tbar, _("Copy"),
                                                _("Copy Selected Project"),
                                                GNOME_STOCK_PIXMAP_COPY,
                                                _("<Main>/Edit/Copy"));
                mytbar->paste = add_stock_button(mytbar->tbar, _("Paste"),
                                                 _("Paste Project"),
                                                 GNOME_STOCK_PIXMAP_PASTE,
                                                 _("<Main>/Edit/Paste"));
                gtk_toolbar_append_space(mytbar->tbar);
        }
        if (config_show_tb_prop)
                mytbar->prop_w = add_stock_button(mytbar->tbar, _("Props"),
                                                  _("Edit Properties..."),
                                                  GNOME_STOCK_PIXMAP_PROPERTIES,
                                                  _("<Main>/Edit/Properties..."));
#else /* not WANT_STOCK */
        /* TODO: If I keep this, I will have to add config_show_tb_bla
           checks */
        add_button(mytbar->tbar, dgettext("gtt", "New"),
                   _("New Project..."), tb_new_xpm,
                   _("<Main>/File/New Project..."));
        gtk_toolbar_append_space(mytbar->tbar);
	add_button(mytbar->tbar, _("Reload"), _("Reload Configuration File"),
                   tb_open_xpm,
                   _("<Main>/File/Reload Configuration File"));
	add_button(mytbar->tbar, _("Save"), _("Save Configuration File"),
                   tb_save_xpm,
                   _("<Main>/File/Save Configuration File"));
        gtk_toolbar_append_space(mytbar->tbar);
	mytbar->cut = add_button(mytbar->tbar, _("Cut"), _("Cut Selected Project"),
                               tb_cut_xpm,
                               _("<Main>/Edit/Cut"));
	mytbar->copy = add_button(mytbar->tbar, _("Copy"),
                                _("Copy Selected Project"),
                                tb_copy_xpm,
                                _("<Main>/Edit/Copy"));
	mytbar->paste = add_button(mytbar->tbar, _("Paste"), _("Paste Project"),
                                 tb_paste_xpm,
                                 _("<Main>/Edit/Paste"));
        gtk_toolbar_append_space(mytbar->tbar);
	mytbar->prop = add_toggle_button(mytbar->tbar, _("Props"),
                                       _("Edit Properties..."),
                                       tb_properties_xpm,
                                       tb_prop_dis_xpm,
                                       _("<Main>/Edit/Properties..."));
#endif /* not WANT_STOCK */
        if (config_show_tb_timer)
                mytbar->timer = add_toggle_button(mytbar->tbar, _("Timer"),
                                                  _("Start/Stop Timer"),
                                                  tb_timer_xpm,
                                                  tb_timer_stopped_xpm,
                                                  _("<Main>/Timer/Timer running"));
        if (((config_show_tb_timer) || (config_show_tb_prop)) &&
            ((config_show_tb_pref) || (config_show_tb_help) ||
             (config_show_tb_exit)))
                gtk_toolbar_append_space(mytbar->tbar);
        if (config_show_tb_pref)
                add_button(mytbar->tbar, _("Prefs"), _("Edit Preferences..."),
                           tb_preferences_xpm,
                           _("<Main>/Edit/Preferences..."));
        if (config_show_tb_help) {
#ifdef USE_GTT_HELP
                add_button(mytbar->tbar, _("Help"), _("Help Contents..."),
                           tb_unknown_xpm,
                           _("<Main>/Help/Contents..."));
#else /* not USE_GTT_HELP */
                add_button(mytbar->tbar, _("About"), _("About..."),
                           tb_unknown_xpm,
                           _("<Main>/Help/About..."));
#endif /* not USE_GTT_HELP */
        }
        if (config_show_tb_exit) {
                add_button(mytbar->tbar, _("Exit"), _("Exit GTimeTracker"),
                           tb_exit_xpm,
                           _("<Main>/File/Exit"));
        }
	return GTK_WIDGET(mytbar->tbar);
}



void
update_toolbar_sections(void)
{
        GtkWidget *tb;
        GtkWidget *w;

        if (!window) return;
        if (!mytbar) return;

        /* TODO: I still get the segfault after destroying a widget */
        w = GTK_WIDGET(mytbar->tbar)->parent;
        gtk_container_remove(GTK_CONTAINER(w), GTK_WIDGET(mytbar->tbar));
        gtk_widget_hide(GTK_WIDGET(mytbar->tbar));

        g_free(mytbar);
        mytbar = NULL;
        tb = build_toolbar();
#ifdef GNOME_USE_APP
        gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(mytbar->tbar));
        gtk_widget_show(GTK_WIDGET(tb));
#else /* not GNOME_USE_APP */
        gtk_box_pack_start(window_vbox, tb, FALSE, FALSE, 0);
        gtk_box_reorder_child(window_vbox, tb, 2);
        gtk_widget_show(tb);
#endif /* not GNOME_USE_APP */
}



#ifdef DEBUG
void
toolbar_test(void)
{
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
}
#endif /* DEBUG */
