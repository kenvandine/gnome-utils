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

#ifndef __GTT_H__
#define __GTT_H__


#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif /* TM_IN_SYS_TIME */
#include <time.h>
#endif /* TIME_WITH_SYS_TIME */


typedef struct _project project;

#include "dialog.h"
#include "menucmd.h"
#include "toolbar.h"

#ifdef DEBUG
#define APP_NAME "GTimeTracker DEBUG"
#define RC_NAME ".gtimetrackerrc-" VERSION
#else
#define APP_NAME "GTimeTracker"
#define RC_NAME ".gtimetrackerrc"
#endif



/* menus.h */

GtkWidget *menus_get_popup(void);
void menus_create(GnomeApp *app);
void menus_set_states(void);

GtkCheckMenuItem *menus_get_toggle_timer(void);


/* err.c */

void err_init(void);



/* timer.c */

extern gint main_timer;
extern time_t last_timer;

void start_timer(void);
void stop_timer(void);



/* proj.c */

struct _project {
	char *title;
	int secs, day_secs;
	char *desc;
        gint row;
};

typedef struct _project_list {
	project *proj;
	struct _project_list *next;
} project_list;

extern project_list *plist;


project *project_new(void);
project *project_new_title(char *);
project *project_dup(project *);
void project_destroy(project *);
void project_set_title(project *proj, char *t);
void project_set_desc(project *proj, char *d);

void project_list_add(project *p);
void project_list_insert(project *p, int pos);
void project_list_remove(project *p);
void project_list_destroy(void);
void project_list_time_reset(void);
int project_list_load(char *fname);
int project_list_save(char *fname);
void project_list_sort_time(void);
void project_list_sort_title(void);

char *project_get_timestr(project *p, int show_secs);


/* prop.c */

void prop_dialog_set_project(project *proj);
void prop_dialog(project *proj);

/* options.c */

void options_dialog(void);


/* log.c */

void log_proj(project *proj);
void log_start(void);
void log_exit(void);
void log_endofday(void);


/* app.c */

extern project *cur_proj;
extern GtkWidget *glist, *window;
extern GtkWidget *status_bar;
extern int config_show_secs;
extern int config_show_statusbar;
extern int config_show_clist_titles;
extern int config_show_tb_icons;
extern int config_show_tb_texts;
extern int config_show_tb_tips;
extern int config_show_tb_new;
extern int config_show_tb_file;
extern int config_show_tb_ccp;
extern int config_show_tb_prop;
extern int config_show_tb_timer;
extern int config_show_tb_pref;
extern int config_show_tb_help;
extern int config_show_tb_exit;
extern char *config_command, *config_command_null, *config_logfile_name;
extern int config_logfile_use, config_logfile_min_secs;

void update_status_bar(void);
void cur_proj_set(project *p);

void app_new(int argc, char *argv[]);


/* clist.c */

GtkWidget *create_clist(void);
void setup_clist(void);
void clist_add(project *p);
void clist_insert(project *p, gint pos);
void clist_remove(project *p);
void clist_update_label(project *p);
void clist_update_title(project *p);


/* main.c */

void unlock_gtt(void);
char *gtt_gettext(char *s);


#endif /* __GTT_H__ */
