/*   Project data manipulation for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
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

#ifndef __GTT_PROJ_H__
#define __GTT_PROJ_H__

#include <glib.h>

typedef enum 
{
	GTT_HOLD = 0,	    /* will not appear on invoice */
	GTT_BILLABLE = 1,   /* billable time */
	GTT_NOT_BILLABLE,   /* not billable to customer, internal only */
	GTT_NO_CHARGE       /* no charge */
} GttBillable;

typedef enum 
{
	GTT_REGULAR = 0,
	GTT_OVERTIME,
	GTT_OVEROVER,
	GTT_FLAT_FEE
} GttBillRate;
		

/* The three basic structures */
typedef struct gtt_project_s GttProject;
typedef struct gtt_task_s GttTask;
typedef struct gtt_interval_s GttInterval;

typedef void (*GttProjectChanged) (GttProject *, gpointer);

/* -------------------------------------------------------- */
/* project data */

/* create, destroy a new project */
GttProject *	gtt_project_new(void);
GttProject *	gtt_project_new_title_desc(const char *, const char *);
void 		gtt_project_destroy(GttProject *);

/* The gtt_project_dup() routine will make a copy of the indicated
 *    project. Note, it will copy the sub-projects, but it will *not*
 *    copy the tasks (I dunno, this is probably a bug, I think it should
 *    copy the tasks as well ....)
 */
GttProject *	gtt_project_dup(GttProject *);

/* The gtt_project_remove() routine will take the project out of 
 *    either the master list, or out of any parents' list of 
 *    sub-projects that it might below to.  As a result, this 
 *    project will dangle there -- don't loose it 
 */
void 		gtt_project_remove(GttProject *p);

void 		gtt_project_set_title(GttProject *, const char *);
void 		gtt_project_set_desc(GttProject *, const char *);
void 		gtt_project_set_notes(GttProject *, const char *);
void 		gtt_project_set_custid(GttProject *, const char *);

/* These two routines return the title & desc strings.
 * Do *not* free these strings when done.  Note that 
 * are freed when project is deleted. */
const char * 	gtt_project_get_title (GttProject *);
const char * 	gtt_project_get_desc (GttProject *);
const char * 	gtt_project_get_notes (GttProject *);
const char * 	gtt_project_get_custid (GttProject *);

/* The gtt_project_compat_set_secs() routine provides a
 *    backwards-compatible routine for setting the total amount of
 *    time spent on a project.  It does this by creating a new task,
 *    labelling it as 'old gtt data', and putting the time info
 *    into that task.  Depricated. Don't use in new code.
 */
void		gtt_project_compat_set_secs (GttProject *proj, 
			int secs_ever, int secs_day, time_t last_update);


/* The billrate is the currency amount to charge for an hour's work.
 *     overtime_rate is the over-time rate (usually 1.5x billrate)
 *     overover_rate is the double over-time rate (usually 2x billrate)
 *     flat_fee is charged, independent of the length of time.
 */
void 		gtt_project_set_billrate (GttProject *, double);
double 		gtt_project_get_billrate (GttProject *);
void 		gtt_project_set_overtime_rate (GttProject *, double);
double 		gtt_project_get_overtime_rate (GttProject *);
void 		gtt_project_set_overover_rate (GttProject *, double);
double 		gtt_project_get_overover_rate (GttProject *);
void 		gtt_project_set_flat_fee (GttProject *, double);
double 		gtt_project_get_flat_fee (GttProject *);

/* The gtt_project_set_min_interval() routine sets the smallest
 *    time unit, below which work intervals will not be recorded
 *    (and will instead be discarded).   Default is 3 seconds, 
 *    but it should be 60 seconds.
 *
 * The gtt_project_auto_merge_interval() routine sets the smallest
 *    time unit, below which work intervals will be merged into 
 *    prior work intervals rather than being counted as seperate.
 *    Default is 1 minute, but should be 5 minutes.
 */
void		gtt_project_set_min_interval (GttProject *, int);
int		gtt_project_get_min_interval (GttProject *);
void		gtt_project_set_auto_merge_interval (GttProject *, int);
int		gtt_project_get_auto_merge_interval (GttProject *);
void		gtt_project_set_auto_merge_gap (GttProject *, int);
int		gtt_project_get_auto_merge_gap (GttProject *);

