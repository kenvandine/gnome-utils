/* By Elliot Lee <sopwith@cuc.edu>
   FIXME copyright notice
 */
#include <unistd.h>
#include <config.h>
#include <gnome.h>

void clicked_cb(GtkWidget *dialog, gint button, GtkWidget *theent)
{
	GString *runme;
	char *t;

	gtk_widget_hide(dialog); /* Will happen anyway, but do it
				    early for aesthetic reasons. */

	/* OK button */
	if (button == 0) { 
	  t = gtk_entry_get_text(GTK_ENTRY(theent));
	  if(t != NULL && *t != '\0') {
	    runme = g_string_new(t);
	    g_string_append(runme," &");
	    system(runme->str);
	  }
	}

	gtk_main_quit();
}

#define APPNAME "grun"
#ifndef VERSION
#define VERSION "0.0.0"
#endif

int
main(int argc, char *argv[])
{
	GtkWidget *theent;
	GtkWidget *thewin;

	argp_program_version = VERSION;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init (APPNAME, 0, argc, argv, 0, 0);

	thewin = gnome_dialog_new(_("Run a Program"),
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL, NULL);
	gnome_dialog_set_default(GNOME_DIALOG(thewin), 0);
	gnome_dialog_set_destroy(GNOME_DIALOG(thewin), TRUE);

	gtk_window_position(GTK_WINDOW(thewin), GTK_WIN_POS_CENTER);

	gtk_signal_connect(GTK_OBJECT(thewin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	theent = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(thewin)->vbox), theent);

	/* If Return is pressed in the text entry, propagate to the buttons */
	gtk_signal_connect_object(GTK_OBJECT(theent), "activate",
				  GTK_SIGNAL_FUNC(gtk_window_activate_default), 
				  GTK_OBJECT(thewin));

	gtk_signal_connect(GTK_OBJECT(thewin), "clicked",
			   GTK_SIGNAL_FUNC(clicked_cb), theent);

	gtk_window_set_focus(GTK_WINDOW(thewin), theent);

	gtk_widget_show_all(thewin);

	gtk_main();
	return 0;
}

