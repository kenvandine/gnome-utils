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
#include <gnome.h>    /* only needed for definition of _() */
#include <libintl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "err-throw.h"
#include "log.h"
#include "proj.h"
#include "proj_p.h"

typedef struct notif_s 
{
	GttProjectChanged func;
	gpointer user_data;
} Notifier;

// hack alert -- should be static/private to this file
GList * plist = NULL;

static void proj_refresh_time (GttProject *proj);
static void proj_modified (GttProject *proj);
static int task_suspend (GttTask *tsk);

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
	proj->billrate = 10.0;
	proj->overtime_rate = 15.0;
	proj->overover_rate = 20.0;
	proj->flat_fee = 1000.0;
	proj->task_list = NULL;
	proj->sub_projects = NULL;
	proj->parent = NULL;
	proj->listeners = NULL;
	proj->being_destroyed = FALSE;
	proj->frozen = FALSE;
	proj->dirty_time = FALSE;
        proj->private_data = NULL;

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


/* remove the project from any lists, etc. */
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

	proj->being_destroyed = TRUE;
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
		while (proj->task_list)
		{
			/* destroying a task pops it off the list */
			gtt_task_destroy (proj->task_list->data);
		}
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

	/* remove notifiers as well */
	{
		Notifier * ntf;
		GList *node;
	
		for (node = proj->listeners; node; node=node->next)
		{
			ntf = node->data;
			g_free (ntf);
		}
		if (proj->listeners) g_list_free (proj->listeners);
	}
	proj->private_data = NULL;
	g_free(proj);
}


/* ========================================================= */
/* set/get project titles and descriptions */

void 
gtt_project_set_title(GttProject *proj, const char *t)
{
	if (!proj) return;
	if (proj->title) g_free(proj->title);
	if (!t) 
	{
		proj->title = g_strdup ("");
		return;
	}
	proj->title = g_strdup(t);
	proj_modified (proj);
}



void 
gtt_project_set_desc(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->desc) g_free(proj->desc);
	if (!d) 
	{
		proj->desc = g_strdup ("");
		return;
	}
	proj->desc = g_strdup(d);
	proj_modified (proj);
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
	proj_modified (proj);
}

void 
gtt_project_set_custid(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->custid) g_free(proj->custid);
	if (!d) 
	{
		proj->custid = NULL;
		return;
	}
	proj->custid = g_strdup(d);
	proj_modified (proj);
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
	proj_modified (proj);
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
	proj_modified (proj);
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
	proj_modified (proj);
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
	proj_modified (proj);
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
	proj_modified (proj);
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
	proj_modified (proj);
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
	proj_modified (proj);
}

int
gtt_project_get_auto_merge_gap (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->auto_merge_gap;
}

void
gtt_project_set_private_data (GttProject *proj, gpointer data)
{
	if (!proj) return;
	proj->private_data = data; 
}