/* The id is a simple id, handy for .. stuff */
void 		gtt_project_set_id (GttProject *, int id);
int  		gtt_project_get_id (GttProject *);

/* return a project, given only its id; NULL if not found */
GttProject * 	gtt_project_locate_from_id (int prj_id);

/* The gtt_project_add_notifier() routine allows anoter component
 *    (e.g. a GUI) to add a signal that will be called whenever the 
 *    time associated with a project changes. (except timers ???)
 *
 * The gtt_project_freeze() routine prevents notifiers from being
 *    invoked.
 *
 * The gtt_project_thaw() routine causes notifiers to be sent.
 */ 

void		gtt_project_freeze (GttProject *prj);
void		gtt_project_thaw (GttProject *prj);
void		gtt_task_freeze (GttTask *tsk);
void		gtt_task_thaw (GttTask *tsk);
void		gtt_interval_freeze (GttInterval *ivl);
void		gtt_interval_thaw (GttInterval *ivl);

void		gtt_project_add_notifier (GttProject *,
			GttProjectChanged, gpointer);
void		gtt_project_remove_notifier (GttProject *,
			GttProjectChanged, gpointer);

/* -------------------------------------------------------- */
/* project manipulation */

/* The project_timer_start() routine logs the time when
 *    a new task interval starts.
 * The project_timer_update() routine updates the end-time
 *    for a task interval. 
 */
void 		gtt_project_timer_start (GttProject *);
void 		gtt_project_timer_update (GttProject *);
void 		gtt_project_timer_stop (GttProject *);


/* The gtt_project_get_secs_day() routine returns the
 *    number of seconds spent on this project today,
 *    *NOT* including its sub-projects.
 *
 * The gtt_project_get_secs_ever() routine returns the
 *    number of seconds spent on this project,
 *    *NOT* including its sub-projects.
 *
 * The gtt_project_total_secs_day() routine returns the
 *    total number of seconds spent on this project today,
 *    including a total of all its sub-projects.
 *
 * The gtt_project_total_secs_ever() routine returns the
 *    total number of seconds spent on this project,
 *    including a total of all its sub-projects.
 */

int 		gtt_project_get_secs_day (GttProject *proj);
int 		gtt_project_get_secs_ever (GttProject *proj);

int 		gtt_project_total_secs_day (GttProject *proj);
int 		gtt_project_total_secs_ever (GttProject *proj);

void gtt_project_compute_secs (GttProject *proj);
void gtt_project_list_compute_secs (void);


/* return a list of the children of this project */
GList * 	gtt_project_get_children (GttProject *);
GList * 	gtt_project_get_tasks (GttProject *);

GttProject * 	gtt_project_get_parent (GttProject *);

/* 
 * The following routines maintain a heirarchical tree of projects.
 * Note, however, that these routines do not sanity check the tree
 * globally, so it is possible to create loops or disconected
 * components if one is sloppy.
 *
 * The gtt_project_append_project() routine makes 'child' a subproject 
 *    of 'parent'.  In doing this, it removes the child from its
 *    present location (whether as child of a diffferent project,
 *    or from the master project list).  It appends the child
 *    to the parent's list: the child becomes the 'last one'.
 *    If 'parent' is NULL, then the child is appended to the 
 *    top-level project list.
 */
void	gtt_project_append_project (GttProject *parent, GttProject *child);

/* 
 * The gtt_project_insert_before() routine will insert 'proj' on the 
 *     same list as 'before_me', be will insert it before it.  Note
 *     that 'before_me' may be in the top-level list, or it may be
 *     in a subproject list; either way, the routine works.  If
 *     'before_me' is null, then the project is appended to the master
 *     list.  The project is removed from its old location before
 *     being inserted into its new location.
 *
 * The gtt_project_insert_after() routine works similarly, except that
 *     if 'after_me' is null, then the proj is prepended to the 
 *     master list.
 */
void	gtt_project_insert_before (GttProject *proj, GttProject *before_me);
void	gtt_project_insert_after (GttProject *proj, GttProject *after_me);

void	gtt_project_add_task (GttProject *, GttTask *);

