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
 
#include "gtt.h"


#define DEFBUTTON_TEST

/* XXX: this is our main window, perhaps it is a bit ugly this way and
 * should be passed around in the data fields */
extern GtkWidget *window;

static void dialog_setup(GnomeDialog *dlg, GtkBox **vbox_return)
{
	g_return_if_fail(dlg != NULL);

	/* This is set up automatically by gnome dialog */
	/* gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE); */
	/* gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER); */

	gnome_dialog_set_parent(GNOME_DIALOG(dlg), GTK_WINDOW(window));

	gnome_dialog_set_close(dlg, TRUE);

	if (vbox_return) *vbox_return = GTK_BOX(dlg->vbox);
}



void new_dialog_ok(char *title, GtkWidget **dlg, GtkBox **vbox,
		       char *s, GtkSignalFunc sigfunc, gpointer data)
{
        char tmp[256];

	g_return_if_fail( dlg != NULL );
	g_return_if_fail( vbox != NULL );

        sprintf(tmp, APP_NAME " - %s", title);
	*dlg = gnome_dialog_new(tmp, s, NULL);
	dialog_setup(GNOME_DIALOG(*dlg), vbox);
	
	if (sigfunc)
	       gnome_dialog_button_connect(GNOME_DIALOG(*dlg), 0,
					   sigfunc, data);
}


void new_dialog_ok_cancel(char *title, GtkWidget **dlg, GtkBox **vbox,
			  char *s_ok, GtkSignalFunc sigfunc, gpointer data,
			  char *s_cancel, GtkSignalFunc c_sigfunc, gpointer c_data)
{
        char tmp[256];
	
	g_return_if_fail(dlg != NULL);
	g_return_if_fail(s_ok != NULL);
	g_return_if_fail(s_cancel != NULL);
	g_return_if_fail(title != NULL);

        sprintf(tmp, APP_NAME " - %s", title);
	*dlg = gnome_dialog_new(tmp, s_ok, s_cancel, NULL);
	dialog_setup(GNOME_DIALOG(*dlg), vbox);

	if (sigfunc)
		gnome_dialog_button_connect(GNOME_DIALOG(*dlg), 0,
					    sigfunc, data);

	if (c_sigfunc)
		gnome_dialog_button_connect(GNOME_DIALOG(*dlg), 1,
					    c_sigfunc, c_data);

	gnome_dialog_set_default(GNOME_DIALOG(*dlg), 0);
}


void msgbox_ok(char *title, char *text, char *ok_text,
	       GtkSignalFunc func)
{
        char s[256];

	GtkWidget *mbox;

        sprintf(s, APP_NAME " - %s", title);
        mbox = gnome_message_box_new(text, GNOME_MESSAGE_BOX_GENERIC, ok_text, NULL, NULL);

	gtk_signal_connect(GTK_OBJECT(mbox), "clicked",
			   func, NULL);
        gtk_window_set_title(GTK_WINDOW(mbox), s);
	gnome_dialog_set_parent(GNOME_DIALOG(mbox), GTK_WINDOW(window));
	gtk_widget_show(mbox);
}



void msgbox_ok_cancel(char *title, char *text,
		      char *ok_text, char *cancel_text,
		      GtkSignalFunc func)
{
        char s[256];

	GtkWidget *mbox;

        sprintf(s, APP_NAME " - %s", title);
	mbox = gnome_message_box_new(text, GNOME_MESSAGE_BOX_GENERIC, ok_text, cancel_text, NULL);
	gnome_dialog_set_default(GNOME_DIALOG(mbox), 1);
	gtk_signal_connect(GTK_OBJECT(mbox), "clicked",
			   func, NULL);
        gtk_window_set_title(GTK_WINDOW(mbox), s);
	gnome_dialog_set_parent(GNOME_DIALOG(mbox), GTK_WINDOW(window));
	gtk_widget_show(mbox);
}

