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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gtt.h"



project *project_new(void)
{
	project *proj;
	
	proj = g_malloc(sizeof(project));
	proj->title = NULL;
	proj->desc = NULL;
	proj->secs = proj->day_secs = 0;
	proj->label = NULL;
	proj->title_label = NULL;
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
	if (proj->title_label)
		update_title_label(proj);
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
		if (t->proj->label) update_label(t->proj);
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



int project_list_load(char *fname)
{
	FILE *f;
	project_list *pl, *t;
	project *proj = NULL;
	char s[1024];
	int i;
	time_t tmp_time = -1;

	if (!fname) fname = build_rc_name();
	if (NULL == (f = fopen(fname, "rt"))) {
		g_warning("could not open %s\n", fname);
		return 0;
	}
	pl = plist;
	plist = NULL;
	
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
	return 1;

	err:
	fclose(f);
	g_warning("error reading %s\n", fname);
	project_list_destroy();
	plist = pl;
	return 0;
}



int project_list_save(char *fname)
{
	FILE *f;
	project_list *pl;

	if (!fname) fname = build_rc_name();
	/* TODO: maybe I should work with backup files */
	if (NULL == (f = fopen(fname, "wt"))) {
		g_warning("could not open %s for writing\n", fname);
		return 0;
	}
	errno = 0;

	fprintf(f, "#\n");
	fprintf(f, "# %s init file\n", APP_NAME);
	fprintf(f, "#\n");
	fprintf(f, "# this file is generated automatically\n");
	fprintf(f, "# DO NOT EDIT UNLESS YOU KNOW WHAT YOU'RE DOING!\n");
	fprintf(f, "#\n");
	
	/* TODO: time_t can be float! */
	fprintf(f, "t%ld\n", last_timer);
	fprintf(f, "s %s\n", (config_show_secs) ? "on" : "off");
	if (config_command)
		fprintf(f, "c %s\n", config_command);
	if (config_command_null)
		fprintf(f, "n %s\n", config_command_null);
	fprintf(f, "lu %s\n", (config_logfile_use) ? "on" : "off");
	if (config_logfile_name)
		fprintf(f, "ln %s\n", config_logfile_name);
	if (config_logfile_min_secs)
		fprintf(f, "ls %d\n", config_logfile_min_secs);
	
	for (pl = plist; pl; pl = pl->next) {
		if (!pl->proj) continue;
		if (!pl->proj->title) continue;
		fprintf(f, "%d %d %s\n", pl->proj->secs, pl->proj->day_secs,
			pl->proj->title);
		if (pl->proj->desc) {
			fprintf(f, " %s\n", pl->proj->desc);
		}
		if (errno) break;
	}
	fclose(f);
	if (errno) {
		g_warning("error while writing to %s\n", fname);
		return 0;
	}
	return 1;
}

