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
#include "gtt-features.h"
#ifdef USE_GTT_HELP
#include "gtthelp.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#include "tb_home_.xpm"
#include "tb_back_.xpm"
#include "tb_forward_.xpm"
#include "tb_unknown.xpm"
#include "tb_exit.xpm"



#ifdef DEBUG
#define CFG_SECTION "/gtt-DEBUG/Help"
#else
#define CFG_SECTION "/gtt/Help"
#endif



/*******************/
/* history helpers */
/*******************/


static void
gtt_help_history_add(GttHelp *help, const char *path, const char *anchor)
{
        char *s;
        int len, i;

        len = strlen(path);
        if (anchor) {
                if (anchor[0])
                        len += strlen(anchor) + 1;
        }
        s = g_malloc(len + 1);
        strcpy(s, path);
        if (anchor) {
                if (anchor[0]) {
                        strcat(s, "#");
                        strcat(s, anchor);
                }
        }
        if (help->history_pos > 0) {
                if (0 == strcmp(help->history[help->history_pos - 1], s)) {
                        g_free(s);
                        return;
                }
        }
        if (help->history_pos == GTT_HELP_MAX_HISTORY) {
                for (i = 1; i < GTT_HELP_MAX_HISTORY; i++)
                        help->history[i-1] = help->history[i];
                help->history_pos--;
                help->history[help->history_pos] = NULL;
        }
        if (help->history[help->history_pos]) {
                if (0 == strcmp(help->history[help->history_pos], s)) {
                        g_free(s);
                        help->history_pos++;
                        return;
                }
                g_free(help->history[help->history_pos]);
        }
        help->history[help->history_pos] = s;
        help->history_pos++;
        for (i = help->history_pos; i < GTT_HELP_MAX_HISTORY; i++) {
                if (help->history[i]) {
                        g_free(help->history[i]);
                        help->history[i] = NULL;
                }
        }
}



static const char *
gtt_help_history_back(GttHelp *help)
{
        if (help->history_pos <= 1) return NULL;
        help->history_pos -= 2;
        return help->history[help->history_pos];
}



static const char *
gtt_help_history_forward(GttHelp *help)
{
        if (help->history_pos == GTT_HELP_MAX_HISTORY) return NULL;
        return help->history[help->history_pos];
}



static gint
gtt_help_history_is_prev(GttHelp *help)
{
        return (help->history_pos > 1);
}



static gint
gtt_help_history_is_next(GttHelp *help)
{
        if (help->history_pos == GTT_HELP_MAX_HISTORY) return 0;
        return (NULL != help->history[help->history_pos]);
}



/****************/
/* widget stuff */
/****************/


static GnomeAppClass *parent_class = NULL;

static void gtt_help_update(GttHelp *help);


static void
gtt_help_destroy(GtkObject *object)
{
	GttHelp *help;
        int i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTT_IS_HELP (object));

	help = GTT_HELP (object);

	/* free GttHelp resources */
	if (help->html_source) g_free(help->html_source);
	if (help->home) g_free(help->home);
        for (i = 0; i < GTT_HELP_MAX_HISTORY; i++) {
                if (help->history[i]) g_free(help->history[i]);
        }

 	if (GTK_OBJECT_CLASS(parent_class)->destroy)
 		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(help));
}



static void
gtt_help_class_init(GttHelpClass *helpclass)
{
        GnomeStockPixmapEntry *entry;
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(helpclass);
	object_class->destroy = gtt_help_destroy;
        entry = g_malloc(sizeof(GnomeStockPixmapEntry));
        entry->type = GNOME_STOCK_PIXMAP_TYPE_DATA;
        entry->data.xpm_data = tb_back_xpm;
        gnome_stock_pixmap_register("GttHelp_Back", GNOME_STOCK_PIXMAP_REGULAR,
                                    entry);
        entry = g_malloc(sizeof(GnomeStockPixmapEntry));
        entry->type = GNOME_STOCK_PIXMAP_TYPE_DATA;
        entry->data.xpm_data = tb_forward_xpm;
        gnome_stock_pixmap_register("GttHelp_Forward",
                                    GNOME_STOCK_PIXMAP_REGULAR,
                                    entry);
}



