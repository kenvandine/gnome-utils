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

#include "dialog.h"
#include "gtt-features.h"



void new_dialog(char *title, GtkWidget **dlg, GtkBox **vbox_return, GtkBox **aa_return)
{
	GtkWidget *t;
	GtkBox *vbox, *aa;

	if (!dlg) return;
	*dlg = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(*dlg), title);
	gtk_window_position(GTK_WINDOW(*dlg), GTK_WIN_POS_MOUSE);
	/* gtk_window_position(GTK_WINDOW(*dlg), GTK_WIN_POS_CENTER); */
	vbox = GTK_BOX(gtk_vbox_new(FALSE, 2));
	gtk_widget_show(GTK_WIDGET(vbox));
	gtk_container_add(GTK_CONTAINER(*dlg), GTK_WIDGET(vbox));
	aa = GTK_BOX(gtk_hbox_new(TRUE, 2));
	gtk_widget_show(GTK_WIDGET(aa));
	gtk_box_pack_end(vbox, GTK_WIDGET(aa), TRUE, TRUE, 5);
	t = gtk_hseparator_new();
	gtk_widget_show(t);
	gtk_box_pack_end(vbox, t, FALSE, FALSE, 1);
	t = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(t), 10);
	
	if (aa_return) *aa_return = aa;
	if (vbox_return) *vbox_return = GTK_BOX(t);
}



void new_dialog_ok(char *title, GtkWidget **dlg, GtkBox **vbox,
		       char *s, GtkSignalFunc sigfunc, gpointer *data)
{
	GtkWidget *t;
	GtkBox *aa;
#ifdef DIALOG_USE_ACCEL
	GtkAcceleratorTable *accel;
#endif

	if (!dlg) return;
	new_dialog(title, dlg, vbox, &aa);
	
	t = gtk_button_new_with_label(s);
	gtk_widget_show(t);
	gtk_box_pack_start(aa, t, FALSE, FALSE, 0);
	if (sigfunc)
		gtk_signal_connect(GTK_OBJECT(t), "clicked",
				   sigfunc, data);
	gtk_signal_connect_object(GTK_OBJECT(t), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(*dlg));
#ifdef DIALOG_USE_ACCEL
	accel = gtk_accelerator_table_new();
	gtk_accelerator_table_install(accel, GTK_OBJECT(t), "clicked",
				      '\n', 0);
	gtk_accelerator_table_install(accel, GTK_OBJECT(t), "clicked",
				      '\x1b', 0);
	gtk_window_add_accelerator_table(GTK_WINDOW(*dlg), accel);
#endif
}


void new_dialog_ok_cancel(char *title, GtkWidget **dlg, GtkBox **vbox,
			  char *s_ok, GtkSignalFunc sigfunc, gpointer *data,
			  char *s_cancel, GtkSignalFunc c_sigfunc, gpointer *c_data)
{
	GtkWidget *t;
	GtkBox *aa;
#ifdef DIALOG_USE_ACCEL
	GtkAcceleratorTable *accel;
#endif 
	
	if (!dlg) return;
	new_dialog(title, dlg, vbox, &aa);
	
	t = gtk_button_new_with_label(s_ok);
	gtk_widget_show(t);
	gtk_box_pack_start(aa, t, TRUE, FALSE, 5);
	if (sigfunc)
		gtk_signal_connect(GTK_OBJECT(t), "clicked",
				   sigfunc, data);
	gtk_signal_connect_object(GTK_OBJECT(t), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(*dlg));
#ifdef DIALOG_USE_ACCEL
	accel = gtk_accelerator_table_new();
	gtk_accelerator_table_install(accel, GTK_OBJECT(t), "clicked",
				      '\n', 0);
#endif
	t = gtk_button_new_with_label(s_cancel);
	gtk_widget_show(t);
	gtk_box_pack_start(aa, t, TRUE, FALSE, 5);
	if (c_sigfunc)
		gtk_signal_connect(GTK_OBJECT(t), "clicked",
				   c_sigfunc, c_data);
	gtk_signal_connect_object(GTK_OBJECT(t), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(*dlg));
#ifdef DIALOG_USE_ACCEL
	gtk_accelerator_table_install(accel, GTK_OBJECT(t), "clicked",
				      '\x1b', 0);
	gtk_window_add_accelerator_table(GTK_WINDOW(*dlg), accel);
#endif 
}


void msgbox_ok(char *title, char *text, char *ok_text,
	       GtkSignalFunc func)
{
#ifdef GNOME_USE_MSGBOX
	GtkWidget *mbox;

	mbox = gnome_messagebox_new(text, GNOME_MESSAGEBOX_GENERIC, ok_text, NULL, NULL);
	gnome_messagebox_set_default(GNOME_MESSAGEBOX(mbox), 1);
	gtk_signal_connect(GTK_OBJECT(mbox), "clicked",
			   func, NULL);
	gtk_widget_show(mbox);
#else /* GNOME_USE_MSGBOX */
	GtkWidget *dlg, *t;
	GtkBox *vbox;

	new_dialog_ok(title, &dlg, &vbox, ok_text, func, (gpointer *)1);
	t = gtk_label_new(text);
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);
	gtk_widget_show(dlg);
#endif /* GNOME_USE_MSGBOX */
}



void msgbox_ok_cancel(char *title, char *text,
		      char *ok_text, char *cancel_text,
		      GtkSignalFunc func)
{
#ifdef GNOME_USE_MSGBOX
	GtkWidget *mbox;
	
	mbox = gnome_messagebox_new(text, GNOME_MESSAGEBOX_GENERIC, ok_text, cancel_text, NULL);
	gnome_messagebox_set_default(GNOME_MESSAGEBOX(mbox), 1);
	gtk_signal_connect(GTK_OBJECT(mbox), "clicked",
			   func, NULL);
	gtk_widget_show(mbox);
#else /* GNOME_USE_MSGBOX */
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	new_dialog_ok_cancel(title, &dlg, &vbox,
			     ok_text, func, (gpointer *)1,
			     cancel_text, func, (gpointer *)2);
	t = gtk_label_new(text);
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);
	gtk_widget_show(dlg);
#endif /* GNOME_USE_MSGBOX */
}

