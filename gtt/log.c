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
#include <gtk/gtk.h>

#include "gtt.h"

#include <stdlib.h>
#include <stdio.h>



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



static void log_proj_intern(project *proj, int log_if_equal)
{
	static project *last_proj = NULL;
	static time_t last_time = -1;
	time_t t;

	if ((!config_logfile_name) || (!config_logfile_use)) {
		last_proj = NULL;
		last_time = -1;
		return;
	}
	if ((last_proj == proj) && (!log_if_equal)) return;
	t = time(NULL);
	if ((last_time != -1) &&
	    ((long int)(t - last_time) < config_logfile_min_secs)) {
		last_proj = proj;
		last_time = t;
		return;
	}
	if (!last_proj) {
		if (last_time != -1)
			if (!log_write(last_time, "stopped project"))
				g_warning("couldn't write to logfile \"%s\"\n",
					  config_logfile_name);
	} else 	if (!log_write(last_time, last_proj->title)) {
		g_warning("couldn't write to logfile \"%s\"\n", config_logfile_name);
	}
	last_time = t;
	last_proj = proj;
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
	log_proj_intern(NULL, 1);
	log_write(-1, "program exited");
}



void log_endofday(void)
{
	if ((!config_logfile_name) || (!config_logfile_use))
		return;
	/* force a flush of the logfile */
	log_proj_intern(NULL, 1);
}

