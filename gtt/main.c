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

#include <config.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "gtt.h"

#include "shorts.h"		/* SMH 2000-03-22: connect_short_cuts() */


#define USE_SM



char *
gtt_gettext(char *s)
{
        g_return_val_if_fail(s != NULL, NULL);
        if (0 == strncmp(s, "[GTT]", 5))
                return &s[5];
        return s;
}




static char *build_lock_fname()
{
	static char fname[1024] = "";
	
	if (fname[0]) return fname;
	if (getenv("HOME")) {
		strcpy(fname, getenv("HOME"));
	} else {
		fname[0] = 0;
	}
	strcat(fname, "/.gtimetracker");
#ifdef DEBUG
	strcat(fname, "-" VERSION);
#endif
	strcat(fname, ".pid");
	return fname;
}



static void lock_gtt()
{
	FILE *f;
	char *fname;
	
	fname = build_lock_fname();
	if (NULL != (f = fopen(fname, "rt"))) {
		GtkWidget *warning;
		fclose(f);
		
#ifdef DEBUG
                g_warning("GTT PID file exists");
#else /* not DEBUG */
		warning = gnome_message_box_new(_("There seems to be another GTimeTracker running.\n"
						  "Press OK to start GTimeTracker anyway, or press Cancel to quit."),
						GNOME_MESSAGE_BOX_WARNING,
						GNOME_STOCK_BUTTON_OK,
						GNOME_STOCK_BUTTON_CANCEL,
						NULL);
		if(gnome_dialog_run_and_close(GNOME_DIALOG(warning))!=0)
			exit(0);
#endif /* not DEBUG */
	}
	if (NULL == (f = fopen(fname, "wt"))) {
		g_error("Cannot create pid-file!");
	}
	fprintf(f, "%d\n", getpid());
	fclose(f);
}



void unlock_gtt(void)
{
	int unlink(const char *);

	log_exit();
	unlink(build_lock_fname());
}



static void init_list_2(GtkWidget *w, gint butnum)
{
	if (butnum == 1) { unlock_gtt(); gtk_main_quit(); }
	else
                setup_clist();
}

static void init_list(void)
{
	if (!project_list_load(NULL)) {
                if (errno == ENOENT) {
                        errno = 0;
                        setup_clist();
                        return;
                }
		msgbox_ok_cancel(_("Error"),
				 _("An error occured while reading the "
                                   "configuration file.\n"
				   "Shall I setup a new configuration?"),
				 GNOME_STOCK_BUTTON_YES, 
				 GNOME_STOCK_BUTTON_NO,
				 GTK_SIGNAL_FUNC(init_list_2));
	} else {
                setup_clist();
	}
}




#ifdef USE_SM

/*
 * session management
 */

#ifdef DEBUG
#define GTT "/gtt-DEBUG/"
#else /* not DEBUG */
#define GTT "/gtt/"
#endif /* not DEBUG */

static int
save_state(GnomeClient *client, gint phase, GnomeRestartStyle save_style,
	   gint shutdown, GnomeInteractStyle interact_styyle, gint fast,
	   gpointer data)
{
	char *sess_id;
	char *argv[5];
	int argc;
	int x, y, w, h;

	sess_id  = gnome_client_get_id(client);
	if (!window)
		return FALSE;

	gdk_window_get_origin(window->window, &x, &y);
	gdk_window_get_size(window->window, &w, &h);
	argv[0] = (char *)data;
	argv[1] = "--geometry";
	argv[2] = g_malloc(32);
	sprintf(argv[2], "%dx%d+%d+%d", w, h, x, y);
	if ((cur_proj) && (cur_proj->title)) {
		argc = 5;
		argv[3] = "--select-project";
		argv[4] = cur_proj->title;
	} else {
		argc = 3;
	}
	gnome_client_set_clone_command(client, argc, argv);
	gnome_client_set_restart_command(client, argc, argv);
	g_free(argv[2]);

	return project_list_save(NULL);
}


static void
session_die(GnomeClient *client)
{
	quit_app(NULL, NULL);
}

#endif /* USE_SM */




int 
main(int argc, char *argv[])
{
	static char *geometry_string = NULL;
#ifdef USE_SM
	GnomeClient *client;
#endif /* USE_SM */
	static const struct poptOption geo_options[] =
	{
		{"geometry", 'g', POPT_ARG_STRING, &geometry_string, 0,
			N_("Specify geometry"), N_("GEOMETRY")},
		{"select-project", 's', POPT_ARG_STRING, &first_proj_title, 0,
			N_("Select a project on startup"), N_("PROJECT")},
		{NULL, '\0', 0, NULL, 0}
	};

	gnome_init_with_popt_table("gtt", VERSION, argc, argv,
				   geo_options, 0, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-cromagnon.png");

#ifdef USE_SM
	client = gnome_master_client();
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(save_state), (gpointer) argv[0]);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(session_die), NULL);
#endif /* USE_SM */

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	signal(SIGCHLD, SIG_IGN);
	lock_gtt();
	app_new(argc, argv, geometry_string);
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_app), NULL);

	/*
	 * Added by SMH 2000-03-22:
	 * Connect short-cut keys. 
	 */
	connect_short_cuts();

	/* start timer before the state of the menu items is set */
	start_timer();
	init_list();
	log_start();

	gtk_main();

	unlock_gtt();
	return 0;
}

