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

#include "gtt.h"

#include <stdlib.h>
#include <stdio.h>

#undef gettext
#undef _
#include <libintl.h>
#define _(String) gettext(String)


static int log_write(time_t t, char *s)
{
	FILE *f;
	char date[20];

	g_return_val_if_fail(config_logfile_name != NULL, 0);
	g_return_val_if_fail(s != NULL, 0);
	f = fopen(config_logfile_name, "at");
	g_return_val_if_fail(f != NULL, 0);
	if (t == -1) t = time(NULL);
	strftime(date, 20, "%b %d %H:%M:%S", localtime(&t));
	fprintf(f, "%s %s\n", date, s);
	fclose(f);
	return 1;
}



static char *last_msg = NULL;
static char *last_logged_msg = NULL;
static time_t last_time = -1;

static void log_last(time_t t, project *proj)
{
	char *s;

	if (last_msg) {
		s = last_msg;
	} else {
		s = g_strdup(_("stopped project"));
	}
	if ((!last_logged_msg) || (0 != strcmp(s, last_logged_msg))) {
		if (!log_write(last_time, s)) {
			g_warning("couldn't write to logfile `%s'.\n",
				  config_logfile_name?config_logfile_name:"");
		}
	}
	if (last_logged_msg) g_free(last_logged_msg);
	last_logged_msg = s;
	if (proj) last_msg = g_strdup(proj->title);
	else last_msg = NULL;
	last_time = t;
}

static void log_proj_intern(project *proj, int log_if_equal)
{
	time_t t;

	if ((log_if_equal) && (last_logged_msg)) {
		g_free(last_logged_msg);
		last_logged_msg = NULL;
	}
	if ((!config_logfile_name) || (!config_logfile_use)) {
		log_last(-1, NULL);
		return;
	}
	if (config_logfile_min_secs == 0) {
		log_last(-1, proj);
		log_last(-1, proj);
	}
	t = time(NULL);
	if ((last_time != -1) &&
	    ((long int)(t - last_time) < config_logfile_min_secs)) {
		if (proj == NULL) {
			if (last_msg) last_time = t;
		} else {
			if ((!last_msg) ||
			    (0 != strcmp(last_msg, proj->title)))
				last_time = t;
		}
		if (last_msg) g_free(last_msg);
		last_msg = (proj) ? g_strdup(proj->title) : NULL;
		return;
	}
	log_last(t, proj);
}



void log_proj(project *proj)
{
	log_proj_intern(proj, 0);
}



void log_exit(void)
{
	static was_run = 0;
	
	if ((!config_logfile_name) || (!config_logfile_use))
		return;
	/* TODO: make shure this isn't run twice at exit */
	g_return_if_fail(was_run == 0);
	was_run++;
	log_last(-1, NULL);
	log_write(-1, _("program exited"));
}



void
log_start(void)
{
	last_msg = g_strdup(_("program started"));
}



void log_endofday(void)
{
	if ((!config_logfile_name) || (!config_logfile_use))
		return;
	/* force a flush of the logfile */
	log_proj_intern(NULL, 1);
}

