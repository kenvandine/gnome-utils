/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Simple Double precision calculator using the GnomeCalculator widget
   Copyright (C) 1998 Free Software Foundation

   Author: George Lebl <jirka@5z.com>
*/

#include <config.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

#include "gnome-calc.h"
#include "sr.h"

/* values for selection info */
enum {
  TARGET_STRING,
  TARGET_TEXT,
  TARGET_COMPOUND_TEXT,
};

static GtkWidget *app;
static GtkWidget *calc;
static char copied_string[13]="";

static void
about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	gchar *authors[] = {
		"George Lebl <jirka@5z.com>",
		"Bastien Nocera <hadess@hadess.net> (fixes)",
		NULL
	};
	gchar *documenters[] = {
		NULL
	};
	/* Translator credits */
	gchar *translator_credits = _("translator_credits");

	if (about != NULL)
	{
		gdk_window_show(about->window);
		gdk_window_raise(about->window);
		return;
	}
	about = gnome_about_new(_("The GNOME Calculator"), VERSION,
				"(C) 1998 the Free Software Foundation",
				_("Simple double precision calculator similiar "
				  "to xcalc"),
				(const char **)authors,
				(const char **)documenters,
				strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				NULL);
	gtk_signal_connect(GTK_OBJECT(about), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about);
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
	strcpy(copied_string, gnome_calc_get_result_string(GNOME_CALC (calc)));
	g_strstrip(copied_string);
	gtk_selection_owner_set (app,
				 GDK_SELECTION_CLIPBOARD,
				 GDK_CURRENT_TIME);
}


static void
selection_handle (GtkWidget *widget, 
		  GtkSelectionData *selection_data,
		  int info, int time,
		  gpointer data)
{
	if (info == TARGET_STRING) {
		gtk_selection_data_set (selection_data,
					GDK_SELECTION_TYPE_STRING,
					8*sizeof(gchar),
					(guchar *)copied_string,
					strlen(copied_string));
	} else if ((info == TARGET_TEXT) || (info == TARGET_COMPOUND_TEXT)) {
		guchar *text;
		GdkAtom encoding;
		gint format;
		gint new_length;

		gdk_string_to_compound_text (copied_string, &encoding, &format,
					     &text, &new_length);
		gtk_selection_data_set (selection_data, encoding, format,
					text, new_length);
		gdk_free_compound_text (text);
	}
}

/* Menus */
static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_MENU_EXIT_ITEM(quit_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit_menu[] = {
	GNOMEUIINFO_MENU_COPY_ITEM(copy_contents,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP("gcalc"),
	GNOMEUIINFO_MENU_ABOUT_ITEM(about_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo gcalc_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE(file_menu),
	GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
        GNOMEUIINFO_END
};

static gboolean analog = FALSE;

struct poptOption options [] = {
  { "analog", '\0', POPT_ARG_NONE|POPT_ARGFLAG_ONEDASH, &analog, 0,
    N_("Run in analog (slide rule) mode"), NULL },
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0}
};

static int save_session(GnomeClient        *client,
                        gint                phase,
                        GnomeRestartStyle   save_style,
                        gint                shutdown,
                        GnomeInteractStyle  interact_style,
                        gint                fast,
                        gpointer            client_data) {
       gchar *argv[]= { NULL };

       argv[0] = (char*) client_data;
       gnome_client_set_clone_command(client, 1, argv);
       gnome_client_set_restart_command(client, 1, argv);

       return TRUE;
}

static gint client_die(GnomeClient *client, gpointer client_data)
{
       gtk_main_quit ();
}



int
main(int argc, char *argv[])
{
	GnomeClient *client;
	static GtkTargetEntry targets[] = {
		{ "STRING", TARGET_STRING },
		{ "TEXT",   TARGET_TEXT }, 
		{ "COMPOUND_TEXT", TARGET_COMPOUND_TEXT }
	};
	static gint n_targets = sizeof(targets) / sizeof(targets[0]);
	
	/* Initialize the i18n stuff */
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gnome_program_init ("gnome-calculator", VERSION, LIBGNOMEUI_MODULE,
			argc, argv, GNOME_PARAM_POPT_TABLE, options, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-calc2.png");

	if (analog) {
		run_slide_rule ();
		return 0;
	}

        app = gnome_app_new("gnome-calculator", _("GNOME Calculator"));
	gtk_window_set_wmclass (GTK_WINDOW (app), "gnome-calculator", "gnome-calculator");
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, FALSE);

        gtk_signal_connect(GTK_OBJECT(app), "delete_event",
			GTK_SIGNAL_FUNC(quit_cb), NULL);

	/*set up the menu*/
        gnome_app_create_menus_with_data(GNOME_APP(app), gcalc_menu, app);

	calc = gnome_calc_new();
	gnome_calc_bind_extra_keys (GNOME_CALC (calc), GTK_WIDGET (app));
	gtk_widget_show(calc);

	gtk_selection_add_targets (GTK_WIDGET (app), GDK_SELECTION_CLIPBOARD,
				   targets, n_targets);

        gtk_signal_connect(GTK_OBJECT(app), "selection_get",
			   GTK_SIGNAL_FUNC(selection_handle), NULL);

	client = gnome_master_client();
	g_signal_connect (client, "save_yourself",
				G_CALLBACK(save_session), (gpointer)argv[0]);
	g_signal_connect (client, "die",
				G_CALLBACK(client_die), NULL);

	gnome_app_set_contents(GNOME_APP(app), calc);

	/* add calculator accel table to our window*/
	gtk_window_add_accel_group(GTK_WINDOW(app),
				   gnome_calc_get_accel_group(GNOME_CALC(calc)));

	gtk_widget_show(app);

	gtk_main ();

	return 0;
}
