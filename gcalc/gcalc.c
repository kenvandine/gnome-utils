/* Simple Double precision calculator using the GnomeCalculator widget
   Copyright (C) 1998 Free Software Foundation

   Author: George Lebl <jirka@5z.com>
*/

#include <config.h>
#include <gnome.h>

static GtkWidget *calc;
static char copied_string[13]="";

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

/* Callback when the user toggles the selection */
static void
copy_contents (GtkWidget *widget, gpointer data)
{
	strcpy(copied_string,GNOME_CALCULATOR(calc)->result_string);
	gtk_selection_owner_set (calc,
				 GDK_SELECTION_PRIMARY,
				 GDK_CURRENT_TIME);
}


static void
selection_handle (GtkWidget *widget, 
		  GtkSelectionData *selection_data,
		  int info, int time,
		  gpointer data)
{
	gtk_selection_data_set (selection_data, GDK_SELECTION_TYPE_STRING,
				8, copied_string, strlen(copied_string));
}

/* Menus */
GnomeUIInfo gcalc_calculator_menu[] = {
	{GNOME_APP_UI_ITEM, N_("E_xit"),  NULL, quit_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	 'X', GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_END
};

GnomeUIInfo gcalc_help_menu[] = {
	{GNOME_APP_UI_ITEM, N_("_About..."), NULL, about_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT},
	GNOMEUIINFO_END
};

GnomeUIInfo gcalc_edit_menu[] = {
	{GNOME_APP_UI_ITEM, N_("_Copy"), NULL, copy_contents, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY},
	GNOMEUIINFO_END
};

GnomeUIInfo gcalc_menu[] = {
        {GNOME_APP_UI_SUBTREE, N_("_Calculator"), NULL, gcalc_calculator_menu, NULL, NULL,
	        GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{GNOME_APP_UI_SUBTREE, N_("_Edit"), NULL, gcalc_edit_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{GNOME_APP_UI_SUBTREE, N_("_Help"), NULL, gcalc_help_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        GNOMEUIINFO_END
};


int
main(int argc, char *argv[])
{
	GtkWidget *app;
	
	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init ("gcalc", VERSION, argc, argv);
	
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

	gtk_selection_add_target (GTK_WIDGET (calc),
				  GDK_SELECTION_PRIMARY,
				  GDK_SELECTION_TYPE_STRING,0);

        gtk_signal_connect(GTK_OBJECT(calc), "selection_get",
			   GTK_SIGNAL_FUNC(selection_handle), NULL);

	gnome_app_set_contents(GNOME_APP(app), calc);

	/* add calculator accel table to our window*/
	gtk_window_add_accel_group(GTK_WINDOW(app),
				   GNOME_CALCULATOR(calc)->accel);

	gtk_widget_show(app);

	gtk_main ();

	return 0;
}
