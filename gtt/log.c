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




void log_proj(project *proj)
{
	static project *last_proj = NULL;
	static time_t last_time = -1;

	if ((!config_logfile_name) || (!config_logfile_use))
		return;
	if (last_proj == proj) return;
	if (last_time == -1) last_time = time(NULL);
}



void log_endofday(void)
{
	if ((!config_logfile_name) || (!config_logfile_use))
		return;
	/* force a flush of the logfile */
	log_proj(NULL);
}

