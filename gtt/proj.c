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

#include <config.h>
#include <gnome.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#include "err-throw.h"
#include "gtt.h"
#include "proj_p.h"
#include "toolbar.h"
#include "xml-gtt.h"

#ifdef DEBUG
#define GTT "/gtt-DEBUG/"
#else /* not DEBUG */
#define GTT "/gtt/"
#endif /* not DEBUG */


char *first_proj_title = NULL;



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
/* zero out day counts if rolled past mignight */

static void
zero_on_rollover (time_t now, time_t last)
{
	struct tm t1, t0;
	time_t diff = now - last;

	/* zero out day counts */
        if (0 < diff) 
	{
                memcpy(&t0, localtime(&last), sizeof(struct tm));
                memcpy(&t1, localtime(&now), sizeof(struct tm));
                if ((t0.tm_year != t1.tm_year) ||
                    (t0.tm_yday != t1.tm_yday)) 
		{
                        project_list_time_reset();
			log_endofday();
                }
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

	/* zero out day counts if rolled past mignight */
	zero_on_rollover (now, prev_update);

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

static GList * plist = NULL;

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
/* file I/O routines */

static const char *
build_rc_name(void)
{
	static char *buf = NULL;

	if (buf != NULL) return buf;
	if (g_getenv("HOME") != NULL) {
		buf = g_concat_dir_and_file (g_getenv ("HOME"), RC_NAME);
	} else {
		buf = g_strdup (RC_NAME);
	}
	return buf;
}



static void
read_tb_sects(char *s)
{
        if (s[2] == 'n') {
                config_show_tb_new = (s[5] == 'n');
        } else if (s[2] == 'f') {
                config_show_tb_file = (s[5] == 'n');
        } else if (s[2] == 'c') {
                config_show_tb_ccp = (s[5] == 'n');
        } else if (s[2] == 'p') {
                config_show_tb_prop = (s[5] == 'n');
        } else if (s[2] == 't') {
                config_show_tb_timer = (s[5] == 'n');
        } else if (s[2] == 'o') {
                config_show_tb_pref = (s[5] == 'n');
        } else if (s[2] == 'h') {
                config_show_tb_help = (s[5] == 'n');
        } else if (s[2] == 'e') {
                config_show_tb_exit = (s[5] == 'n');
        }
}



static int
project_list_load_old(const char *fname)
{
	FILE *f;
	const char *realname;
	GList *pl, *t;
	GttProject *proj = NULL;
	char s[1024];
	int i;
	time_t tmp_time = -1;
        int _n, _f, _c, _p, _t, _o, _h, _e;

	if (fname != NULL)
		realname = fname;
	else
		realname = build_rc_name();
	if (NULL == (f = fopen(realname, "rt"))) {
#ifdef ENOENT
                if (errno == ENOENT) return 0;
#endif
		g_warning("could not open %s\n", realname);
		return 0;
	}
	pl = plist;
	plist = NULL;

        _n = config_show_tb_new;
        _f = config_show_tb_file;
        _c = config_show_tb_ccp;
        _p = config_show_tb_prop;
        _t = config_show_tb_timer;
        _o = config_show_tb_pref;
        _h = config_show_tb_help;
        _e = config_show_tb_exit;
	errno = 0;
	while ((!feof(f)) && (!errno)) {
		if (!fgets(s, 1023, f)) continue;
		if (s[0] == '#') continue;
		if (s[0] == '\n') continue;
		if (s[0] == ' ') {
			/* desc for last project */
			while (s[strlen(s) - 1] == '\n')
				s[strlen(s) - 1] = 0;
			gtt_project_set_desc(proj, &s[1]);
		} else if (s[0] == 't') {
			/* last_timer */
			tmp_time = (time_t)atol(&s[1]);
		} else if (s[0] == 's') {
			/* show seconds? */
			config_show_secs = (s[3] == 'n');
		} else if (s[0] == 'b') {
			if (s[1] == 'i') {
				/* show icons in the toolbar */
				config_show_tb_icons = (s[4] == 'n');
			} else if (s[1] == 't') {
				/* show text in the toolbar */
				config_show_tb_texts = (s[4] == 'n');
			} else if (s[1] == 'p') {
				/* show tooltips */
				config_show_tb_tips = (s[4] == 'n');
			} else if (s[1] == 'h') {
				/* show clist titles */
				config_show_clist_titles = (s[4] == 'n');
			} else if (s[1] == 's') {
				/* show status bar */
				if (s[4] == 'n') {
					gtk_widget_show(GTK_WIDGET(status_bar));
                                        config_show_statusbar = 1;
				} else {
					gtk_widget_hide(GTK_WIDGET(status_bar));
                                        config_show_statusbar = 0;
				}
                        } else if (s[1] == '_') {
                                read_tb_sects(s);
			}
		} else if (s[0] == 'c') {
			/* switch project command */
			while (s[strlen(s) - 1] == '\n')
				s[strlen(s) - 1] = 0;
			if (config_command) g_free(config_command);
			config_command = g_strdup(&s[2]);
		} else if (s[0] == 'n') {
			/* no project command */
			while (s[strlen(s) - 1] == '\n')
				s[strlen(s) - 1] = 0;
			if (config_command_null) g_free(config_command_null);
			config_command_null = g_strdup(&s[2]);
		} else if (s[0] == 'l') {
			if (s[1] == 'u') {
				/* use logfile? */
				config_logfile_use = (s[4] == 'n');
			} else if (s[1] == 'n') {
				/* logfile name */
				while (s[strlen(s) - 1] == '\n')
					s[strlen(s) - 1] = 0;
				if (config_logfile_name) g_free(config_logfile_name);
				config_logfile_name = g_strdup(&s[3]);
			} else if (s[1] == 's') {
				/* minimum time for a project to get logged */
				config_logfile_min_secs = atoi(&s[3]);
			}
		} else if ((s[0] >= '0') && (s[0] <='9')) {
			/* new project */
			proj = gtt_project_new();
			project_list_add(proj);
			proj->secs = atol(s);
			for (i = 0; s[i] != ' '; i++) ;
			i++;
			proj->day_secs = atol(&s[i]);
			for (; s[i] != ' '; i++) ;
			i++;
			while (s[strlen(s) - 1] == '\n')
				s[strlen(s) - 1] = 0;
			gtt_project_set_title(proj, &s[i]);
		}
	}
	if ((errno) && (!feof(f))) goto err;
	fclose(f);

	t = plist;
	plist = pl;
	project_list_destroy();
	plist = t;
	if (tmp_time > 0) {
		zero_on_rollover (time(0), tmp_time);
	}
	update_status_bar();
        if ((_n != config_show_tb_new) ||
            (_f != config_show_tb_file) ||
            (_c != config_show_tb_ccp) ||
            (_p != config_show_tb_prop) ||
            (_t != config_show_tb_timer) ||
            (_o != config_show_tb_pref) ||
            (_h != config_show_tb_help) ||
            (_e != config_show_tb_exit)) {
                update_toolbar_sections();
        }
	return 1;

	err:
	fclose(f);
	g_warning("error reading %s\n", realname);
	project_list_destroy();
	plist = pl;
	return 0;
}



int
project_list_load(const char *fname)
{
	time_t last_timer = -1;
        char s[256];
        int i, num;
        GttProject *proj;
        int _n, _f, _c, _p, _t, _o, _h, _e;
	gboolean got_default;

        gnome_config_get_int_with_default(GTT"Misc/NumProjects=0", &got_default);
        if (got_default) {
                return project_list_load_old(fname);
        }
	if ((cur_proj) && (cur_proj->title) && (!first_proj_title)) {
		first_proj_title = cur_proj->title;
	}
        project_list_destroy();
        _n = config_show_tb_new;
        _f = config_show_tb_file;
        _c = config_show_tb_ccp;
        _p = config_show_tb_prop;
        _t = config_show_tb_timer;
        _o = config_show_tb_pref;
        _h = config_show_tb_help;
        _e = config_show_tb_exit;
        last_timer = atol(gnome_config_get_string(GTT"Misc/LastTimer=-1"));
	if (last_timer > 0) {
		zero_on_rollover (time(0), last_timer);
	}
	if (gnome_config_get_int(GTT"Misc/TimerRunning=1")) {
// FIXME. Actually, we need to make a policy decision in order to fix this ...
		 // start_timer();
 // printf ("duuude we started a timer running on startup but we don't know which task to time !!\n");
	} else {
//		stop_timer();
// printf ("duuude timer is stopped on startup\n");
	}
        config_show_secs = gnome_config_get_bool(GTT"Display/ShowSecs=false");
        config_show_clist_titles = gnome_config_get_bool(GTT"Display/ShowTableHeader=false");
        config_show_tb_icons = gnome_config_get_bool(GTT"Toolbar/ShowIcons=true");
        config_show_tb_texts = gnome_config_get_bool(GTT"Toolbar/ShowTexts=true");
        config_show_tb_tips = gnome_config_get_bool(GTT"Toolbar/ShowTips=true");
        config_show_statusbar = gnome_config_get_bool(GTT"Display/ShowStatusbar=true");
        config_show_tb_new = gnome_config_get_bool(GTT"Toolbar/ShowNew=true");
        config_show_tb_file = gnome_config_get_bool(GTT"Toolbar/ShowFile=false");
        config_show_tb_ccp = gnome_config_get_bool(GTT"Toolbar/ShowCCP=false");
        config_show_tb_prop = gnome_config_get_bool(GTT"Toolbar/ShowProp=true");
        config_show_tb_timer = gnome_config_get_bool(GTT"Toolbar/ShowTimer=true");
        config_show_tb_pref = gnome_config_get_bool(GTT"Toolbar/ShowPref=false");
        config_show_tb_help = gnome_config_get_bool(GTT"Toolbar/ShowHelp=true");
        config_show_tb_exit = gnome_config_get_bool(GTT"Toolbar/ShowExit=true");
        config_command = gnome_config_get_string(GTT"Actions/ProjCommand");
        config_command_null = gnome_config_get_string(GTT"Actions/NullCommand");
        config_logfile_use = gnome_config_get_bool(GTT"LogFile/Use=false");
        config_logfile_name = gnome_config_get_string(GTT"LogFile/Filename");
	config_logfile_str = gnome_config_get_string(GTT"LogFile/Entry");
	if (!config_logfile_str)
		config_logfile_str = g_strdup(_("project %t started"));
	config_logfile_stop = gnome_config_get_string(GTT"LogFile/EntryStop");
	if (!config_logfile_stop)
		config_logfile_stop = g_strdup(_("stopped project %t"));
        config_logfile_min_secs = gnome_config_get_int(GTT"LogFile/MinSecs");
	if (config_show_statusbar)
		gtk_widget_show(status_bar);
	else
		gtk_widget_hide(status_bar);
	for (i = 0; i < GTK_CLIST(glist)->columns; i++) {
		g_snprintf(s, sizeof (s), GTT"CList/ColumnWidth%d=0", i);
		num = gnome_config_get_int(s);
		if (num) {
			clist_header_width_set = 1;
			gtk_clist_set_column_width(GTK_CLIST(glist),
						   i, num);
		}
	}
        num = gnome_config_get_int(GTT"Misc/NumProjects=0");
        for (i = 0; i < num; i++) {
                proj = gtt_project_new();
                project_list_add(proj);
                g_snprintf(s, sizeof (s), GTT"Project%d/Title", i);
                gtt_project_set_title(proj, gnome_config_get_string(s));
		if ((proj->title) && (first_proj_title)) {
			if (0 == strcmp(proj->title, first_proj_title)) {
				cur_proj_set(proj);
				first_proj_title = NULL;
			}
		}
                g_snprintf(s, sizeof (s), GTT"Project%d/Desc", i);
                gtt_project_set_desc(proj, gnome_config_get_string(s));
                g_snprintf(s, sizeof (s), GTT"Project%d/SecsEver=0", i);
                proj->secs = gnome_config_get_int(s);
                g_snprintf(s, sizeof (s), GTT"Project%d/SecsDay=0", i);
                proj->day_secs = gnome_config_get_int(s);
        }
	first_proj_title = NULL;
        update_status_bar();
        if ((_n != config_show_tb_new) ||
            (_f != config_show_tb_file) ||
            (_c != config_show_tb_ccp) ||
            (_p != config_show_tb_prop) ||
            (_t != config_show_tb_timer) ||
            (_o != config_show_tb_pref) ||
            (_h != config_show_tb_help) ||
            (_e != config_show_tb_exit)) {
                update_toolbar_sections();
        }
#if 0
	/* hack alert XXX FIXME bogus */
	gtt_err_set_code (GTT_NO_ERR);
	gtt_xml_read_file ("/home/linas/.gnome/gtt.xml");
	printf ("duuude err = %d\n", gtt_err_get_code());
#endif
        return 1;
}



int
project_list_save(const char *fname)
{
        char s[64];
	GList *node;
        int i, old_num;
#if 0
	/* hack alert XXX FIXME bogus */
	gtt_xml_write_file ("/home/linas/.gnome/gtt.xml");
#endif

        old_num = gnome_config_get_int(GTT"Misc/NumProjects=0");
        g_snprintf(s, sizeof (s), "%ld", time(0));
        gnome_config_set_string(GTT"Misc/LastTimer", s);
        gnome_config_set_int(GTT"Misc/TimerRunning", (timer_is_running()));
        gnome_config_set_bool(GTT"Display/ShowSecs", config_show_secs);
        gnome_config_set_bool(GTT"Display/ShowTableHeader", config_show_clist_titles);
        gnome_config_set_bool(GTT"Toolbar/ShowIcons", config_show_tb_icons);
        gnome_config_set_bool(GTT"Toolbar/ShowTexts", config_show_tb_texts);
        gnome_config_set_bool(GTT"Toolbar/ShowTips", config_show_tb_tips);
        gnome_config_set_bool(GTT"Display/ShowStatusbar", config_show_statusbar);
        gnome_config_set_bool(GTT"Toolbar/ShowNew", config_show_tb_new);
        gnome_config_set_bool(GTT"Toolbar/ShowFile", config_show_tb_file);
        gnome_config_set_bool(GTT"Toolbar/ShowCCP", config_show_tb_ccp);
        gnome_config_set_bool(GTT"Toolbar/ShowProp", config_show_tb_prop);
        gnome_config_set_bool(GTT"Toolbar/ShowTimer", config_show_tb_timer);
        gnome_config_set_bool(GTT"Toolbar/ShowPref", config_show_tb_pref);
        gnome_config_set_bool(GTT"Toolbar/ShowHelp", config_show_tb_help);
        gnome_config_set_bool(GTT"Toolbar/ShowExit", config_show_tb_exit);
        if (config_command)
                gnome_config_set_string(GTT"Actions/ProjCommand", config_command);
        else
                gnome_config_clean_key(GTT"Actions/ProjCommand");
        if (config_command_null)
                gnome_config_set_string(GTT"Actions/NullCommand", config_command_null);
        else
                gnome_config_clean_key(GTT"Actions/NullCommand");
        gnome_config_set_bool(GTT"LogFile/Use", config_logfile_use);
        if (config_logfile_name)
                gnome_config_set_string(GTT"LogFile/Filename", config_logfile_name);
        else
                gnome_config_clean_key(GTT"LogFile/Filename");
	if (config_logfile_str)
		gnome_config_set_string(GTT"LogFile/Entry", config_logfile_str);
	else
		gnome_config_set_string(GTT"LogFile/Entry", "");
	if (config_logfile_stop)
		gnome_config_set_string(GTT"LogFile/EntryStop",
					config_logfile_stop);
	else
		gnome_config_set_string(GTT"LogFile/EntryStop", "");
        gnome_config_set_int(GTT"LogFile/MinSecs", config_logfile_min_secs);
	for (i = 0; i < GTK_CLIST(glist)->columns; i++) {
		g_snprintf(s, sizeof (s), GTT"CList/ColumnWidth%d", i);
		gnome_config_set_int(s, GTK_CLIST(glist)->column[i].width);
	}
        i = 0;
        for (node = plist; node; node = node->next) 
	{
		GttProject *prj = node->data;
                if (!prj->title) continue;
                g_snprintf(s, sizeof (s), GTT"Project%d/Title", i);
                gnome_config_set_string(s, prj->title);
                g_snprintf(s, sizeof (s), GTT"Project%d/Desc", i);
                if (prj->desc) {
                        gnome_config_set_string(s, prj->desc);
                } else {
                        gnome_config_clean_key(s);
                }
                g_snprintf(s, sizeof (s), GTT"Project%d/SecsEver", i);
                gnome_config_set_int(s, prj->secs);
                g_snprintf(s, sizeof (s), GTT"Project%d/SecsDay", i);
                gnome_config_set_int(s, prj->day_secs);
                i++;
        }
        gnome_config_set_int(GTT"Misc/NumProjects", i);
        for (; i < old_num; i++) {
                g_snprintf(s, sizeof (s), GTT"Project%d", i);
                gnome_config_clean_section(s);
        }
        gnome_config_sync();
        return 1;
}

/* ======================================================= */
/* project list export */

static char *
get_time (int secs)
{
	/* Translators: This is a "time format", that is
	 * format on how to print the elapsed time with
	 * hours:minutes:seconds. */
	return g_strdup_printf (_("%d:%02d:%02d"),
				secs / (60*60),
				(secs / 60) % 60,
				secs % 60);
}

gboolean
project_list_export (const char *fname)
{
	FILE *fp;
	GList *node;

	fp = fopen (fname, "w");
	if (fp == NULL)
		return FALSE;

	/* Translators: this is the header of a table separated file,
	 * it should really be all ASCII, or at least not multibyte,
	 * I don't think most spreadsheets would handle that well. */
	fprintf (fp, "Title\tDescription\tTotal time\tTime today\n");

        for (node = plist; node; node = node->next) 
	{
		GttProject *prj = node->data;
		char *total_time, *time_today;
                if (!prj->title) continue;
		total_time = get_time (prj->secs);
		time_today = get_time (prj->day_secs);
		fprintf (fp, "%s\t%s\t%s\t%s\n",
			 gtt_sure_string (prj->title),
			 gtt_sure_string (prj->desc),
			 total_time,
			 time_today);
		g_free (total_time);
		g_free (time_today);
        }

	fclose (fp);

        return TRUE;
}



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
