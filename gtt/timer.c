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


gint main_timer = 0;
time_t last_timer = -1;
static int timer_stopped = 1;



static gint timer_func(gpointer data)
{
	time_t t, diff_time;
	struct tm t1, t0;

	log_proj(cur_proj);
	t = time(NULL);
	if (last_timer != -1) {
		memcpy(&t0, localtime(&last_timer), sizeof(struct tm));
		memcpy(&t1, localtime(&t), sizeof(struct tm));
		if ((t0.tm_year != t1.tm_year) ||
		    (t0.tm_yday != t1.tm_yday)) {
			log_endofday();
			project_list_time_reset();
		}
                if (!timer_stopped)
                        diff_time = t - last_timer;
                else
                        diff_time = 1;
	} else {
                diff_time = 1;
        }
        timer_stopped = 0;
	last_timer = t;
	if (!cur_proj) return 1;
	cur_proj->secs += diff_time;
	cur_proj->day_secs += diff_time;
	if (config_show_secs) {
		if (cur_proj->row != -1) clist_update_label(cur_proj);
	} else if (cur_proj->day_secs % 60 == 0) {
		if (cur_proj->row != -1) clist_update_label(cur_proj);
	}
	return 1;
}




void menu_timer_state(int);

void start_timer(void)
{
	if (main_timer) return;
	main_timer = gtk_timeout_add(1000, timer_func, NULL);
}



void stop_timer(void)
{
	if (!main_timer) return;
	gtk_timeout_remove(main_timer);
	main_timer = 0;
        timer_stopped = 1;
}

