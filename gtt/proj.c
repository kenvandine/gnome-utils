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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#include "gtt.h"

#ifdef DEBUG
#define GTT "/gtt-DEBUG/"
#else /* not DEBUG */
#define GTT "/gtt/"
#endif /* not DEBUG */


char *first_proj_title = NULL;



project *project_new(void)
{
	project *proj;
	
	proj = g_malloc(sizeof(project));
	proj->title = NULL;
	proj->desc = NULL;
	proj->secs = proj->day_secs = 0;
        proj->row = -1;
	return proj;
}


project *project_new_title(char *t)
{
	project *proj;

	proj = project_new();
	if (!t) return proj;
	proj->title = g_malloc(strlen(t) + 1);
	strcpy(proj->title, t);
	return proj;
}



project *project_dup(project *proj)
{
	project *p;

	if (!proj) return NULL;
	p = project_new();
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
	return p;
}



void project_destroy(project *proj)
{
	if (!proj) return;
	project_list_remove(proj);
	if (proj->title) g_free(proj->title);
	if (proj->desc) g_free(proj->desc);
	g_free(proj);
}



void project_set_title(project *proj, char *t)
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



void project_set_desc(project *proj, char *d)
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




/*******************************************************************
 * project_list
 */

project_list *plist = NULL;


void project_list_add(project *p)
{
	project_list *t, *t2;

	if (!p) return;
	t = g_malloc(sizeof(project_list));
	t->proj = p;
	t->next = NULL;
	if (!plist) {
		plist = t;
		return;
	}
	for (t2 = plist; t2->next; t2 = t2->next) ;
	t2->next = t;
}



void project_list_insert(project *p, int pos)
{
	project_list *t, *t2;
	int i;

	if (!p) return;
	t = g_malloc(sizeof(project_list));
	t->proj = p;
	t->next = NULL;
	if (!plist) {
		plist = t;
		return;
	}
	if (pos == 0) {
		t->next = plist;
		plist = t;
		return;
	}
	for (t2 = plist, i = 1; (t2->next) && (i < pos); t2 = t2->next, i++) ;
	t->next = t2->next;
	t2->next = t;
}



void project_list_remove(project *p)
{
	project_list *t, *t2;
	
	if ((!p) || (!plist)) return;
	if (plist->proj == p) {
		t = plist;
		plist = plist->next;
		g_free(t);
		return;
	}
	for (t = plist; (t->next) && (t->next->proj != p); t = t->next) ;
	if (!t->next) return;
	t2 = t->next;
	t->next = t2->next;
	g_free(t2);
}



void project_list_destroy(void)
{
	project_list *t;
	
	while (plist) {
		t = plist->next;
		project_destroy(plist->proj);
		plist = t;
	}
}



void project_list_time_reset(void)
{
	project_list *t;
	
	for (t = plist; t; t = t->next) {
		t->proj->day_secs = 0;
		if (t->proj->row != -1) clist_update_label(t->proj);
	}
}