static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs)
{
	gtt_help_goto(GTT_HELP(w), cbs->href);
}




static int
my_false(GtkWidget *w, gpointer *data)
{
	return FALSE;
}


void
gtt_help_on_help(GttHelp *help)
{
        gtt_help_goto(help, "<help>");
}


#define USE_HELP_MENU
#define USE_HELP_TOOLBAR

#if defined(USE_HELP_MENU) || defined(USE_HELP_TOOLBAR)
static void
help_contents(GtkWidget *w, gpointer *data)
{
	gtt_help_goto(GTT_HELP(w), GTT_HELP(w)->home);
}

static void
help_back(GtkWidget *w, gpointer *data)
{
        g_return_if_fail(gtt_help_history_is_prev(GTT_HELP(w)));
        gtt_help_goto(GTT_HELP(w), gtt_help_history_back(GTT_HELP(w)));
}

static void
help_forward(GtkWidget *w, gpointer *data)
{
        g_return_if_fail(gtt_help_history_is_next(GTT_HELP(w)));
        gtt_help_goto(GTT_HELP(w), gtt_help_history_forward(GTT_HELP(w)));
}
#endif /* defined(USE_HELP_MENU) || defined(USE_HELP_TOOLBAR) */


static void
gtt_help_init_menu(GttHelp *help)
{
#ifdef USE_HELP_MENU
	GtkWidget *w, *menu, *menubar;
	GtkAcceleratorTable *accel;

	accel = gtk_accelerator_table_new();
	menubar = gtk_menu_bar_new();
	gtk_widget_show(menubar);

	menu = gtk_menu_new();
#if 1
        w = gnome_stock_menu_item(GNOME_STOCK_MENU_EXIT, "Close Help Window");
#else
	w = gtk_menu_item_new_with_label("Close Help Window");
#endif
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gtt_help_close),
				  GTK_OBJECT(help));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'W', GDK_CONTROL_MASK);
	w = gtk_menu_item_new_with_label("File");
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

	menu = gtk_menu_new();
	w = gtk_menu_item_new_with_label("Show Contents");
        help->menu_contents = w;
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(help_contents),
				  GTK_OBJECT(help));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'H', GDK_CONTROL_MASK);
	w = gtk_menu_item_new_with_label("Back");
        help->menu_back = w;
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(help_back),
				  GTK_OBJECT(help));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'B', GDK_CONTROL_MASK);
	w = gtk_menu_item_new_with_label("Forward");
        help->menu_forward = w;
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(help_forward),
				  GTK_OBJECT(help));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'F', GDK_CONTROL_MASK);
	w = gtk_menu_item_new_with_label("Go");
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

#ifdef USE_HELP_TOOLBAR
	menu = gtk_menu_new();
	w = gtk_check_menu_item_new_with_label("Toolbar Icons");
        gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(w),
                                      gnome_config_get_bool(CFG_SECTION "/ShowIcons=true"));
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gtt_help_update),
				  GTK_OBJECT(help));
        help->tbar_icon = GTK_CHECK_MENU_ITEM(w);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'H', GDK_MOD1_MASK);
	w = gtk_check_menu_item_new_with_label("Toolbar Texts");
        gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(w),
                                      gnome_config_get_bool(CFG_SECTION "/ShowTexts=true"));
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gtt_help_update),
				  GTK_OBJECT(help));
        help->tbar_text = GTK_CHECK_MENU_ITEM(w);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	w = gtk_check_menu_item_new_with_label("Show Tooltips");
        gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(w),
                                      gnome_config_get_bool(CFG_SECTION "/ShowTips=true"));
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gtt_help_update),
				  GTK_OBJECT(help));
        help->tbar_tooltips = GTK_CHECK_MENU_ITEM(w);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	w = gtk_menu_item_new_with_label("Options");
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);
#endif

	menu = gtk_menu_new();
	w = gtk_menu_item_new_with_label("Help on Help");
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gtt_help_on_help),
				  GTK_OBJECT(help));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'H', GDK_MOD1_MASK);
	w = gtk_menu_item_new_with_label("Help");
	gtk_widget_show(w);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(w));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

	gnome_app_set_menus(GNOME_APP(help), GTK_MENU_BAR(menubar));

	gtk_window_add_accelerator_table(GTK_WINDOW(help), accel);