/* The gtt_clear_daily_counter() will delete all intervals from 
 *    the project that started after midnight.  Typically, this 
 *    will result in the daily counters being zero'ed out, although 
 *    if a project started before midnight, some time may remain
 *    on deck.
 */

void gtt_clear_daily_counter (GttProject *proj);

/* -------------------------------------------------------- */
/* master project list */

/* Return a list of all projects */
GList * 	gtt_get_project_list (void);

/* append project to the master project list */
void 		gtt_project_list_append(GttProject *p);

void project_list_destroy(void);
void project_list_sort_time(void);
void project_list_sort_total_time(void);
void project_list_sort_title(void);
void project_list_sort_desc(void);

/* The gtt_project_list_total_secs_day() routine returns the
 *    total number of seconds spent on all projects today,
 *    including a total of all sub-projects.
 *
 * The gtt_project_list_total_secs_ever() routine returns the
 *    total number of seconds spent all projects,
 *    including a total of all sub-projects.
 */

int 		gtt_project_list_total_secs_day (void);
int 		gtt_project_list_total_secs_ever (void);

/* -------------------------------------------------------- */
/* tasks */
GttTask *	gtt_task_new (void);
void 		gtt_task_destroy (GttTask *);

void		gtt_task_set_memo (GttTask *, const char *);
const char *	gtt_task_get_memo (GttTask *);
void		gtt_task_set_notes (GttTask *, const char *);
const char *	gtt_task_get_notes (GttTask *);

void		gtt_task_set_billable (GttTask *, GttBillable);
GttBillable	gtt_task_get_billable (GttTask *);
void		gtt_task_set_billrate (GttTask *, GttBillRate);
GttBillRate	gtt_task_get_billrate (GttTask *);

/* The bill_unit is the minimum billable unit of time. 
 * Typically 15 minutes or an hour, it represents the smallest unit
 * of time that will appear on the invoice.
 */
void		gtt_task_set_bill_unit (GttTask *, int);
int		gtt_task_get_bill_unit (GttTask *);

GList *		gtt_task_get_intervals (GttTask *);
void		gtt_task_add_interval (GttTask *, GttInterval *);
void		gtt_task_append_interval (GttTask *, GttInterval *);

/* gtt_task_get_secs_ever() adds up and returns the total number of 
 * seconds in the intervals in this task. */
int 		gtt_task_get_secs_ever (GttTask *tsk);


/* intervals */
/* The start and stop times define the interval.   
 * The fuzz (measured in seconds) indicates how 'fuzzy' the 
 *    true start time was.  Typically, its 0, 300, 3600 or 12*3600
 *    Just because the start time is fuzy doesn't mean that the total 
 *    interval is inaccurate: the delta stop-start is still accurate 
 *    down to the second.  The fuzz is 'merely' used by the GUI
 *    to help the user report time spent post-facto, without having 
 *    to exactly nail down the start time.
 *
 * The is_running flag indicates whether the timer is running on this
 * interval.
 */
GttInterval *	gtt_interval_new (void);
void		gtt_interval_destroy (GttInterval *);

void		gtt_interval_set_start (GttInterval *, time_t);
void		gtt_interval_set_stop (GttInterval *, time_t);
void		gtt_interval_set_running (GttInterval *, gboolean);
void		gtt_interval_set_fuzz (GttInterval *, int);
time_t		gtt_interval_get_start (GttInterval *);
time_t		gtt_interval_get_stop (GttInterval *);
gboolean	gtt_interval_get_running (GttInterval *);
int		gtt_interval_get_fuzz (GttInterval *);

/* The gtt_interval_merge_up() routine merges the given interval with 
 *    the immediately more recent one above it.  It does this by 
 *    decrementing the start time.  The resulting interval has the
 *    max of the two fuzz factors, and is running if the first was.
 *    The merged interval is returned.
 *
 * The gtt_interval_merge_down() routine does the same, except that
 *    it merges with the next interval by incrementing its stop time.
 *
 * The gtt_interval_split() routine splits the list of intervals 
 *    into two pieces, with the indicated interval and everything
 *    following it going into the new task.  It returns the new task.
 */
GttInterval *	gtt_interval_merge_up (GttInterval *);
GttInterval *	gtt_interval_merge_down (GttInterval *);
GttTask *	gtt_interval_split (GttInterval *);

#endif /* __GTT_PROJ_H__ */
