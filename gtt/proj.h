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

#ifndef __GTT_PROJ_H__
#define __GTT_PROJ_H__

#include <glib.h>

typedef struct gtt_project_s GttProject;

typedef struct gtt_task_s GttTask;
typedef struct gtt_interval_s GttInterval;

/* create, destroy a new project */
GttProject *gtt_project_new(void);
GttProject *gtt_project_new_title_desc(const char *, const char *);
GttProject *project_dup(GttProject *);
void project_destroy(GttProject *);

void gtt_project_set_title(GttProject *, const char *);
void gtt_project_set_desc(GttProject *, const char *);

/* These two routines return the title & desc strings.
 * Do *not* free these strings when done.  Note that 
 * are freed when project is deleted. */
const char * gtt_project_get_title (GttProject *);
const char * gtt_project_get_desc (GttProject *);

void gtt_project_set_rate (GttProject *, double);
double gtt_project_get_rate (GttProject *);


/* The project_timer_start() routine logs the time when
 *    a new task interval starts.
 * The project_timer_update() routine updates the end-time
 *    for a task interval. 
 */
void gtt_project_timer_start (GttProject *);
void gtt_project_timer_update (GttProject *);
void gtt_project_timer_stop (GttProject *);

/* return a list of the children of this project */
GList * 	gtt_project_get_children (GttProject *);
GList * 	gtt_project_get_tasks (GttProject *);

void		gtt_project_add_project (GttProject *, GttProject *);
void		gtt_project_add_task (GttProject *, GttTask *);

/* -------------------------------------------------------- */

/* Return a list of all projects */
GList * gtt_get_project_list (void);

/* add project to the master project list */
void gtt_project_list_add(GttProject *p);

void project_list_add(GttProject *p);
void project_list_insert(GttProject *p, int pos);
void project_list_remove(GttProject *p);
void project_list_destroy(void);
void project_list_time_reset(void);
int project_list_load(const char *fname);
int project_list_save(const char *fname);
gboolean project_list_export (const char *fname);
void project_list_sort_time(void);
void project_list_sort_total_time(void);
void project_list_sort_title(void);
void project_list_sort_desc(void);

char *project_get_timestr(GttProject *p, int show_secs);
char *project_get_total_timestr(GttProject *p, int show_secs);

/* -------------------------------------------------------- */
/* tasks */
GttTask *	gtt_task_new (void);
void 		gtt_task_destroy (GttTask *);

void		gtt_task_set_memo (GttTask *, const char *);
const char *	gtt_task_get_memo (GttTask *);
GList *		gtt_task_get_intervals (GttTask *);
void		gtt_task_add_interval (GttTask *, GttInterval *);


/* intervals */
GttInterval *	gtt_interval_new (void);
void		gtt_interval_destroy (GttInterval *);

void		gtt_interval_set_start (GttInterval *, time_t);
void		gtt_interval_set_stop (GttInterval *, time_t);
void		gtt_interval_set_running (GttInterval *, gboolean);
time_t		gtt_interval_get_start (GttInterval *);
time_t		gtt_interval_get_stop (GttInterval *);
gboolean	gtt_interval_get_running (GttInterval *);


#endif /* __GTT_PROJ_H__ */
