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

#include "ctree.h"
#include "cur-proj.h"
#include "dialog.h"
#include "gtt.h"
#include "idle-timer.h"
#include "log.h"
#include "prefs.h"
#include "proj.h"


static gint main_timer = 0;
static IdleTimeout *idt = NULL;

int config_idle_timeout = -1;

static void
restart_proj (GtkWidget *w, gpointer data)
{
	GttProject *prj = data;
	ctree_select (prj);
	cur_proj_set (prj);
}

static gint 
timer_func(gpointer data)
{
	log_proj(cur_proj);
	if (!cur_proj) return 1;

	gtt_project_timer_update (cur_proj);

	if (config_show_secs) 
	{
		ctree_update_label(cur_proj);
	} 
	else if (0 == gtt_project_total_secs_day(cur_proj)) 
	{
		ctree_update_label(cur_proj);
	}

	if (0 < config_idle_timeout) 
	{
		int idle_time;
		idle_time = time(0) - poll_last_activity (idt);
		if (idle_time > config_idle_timeout) 
		{
			char *msg;
			GttProject *prj = cur_proj;
			ctree_unselect (cur_proj);

			/* don't just stop the timer, make the needed 
			 * higher-level calls */
			/* stop_timer(); */
			cur_proj_set (NULL);

			/* warn the user */
			msg = g_strdup_printf (
				_("It seems that the system has been idle\n"
				  "for %d minutes, and the currently running\n"
				  "project (%s - %s)\n"
				  "has been stopped.\n"
				  "Do you want to restart it?"),
				(config_idle_timeout+30)/60,
				gtt_project_get_title(prj),
				gtt_project_get_desc(prj));
			qbox_ok_cancel (_("System Idle"), msg,
				GNOME_STOCK_BUTTON_YES, restart_proj, prj, 
				GNOME_STOCK_BUTTON_NO, NULL, NULL);
			return 0;
		}
	}
	return 1;
}


void menu_timer_state(int);

void 
start_timer(void)
{
	if (main_timer) return;
	log_proj(cur_proj);
	gtt_project_timer_start (cur_proj);

	if ((0 < config_idle_timeout) && !idt) idt = idle_timeout_new();

	/* the timer is measured in milliseconds, so 1000
         * means it pops once a second. */
	main_timer = gtk_timeout_add(1000, timer_func, NULL);
	update_status_bar();
}


void 
stop_timer(void)
{
	if (!main_timer) return;
	log_proj(NULL);
	gtt_project_timer_stop (cur_proj);
	gtk_timeout_remove(main_timer);
	main_timer = 0;
	update_status_bar();
}

gboolean 
timer_is_running (void)
{
	return (0 != main_timer);
}
