#ifndef __DIALOG_H__
#define __DIALOG_H__

/*
 * The dialog created is a window widget - not a dialog widget!
 */

void new_dialog(char *title, GtkWidget **dlg, GtkBox **vbox_return, GtkBox **aa_return);
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
