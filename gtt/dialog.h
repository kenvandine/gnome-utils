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
#ifndef __DIALOG_H__
#define __DIALOG_H__


gint gtt_delete_event(GtkWidget *w, gpointer *data);

void new_dialog_ok(char *title, GtkWidget **dlg, GtkBox **vbox,
		       char *s, GtkSignalFunc sigfunc, gpointer *data);
void new_dialog_ok_cancel(char *title, GtkWidget **dlg, GtkBox **vbox,
			  char *s_ok, GtkSignalFunc sigfunc, gpointer *data,
			  char *s_cancel, GtkSignalFunc c_sigfunc, gpointer *c_data);
void msgbox_ok(char *title, char *text, char *ok_text,
	       GtkSignalFunc func);
void msgbox_ok_cancel(char *title, char *text,
		      char *ok_text, char *cancel_text,
		      GtkSignalFunc func);

#endif
