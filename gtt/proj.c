/*   project structure handling for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "err-throw.h"
#include "proj.h"
#include "proj_p.h"


GttProject *
gtt_project_new(void)
{
	GttProject *proj;
	
	proj = g_new0(GttProject, 1);
	proj->title = NULL;
	proj->desc = NULL;
	proj->secs = proj->day_secs = 0;
	proj->rate = 1.0;
	proj->task_list = NULL;
	proj->sub_projects = NULL;
        proj->row = -1;
	return proj;
}


GttProject *
gtt_project_new_title_desc(const char *t, const char *d)
{
	GttProject *proj;

	proj = gtt_project_new();
	if (!t || !d) return proj;
	proj->title = g_strdup (t);
        proj->desc = g_strdup (d);
	return proj;
}



GttProject *
project_dup(GttProject *proj)
{
	GttProject *p;

	if (!proj) return NULL;
	p = gtt_project_new();
	p->secs = proj->secs;
	p->day_secs = proj->day_secs;
	if (proj->title)
		p->title = g_strdup(proj->title);
	else
		p->title = NULL;
	if (proj->desc)
		p->desc = g_strdup(proj->desc);
	else
		p->desc = NULL;

	p->rate = proj->rate;
	p->task_list = NULL;
	p->sub_projects = NULL;
	return p;
}



void 
project_destroy(GttProject *proj)
{
	if (!proj) return;
	project_list_remove(proj);
	g_free(proj->title);
	proj->title = NULL;
	g_free(proj->desc);
	proj->desc = NULL;

        if (proj->task_list)
	{
		GList *node;
		for (node=proj->task_list; node; node=node->next)
		{
			gtt_task_destroy (node->data);
		}
		g_list_free (proj->task_list);
	}
	proj->task_list = NULL;

        if (proj->sub_projects)
	{
		GList *node;
		for (node=proj->sub_projects; node; node=node->next)
		{
			project_destroy (node->data);
		}
		g_list_free (proj->sub_projects);
	}
	proj->sub_projects = NULL;

	g_free(proj);
}


/* ========================================================= */

void 
gtt_project_set_title(GttProject *proj, const char *t)
{
	if (!proj) return;
	if (proj->title) g_free(proj->title);
	if (!t) {
		proj->title = NULL;
		return;
	}
	proj->title = g_strdup(t);
        if (proj->row != -1)
                clist_update_title(proj);
}



void 
gtt_project_set_desc(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->desc) g_free(proj->desc);
	if (!d) {
		proj->desc = NULL;
		return;
	}
	proj->desc = g_strdup(d);
	if (proj->row != -1)
		clist_update_desc(proj);
}

const char * 
gtt_project_get_title (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->title);
}

const char * 
gtt_project_get_desc (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->desc);
}

void
gtt_project_set_rate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->rate = r;
}

double
gtt_project_get_rate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->rate;
}

/* =========================================================== */

void 
gtt_project_add_project (GttProject *proj, GttProject *child)
{
	if (!proj || !child) return;
	proj->sub_projects = g_list_append (proj->sub_projects, child);
}

void 
gtt_project_add_task (GttProject *proj, GttTask *task)
{
	if (!proj || !task) return;
	proj->task_list = g_list_append (proj->task_list, task);
}


GList * 
gtt_project_get_children (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->sub_projects;
}


GList * 
gtt_project_get_tasks (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->task_list;
}

/* =========================================================== */
/* zero out day counts if rolled past midnight */

static int day_last_reset = -1;
static int year_last_reset = -1;

void
set_last_reset (time_t last)
{
	struct tm *t0;
	t0 = localtime (&last);
	day_last_reset = t0->tm_yday;
	year_last_reset = t0->tm_year;
}


void
zero_on_rollover (time_t now)
{
	struct tm *t1;

	/* zero out day counts */
	t1 = localtime(&now);
	if ((year_last_reset != t1->tm_year) ||
	    (day_last_reset != t1->tm_yday)) 
	{
		project_list_time_reset();
		log_endofday();
		year_last_reset = t1->tm_year;
	    	day_last_reset = t1->tm_yday;
	}
}


/* =========================================================== */

