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


#include "ctree.h"
#include "err-throw.h"
#include "proj.h"
#include "proj_p.h"

static void project_list_time_reset (void);

// hack alert -- should be static
GList * plist = NULL;

/* ============================================================= */

static int next_free_id = 1;

GttProject *
gtt_project_new(void)
{
	GttProject *proj;
	
	proj = g_new0(GttProject, 1);
	proj->title = g_strdup ("");
	proj->desc = g_strdup ("");
	proj->notes = g_strdup ("");
	proj->custid = NULL;
	proj->min_interval = 3;
	proj->auto_merge_interval = 60;
	proj->auto_merge_gap = 60;
	proj->secs_ever = 0;
	proj->secs_day = 0;
	proj->billrate = 1.0;
	proj->overtime_rate = 1.5;
	proj->overover_rate = 2.0;
	proj->flat_fee = 1.0;
	proj->task_list = NULL;
	proj->sub_projects = NULL;
	proj->parent = NULL;
        proj->trow = NULL;

        proj->id = next_free_id;
	next_free_id ++;

	return proj;
}


GttProject *
gtt_project_new_title_desc(const char *t, const char *d)
{
	GttProject *proj;

	proj = gtt_project_new();
	if (t) { g_free(proj->title); proj->title = g_strdup (t); }
        if (d) { g_free(proj->desc); proj->desc = g_strdup (d); }
	return proj;
}



GttProject *
gtt_project_dup(GttProject *proj)
{
	GList *node;
	GttProject *p;

	if (!proj) return NULL;
	p = gtt_project_new();
	p->min_interval = proj->min_interval;
	p->auto_merge_interval = proj->auto_merge_interval;
	p->auto_merge_gap = proj->auto_merge_gap;
	p->secs_ever = proj->secs_ever;
	p->secs_day = proj->secs_day;

	g_free(p->title);
	g_free(p->desc);
	g_free(p->notes);

	p->title = g_strdup(proj->title);
	p->desc = g_strdup(proj->desc);
	p->notes = g_strdup(proj->notes);
	if (proj->custid) p->custid = g_strdup(proj->custid);

	p->billrate = proj->billrate;
	p->overtime_rate = proj->overtime_rate;
	p->overover_rate = proj->overover_rate;
	p->flat_fee = proj->flat_fee;

	/* Don't copy the tasks.  Do copy the sub-projects */
	for (node=proj->sub_projects; node; node=node->next)
	{
		GttProject *sub = node->data;
		sub = gtt_project_dup (sub);
		gtt_project_append_project (p, sub);
	}

	p->sub_projects = NULL;
	return p;
}


/* remove the roject from any lists, etc. */
void 
gtt_project_remove(GttProject *p)
{
	/* if we are in someone elses list, remove */
	if (p->parent)
	{
		p->parent->sub_projects = 
			g_list_remove (p->parent->sub_projects, p);
		p->parent = NULL;
	}
	else
	{
		/* else we are in the master list ... remove */
		plist = g_list_remove(plist, p);
	}
}


void 
gtt_project_destroy(GttProject *proj)
{
	if (!proj) return;
	gtt_project_remove(proj);

	if (proj->title) g_free(proj->title);
	proj->title = NULL;

	if (proj->desc) g_free(proj->desc);
	proj->desc = NULL;

	if (proj->notes) g_free(proj->desc);
	proj->notes = NULL;

	if (proj->custid) g_free(proj->custid);
	proj->custid = NULL;

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
		while (proj->sub_projects)
		{
			/* destroying a sub-project pops it off the list */
			gtt_project_destroy (proj->sub_projects->data);
		}
	}

	g_free(proj);
}


/* ========================================================= */
/* set/get project titles and descriptions */

void 
gtt_project_set_title(GttProject *proj, const char *t)
{
	if (!proj) return;
	if (proj->title) g_free(proj->title);
	if (!t) {
		proj->title = g_strdup ("");
		return;
	}
	proj->title = g_strdup(t);
        ctree_update_title(proj);
}



void 
gtt_project_set_desc(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->desc) g_free(proj->desc);
	if (!d) {
		proj->desc = g_strdup ("");
		return;
	}
	proj->desc = g_strdup(d);
	ctree_update_desc(proj);
}

void 
gtt_project_set_notes(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->notes) g_free(proj->notes);
	if (!d) {
		proj->notes = g_strdup ("");
		return;
	}
	proj->notes = g_strdup(d);
}

