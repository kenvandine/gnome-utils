/*
    Copyright (C) 2000 Adrian Johnston
    Copyright (C) 1998, 1999, 2000  James Henstridge <james@daa.com.au>
      GNOME option parsing code by Miguel de Icaza.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#else
#  define VERSION "0.0.2"
#endif

#include <string.h>
#include <gnome.h>
#include <glade/glade.h>
#include <popt.h>
#include "gsu-gui-api.h"

#define DEFAULT_USER	"root"
#define LOOKUP_DIE	0
#define LOOKUP_NODIE	1

void on_ok_clicked(GtkObject * object, gpointer user_data);
void on_cancel_clicked(GtkObject * object, gpointer user_data);
void on_details_clicked(GtkObject * object, gpointer user_data);
GtkWidget *lookup_widget(const gchar * widget_name, gint fail_state);

void main_window_show();
void simple_window_show();

/* The top level window(s) created by libglade */
static GladeXML *xml;
static const gchar *gladexmlfile = NULL;
static gint is_ok = 1;			/* Allow ok_clicked signal */

/* Values passed to the suid */
static gchar *global_command = NULL;	/* Implicit NULL's as well */
static gchar *global_name = NULL;
static gchar *global_password = "";

int main(int argc, char **argv)
{
	const char **list = NULL;
	static poptContext ctx;

	const struct poptOption options[] = {
		{"command", 'c', POPT_ARG_STRING, &global_command, 0,
		 N_("Command to be run as another user")},
		{"user-name", 'l', POPT_ARG_STRING, &global_name, 0,
		 N_("User name to run command as")},
		{"glade-xml-file", 'x', POPT_ARG_STRING, &gladexmlfile, 0,
		 N_("File containing interface description")},
		{NULL, '\0', 0, NULL, 0}
	};

	gnome_init_with_popt_table("gnome-su", VERSION, argc, argv, options, 0,
							   &ctx);
	glade_gnome_init();
	list = poptGetArgs(ctx);

	if (list && list[1] == NULL) {
		gladexmlfile = list[0];	/* First arg. */
	} else {
		g_print("Usage: %s -x glade-xml-file [-c command] [user-name]\n",
				argv[0]);
		exit(1);
	}

	if (!global_name && global_command) {
		global_name = DEFAULT_USER;
		simple_window_show();
	} else {
		if (!global_name)
			global_name = DEFAULT_USER;
		if (!global_command)
			global_command = "";
		main_window_show();
	}

	gtk_main();

	if (global_command) {
		/* This call creates it's own error dialogs */
		gsu_call_suid(global_command, global_name, global_password, NULL);
		return 1;
	}

	return 0;
}

void on_cancel_clicked(GtkObject * object, gpointer user_data)
{
	global_command = NULL;

	gtk_main_quit();
}

void on_details_clicked(GtkObject * object, gpointer user_data)
{
	GtkWidget *entry;

	entry = lookup_widget("password", LOOKUP_DIE);
	global_password = g_strdup(gtk_entry_get_text((GtkEntry *) entry));

	gtk_widget_hide(lookup_widget("windowsimple", LOOKUP_DIE));
	main_window_show();
}

void on_ok_clicked(GtkObject * thing, gpointer user_data)
{
	GtkWidget *entry;

	if (!is_ok)
		return;					/* The editables call this on "activate" */

	/* the name and command widgets don't appear in windowsimple */
	if ((entry = lookup_widget("command", LOOKUP_NODIE)) != NULL)
		global_command = g_strdup(gtk_entry_get_text((GtkEntry *) entry));
	if ((entry = lookup_widget("name", LOOKUP_NODIE)) != NULL)
		global_name = g_strdup(gtk_entry_get_text((GtkEntry *) entry));
	entry = lookup_widget("password", LOOKUP_DIE);
	global_password = g_strdup(gtk_entry_get_text((GtkEntry *) entry));

	gtk_main_quit();
}

void on_update_verify(GtkObject * object, gpointer user_data)
{
	GtkWidget *entry;
	/* FIXME optimize this.  Note: widgets could change. */

	is_ok = 1;

	/* Check that there is a user name and command
	 * but allow no password to be used. */
	if (((entry = lookup_widget("command", LOOKUP_NODIE)) != NULL)
		&& !strlen(gtk_entry_get_text((GtkEntry *) entry)))
		is_ok = 0;
	else if (((entry = lookup_widget("name", LOOKUP_NODIE)) != NULL)
			 && !strlen(gtk_entry_get_text((GtkEntry *) entry)))
		is_ok = 0;

	entry = lookup_widget("ok", LOOKUP_DIE);
	gtk_widget_set_sensitive(entry, is_ok);
}

GtkWidget *lookup_widget(const gchar * widget_name, gint fail_state)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget(xml, widget_name);

	if (widget == NULL && fail_state == LOOKUP_DIE) {
		g_warning("Widget not found: %s", widget_name);
		gtk_main_quit();
	}

	return widget;
}

void main_window_show()
{
	GtkWidget *entry;

	if ((xml = glade_xml_new(gladexmlfile, "windowmain")) == NULL) {
		g_warning("Window not found: %s", "windowmain");
		gtk_main_quit();
	}
	glade_xml_signal_autoconnect(xml);

	entry = lookup_widget("command", LOOKUP_DIE);
	gtk_entry_set_text((GtkEntry *) entry, global_command);

	entry = lookup_widget("name", LOOKUP_DIE);
	gtk_entry_set_text((GtkEntry *) entry, global_name);

	entry = lookup_widget("password", LOOKUP_DIE);
	gtk_entry_set_text((GtkEntry *) entry, global_password);
}

void simple_window_show()
{
	if ((xml = glade_xml_new(gladexmlfile, "windowsimple")) == NULL) {
		g_warning("Window not found: %s", "windowsimple");
		gtk_main_quit();
	}
	glade_xml_signal_autoconnect(xml);
}