void 
gtt_project_timer_start (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;

	if (!proj) return;
	if (NULL == proj->task_list)
	{
		task = gtt_task_new();
		proj->task_list = g_list_prepend (NULL, task);
	}

	/* by definition, the current task is the one at the head 
	 * of the list, and the current interval is  at the ehad 
	 * of the task */
	task = proj->task_list->data;
	g_return_if_fail (task);
	ival = g_new0 (GttInterval, 1);
	ival->start = time(0);
	ival->stop = 0;
	ival->running = TRUE;
	task->interval_list = g_list_prepend (task->interval_list, ival);
}

void 
gtt_project_timer_update (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;
	time_t prev_update, now, diff;

	if (!proj) return;
	g_return_if_fail (proj->task_list);

	/* by definition, the current task is the one at the head 
	 * of the list, and the current interval is at the head 
	 * of the task */
	task = proj->task_list->data;
	g_return_if_fail (task);
	g_return_if_fail (task->interval_list);
	ival = task->interval_list->data;
	g_return_if_fail (ival);


	/* compute the delta change, update cached data */
	if (0 != ival->stop)
	{
		prev_update = ival->stop;
	}
	else
	{
		prev_update = ival->start;
	}
	now = time(0);
	zero_on_rollover (now);

	/* if timer isn't running, do nothing.  Its arguabley
	 * an error condition if this routine was called with the timer
	 * stopped .... maybe we should printf a complaint ?
	 */
	if (FALSE == ival->running) 
	{
		g_warning ("update called while timer stopped!\n");
		return;
	}

	ival->stop = now;
	diff = now - prev_update;
	proj->secs += diff;
	proj->day_secs += diff;
}

void 
gtt_project_timer_stop (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;

	if (!proj) return;
	if (!proj->task_list) return;

	gtt_project_timer_update (proj);

	/* Also note that the timer really has stopped. */
	task = proj->task_list->data;
	g_return_if_fail (task);
	g_return_if_fail (task->interval_list);
	ival = task->interval_list->data;
	g_return_if_fail (ival);

	ival->running = FALSE;
}

/* =========================================================== */

GttTask *
gtt_task_new (void)
{
	GttTask *task;

	task = g_new0(GttTask, 1);
	task->memo = NULL;
	task->interval_list = NULL;
	return task;
}

void
gtt_task_destroy (GttTask *task)
{
	if (!task) return;
	if (task->memo) g_free(task->memo);
	task->memo = NULL;
	if (task->interval_list)
	{
		GList *node;
		for (node = task->interval_list; node; node=node->next)
		{
			/* free the individual intervals */
			g_free (node->data);
		}
		g_list_free (task->interval_list);
		task->interval_list = NULL;
	}
}


void
gtt_task_add_interval (GttTask *tsk, GttInterval *ival)
{
	if (!tsk || !ival) return;
	tsk->interval_list = g_list_append (tsk->interval_list, ival);
}

void 
gtt_task_set_memo(GttTask *tsk, const char *m)
{
	if (!tsk) return;
	if (tsk->memo) g_free(tsk->memo);
	if (!m) 
	{
		tsk->memo = NULL;
		return;
	}
	tsk->memo = g_strdup(m);
}

const char * 
gtt_task_get_memo (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->memo;
}

GList *
gtt_task_get_intervals (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->interval_list;
}

/* =========================================================== */

GttInterval *
gtt_interval_new (void)
{
	GttInterval * ivl;
	ivl = g_new0 (GttInterval, 1);
	ivl->start = 0;
	ivl->stop = 0;
	ivl->running = FALSE;
	return ivl;
}

void 
gtt_interval_destroy (GttInterval * ivl)
{
	if (!ivl) return;
	g_free (ivl);
}

void
gtt_interval_set_start (GttInterval *ivl, time_t st)
{
	if (!ivl) return;
	ivl->start = st;
}

void
gtt_interval_set_stop (GttInterval *ivl, time_t st)
{
	if (!ivl) return;
	ivl->stop = st;
}

void
gtt_interval_set_running (GttInterval *ivl, gboolean st)
{
	if (!ivl) return;
	ivl->running = st;
}

time_t 
gtt_interval_get_start (GttInterval * ivl)
{
	if (!ivl) return 0;
	return ivl->start;
}

time_t 
gtt_interval_get_stop (GttInterval * ivl)
{
	if (!ivl) return 0;
	return ivl->stop;
}