void 
gtt_project_set_custid(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->custid) g_free(proj->custid);
	if (!d) {
		proj->custid = NULL;
		return;
	}
	proj->custid = g_strdup(d);
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

const char * 
gtt_project_get_notes (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->notes);
}

const char * 
gtt_project_get_custid (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->custid);
}

void
gtt_project_set_billrate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->billrate = r;
}

double
gtt_project_get_billrate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->billrate;
}

void
gtt_project_set_overtime_rate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->overtime_rate = r;
}

double
gtt_project_get_overtime_rate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->overtime_rate;
}

void
gtt_project_set_overover_rate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->overover_rate = r;
}

double
gtt_project_get_overover_rate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->overover_rate;
}

void
gtt_project_set_flat_fee (GttProject *proj, double r)
{
	if (!proj) return;
	proj->flat_fee = r;
}

double
gtt_project_get_flat_fee (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->flat_fee;
}

void
gtt_project_set_min_interval (GttProject *proj, int r)
{
	if (!proj) return;
	proj->min_interval = r;
}

int
gtt_project_get_min_interval (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->min_interval;
}

void
gtt_project_set_auto_merge_interval (GttProject *proj, int r)
{
	if (!proj) return;
	proj->auto_merge_interval = r;
}

int
gtt_project_get_auto_merge_interval (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->auto_merge_interval;
}

void
gtt_project_set_auto_merge_gap (GttProject *proj, int r)
{
	if (!proj) return;
	proj->auto_merge_gap = r;
}

int
gtt_project_get_auto_merge_gap (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->auto_merge_gap;
}

/* =========================================================== */

void
gtt_project_set_id (GttProject *proj, int new_id)
{
	if (!proj) return;

	if (gtt_project_locate_from_id (new_id))
	{
		g_warning ("a project with id =%d already exists\n", new_id);
	}

	/* We try to conserve id numbers by seeing if this was a fresh
	 * project. Conserving numbers is good, since things are less
	 * confusing when looking at the data */
	if ((proj->id+1) == next_free_id) next_free_id--;

	proj->id = new_id;
	if (new_id >= next_free_id) next_free_id = new_id + 1;
}

int
gtt_project_get_id (GttProject *proj)
{
	if (!proj) return -1;
	return proj->id;
}

/* =========================================================== */
/* compatibility interface */

static time_t
get_midnight (time_t last)
{
	struct tm lt;
	time_t midnight;

	if (0 >= last)
	{
		last = time(0);
	}

	memcpy (&lt, localtime (&last), sizeof (struct tm));
	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 0;
	midnight = mktime (&lt);

	return midnight;
}

void 
gtt_project_compat_set_secs (GttProject *proj, int sever, int sday, time_t last)
{
	time_t midnight;
	GttTask *tsk;
	GttInterval *ivl;

	proj->secs_ever = sever;
	proj->secs_day = sday;

	/* get the midnight of the last update */
	midnight = get_midnight (last);

	/* old GTT data will just be one big interval 
	 * lumped under one task */
	tsk = gtt_task_new ();
	gtt_task_set_memo (tsk, _("Old GTT Tasks"));

	ivl = gtt_interval_new();
	ivl->stop = midnight - 1;
	ivl->start = midnight - 1 - sever + sday;
	gtt_task_add_interval (tsk, ivl);

	if (0 < sday)
	{
		ivl = gtt_interval_new();
		ivl->start = midnight +1;
		ivl->stop = midnight + sday + 1;
		gtt_task_add_interval (tsk, ivl);
	}

	gtt_project_add_task (proj, tsk);

	/* All new data will get its own task */
	tsk = gtt_task_new ();
	gtt_task_set_memo (tsk, _("New Task"));
	gtt_project_add_task (proj, tsk);
}

/* =========================================================== */
/* add subprojects, tasks */

void 
gtt_project_append_project (GttProject *proj, GttProject *child)
{
	if (!child) return;
	gtt_project_remove(child);
	if (!proj)
	{
		/* if no parent specified, put in top-level list */
		gtt_project_list_append (child);
		return;
	}

	proj->sub_projects = g_list_append (proj->sub_projects, child);
	child->parent = proj;
}

