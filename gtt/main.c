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

#include "config.h"

#include <errno.h>
#include <gconf/gconf.h>
#include <glade/glade.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "ctree.h"
#include "dialog.h"
#include "err-throw.h"
#include "file-io.h"
#include "gtt.h"
#include "menucmd.h"
#include "shorts.h"		/* SMH 2000-03-22: connect_short_cuts() */
#include "xml-gtt.h"


char *first_proj_title = NULL;  /* command line over-ride */

/* SM == session management */
#define USE_SM


const char *
gtt_gettext(const char *s)
{
        g_return_val_if_fail(s != NULL, NULL);
        if (0 == strncmp(s, "[GTT]", 5))
                return &s[5];
        return s;
}




static char *build_lock_fname(void)
{
	GString *str;
	static char *fname = NULL;
	
	if (fname != NULL) return fname;

	/* note it will handle unset "HOME" fairly gracefully */
	str = g_string_new (g_getenv ("HOME"));
	g_string_append (str, "/.gtimetracker");
#ifdef DEBUG
	g_string_append (str, "-" VERSION);
#endif
	g_string_append (str, ".pid");

	fname = str->str;
	g_string_free (str, FALSE);
	return fname;
}



static void lock_gtt(void)
{
	FILE *f;
	char *fname;
	gboolean warn = FALSE;
	
	fname = build_lock_fname ();

	/* if the pid file exists and such a process exists
	 * and this process is owned by the current user,
	 * else this pid file is very very stale and can be
	 * ignored */
	if (NULL != (f = fopen(fname, "rt"))) {
		int pid;

		if (fscanf (f, "%d", &pid) == 1 &&
		    pid > 0 &&
		    kill (pid, 0) == 0) {
			warn = TRUE;
		}
		fclose(f);
	}
		
	if (warn) {
		GtkWidget *warning;
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
		g_warning(_("Cannot create pid-file!"));
	}
	fprintf(f, "%d\n", getpid());
	fclose(f);
}



void 
unlock_gtt(void)
{
	log_exit();
	unlink(build_lock_fname());
}



static void 
init_list_2(GtkWidget *w, gint butnum)
{
	if (butnum == 1)
		gtk_main_quit();
	else
                setup_ctree();
}

static void 
init_list(void)
{
	GttErrCode xml_errcode, conf_errcode;
	const char * xml_filepath;

	/* Read the data file first, and the config later.
	 * The config cile contains things like the 'current project',
	 * which is undefined until the proejcts have been read in.
	 */
	xml_filepath = gnome_config_get_real_path (XML_DATA_FILENAME);
        gtt_err_set_code (GTT_NO_ERR);
        gtt_xml_read_file (xml_filepath);

	xml_errcode = gtt_err_get_code();

        gtt_err_set_code (GTT_NO_ERR);
	gtt_load_config (NULL);
	conf_errcode = gtt_err_get_code();

	/* If the xml file read bombed because the file doesn't exist,
	 * and yet the project list isn't null, that's because we read
	 * and old-format config file that had the proejcts in it.
	 * This is not an arror. This is OK.
	 */
	if (!((GTT_NO_ERR == xml_errcode) ||
	      ((GTT_CANT_OPEN_FILE == xml_errcode) &&
	        gtt_get_project_list())
	    ))
	{
		g_warning ("xml file read bombed, errcode = %d\n", xml_errcode);
	}

	if (GTT_NO_ERR != conf_errcode) 
	{
                if (errno == ENOENT) {
                        errno = 0;
                        setup_ctree();
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
                setup_ctree();
	}

}


void
save_all (void)
{
	const char * xml_filepath;
	xml_filepath = gnome_config_get_real_path (XML_DATA_FILENAME);

	gtt_save_config (NULL);

	gtt_xml_write_file (xml_filepath);
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
	int rc;

	sess_id  = gnome_client_get_id(client);
	if (!window)
		return FALSE;

	gdk_window_get_origin(window->window, &x, &y);
	gdk_window_get_size(window->window, &w, &h);
	argv[0] = (char *)data;
	argv[1] = "--geometry";
	argv[2] = g_strdup_printf("%dx%d+%d+%d", w, h, x, y);
	if ((cur_proj) && (gtt_project_get_title(cur_proj))) {
		argc = 5;
		argv[3] = "--select-project";
		argv[4] = (char *) gtt_project_get_title(cur_proj);
	} else {
		argc = 3;
	}
	gnome_client_set_clone_command(client, argc, argv);
	gnome_client_set_restart_command(client, argc, argv);
	g_free(argv[2]);

	/* save both te user preferences/config and the project lists */
	gtt_err_set_code (GTT_NO_ERR);
	save_all();
	rc = 0;
	if (GTT_NO_ERR == gtt_err_get_code()) rc = 1;

	return rc;
}


static void
session_die(GnomeClient *client)
{
	quit_app(NULL, NULL);
}

#endif /* USE_SM */


static void
got_signal (int sig)
{
	unlock_gtt ();
	
	/* whack thyself */
	signal (sig, SIG_DFL);
	kill (getpid (), sig);
}

static void 
beta_run_or_abort(GtkWidget *w, gint butnum)
{
	if (butnum == 1)
	{
		gtk_main_quit();
	}
	else
	{
		init_list();
		log_start();
	}
}



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

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

#ifdef USE_SM
	client = gnome_master_client();
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(save_state), (gpointer) argv[0]);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(session_die), NULL);
#endif /* USE_SM */

	signal (SIGCHLD, SIG_IGN);
	signal (SIGINT, got_signal);
	signal (SIGTERM, got_signal);
	lock_gtt();
	app_new(argc, argv, geometry_string);

	glade_gnome_init();
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_app), NULL);

	/* gconf init is needed by gtkhtml */
	gconf_init (argc, argv, NULL);

	/*
	 * Added by SMH 2000-03-22:
	 * Connect short-cut keys. 
	 */
	connect_short_cuts();

	msgbox_ok_cancel(_("Warning"),
		"WARNING !!! Achtung !!! Avertisment !!!\n"
		"\n"
		"This is a development version of GTT.  It uses a new\n"
		"file format that may leave your old data damaged and\n"
		"unrecoverable.  There may be incompatible file format\n"
		"changes in the near future. Use at own risk!\n"
		"\n"
		"The last stable, working version can be obtained with\n"
		"cvs checkout -D \"Aug 27 2001\" gnome-utils/gtt\n",
	     "Continue", "Exit", 
		GTK_SIGNAL_FUNC(beta_run_or_abort));

	gtk_main();

	unlock_gtt();
	return 0;
}