static char *build_rc_name(void)
{
	static char buf[1024] = { 0 };

	if (buf[0]) return buf;
	if (getenv("HOME")) {
		strcpy(buf, getenv("HOME"));
		strcat(buf, "/" RC_NAME);
	} else {
		strcpy(buf, RC_NAME);
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
project_list_load_old(char *fname)
{
	FILE *f;
	project_list *pl, *t;
	project *proj = NULL;
	char s[1024];
	int i;
	time_t tmp_time = -1;
        int _n, _f, _c, _p, _t, _o, _h, _e;

	if (!fname) fname = build_rc_name();
	if (NULL == (f = fopen(fname, "rt"))) {
#ifdef ENOENT
                if (errno == ENOENT) return 0;
#endif
		g_warning("could not open %s\n", fname);
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
			project_set_desc(proj, &s[1]);
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
			proj = project_new();
			project_list_add(proj);
			proj->secs = atol(s);
			for (i = 0; s[i] != ' '; i++) ;
			i++;
			proj->day_secs = atol(&s[i]);
			for (; s[i] != ' '; i++) ;
			i++;
			while (s[strlen(s) - 1] == '\n')
				s[strlen(s) - 1] = 0;
			project_set_title(proj, &s[i]);
		}
	}
	if ((errno) && (!feof(f))) goto err;
	fclose(f);

	t = plist;
	plist = pl;
	project_list_destroy();
	plist = t;
	if (tmp_time > 0)
		last_timer = tmp_time;
	else
		last_timer = 0;
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
	g_warning("error reading %s\n", fname);
	project_list_destroy();
	plist = pl;
	return 0;
}



int
project_list_load(char *fname)
{
        char s[64];
        int i, num;
        project *proj;
        int _n, _f, _c, _p, _t, _o, _h, _e;
	gboolean got_default;

        gnome_config_get_int_with_default(GTT"Misc/NumProjects=0", &got_default);
        if (got_default) {
                return project_list_load_old(fname);
        }
	if ((cur_proj) && (cur_proj->title) && (!first_proj_title)) {
		first_proj_title = g_strdup(cur_proj->title);
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
	if (gnome_config_get_int(GTT"Misc/TimerRunning=1")) {
		start_timer();
	} else {
		stop_timer();
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
        config_logfile_min_secs = gnome_config_get_int(GTT"LogFile/MinSecs");
	for (i = 0; i < GTK_CLIST(glist)->columns; i++) {
		sprintf(s, GTT"CList/ColumnWidth%d=0", i);
		num = gnome_config_get_int(s);
		if (num) {
			clist_header_width_set = 1;
			gtk_clist_set_column_width(GTK_CLIST(glist),
						   i, num);
		}
	}
        num = gnome_config_get_int(GTT"Misc/NumProjects=0");
        for (i = 0; i < num; i++) {
                proj = project_new();
                project_list_add(proj);
                sprintf(s, GTT"Project%d/Title", i);
                project_set_title(proj, gnome_config_get_string(s));
		if ((proj->title) && (first_proj_title)) {
			if (0 == strcmp(proj->title, first_proj_title)) {
				cur_proj_set(proj);
				g_free(first_proj_title);
				first_proj_title = NULL;
			}
		}
                sprintf(s, GTT"Project%d/Desc", i);
                project_set_desc(proj, gnome_config_get_string(s));
                sprintf(s, GTT"Project%d/SecsEver=0", i);
                proj->secs = gnome_config_get_int(s);
                sprintf(s, GTT"Project%d/SecsDay=0", i);
                proj->day_secs = gnome_config_get_int(s);
        }
	g_free(first_proj_title);
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
        return 1;
}



int
project_list_save(char *fname)
{
        char s[64];
        project_list *pl;
        int i, old_num;

        old_num = gnome_config_get_int(GTT"Misc/NumProjects=0");
        sprintf(s, "%ld", last_timer);
        gnome_config_set_string(GTT"Misc/LastTimer", s);
        gnome_config_set_int(GTT"Misc/TimerRunning", (main_timer != 0));
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
        gnome_config_set_int(GTT"LogFile/MinSecs", config_logfile_min_secs);
	for (i = 0; i < GTK_CLIST(glist)->columns; i++) {
		sprintf(s, GTT"CList/ColumnWidth%d", i);
		gnome_config_set_int(s, GTK_CLIST(glist)->column[i].width);
	}
        i = 0;
        for (pl = plist; pl; pl = pl->next) {
                if (!pl->proj) continue;
                if (!pl->proj->title) continue;
                sprintf(s, GTT"Project%d/Title", i);
                gnome_config_set_string(s, pl->proj->title);
                sprintf(s, GTT"Project%d/Desc", i);
                if (pl->proj->desc) {
                        gnome_config_set_string(s, pl->proj->desc);
                } else {
                        gnome_config_clean_key(s);
                }
                sprintf(s, GTT"Project%d/SecsEver", i);
                gnome_config_set_int(s, pl->proj->secs);
                sprintf(s, GTT"Project%d/SecsDay", i);
                gnome_config_set_int(s, pl->proj->day_secs);
                i++;
        }
        gnome_config_set_int(GTT"Misc/NumProjects", i);
        for (; i < old_num; i++) {
                sprintf(s, GTT"Project%d", i);
                gnome_config_clean_section(s);
        }
        gnome_config_sync();
        return 1;
}



char *project_get_timestr(project *proj, int show_secs)
{
	static char s[20];
	time_t t;
	
	if (proj == NULL) {
		project_list *p;
		t = 0;
		for (p = plist; p != NULL; p = p->next)
			t += p->proj->day_secs;
	} else {
		t = proj->day_secs;
	}
        if (show_secs)
                sprintf(s, "%02d:%02d:%02d", (int)(t / 3600),
                        (int)((t % 3600) / 60), (int)(t % 60));
        else
                sprintf(s, "%02d:%02d", (int)(t / 3600),
                        (int)((t % 3600) / 60));
	return s;
}

char *project_get_total_timestr(project *proj, int show_secs)
{
	static char s[20];
	time_t t;

	if (proj == NULL) {
		return NULL;
	} else {
		t = proj->secs;
	}
	if (show_secs)
		sprintf(s, "%02d:%02d:%02d", (int)(t / 3600),
			(int)((t % 3600) / 60), (int)(t % 60));
	else
		sprintf(s, "%02d:%02d", (int)(t / 3600),
			(int)((t % 3600) / 60));
	return s;
}



static void
project_list_sort(int (cmp)(const void *, const void *))
{
        project_list *pl;
        project_list **parray;
        int i, num;

        if (!plist) return;
        for (i = 0, pl = plist; pl; pl = pl->next, i++) ;
        parray = g_malloc(i * sizeof(project_list *));
        for (i = 0, pl = plist; pl; pl = pl->next, i++)
                parray[i] = pl;
        qsort(parray, i, sizeof(project_list *), cmp);
        num = i;
        plist = parray[0];
        pl = plist;
        for (i = 1; i < num; i++) {
                pl->next = parray[i];
                pl = parray[i];
        }
        pl->next = NULL;
        g_free(parray);
}



static int
cmp_time(const void *aa, const void *bb)
{
        project_list *a = *(project_list **)aa;
        project_list *b = *(project_list **)bb;
        return (int)(b->proj->day_secs - a->proj->day_secs);
}

static int
cmp_total_time(const void *aa, const void *bb)
{
	project_list *a = *(project_list **)aa;
	project_list *b = *(project_list **)bb;
	return (int)(b->proj->secs - a->proj->secs);
}



static int
cmp_title(const void *aa, const void *bb)
{
        project_list *a = *(project_list **)aa;
        project_list *b = *(project_list **)bb;
        return strcmp(a->proj->title, b->proj->title);
}

static int
cmp_desc(const void *aa, const void *bb)
{
	project_list *a = *(project_list **)aa;
	project_list *b = *(project_list **)bb;
	return strcmp(a->proj->desc, b->proj->desc);
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

