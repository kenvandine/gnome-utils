/*   gtt project private data structure file.
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

#ifndef __GTT_PROJ_P_H__
#define __GTT_PROJ_P_H__

#include "config.h"

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif /* TM_IN_SYS_TIME */
#include <time.h>
#endif /* TIME_WITH_SYS_TIME */

#include "gtt.h"
#include "proj.h"


struct gtt_project_s {
	char *title;     /* short title */
	char *desc;      /* breif description for invoice */
	char *notes;     /* long description */
	char *custid;    /* customer id */

	int min_interval;  /* smallest recorded interval */
	int auto_merge_interval;  /* merge intervals smaller than this */
	int secs_ever;   /* seconds spend on this project */
	int secs_day;    /* seconds spent on this project today */

        double billrate;   /* billing rate, in units of currency per hour */
        double overtime_rate;  /*  in units of currency per hour */
        double overover_rate;  /*  the good money is here ... */
        double flat_fee;  /* flat price, in units of currency */

	GList *task_list;      /* annotated chunks of time */

	/* hack alert -- the project heriarachy should probably be 
	 * reimplemented as a GNode rather than a GList */
	GList *sub_projects;   /* sub-projects */
	GttProject *parent;    /* back pointer to parent project */

	int id;		/* simple id number */

        /* miscellaneous -- used bu GUI to display */
        GtkCTreeNode *trow;
};


/* A 'task' is a group of start-stops that have a common 'memo'
 * associated with them.   Note that by definition, the 'current',
 * active interval is the one at the head of the list.
 */
struct gtt_task_s {
	char * memo;            /* invoiceable memo (customer sees this) */
	char * notes;           /* internal notes (office private) */
	GttBillable  billable;  /* if fees can be collected for this task */
	GttBillRate  billrate;  /* hourly rate at which to bill */
	GList *interval_list;   /* collection of start-stop's */
};

/* one start-stop interval */
struct gtt_interval_s {
	time_t	start;		/* when the timer started */
	time_t	stop;		/* if stopped, shows when timer stopped, 
				 * if running, then the most recent log point */
	short	running;	/* boolean: is the timer running? */
};


/* some misc 'private' routines */
void zero_on_rollover (time_t now);
void set_last_reset (time_t last);


#endif /* __GTT_PROJ_P_H__ */
