/* By Elliot Lee <sopwith@cuc.edu>
   A ten-minute hack. The escape key handling is ugly - what's
   wrong with GtkAccelerator?!?!
 */
#include <unistd.h>
#include <gnome.h>
#include <gdk/gdkkeysyms.h>

GtkWidget *thewin;

void dokey(GtkWidget *widget, GdkEventKey *keyevent)
{
	if(keyevent->keyval == GDK_Escape)
	{
		gtk_widget_hide(thewin);
		gtk_main_quit();
	}
}

void dorun(GtkWidget *widget, GtkWidget *theent)
{
	GString *runme;
	char *t=gtk_entry_get_text(GTK_ENTRY(theent));
	gtk_widget_hide(thewin);
	if(t != NULL && *t != '\0') {
	        runme = g_string_new(t);
		g_string_append(runme," &");
		system(runme->str);
	}
	gtk_main_quit();
}

#define APPNAME "grun"

int
main(int argc, char *argv[])
{
	GtkWidget *theent;

	gnome_init (APPNAME, 0, argc, argv, 0, 0);

	thewin = gnome_dialog_new(_("Run a Program"),
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL, NULL);
	gtk_window_position(GTK_WINDOW(thewin), GTK_WIN_POS_CENTER);
	gtk_window_set_policy(GTK_WINDOW(thewin), FALSE, FALSE, TRUE);
	gtk_signal_connect(GTK_OBJECT(thewin), "delete_event",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	gnome_dialog_button_connect(GNOME_DIALOG(thewin), 0, 
				    GTK_SIGNAL_FUNC(dorun), theent);
	gnome_dialog_button_connect(GNOME_DIALOG(thewin), 1, 
				    GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	theent = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(thewin)->vbox), theent);
	gtk_signal_connect(GTK_OBJECT(theent), "key_press_event",
			   GTK_SIGNAL_FUNC(dokey), NULL);
	gtk_signal_connect(GTK_OBJECT(theent), "activate",
			   GTK_SIGNAL_FUNC(dorun), theent);
	gtk_window_set_focus(GTK_WINDOW(thewin), theent);

	gtk_widget_show_all(thewin);

	gtk_main();
	return 0;
}