#endif /* USE_HELP_MENU */
}


static void
gtt_help_init_toolbar(GttHelp *help)
{
#ifdef USE_HELP_TOOLBAR
        GtkWidget *w, *pixmap;
        GtkToolbar *tbar;

        tbar = (GtkToolbar *)gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
                                             GTK_TOOLBAR_BOTH);

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(help),
                                              GTK_WIDGET(tbar),
                                              tb_home_xpm);
        w = gtk_toolbar_append_item(tbar, "Contents", "Show Contents",
                                    pixmap, NULL, NULL);
        help->tbar_contents = w;
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_contents),
                                  GTK_OBJECT(help));

        gtk_toolbar_append_space(tbar);

#if 1
        pixmap = gnome_stock_pixmap_widget(GTK_WIDGET(tbar), "GttHelp_Back");
#else
        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(help),
                                              GTK_WIDGET(tbar),
                                              tb_back_xpm);
#endif
        w = gtk_toolbar_append_item(tbar, "Back",
                                    "Go to the previouse location "
                                    "in history list",
                                    pixmap, NULL, NULL);
        help->tbar_back = w;
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_back),
                                  GTK_OBJECT(help));

#if 1
        pixmap = gnome_stock_pixmap_widget(GTK_WIDGET(tbar),
                                           "GttHelp_Forward");
#else
        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(help),
                                              GTK_WIDGET(tbar),
                                              tb_forward_xpm);
#endif
        w = gtk_toolbar_append_item(tbar, "Forward",
                                    "Go to the next location in history list",
                                    pixmap, NULL, NULL);
        help->tbar_forward = w;
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_forward),
                                  GTK_OBJECT(help));

        gtk_toolbar_append_space(tbar);

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(help),
                                              GTK_WIDGET(tbar),
                                              tb_unknown_xpm);
        w = gtk_toolbar_append_item(tbar, "Help", "Help on Help",
                                    pixmap, NULL, NULL);
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(gtt_help_on_help),
                                  GTK_OBJECT(help));

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(help),
                                              GTK_WIDGET(tbar),
                                              tb_exit_xpm);
        w = gtk_toolbar_append_item(tbar, "Close", "Close Help Window",
                                    pixmap, NULL, NULL);
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(gtk_widget_hide),
                                  GTK_OBJECT(help));

        gnome_app_set_toolbar(GNOME_APP(help), tbar);

        help->toolbar = tbar;
#endif /* USE_HELP_TOOLBAR */
}


static void
gtt_help_init(GttHelp *help)
{
        int i;

	/* init struct _GttHelp */
	gtk_signal_connect(GTK_OBJECT(GTK_WINDOW(help)), "delete_event",
			   (GtkSignalFunc)my_false, NULL);

        help->history_pos = 0;
        for (i = 0; i < GTT_HELP_MAX_HISTORY; i++) {
                help->history[i] = NULL;
        }
        help->menu_contents = NULL;
        help->menu_back = NULL;
        help->menu_forward = NULL;
        help->tbar_contents = NULL;
        help->tbar_back = NULL;
        help->tbar_forward = NULL;
	help->html_source = help->home = NULL;
	help->document_path[0] = 0;

        gtt_help_init_menu(help);
        gtt_help_init_toolbar(help);
	
	help->gtk_xmhtml = GTK_XMHTML(gtk_xmhtml_new());
	gtk_signal_connect_object(GTK_OBJECT(help->gtk_xmhtml), "activate",
			   (GtkSignalFunc)xmhtml_activate, GTK_OBJECT(help));
	gtk_widget_show(GTK_WIDGET(help->gtk_xmhtml));
	gnome_app_set_contents(GNOME_APP(help), GTK_WIDGET(help->gtk_xmhtml));
}