void 
gtt_project_insert_before(GttProject *p, GttProject *before_me)
{
	gint pos;

	if (!p) return;
	gtt_project_remove(p);

	/* no before ?? then append to master list */
	if (!before_me)
	{
		plist = g_list_append (plist, p);
		p->parent = NULL;
		return;
	}
	else
	{
		if (!before_me->parent)
		{
			/* if before_me has no parent, then its in the 
			 * master list */
			pos  = g_list_index (plist, before_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos) 
			{
				plist = g_list_append (plist, p);
				p->parent = NULL;
				return;
			}
			plist = g_list_insert (plist, p, pos);
			p->parent = NULL;
			return;
		}
		else
		{
			GList *sub;
			sub = before_me->parent->sub_projects;
			pos  = g_list_index (sub, before_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos) 
			{
				sub = g_list_append (sub, p);
				before_me->parent->sub_projects = sub;
				p->parent = before_me->parent;
				return;
			}
			sub = g_list_insert (sub, p, pos);
			before_me->parent->sub_projects = sub;
			p->parent = before_me->parent;
			return;
		}
	}
}

void 
gtt_project_insert_after(GttProject *p, GttProject *after_me)
{
	gint pos;

	if (!p) return;
	gtt_project_remove(p);

	/* no after ?? then prepend to master list */
	if (!after_me)
	{
		plist = g_list_prepend (plist, p);
		p->parent = NULL;
		return;
	}
	else
	{
		if (!after_me->parent)
		{
			/* if after_me has no parent, then its in the 
			 * master list */
			pos  = g_list_index (plist, after_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos) 
			{
				plist = g_list_prepend (plist, p);
				p->parent = NULL;
				return;
			}
			pos ++;
			plist = g_list_insert (plist, p, pos);
			p->parent = NULL;
			return;
		}
		else
		{
			GList *sub;
			sub = after_me->parent->sub_projects;
			pos  = g_list_index (sub, after_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos) 
			{
				sub = g_list_prepend (sub, p);
				after_me->parent->sub_projects = sub;
				p->parent = after_me->parent;
				return;
			}
			pos ++;
			sub = g_list_insert (sub, p, pos);
			after_me->parent->sub_projects = sub;
			p->parent = after_me->parent;
			return;
		}
	}
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

GttProject * 
gtt_project_get_parent (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->parent;
}


GList * 
gtt_project_get_tasks (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->task_list;
}

/* =========================================================== */
/* get totals */

static int
project_list_total_secs_day (GList *prjs)
{
	GList *node;
	int total = 0;
	if (!prjs) return 0;

	for (node=prjs; node; node=node->next)
	{
		GttProject *proj = node->data;
		total += proj->secs_day;
		if (proj->sub_projects)
		{
			total += project_list_total_secs_day (proj->sub_projects);
		}
	}
	return total;
}

int
gtt_project_list_total_secs_day (void)
{
	return project_list_total_secs_day(plist);
}



/* this routine adds up total day-secs for this project, and its sub-projects */
int
gtt_project_total_secs_day (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;

	total = proj->secs_day;
	if (proj->sub_projects)
	{
		total += project_list_total_secs_day (proj->sub_projects);
	}
	return total;
}

int
gtt_project_get_secs_day (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_day;
}


static int
project_list_total_secs_ever (GList *prjs)
{
	GList *node;
	int total = 0;
	if (!prjs) return 0;

	for (node=prjs; node; node=node->next)
	{
		GttProject *proj = node->data;
		total += proj->secs_ever;
		if (proj->sub_projects)
		{
			total += project_list_total_secs_ever (proj->sub_projects);
		}
	}
	return total;
}

int
gtt_project_list_total_secs_ever (void)
{
	return project_list_total_secs_ever(plist);
}


/* this routine adds up total secs for this project, and its sub-projects */
int
gtt_project_total_secs_ever (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;

	total = proj->secs_ever;
	if (proj->sub_projects)
	{
		total += project_list_total_secs_ever (proj->sub_projects);
	}
	return total;
}

int
gtt_project_get_secs_ever (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_ever;
}


/* =========================================================== */
/* Recomputed cached data.  Scrub it while we're at it. */

static void
scrub_intervals (GttTask *tsk, GttProject *prj)
{
	GList *node;
	int not_done = TRUE;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node=node->next)
		{
			GttInterval *ivl = node->data;
			if ((ivl->stop - ivl->start) <= prj->min_interval)
			{
				tsk->interval_list = g_list_remove (tsk->interval_list, ivl);
				not_done = TRUE;
				break;
			}
		}
	}
}