gpointer
gtt_project_get_private_data (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->private_data;
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

	gtt_project_append_task (proj, tsk);

	/* All new data will get its own task */
	tsk = gtt_task_new ();
	gtt_task_set_memo (tsk, _("New Task"));
	gtt_project_append_task (proj, tsk);

	proj_refresh_time (proj);
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
gtt_project_append_task (GttProject *proj, GttTask *task)
{
	if (!proj || !task) return;

	/* if task has a different parent, then reparent */
	if (task->parent)
	{
		task->parent->task_list =
			g_list_remove (task->parent->task_list, task);
		proj_refresh_time(task->parent);
	}

	proj->task_list = g_list_append (proj->task_list, task);
	task->parent = proj;
	proj_refresh_time(proj);
}

void 
gtt_project_prepend_task (GttProject *proj, GttTask *task)
{
	int is_running = FALSE;
	if (!proj || !task) return;

	/* if task has a different parent, then reparent */
	if (task->parent)
	{
		task->parent->task_list =
			g_list_remove (task->parent->task_list, task);
		proj_refresh_time(task->parent);
	}

	/* avoid misplaced running intervals, stop the task */
	if (proj->task_list)
	{
		GttTask *leadtask = proj->task_list->data;
		is_running = task_suspend (leadtask);
	}

	proj->task_list = g_list_prepend (proj->task_list, task);
	task->parent = proj;

	if (is_running) gtt_project_timer_start (proj);
	proj_refresh_time(proj);
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
/* Scrub algorithm as follows (as documented in header files):
 *     Discard all intervals shorter than proj->min_interval
 *     Merge all intervals whose gaps between them are shorter
 *     than proj->auto_merge_gap
 *     Merge all intervals that are shorter than 
 *     proj->auto_merge_interval into the nearest interval.
 *     (but only do this if the nearest is in the same day).
 */

static int
scrub_intervals (GttTask *tsk)
{
	GttProject *prj;
	GList *node;
	int changed = FALSE;
	int mini, merge, mgap;
	int not_done = TRUE;
	int save_freeze;

	/* prevent recursion */
	prj = tsk->parent;
	save_freeze = prj->frozen;
	prj->frozen = TRUE;

	/* first, eliminate very short intervals */
	mini = prj->min_interval;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node=node->next)
		{
			GttInterval *ivl = node->data;
			if ((FALSE == ivl->running) &&
			    ((ivl->stop - ivl->start) <= mini))
			{
				tsk->interval_list = g_list_remove (tsk->interval_list, ivl);
				g_free (ivl);
				not_done = TRUE;
				changed = TRUE;
				break;
			}
		}
	}

	/* merge intervals with small gaps between them */
	mgap = prj->auto_merge_gap;
	not_done = TRUE;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node=node->next)
		{
			GttInterval *ivl = node->data;
			GttInterval *nivl;
			int gap;

			if (ivl->running) continue;
			if (!node->next) break;
			nivl = node->next->data;
			gap = ivl->start - nivl->stop;
			if ((mgap > gap) ||
			    (ivl->fuzz > gap) ||
			    (nivl->fuzz > gap))
			{
				gtt_interval_merge_down (ivl);
				not_done = TRUE;
				changed = TRUE;
				break;
			}
		}
	}

	/* merge short intervals into neighbors */
	merge = prj->auto_merge_interval;
	not_done = TRUE;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node=node->next)
		{
			GttInterval *ivl = node->data;
			int gap_up=1000000000;
			int gap_down=1000000000;
			int do_merge = FALSE;
			int len;

			if (ivl->running) continue;
			len = ivl->stop - ivl->start;
			if (len > merge) continue;
			if (node->next)
			{
				GttInterval *nivl = node->next->data;

				/* merge only if the intervals are in teh same day */
				if (get_midnight (ivl->start) == get_midnight (nivl->stop))
				{
					gap_down = ivl->start - nivl->stop;
					do_merge = TRUE;
				}
			} 
			if (node->prev)
			{
				GttInterval *nivl = node->prev->data;

				/* merge only if the intervals are in teh same day */
				if (get_midnight (nivl->start) == get_midnight (ivl->stop))
				{
					gap_up = nivl->start - ivl->stop;
					do_merge = TRUE;
				}
			}
			if (!do_merge) continue;
			if (gap_up < gap_down)
			{
				gtt_interval_merge_up (ivl);
			}
			else 
			{
				gtt_interval_merge_down (ivl);
			}
			not_done = TRUE;
			changed = TRUE;
			break;
		}
	}

	prj->frozen = save_freeze;
	return changed;
}

