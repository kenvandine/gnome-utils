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
#include <string.h>

#include "gtt.h"



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



static int w_x = 0, w_y = 0, w_w = 0, w_h = 0, w_xyset = 0, w_sx = 0, w_sy = 0;

static error_t
argp_parser(int key, char *arg, struct argp_state *state)
{
	char *p, *p0;
	char c;

	if (key != 'g') return ARGP_ERR_UNKNOWN;
	p = arg;
	if ((*p >= '0') && (*p <= '9')) {
		p0 = p;
		for (; (*p >= '0') && (*p <= '9'); p++) ;
		if (*p != 'x') {
			g_print(_("error in geometry string \"%s\"\n"), arg);
			return 0;
		}
		*p = 0;
		w_w = atoi(p0);
		*p = 'x';
		p0 = ++p;
		for (; (*p >= '0') && (*p <= '9'); p++) ;
		c = *p;
		*p = 0;
		w_h = atoi(p0);
		*p = c;
	}
	if (*p == 0) return 0;
	if ((*p != '-') && (*p != '+')) {
		g_print(_("error in geometry string \"%s\"\n"), arg);
		return 0;
	}
	p0 = p;
	for (p++; (*p >= '0') && (*p <= '9'); p++) ;
	c = *p;
	*p = 0;
	w_sx = (*p0 != '-');
	w_x = atoi(p0);
	*p = c;
	if ((*p != '-') && (*p != '+')) {
		g_print(_("error in geometry string \"%s\"\n"), arg);
		return 0;
	}
	p0 = p;
	for (p++; (*p >= '0') && (*p <= '9'); p++) ;
	if (*p != 0) {
		g_print(_("error in geometry string \"%s\"\n"), arg);
		return 0;
	}
	w_sy = (*p0 != '-');
	w_y = atoi(p0);
	w_xyset++;
	return 0;
}



int main(int argc, char *argv[])
{
	struct argp_option geo_options[] = {
		{"geometry", 'g', "GEOM", 0, N_("specify geometry"), 0},
		{NULL, 0, NULL, 0, NULL, 0}
	};
	struct argp args = {
		geo_options,
		argp_parser, NULL, NULL, NULL, NULL, NULL
	};
	gnome_init("gtt", &args, argc, argv, 0, NULL);

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	lock_gtt();
	app_new(argc, argv);
	if (!w_w) {
		gtk_widget_set_usize(glist, -1, 120);
	}
	gtk_widget_size_request(window, &window->requisition);
	if (w_w != 0) {
		if (window->requisition.width > w_w) w_w = window->requisition.width;
		if (window->requisition.height > w_h) w_h = window->requisition.height;
		gtk_widget_set_usize(window, w_w, w_h);
	} else {
		w_w = window->requisition.width;
		w_h = window->requisition.height;
	}
	if (w_xyset) {
		int t;
		t = gdk_screen_width();
		if (!w_sx) w_x += t - w_w;
		while (w_x < 0) w_x += t;
		while (w_x > t) w_x -= t;
		t = gdk_screen_height();
		if (!w_sy) w_y += t - w_h;
		while (w_y < 0) w_y += t;
		while (w_y > t) w_y -= t;
		gtk_widget_set_uposition(window, w_x, w_y);
	}
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_app), NULL);

	log_start();
	gtk_main();
	unlock_gtt();
	return 0;
}