void
gtt_project_compute_secs (GttProject *proj)
{
	int total_ever = 0;
	int total_day = 0;
	time_t midnight;

	GList *tsk_node, *ivl_node, *prj_node;

	midnight = get_midnight (-1);
	/* total up tasks */
	for (tsk_node= proj->task_list; tsk_node; tsk_node=tsk_node->next)
	{
		GttTask * task = tsk_node->data;
		scrub_intervals (task, proj);
		for (ivl_node= task->interval_list; ivl_node; ivl_node=ivl_node->next)
		{
			GttInterval *ivl = ivl_node->data;
			total_ever += ivl->stop - ivl->start;
			if (ivl->start >= midnight)
			{
				total_day += ivl->stop - ivl->start;
			}
			else if (ivl->stop > midnight)
			{
				total_day += ivl->stop - midnight;
			}
		}
	}

	proj->secs_ever = total_ever;
	proj->secs_day = total_day;

	/* do the subprojects as well */
	for (prj_node= proj->sub_projects; prj_node; prj_node=prj_node->next)
	{
		GttProject * prj = prj_node->data;
		gtt_project_compute_secs (prj);
	}
}

void
gtt_project_list_compute_secs (void)
{
	GList *node;
	for (node= plist; node; node=node->next)
	{
		GttProject * prj = node->data;
		gtt_project_compute_secs (prj);
	}
}

/* =========================================================== */

void 
gtt_clear_daily_counter (GttProject *proj)
{
	if (!proj) return;
	proj->secs_day = 0;
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
		gtt_project_list_compute_secs ();
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
		task->memo = g_strdup (_("New Task"));
	}

	/* By definition, the current task is the one at the head 
	 * of the list, and the current interval is  at the head 
	 * of the task */
	task = proj->task_list->data;
	g_return_if_fail (task);
	ival = g_new0 (GttInterval, 1);
	ival->start = time(0);
	ival->stop = ival->start;
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

	prev_update = ival->stop;
	ival->stop = now;
	diff = now - prev_update;
	proj->secs_ever += diff;
	proj->secs_day += diff;
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

	/* do not record zero-length or very short intervals */
	if (proj->min_interval >= (ival->start - ival->stop))
	{
		task->interval_list = g_list_remove (task->interval_list, ival);
		g_free (ival);
	}
}

/* =========================================================== */

GttTask *
gtt_task_new (void)
{
	GttTask *task;

	task = g_new0(GttTask, 1);
	task->memo = NULL;
	task->notes = NULL;
	task->billable = GTT_BILLABLE;
	task->billrate = GTT_REGULAR;
	task->bill_unit = 900;
	task->interval_list = NULL;
	return task;
}

void
gtt_task_destroy (GttTask *task)
{
	if (!task) return;
	if (task->memo) g_free(task->memo);
	task->memo = NULL;
	if (task->notes) g_free(task->notes);
	task->notes = NULL;
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
	tsk->interval_list = g_list_prepend (tsk->interval_list, ival);
	ival->parent = tsk;
}

