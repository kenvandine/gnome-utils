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
#if HAS_GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif


#include <string.h>


#include "gtt.h"

#undef bindtextdomain
#undef textdomain
#undef gettext
#undef dgettext
#undef fcgettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


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
	int getpid(void);
	
	fname = build_lock_fname();
	if (NULL != (f = fopen(fname, "rt"))) {
		fclose(f);
#ifdef DEBUG
                g_warning("GTT PID file exists");
#else /* not DEBUG */
		msgbox_ok(_("Error"), _("There seems to be another GTimeTracker running.\n"
			  "Please remove the pid file, if that is not correct."),
			  _("Oops"),
			  GTK_SIGNAL_FUNC(gtk_main_quit));
		gtk_main();
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



int main(int argc, char *argv[])
{
#if defined(DEBUG) && 0
#define locale_debug(a,b) g_print((a),(b))
#else
#define locale_debug(a,b) b
#endif
	locale_debug("%s\n", getenv("LANG"));
	setlocale(LC_MESSAGES, "");
	locale_debug(LOCALEDIR " %s\n", bindtextdomain(PACKAGE, LOCALEDIR));
	locale_debug(PACKAGE " %s\n", textdomain(PACKAGE));

#if HAS_GNOME
	gnome_init(&argc, &argv);
#else 
	gtk_init(&argc, &argv);
#endif 

	lock_gtt();
	app_new(argc, argv);
	gtk_main();
	/* TODO: verify, that unlock_gtt is always run at exit */
	unlock_gtt();
	return 0;
}

