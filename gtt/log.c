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


static int log_write(time_t t, char *s)
{
	FILE *f;
	char date[20];
	char *filename;

	if (!config_logfile_name) return 1;
	g_return_val_if_fail(s != NULL, 0);
	if ((config_logfile_name[0] == '~') &&
	    (config_logfile_name[1] == '/') &&
	    (config_logfile_name[2] != 0)) {
		filename = gnome_util_prepend_user_home(&config_logfile_name[2]);
		f = fopen(filename, "at");
		g_free(filename);
	} else {
		f = fopen(config_logfile_name, "at");
	}
	g_return_val_if_fail(f != NULL, 0);
	if (t == -1) t = time(NULL);
	strftime(date, 20, "%b %d %H:%M:%S", localtime(&t));
	fprintf(f, "%s %s\n", date, s);
	fclose(f);
	return 1;
}


static char *
build_log_entry(char *format, project *proj)
{
	GString *str;
	char tmp[256];
	char *p;

	if (!format)
		format = config_logfile_str;
	if (!proj)
		return g_strdup(_("program started"));
	if ((!format) || (!format[0]))
		return g_strdup(proj->title);
	str = g_string_new(NULL);
	for (p = format; *p; p++) {
		if (*p != '%') {
			g_string_append_c(str, *p);
		} else {
			p++;
			switch (*p) {
			case 't':
				g_string_append(str, proj->title);
				break;
			case 'd':
				if (proj->desc)
					g_string_append(str, proj->desc);
				else
					g_string_append(str,
							_("no description"));
				break;
			case 'T':
				sprintf(tmp, "%d:%02d:%02d", proj->secs / 3600,
					(proj->secs / 60) % 60,
					proj->secs % 60);
				g_string_append(str, tmp);
				break;
			case 'm':
				sprintf(tmp, "%d", proj->day_secs / 60);
				g_string_append(str, tmp);
				break;
			case 'M':
				sprintf(tmp, "%02d",
					(proj->day_secs / 60) % 60);
				g_string_append(str, tmp);
				break;
			case 's':
				sprintf(tmp, "%d", proj->day_secs);
				g_string_append(str, tmp);
				break;
			case 'S':
				sprintf(tmp, "%02d", proj->day_secs % 60);
				g_string_append(str, tmp);
				break;
			case 'h':
				sprintf(tmp, "%d", proj->day_secs / 3600);
				g_string_append(str, tmp);
				break;
			case 'H':
				sprintf(tmp, "%02d", proj->day_secs / 3600);
				g_string_append(str, tmp);
				break;
			default:
				g_string_append_c(str, *p);
				break;
			}
		}
	}
	p = str->str;
	g_string_free(str, 0);
	return p;
}



static char *last_msg = NULL;
static char *last_logged_msg = NULL;
static time_t last_time = -1;

static void log_last(time_t t, project *proj)
{
	char *s;
	static project *last_proj = NULL;

	if (last_msg) {
		s = last_msg;
	} else {
		/* s = g_strdup(_("stopped project")); */
		s = build_log_entry(config_logfile_stop, last_proj);
	}
	if (proj)
		last_proj = proj;
	if ((!last_logged_msg) || (0 != strcmp(s, last_logged_msg))) {
		if (!log_write(last_time, s)) {
			g_warning("couldn't write to logfile `%s'.\n",
				  config_logfile_name?config_logfile_name:"");
		}
	}
	if (last_logged_msg) g_free(last_logged_msg);
	last_logged_msg = s;
	if (proj) last_msg = build_log_entry(NULL, proj);
	else last_msg = NULL;
	last_time = t;
}

static void log_proj_intern(project *proj, int log_if_equal)
{
	time_t t;
	char *new_msg;

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
		new_msg = build_log_entry(NULL, proj);
		if (proj == NULL) {
			if (last_msg) last_time = t;
		} else {
			if ((!last_msg) ||
			    (0 != strcmp(last_msg, new_msg)))
				last_time = t;
		}
		if (last_msg) g_free(last_msg);
		last_msg = (proj) ? g_strdup(new_msg) : NULL;
		g_free(new_msg);
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
	if ((!config_logfile_name) || (!config_logfile_use))
		return;
	log_proj_intern(NULL, 0);
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