guint
gtt_help_get_type(void)
{
	static guint GttHelp_type = 0;
	if (!GttHelp_type) {
		GtkTypeInfo GttHelp_info = {
			"GttHelp",
			sizeof(GttHelp),
			sizeof(GttHelpClass),
			(GtkClassInitFunc) gtt_help_class_init,
			(GtkObjectInitFunc) gtt_help_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};
		GttHelp_type = gtk_type_unique(gnome_app_get_type(), &GttHelp_info);
		parent_class = gtk_type_class(gnome_app_get_type());
	}
	return GttHelp_type;
}



guint
gtt_help_close(GttHelp *w)
{
        gtk_widget_hide(GTK_WIDGET(w));
        return FALSE;
}



static void
gtt_help_update(GttHelp *help)
{
        if (help->menu_contents)
                gtk_widget_set_sensitive(help->menu_contents,
                                         (help->home != NULL));
        if (help->menu_back)
                gtk_widget_set_sensitive(help->menu_back,
                                         gtt_help_history_is_prev(help));
        if (help->menu_forward)
                gtk_widget_set_sensitive(help->menu_forward,
                                         gtt_help_history_is_next(help));
        if ((help->tbar_icon) && (help->tbar_text)) {
                if ((help->tbar_icon->active) &&
                    (help->tbar_text->active)) {
                        gtk_toolbar_set_style(help->toolbar,
                                              GTK_TOOLBAR_BOTH);
                } else if ((!help->tbar_icon->active) &&
                           (help->tbar_text->active)) {
                        gtk_toolbar_set_style(help->toolbar,
                                              GTK_TOOLBAR_TEXT);
                } else {
                        gtk_toolbar_set_style(help->toolbar,
                                              GTK_TOOLBAR_ICONS);
                }
                gnome_config_set_bool(CFG_SECTION "/ShowIcons",
                                      help->tbar_icon->active);
                gnome_config_set_bool(CFG_SECTION "/ShowTexts",
                                      help->tbar_text->active);
        }
        if (help->tbar_contents)
                gtk_widget_set_sensitive(help->tbar_contents,
                                         (help->home != NULL));
        if (help->tbar_back)
                gtk_widget_set_sensitive(help->tbar_back,
                                         gtt_help_history_is_prev(help));
        if (help->tbar_forward)
                gtk_widget_set_sensitive(help->tbar_forward,
                                         gtt_help_history_is_next(help));
        if (help->tbar_tooltips) {
                gtk_toolbar_set_tooltips(help->toolbar,
                                         help->tbar_tooltips->active);
                gnome_config_set_bool(CFG_SECTION "/ShowTips",
                                      help->tbar_tooltips->active);
        }
}



GtkWidget *
gtt_help_new(char *title, const char *filename)
{
	GtkWidget *w;
	GttHelp *help;

	w = gtk_type_new(gtt_help_get_type());
	help = GTT_HELP(w);
 	if (title)
		gtk_window_set_title(GTK_WINDOW(w), title);
	if (filename) {
		gtt_help_goto(help, filename);
		help->home = g_strdup(filename);
	}

        gtk_signal_connect(GTK_OBJECT(help), "delete_event",
                           GTK_SIGNAL_FUNC(gtt_help_close), NULL);
	gtk_widget_set_usize(GTK_WIDGET(help), 400, 300);

        gtt_help_update(help);

	return w;
}



