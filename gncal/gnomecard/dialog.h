#ifndef __GNOMECARD_DIALOG
#define __GNOMECARD_DIALOG

#include <gnome.h>

extern void gnomecard_add_email_call(GtkWidget *widget, gpointer data);
extern void gnomecard_add_phone_call(GtkWidget *widget, gpointer data);
extern void gnomecard_add_deladdr_call(GtkWidget *widget, gpointer data);
extern void gnomecard_add_dellabel_call(GtkWidget *widget, gpointer data);
extern void gnomecard_find_card_call(GtkWidget *widget, gpointer data);
extern void gnomecard_about(GtkWidget *widget, gpointer data);

extern void gnomecard_edit(GList *node);
extern void gnomecard_edit_card(GtkWidget *widget, gpointer data);
extern void gnomecard_open(GtkWidget *widget, gpointer data);
extern gboolean gnomecard_open_file(char *fname);
extern void gnomecard_save(void);
extern void gnomecard_save_as(GtkWidget *widget, gpointer data);
extern void gnomecard_setup(GtkWidget *widget, gpointer data);

#endif
