/* Simple Double precision calculator using the GnomeCalculator widget
   Copyright (C) 1998 Free Software Foundation

   Author: George Lebl <jirka@5z.com>
*/

#include <config.h>
#include <gnome.h>

static void
about_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *about;
	gchar *authors[] = {
		"George Lebl",
		NULL
	};

	about = gnome_about_new(_("The Gnome Calculator"), VERSION,
				"(C) 1998 the Free Software Foundation",
				authors,
				_("Simple double precision calculator similiar "
				  "to xcalc"),
				NULL);
	gtk_widget_show (about);
}

static void
quit_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}



GnomeUIInfo calc_menu[] = {
	{GNOME_APP_UI_ITEM, N_("Exit"), NULL, quit_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'X', GDK_CONTROL_MASK, NULL},
	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo help_menu[] = {  
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}, 
	
	{GNOME_APP_UI_ITEM, N_("About..."), NULL, about_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo gcalc_menu[] = {
	{GNOME_APP_UI_SUBTREE, N_("Calculator"), NULL, calc_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_SUBTREE, N_("Help"), NULL, help_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};


int
main(int argc, char *argv[])
{
	GtkWidget *app;
	GtkWidget *calc;

	argp_program_version = VERSION;

	gnome_init ("gcalc", NULL, argc, argv, 0, NULL);
	
        app=gnome_app_new("gcalc", _("Gnome Calculator"));
	gtk_window_set_wmclass (GTK_WINDOW (app), "gcalc", "gcalc");
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, FALSE, TRUE);

        gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		GTK_SIGNAL_FUNC(quit_cb), NULL);
        gtk_window_set_policy(GTK_WINDOW(app),1,1,0);

	/*set up the menu*/
        gnome_app_create_menus(GNOME_APP(app), gcalc_menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(gcalc_menu[1].widget));

	calc = gnome_calculator_new();
	gtk_widget_show(calc);

	gnome_app_set_contents(GNOME_APP(app), calc);

	/* add calculator accel table to our window*/
	gtk_window_add_accel_group(GTK_WINDOW(app),
				   GNOME_CALCULATOR(calc)->accel);

	gtk_widget_show(app);

	gtk_main ();

	return 0;
}