static void
jump_to_anchor(GtkXmHTML *w, char *html, char *anchor)
{
        /* Okay, after reading much of the sources of gtk-XmHTML, I
           found the right function to call for this. But it seems not
           to be implemented yet. To let the user see, that we're
           trying to do something, I use my improper set_topline */
#if 0
        XmHTMLAnchorScrollToName(GTK_WIDGET(w), anchor);
#else
	/* TODO: this doesn't seem to work properly */
	char *p1, *p2, *s;
	int line = 0;
	
	if (!anchor) return;
	p1 = html;
	p2 = NULL;
	s = g_malloc(strlen(anchor) + 10);
	sprintf(s, "name=\"%s\"", anchor);
	while (*p1) {
		if (*p1 == '\n') {
			line++;
			p1++;
			continue;
		}
		if (*p1 == '<') {
			p1++;
			if ((*p1 == 'a') || (*p1 == 'A')) {
				p2 = s;
			}
		} else if (*p1 == '>') {
			p2 = NULL;
		}
		if (p2) {
			if (*p1 == *p2) {
				p2++;
				if (*p2 == 0) {
					/* found the line */
					gtk_xmhtml_set_topline(w, line);
					break;
				}
			} else {
				p2 = s;
			}
		}
		p1++;
	}
	g_free(s);
#endif
}



void
gtt_help_goto(GttHelp *help, const char *filename)
{
	FILE *f;
	char *str, *tmp, s[1024], *anchor, *path;

	g_return_if_fail(help != NULL);
	g_return_if_fail(GTT_IS_HELP(help));
	g_return_if_fail(filename != NULL);

        if (0 == strcmp(filename, "<help>")) {
                gtt_help_history_add(help, "<help>", NULL);
                gtt_help_update(help);
                gtk_xmhtml_source(help->gtk_xmhtml,
                                  "<html><head><title>Not implemented yet"
                                  "</title></head>\n"
                                  "<body><h3>Not implemented yet"
                                  "</h3></body>\n");
                return;
        }

	path = help->document_path;
	str = help->html_source;
	/* TODO: parse filename for '..' */
	if (filename[0] == '/') {
		strcpy(path, filename);
	} else if (strrchr(path, '/')) {
		strcpy(strrchr(path, '/') + 1, filename);
	} else {
		strcpy(path, filename);
	}
	/* check for '#' */
	if (NULL != (anchor = strrchr(path, '#'))) {
		*anchor = 0;
		anchor++;
	}
        /* TODO: jump to a "#anchor" in the same document should work */
	if ((path[strlen(path) - 1] == '/') ||
	    (path[0] == 0)) {
		if (!str) return;
		/* just jump to the anchor */
		jump_to_anchor(help->gtk_xmhtml, 
			       str,
			       anchor);
		return;
	}
        gtt_help_history_add(help, path, anchor);
        gtt_help_update(help);
	errno = 0;
	f = fopen(path, "rt");
	if ((errno) || (!f)) {
		if (f) fclose(f);
		gtk_xmhtml_source(help->gtk_xmhtml,
				  "<body><h2>Error: file not found</h2></body>");
		return;
	}
	if (str) g_free(str);
	str = g_malloc(1);
	*str = 0;
	while ((!feof(f)) && (!errno)) {
		if (!fgets(s, 1023, f)) continue;
		tmp = str;
		str = g_malloc(strlen(tmp) + strlen(s) + 1);
		strcpy(str, tmp);
		strcat(str, s);
		g_free(tmp);
	}
	fclose(f);
	if (errno) {
		g_warning("gtt_help_goto: error reading file \"%s\".\n", path);
		errno = 0;
	}
	gtk_xmhtml_source(help->gtk_xmhtml, str);
	jump_to_anchor(help->gtk_xmhtml,
		       str,
		       anchor);
}


#endif /* USE_GTT_HELP */

