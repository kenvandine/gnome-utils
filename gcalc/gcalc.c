/* Simple Double precision calculator using the GnomeCalculator widget
   Copyright (C) 1998 Free Software Foundation

   Author: George Lebl <jirka@5z.com>
*/
#define WITH_FOOT


#include <config.h>
#include <gnome.h>

#ifdef WITH_FOOT
#include "foot.xpm"
#endif


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
				(const char **)authors,
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

/* Menus */
GnomeUIInfo gcalc_program_menu[] = {
	{GNOME_APP_UI_ITEM, N_("_About..."), NULL, about_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT},
	/*{GNOME_APP_UI_ITEM, N_("Preferences..."), NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF},*/
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("_Quit"),  NULL, quit_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	 'Q', GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_END
};

GnomeUIInfo gcalc_edit_menu[] = {
	{GNOME_APP_UI_ITEM, N_("_Copy"), NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY},
	GNOMEUIINFO_END
};

GnomeUIInfo gcalc_menu[] = {
#ifdef WITH_FOOT
	{GNOME_APP_UI_SUBTREE, "", NULL, &gcalc_program_menu, NULL, NULL,
	        GNOME_APP_PIXMAP_DATA, foot_xpm, 0, 0, NULL },            
#else
        {GNOME_APP_UI_SUBTREE, N_("_Program"), NULL, gcalc_program_menu, NULL, NULL,
	        GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#endif
	{GNOME_APP_UI_SUBTREE, N_("_Edit"), NULL, gcalc_edit_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        GNOMEUIINFO_END
};


int
main(int argc, char *argv[])
{
	GtkWidget *app;
	GtkWidget *calc;

	argp_program_version = VERSION;
	
	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init ("gcalc", NULL, argc, argv, 0, NULL);
	
        app=gnome_app_new("gcalc", _("Gnome Calculator"));
	gtk_window_set_wmclass (GTK_WINDOW (app), "gcalc", "gcalc");
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, FALSE, TRUE);

        gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		GTK_SIGNAL_FUNC(quit_cb), NULL);
        gtk_window_set_policy(GTK_WINDOW(app),1,1,0);

	/*set up the menu*/
        gnome_app_create_menus(GNOME_APP(app), gcalc_menu);

	calc = gnome_calculator_new();
	gtk_widget_show(calc);

	gnome_app_set_contents(GNOME_APP(app), calc);

	/* add calculator accel table to our window*/
#ifdef GTK_HAVE_FEATURES_1_1_0
	gtk_window_add_accel_group(GTK_WINDOW(app),
				   GNOME_CALCULATOR(calc)->accel);
#else
	gtk_window_add_accelerator_table (GTK_WINDOW (app),
					  GNOME_CALCULATOR (calc)->accel);
#endif

	gtk_widget_show(app);

	gtk_main ();

	return 0;
}
