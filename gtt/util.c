/*   utilities for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas
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
#include <glib.h>

#include "util.h"

/* ============================================================== */

void 
print_time (char * buff, int len, int secs, gboolean show_secs)
{
	if (0 <= secs)
	{
		if (show_secs)
		{
			g_snprintf(buff, len,
			   "%02d:%02d:%02d", (int)(secs / 3600),
			   (int)((secs % 3600) / 60), (int)(secs % 60));
		}
		else
		{
			g_snprintf(buff, len, 
			   "%02d:%02d", (int)(secs / 3600),
			   (int)((secs % 3600) / 60));
		}
	} 
	else 
	{
		if (show_secs)
		{
			g_snprintf(buff, len,
			   "-%02d:%02d:%02d", (int)(-secs / 3600),
			   (int)((-secs % 3600) / 60), (int)(-secs % 60));
		}
		else
		{
			g_snprintf(buff, len,
			   "-%02d:%02d", (int)(-secs / 3600),
			   (int)((-secs % 3600) / 60));
		}
	}
}

/* ============================================================== */
/* ===================== END OF FILE ============================ */