static void
project_compute_secs (GttProject *proj)
{
	int total_ever = 0;
	int total_day = 0;
	time_t midnight;
	GList *tsk_node, *ivl_node, *prj_node;

	if (!proj) return;

	/* Total up the subprojects first */
	for (prj_node= proj->sub_projects; prj_node; prj_node=prj_node->next)
	{
		GttProject * prj = prj_node->data;
		project_compute_secs (prj);
	}

	midnight = get_midnight (-1);
	/* total up tasks */
	for (tsk_node= proj->task_list; tsk_node; tsk_node=tsk_node->next)
	{
		GttTask * task = tsk_node->data;
		scrub_intervals (task);
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
	proj->dirty_time = FALSE;
}

static void
children_modified (GttProject *prj)
{
	GList *node;
	if (!prj) return;
	for (node= prj->sub_projects; node; node=node->next)
	{
		GttProject * subprj = node->data;
		children_modified (subprj);
	}
	proj_modified (prj);
}

void
gtt_project_list_compute_secs (void)
{
	GList *node;
	for (node= plist; node; node=node->next)
	{
		GttProject * prj = node->data;
		project_compute_secs (prj);
		children_modified (prj);
	}
}

/* =========================================================== */
/* even notification subsystem */

void
gtt_project_add_notifier (GttProject *prj, 
                          GttProjectChanged cb, 
                          gpointer user_stuff)
{
	Notifier * ntf;

	if (!prj || !cb) return;

	ntf = g_new0 (Notifier, 1);
	ntf->func = cb;
	ntf->user_data = user_stuff;
	prj->listeners = g_list_append (prj->listeners, ntf);
}

void
gtt_project_remove_notifier (GttProject *prj, 
                          GttProjectChanged cb, 
                          gpointer user_stuff)
{
	Notifier * ntf;
	GList *node;

	if (!prj || !cb) return;

	for (node = prj->listeners; node; node=node->next)
	{
		ntf = node->data;
		if ((ntf->func == cb) && (ntf->user_data == user_stuff)) break;
	}

	if (node)
	{
		prj->listeners = g_list_remove (prj->listeners, ntf);
		g_free (ntf);
	}
}

void
gtt_project_freeze (GttProject *prj)
{
	if (!prj) return;
	prj->frozen = TRUE;
}

void
gtt_project_thaw (GttProject *prj)
{
	if (!prj) return;
	prj->frozen = FALSE;
	proj_refresh_time (prj);
}

void 
gtt_task_freeze (GttTask *tsk)
{
	if (!tsk ||!tsk->parent) return;
	tsk->parent->frozen = TRUE;
}

void 
gtt_task_thaw (GttTask *tsk)
{
	if (!tsk ||!tsk->parent) return;
	tsk->parent->frozen = FALSE;
	proj_refresh_time (tsk->parent);
}

void 
gtt_interval_freeze (GttInterval *ivl)
{
	if (!ivl ||!ivl->parent || !ivl->parent->parent) return;
	ivl->parent->parent->frozen = TRUE;
}

void 
gtt_interval_thaw (GttInterval *ivl)
{
	if (!ivl ||!ivl->parent || !ivl->parent->parent) return;
	ivl->parent->parent->frozen = FALSE;
	proj_refresh_time (ivl->parent->parent);
}

static void
proj_refresh_time (GttProject *proj)
{
	GList *node;

	if (!proj) return;
	if (proj->being_destroyed) return;
	proj->dirty_time = TRUE;
	if (proj->frozen) return;
	project_compute_secs (proj);

	/* let listeners know that the times have changed */
	for (node=proj->listeners; node; node=node->next)
	{
		Notifier *ntf = node->data;
		(ntf->func) (proj, ntf->user_data);
	}
}

static void
proj_modified (GttProject *proj)
{
	GList *node;

	if (!proj) return;
	if (proj->being_destroyed) return;
	if (proj->frozen) return;

	/* let listeners know that the times have changed */
	for (node=proj->listeners; node; node=node->next)
	{
		Notifier *ntf = node->data;
		(ntf->func) (proj, ntf->user_data);
	}
}

/* =========================================================== */

void 
gtt_clear_daily_counter (GttProject *proj)
{
	time_t midnight;
	int not_done = TRUE;
	GList *tsk_node, *in;

	if (!proj) return;

	gtt_project_timer_stop (proj);
	gtt_project_freeze (proj);
	midnight = get_midnight (-1);

	/* loop over tasks */
	for (tsk_node= proj->task_list; tsk_node; tsk_node=tsk_node->next)
	{
		GttTask * task = tsk_node->data;

		not_done = TRUE;
		while (not_done)
		{
			not_done = FALSE;
			for (in= task->interval_list; in; in=in->next)
			{
				GttInterval *ivl = in->data;
	
				/* only nuke the ones that started after midnight.
			 	 * The ones that started before midnight remain */
				if (ivl->start >= midnight)
				{
					task->interval_list = 
					     g_list_remove (task->interval_list, ivl);
					g_free (ivl);
					not_done = TRUE;
					break;
				}
			}
		}
	}
	gtt_project_thaw (proj);
	gtt_project_timer_start (proj);
}

/* =========================================================== */

void 
gtt_project_timer_start (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;
	time_t now;

	if (!proj) return;

	/* By definition, the current task is the one at the head 
	 * of the list, and the current interval is  at the head 
	 * of the task */
	if (NULL == proj->task_list)
	{
		task = gtt_task_new();
		gtt_task_set_memo (task, _("New Task"));
	}
	else 
	{
		task = proj->task_list->data;
		g_return_if_fail (task);
	}

	now = time(0);

	/* only add a new interval if there's been a bit of a gap,
	 * otherwise, reuse the most recent running interval.  */
	if (task->interval_list)
	{
		int delta;
		ival = task->interval_list->data;
		delta = now - ival->stop;

		if (delta <= proj->auto_merge_gap)
		{
			
			ival->start += delta;
			ival->fuzz += delta;
			ival->stop = now;
			ival->running = TRUE;
			return;
		}
	} 

	ival = g_new0 (GttInterval, 1);
	ival->start = now;
	ival->stop = ival->start;
	ival->running = TRUE;
	task->interval_list = g_list_prepend (task->interval_list, ival);
	ival->parent = task;

	/* don't add the task until after we've done above */
	if (NULL == proj->task_list)
	{
		gtt_project_append_task (proj, task);
	}
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

	/* If timer isn't running, do nothing.  Normally,
	 * this function should never be called when timer is stopped,
	 * but there are a few rare cases (e.g. clear daily counter).
	 * where it is.
	 */
	if (FALSE == ival->running) return;

	/* compute the delta change, update cached data */
	now = time(0);

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

	/* When we stop the timer, call proj_refresh_time(),
	 * as this will do several things: first, it will 
	 * scrub away short intervals and/or short gaps,
	 * and, secondly, it will force redraws of affected 
	 * windows so that the old data doesn't show.
	 */
	proj_refresh_time (proj);
}

/* =========================================================== */

GttTask *
gtt_task_new (void)
{
	GttTask *task;

	task = g_new0(GttTask, 1);
	task->parent = NULL;
	task->memo = g_strdup (_("New Task"));
	task->notes = g_strdup ("");
	task->billable = GTT_BILLABLE;
	task->billrate = GTT_REGULAR;
	task->billstatus = GTT_BILL,
	task->bill_unit = 900;
	task->interval_list = NULL;
	return task;
}

GttTask *
gtt_task_dup (GttTask *old)
{
	GttTask *task;

	if (!old) return gtt_task_new();

	task = g_new0(GttTask, 1);
	task->parent = NULL;
	task->memo = g_strdup (old->memo);
	task->notes = g_strdup (old->notes);

	/* inherit the properties ... important for user */
	task->billable = old->billable;
	task->billrate = old->billrate;
	task->billstatus = old->billstatus;
	task->bill_unit = old->bill_unit;
	task->interval_list = NULL;

	return task;
}

static int
task_suspend (GttTask *tsk)
{
	int is_running = 0;

	/* avoid misplaced running intervals, stop the task */
	if (tsk->interval_list)
	{
		GttInterval *first_ivl;
		first_ivl = (GttInterval *) (tsk->interval_list->data);
		is_running = first_ivl -> running;
		if (is_running) 
		{
			/* don't call stop here, avoid dispatching redraw events */
			gtt_project_timer_update (tsk->parent);
			first_ivl->running = FALSE;
		}
	}
	return is_running;
}


void
gtt_task_destroy (GttTask *task)
{
	int is_running;
	if (!task) return;

	is_running = task_suspend (task);
	if (task->parent)
	{
		task->parent->task_list = 
			g_list_remove (task->parent->task_list, task);
		if (is_running) gtt_project_timer_start (task->parent);
		proj_refresh_time (task->parent);
		task->parent = NULL;
	}

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
gtt_task_remove (GttTask *task)
{
	int is_running;
	if (!task) return;

	is_running = task_suspend (task);

	if (task->parent)
	{
		task->parent->task_list = 
			g_list_remove (task->parent->task_list, task);
		if (is_running) gtt_project_timer_start (task->parent);
		proj_refresh_time (task->parent);
		task->parent = NULL;
	}
}


void
gtt_task_insert (GttTask *where, GttTask *insertee)
{
	GttProject *prj;
	int idx;
	int is_running = 0;

	if (!where || !insertee) return;

	prj = where->parent;
	if (!prj) return;

	gtt_task_remove (insertee);
	is_running = task_suspend (where);

	insertee->parent = prj;
	idx = g_list_index (prj->task_list, where);
	prj->task_list = g_list_insert (prj->task_list, insertee, idx);

	if (is_running) gtt_project_timer_start (prj);
	proj_refresh_time (prj);
}

GttTask *
gtt_task_new_insert (GttTask *old)
{
	int is_running = 0;
	GttProject *prj;
	GttTask *task;
	int idx;

	if (!old) return NULL;

	task = g_new0(GttTask, 1);
	task->memo = g_strdup (_("New Task"));
	task->notes = g_strdup ("");

	/* inherit the properties ... important for user */
	task->billable = old->billable;
	task->billrate = old->billrate;
	task->billstatus = old->billstatus;
	task->bill_unit = old->bill_unit;
	task->interval_list = NULL;

	/* chain into place */
	prj = old->parent;
	task->parent = prj;
	if (!prj) return task;

	/* avoid misplaced running intervals, stop the task */
	is_running = task_suspend (old);

	idx = g_list_index (prj->task_list, old);
	prj->task_list = g_list_insert (prj->task_list, task, idx);

	if (is_running) gtt_project_timer_start (prj);
	proj_refresh_time (prj);
	return task;
}

void
gtt_task_add_interval (GttTask *tsk, GttInterval *ival)
{
	if (!tsk || !ival) return;
	tsk->interval_list = g_list_prepend (tsk->interval_list, ival);
	ival->parent = tsk;
	proj_refresh_time (tsk->parent);
}

void
gtt_task_append_interval (GttTask *tsk, GttInterval *ival)
{
	if (!tsk || !ival) return;
	tsk->interval_list = g_list_append (tsk->interval_list, ival);
	ival->parent = tsk;
	proj_refresh_time (tsk->parent);
}

void 
gtt_task_set_memo(GttTask *tsk, const char *m)
{
	if (!tsk) return;
	if (tsk->memo) g_free(tsk->memo);
	if (!m) 
	{
		tsk->memo = g_strdup ("");
		return;
	}
	tsk->memo = g_strdup(m);
	proj_modified (tsk->parent);
}

void 
gtt_task_set_notes(GttTask *tsk, const char *m)
{
	if (!tsk) return;
	if (tsk->notes) g_free(tsk->notes);
	if (!m) 
	{
		tsk->notes = g_strdup ("");
		return;
	}
	tsk->notes = g_strdup(m);
	proj_modified (tsk->parent);
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
	proj_modified (tsk->parent);
}

GttBillable
gtt_task_get_billable (GttTask *tsk)
{
	if (!tsk) return GTT_NOT_BILLABLE;
	return tsk->billable;
}

void
gtt_task_set_billrate (GttTask *tsk, GttBillRate b)
{
	if (!tsk) return;
	tsk->billrate = b;
	proj_modified (tsk->parent);
}

GttBillRate
gtt_task_get_billrate (GttTask *tsk)
{
	if (!tsk) return GTT_REGULAR;
	return tsk->billrate;
}

void
gtt_task_set_billstatus (GttTask *tsk, GttBillStatus b)
{
	if (!tsk) return;
	tsk->billstatus = b;
	proj_modified (tsk->parent);
}

GttBillStatus
gtt_task_get_billstatus (GttTask *tsk)
{
	if (!tsk) return GTT_HOLD;
	return tsk->billstatus;
}

void
gtt_task_set_bill_unit (GttTask *tsk, int b)
{
	if (!tsk) return;
	tsk->bill_unit = b;
	proj_modified (tsk->parent);
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

gboolean
gtt_task_is_first_task (GttTask *tsk)
{
	if (!tsk || !tsk->parent || !tsk->parent->task_list) return TRUE;
	
	if ((GttTask *) tsk->parent->task_list->data == tsk) return TRUE;
	return FALSE;
}

/* =========================================================== */

void 
gtt_task_merge_up (GttTask *tsk)
{
	GttTask *mtask;
	GList * node;
	GttProject *prj;
	if (!tsk) return;

	prj = tsk->parent;
	if (!prj || !prj->task_list) return;

	node = g_list_find (prj->task_list, tsk);
	if (!node) return;

	node = node->prev;
	if (!node) return;

	mtask = node->data;

	for (node=tsk->interval_list; node; node=node->next)
	{
		GttInterval *ivl = node->data;
		ivl->parent = mtask;
	}
	mtask->interval_list = g_list_concat (mtask->interval_list, tsk->interval_list);
	tsk->interval_list = NULL;
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
		proj_refresh_time (ivl->parent->parent);
	}
	ivl->parent = NULL;
	g_free (ivl);
}

void
gtt_interval_set_start (GttInterval *ivl, time_t st)
{
	if (!ivl) return;
	ivl->start = st;
	if (ivl->parent) proj_refresh_time (ivl->parent->parent);
}

void
gtt_interval_set_stop (GttInterval *ivl, time_t st)
{
	if (!ivl) return;
	ivl->stop = st;
	if (ivl->parent) proj_refresh_time (ivl->parent->parent);
}

void
gtt_interval_set_fuzz (GttInterval *ivl, int st)
{
	if (!ivl) return;
	ivl->fuzz = st;
	if (ivl->parent) proj_modified (ivl->parent->parent);
}

void
gtt_interval_set_running (GttInterval *ivl, gboolean st)
{
	if (!ivl) return;
	ivl->running = st;
	if (ivl->parent) proj_modified (ivl->parent->parent);
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

GttTask *
gtt_interval_get_parent (GttInterval * ivl)
{
	if (!ivl) return NULL;
	return ivl->parent;
}

gboolean
gtt_interval_is_first_interval (GttInterval *ivl)
{
	if (!ivl || !ivl->parent || !ivl->parent->interval_list) return TRUE;
	
	if ((GttInterval *) ivl->parent->interval_list->data == ivl) return TRUE;
	return FALSE;
}

gboolean
gtt_interval_is_last_interval (GttInterval *ivl)
{
	if (!ivl || !ivl->parent || !ivl->parent->interval_list) return TRUE;

	if ((GttInterval *) ((g_list_last(ivl->parent->interval_list))->data) == ivl) return TRUE;
	return FALSE;
}

/* ============================================================= */

GttInterval *
gtt_interval_merge_up (GttInterval *ivl)
{
	int more_fuzz;
	int ivl_len;
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

	/* the fuzz is the gap between stop and start times */
	more_fuzz = merge->start - ivl->stop;
	ivl_len = ivl->stop - ivl->start;
	if (more_fuzz > ivl_len) more_fuzz = ivl_len;

	merge->start -= ivl_len;
	if (ivl->fuzz > merge->fuzz) merge->fuzz = ivl->fuzz;
	if (more_fuzz > merge->fuzz) merge->fuzz = more_fuzz;

	gtt_interval_destroy (ivl);

	proj_refresh_time (prnt->parent);
	return merge;
}


GttInterval *
gtt_interval_merge_down (GttInterval *ivl)
{
	int more_fuzz;
	int ivl_len;
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

	/* the fuzz is the gap between stop and start times */
	more_fuzz = ivl->start - merge->stop;
	ivl_len = ivl->stop - ivl->start;
	if (more_fuzz > ivl_len) more_fuzz = ivl_len;

	merge->stop += ivl_len;
	if (ivl->fuzz > merge->fuzz) merge->fuzz = ivl->fuzz;
	if (more_fuzz > merge->fuzz) merge->fuzz = more_fuzz;
	gtt_interval_destroy (ivl);

	proj_refresh_time (prnt->parent);
	return merge;
}

void
gtt_interval_split (GttInterval *ivl, GttTask *newtask)
{
	int is_running = 0;
	gint idx;
	GttProject *prj;
	GttTask *prnt;
	GList *node;
	GttInterval *first_ivl;

	if (!ivl || !newtask) return;
	prnt = ivl->parent;
	if (!prnt) return;
	prj = prnt->parent;
	if (!prj) return;
	node = g_list_find (prnt->interval_list, ivl);
	if (!node) return;

	gtt_task_remove (newtask);

	/* avoid misplaced running intervals, stop the task */
	first_ivl = (GttInterval *) (prnt->interval_list->data);
	is_running = first_ivl -> running;
	if (is_running) 
	{
		/* don't call stop here, avoid dispatching redraw events */
		gtt_project_timer_update (prj);
		first_ivl->running = FALSE;
	}

	/* chain the new task into proper order in the parent project */
	idx = g_list_index (prj->task_list, prnt);
	idx ++;
	prj->task_list = g_list_insert (prj->task_list, newtask, idx);
	newtask->parent = prj;

	/* Rechain the intervals.  We do this by hand, since it
	 * seems that glib doesn't provide this function. */
	if (node->prev)
	{
		node->prev->next = NULL;
		node->prev = NULL;
	}
	else
	{
		prnt->interval_list = NULL;
	}

	newtask->interval_list = node;

	for (; node; node=node->next)
	{
		GttInterval *nivl = node->data;
		nivl->parent = newtask;
	}

	if (is_running) gtt_project_timer_start (prj);

	proj_refresh_time (prnt->parent);
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