gboolean 
gtt_interval_get_running (GttInterval * ivl)
{
	if (!ivl) return FALSE;
	return (gboolean) ivl->running;
}

/*******************************************************************
 * project_list -- simple wrapper around glib routines
 */

GList * plist = NULL;

GList *
gtt_get_project_list (void)
{
	return plist;
}

void 
gtt_project_list_add(GttProject *p)
{
	if (!p) return;
// hack alert XXX FIXME --- disabled for testing
//	plist = g_list_append (plist, p);
}

void 
project_list_add(GttProject *p)
{
	if (!p) return;
	plist = g_list_append (plist, p);
}

void 
project_list_insert(GttProject *p, int pos)
{
	if (!p) return;
	plist = g_list_insert (plist, p, pos);
}

void 
project_list_remove(GttProject *p)
{
	if (!p) return;
	plist = g_list_remove (plist, p);
}

void 
project_list_destroy(void)
{
	GList *node;
	for (node = plist; node; node=node->next)
	{
		project_destroy(node->data);
	}
	g_list_free (plist);
	plist = NULL;
}

static void
project_list_sort(int (cmp)(const void *, const void *))
{
	plist = g_list_sort (plist, cmp);
}


/* ============================================================= */
/* misc GUI-related routines */

void 
project_list_time_reset(void)
{
	GList *node;
	for (node = plist; node; node=node->next)
	{
		GttProject *prj = node->data;
		prj->day_secs = 0;
		if (prj->row != -1) clist_update_label(prj);
	}
}

/* ============================================================= */



char *
project_get_timestr(GttProject *proj, int show_secs)
{
	static char s[20];
	time_t t;
	
	/* if NULL, then we total up over *all* projects */
	if (proj == NULL) 
	{
		GList *node;
		t = 0;
		for (node = plist; node; node = node->next)
		{
			GttProject *prj = node->data;
			t += prj->day_secs;
		}
	} else 
	{
		t = proj->day_secs;
	}
	if (t >= 0) {
		if (show_secs)
			g_snprintf(s, sizeof (s),
				   "%02d:%02d:%02d", (int)(t / 3600),
				   (int)((t % 3600) / 60), (int)(t % 60));
		else
			g_snprintf(s, sizeof (s), "%02d:%02d", (int)(t / 3600),
				   (int)((t % 3600) / 60));
	} else {
		if (show_secs)
			g_snprintf(s, sizeof (s),
				   "-%02d:%02d:%02d", (int)(-t / 3600),
				   (int)((-t % 3600) / 60), (int)(-t % 60));
		else
			g_snprintf(s, sizeof (s),
				   "-%02d:%02d", (int)(-t / 3600),
				   (int)((-t % 3600) / 60));
	}
	return s;
}

char *
project_get_total_timestr(GttProject *proj, int show_secs)
{
	static char s[20];
	time_t t;

	if (proj == NULL) {
		return NULL;
	} else {
		t = proj->secs;
	}
	if (show_secs)
		g_snprintf(s, sizeof (s), "%02d:%02d:%02d", (int)(t / 3600),
			   (int)((t % 3600) / 60), (int)(t % 60));
	else
		g_snprintf(s, sizeof (s), "%02d:%02d", (int)(t / 3600),
			   (int)((t % 3600) / 60));
	return s;
}


/* ==================================================================== */
/* sort funcs */


static int
cmp_time(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
        return (int)(b->day_secs - a->day_secs);
}

static int
cmp_total_time(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (int)(b->secs - a->secs);
}



static int
cmp_title(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
        return strcmp(a->title, b->title);
}

static int
cmp_desc(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	if (!a->desc) {
		return (b->desc == NULL)? 0 : 1;
	}
	if (!b->desc) {
		return -1;
	}
	return strcmp(a->desc, b->desc);
}


void
project_list_sort_time(void)
{
        project_list_sort(cmp_time);
}

void
project_list_sort_total_time(void)
{
	project_list_sort(cmp_total_time);
}

void
project_list_sort_title(void)
{
        project_list_sort(cmp_title);
}

void
project_list_sort_desc(void)
{
	project_list_sort(cmp_desc);
}

/* =========================== END OF FILE ========================= */
