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



static GnomeAppClass *parent_class = NULL;



static void gtt_help_destroy(GtkObject *object)
{
	GttHelp *help;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTT_IS_HELP (object));

	help = GTT_HELP (object);
	
	/* free GttHelp resources */
	if (help->html_source) g_free(help->html_source);
	if (help->home) g_free(help->home);
	/* TODO: verify that children of GnomeApp
	 * (that is GtkWindow), will be destroyed */

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(help));
}



static void gtt_help_class_init(GttHelpClass *helpclass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(helpclass);
	object_class->destroy = gtt_help_destroy;
}



static void xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs)
{
	gtt_help_goto(GTT_HELP(w), cbs->href);
}




static int my_false(GtkWidget *w, gpointer *data)
{
	return FALSE;
}


void gtt_help_on_help(GttHelp *help)
{
	gtk_xmhtml_source(help->gtk_xmhtml,
			  "<html><head><title>Not implemented yet</title></head>\n"
			  "<body><h3>Not implemented yet</h3></body>\n");
}


#define USE_HELP_MENU
#define USE_HELP_TOOLBAR

#if defined(USE_HELP_MENU) || defined(USE_HELP_TOOLBAR)
static void help_contents(GtkWidget *w, gpointer *data)
{
	gtt_help_goto(GTT_HELP(w), GTT_HELP(w)->home);
}
#endif /* defined(USE_HELP_MENU) || defined(USE_HELP_TOOLBAR) */


#ifdef USE_HELP_TOOLBAR
static GnomeToolbarInfo tb_info[] = {
};
#endif

static void gtt_help_init(GttHelp *help)
{
	GtkWidget *w, *menu, *menubar;
	GtkAcceleratorTable *accel;

	/* init struct _GttHelp */
	gtk_signal_connect(GTK_OBJECT(GTK_WINDOW(help)), "delete_event",
			   (GtkSignalFunc)my_false, NULL);

#ifdef USE_HELP_MENU
	accel = gtk_accelerator_table_new();
	menubar = gtk_menu_bar_new();
	gtk_widget_show(menubar);
	menu = gtk_menu_new();
	w = gtk_menu_item_new_with_label("Show Contents");
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(help_contents),
				  GTK_OBJECT(help));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	gtk_widget_install_accelerator(w, accel, "activate",
				       'H', GDK_CONTROL_MASK);
	w = gtk_menu_item_new_with_label("Close Help Window");
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
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
#endif /* USE_HELP_MENU */
	
	gtk_window_add_accelerator_table(GTK_WINDOW(help), accel);

	help->html_source = help->home = NULL;
	help->document_path[0] = 0;

	help->gtk_xmhtml = GTK_XMHTML(gtk_xmhtml_new());
	gtk_signal_connect_object(GTK_OBJECT(help->gtk_xmhtml), "activate",
			   (GtkSignalFunc)xmhtml_activate, GTK_OBJECT(help));
	gtk_widget_show(GTK_WIDGET(help->gtk_xmhtml));
	gnome_app_set_contents(GNOME_APP(help), GTK_WIDGET(help->gtk_xmhtml));
}



guint gtt_help_get_type(void)
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
			(GtkArgGetFunc) NULL,
		};
		GttHelp_type = gtk_type_unique(gnome_app_get_type(), &GttHelp_info);
		parent_class = gtk_type_class(gnome_app_get_type());
	}
	return GttHelp_type;
}



GtkWidget *gtt_help_new(char *title, char *filename)
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
	
	gtk_widget_set_usize(GTK_WIDGET(help), 400, 300);

	return w;
}



static void jump_to_anchor(GtkXmHTML *w, char *html, char *anchor)
{
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
}



void gtt_help_goto(GttHelp *help, char *filename)
{
	FILE *f;
	char *str, *tmp, s[1024], *anchor, *path;

	g_return_if_fail(help != NULL);
	g_return_if_fail(GTT_IS_HELP(help));
	g_return_if_fail(filename != NULL);

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
	if ((path[strlen(path) - 1] == '/') ||
	    (path[0] == 0)) {
		if (!str) return;
		/* just jump to the anchor */
		jump_to_anchor(help->gtk_xmhtml, 
			       str,
			       anchor);
		return;
	}
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

