/* By Elliot Lee <sopwith@cuc.edu>
   A ten-minute hack. The escape key handling is ugly - what's
   wrong with GtkAccelerator?!?!
 */
#include <unistd.h>
#include <gtk/gtk.h>
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

int
main(int argc, char *argv[])
{
	GtkWidget *theent;
#ifdef GTK_HAVE_ACCEL_GROUP
	GtkAccelGroup *accel;
#else
	GtkAcceleratorTable *accel;
#endif

	gtk_init(&argc, &argv);

	thewin = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_position(GTK_WINDOW(thewin), GTK_WIN_POS_CENTER);
	gtk_window_set_policy(GTK_WINDOW(thewin), FALSE, FALSE, TRUE);
	gtk_signal_connect(GTK_OBJECT(thewin), "delete_event",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	theent = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(thewin), theent);
	gtk_signal_connect(GTK_OBJECT(theent), "key_press_event",
			   GTK_SIGNAL_FUNC(dokey), NULL);
	gtk_signal_connect(GTK_OBJECT(theent), "activate",
			   GTK_SIGNAL_FUNC(dorun), theent);
	gtk_window_set_focus(GTK_WINDOW(thewin), theent);

#ifdef GTK_HAVE_ACCEL_GROUP
	accel = gtk_accel_group_new();
#else
	accel = gtk_accelerator_table_new ();
#endif

#if 0
	gtk_widget_install_accelerator (thewin, accel,
				"delete_event",
				GDK_Escape, 0);
	gtk_widget_install_accelerator (theent, accel,
				"activate",
				GDK_Escape, 0);
#endif
	gtk_widget_show_all(thewin);

	gtk_main();
	return 0;
}