void
gtt_task_append_interval (GttTask *tsk, GttInterval *ival)
{
	if (!tsk || !ival) return;
	tsk->interval_list = g_list_append (tsk->interval_list, ival);
	ival->parent = tsk;
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

void 
gtt_task_set_notes(GttTask *tsk, const char *m)
{
	if (!tsk) return;
	if (tsk->notes) g_free(tsk->notes);
	if (!m) 
	{
		tsk->notes = NULL;
		return;
	}
	tsk->notes = g_strdup(m);
}

const char * 
gtt_task_get_memo (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->memo;
}

const char * 
gtt_task_get_notes (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->notes;
}

void
gtt_task_set_billable (GttTask *tsk, GttBillable b)
{
	if (!tsk) return;
	tsk->billable = b;
}

GttBillable
gtt_task_get_billable (GttTask *tsk)
{
	if (!tsk) return GTT_HOLD;
	return tsk->billable;
}

void
gtt_task_set_billrate (GttTask *tsk, GttBillRate b)
{
	if (!tsk) return;
	tsk->billrate = b;
}

GttBillRate
gtt_task_get_billrate (GttTask *tsk)
{
	if (!tsk) return GTT_REGULAR;
	return tsk->billrate;
}

void
gtt_task_set_bill_unit (GttTask *tsk, int b)
{
	if (!tsk) return;
	tsk->bill_unit = b;
}

int
gtt_task_get_bill_unit (GttTask *tsk)
{
	if (!tsk) return GTT_REGULAR;
	return tsk->bill_unit;
}

GList *
gtt_task_get_intervals (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->interval_list;
}

/* =========================================================== */

int
gtt_task_get_secs_ever (GttTask *tsk)
{
	GList *node;
	int total = 0;

	for (node=tsk->interval_list; node; node=node->next)
	{
		GttInterval * ivl = node->data;
		total += ivl->stop - ivl->start;
	}
	return total;
}

/* =========================================================== */

GttInterval *
gtt_interval_new (void)
{
	GttInterval * ivl;
	ivl = g_new0 (GttInterval, 1);
	ivl->parent = NULL;
	ivl->start = 0;
	ivl->stop = 0;
	ivl->running = FALSE;
	ivl->fuzz = 0;
	return ivl;
}

void 
gtt_interval_destroy (GttInterval * ivl)
{
	if (!ivl) return;
	if (ivl->parent)
	{
		/* unhook myself from the chain */
		ivl->parent->interval_list = 
			g_list_remove (ivl->parent->interval_list, ivl);
	}
	ivl->parent = NULL;
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
gtt_interval_set_fuzz (GttInterval *ivl, int st)
{
	if (!ivl) return;
	ivl->fuzz = st;
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

int
gtt_interval_get_fuzz (GttInterval * ivl)
{
	if (!ivl) return 0;
	return ivl->fuzz;
}

gboolean 
gtt_interval_get_running (GttInterval * ivl)
{
	if (!ivl) return FALSE;
	return (gboolean) ivl->running;
}

/* ============================================================= */

GttInterval *
gtt_interval_merge_up (GttInterval *ivl)
{
	GList *node;
	GttInterval *merge;
	GttTask *prnt;

	if (!ivl) return NULL;
	prnt = ivl->parent;
	if (!prnt) return NULL;

	for (node=prnt->interval_list; node; node=node->next)
	{
		if (ivl == node->data) break;
	}
	if (!node) return NULL;
	node = node->prev;
	if (!node) return NULL;

	merge = node->data;
	if (!merge) return NULL;

	merge->start -= (ivl->stop - ivl->start);
	if (ivl->fuzz > merge->fuzz) merge->fuzz = ivl->fuzz;
	gtt_interval_destroy (ivl);
	return merge;
}


GttInterval *
gtt_interval_merge_down (GttInterval *ivl)
{
	GList *node;
	GttInterval *merge;
	GttTask *prnt;

	if (!ivl) return NULL;
	prnt = ivl->parent;
	if (!prnt) return NULL;

	for (node=prnt->interval_list; node; node=node->next)
	{
		if (ivl == node->data) break;
	}
	if (!node) return NULL;
	node = node->next;
	if (!node) return NULL;

	merge = node->data;
	if (!merge) return NULL;

	merge->stop -= (ivl->stop - ivl->start);
	if (ivl->fuzz > merge->fuzz) merge->fuzz = ivl->fuzz;
	gtt_interval_destroy (ivl);
	return merge;
}

/* ============================================================= */
/* project_list -- simple wrapper around glib routines */
/* hack alert -- this list should probably be replaced with a 
 * GNode tree ... */


GList *
gtt_get_project_list (void)
{
	return plist;
}

void 
gtt_project_list_append(GttProject *p)
{
	gtt_project_insert_before (p, NULL);
}


void 
project_list_destroy(void)
{
	while (plist)
	{
		/* destroying a project pops the plist ... */
		gtt_project_destroy(plist->data);
	}
}

static void
project_list_sort(int (cmp)(const void *, const void *))
{
	plist = g_list_sort (plist, cmp);
}

/* -------------------- */
/* given id, walk the tree of projects till we find it. */

static GttProject *
locate_from_id (GList *prj_list, int prj_id)
{
	GList *node;
	for (node = prj_list; node; node=node->next)
	{
		GttProject *prj = node->data;
		if (prj_id == prj->id) return prj;

		/* recurse to handle sub-projects */
		if (prj->sub_projects)
		{
			prj = locate_from_id (prj->sub_projects, prj_id);
			if (prj) return prj;
		}
	}
	return NULL;  /* not found */
}

GttProject *
gtt_project_locate_from_id (int prj_id)
{
	return locate_from_id (plist, prj_id);
}

/* ============================================================= */
/* misc GUI-related routines */

static void 
project_list_time_reset(void)
{
	GList *node;
	for (node = plist; node; node=node->next)
	{
		GttProject *prj = node->data;
		prj->secs_day = 0;
		ctree_update_label(prj);
	}
}

/* ==================================================================== */
/* sort funcs */


static int
cmp_time(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
        return (gtt_project_total_secs_day(b) - gtt_project_total_secs_day(a));
}

static int
cmp_total_time(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
        return (gtt_project_total_secs_ever(b) - gtt_project_total_secs_ever(a));
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
