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


typedef struct gtt_project_s GttProject;

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

void start_timer(void);
void stop_timer(void);



/* proj.c */
typedef struct gtt_task_s GttTask;
typedef struct gtt_interval_s GttInterval;

typedef struct _project_list {
	GttProject *proj;
	struct _project_list *next;
} project_list;



extern project_list *plist;
extern char *first_proj_title;


GttProject *project_new(void);
GttProject *project_new_title_desc(const char *, const char *);
GttProject *project_dup(GttProject *);
void project_destroy(GttProject *);
void project_set_title(GttProject *proj, const char *t);
void project_set_desc(GttProject *proj, const char *d);

/* The project_timer_start() routine logs the time when
 *    a new task interval starts.
 * The project_timer_update() routine updates the end-time
 *    for a task interval. 
 */
void gtt_project_timer_start (GttProject *);
void gtt_project_timer_update (GttProject *);
void gtt_project_timer_stop (GttProject *);

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

/* tasks */
GttTask *	gtt_task_new (void);
void 		gtt_task_destroy (GttTask *task);


/* prop.c */

void prop_dialog_set_project(GttProject *proj);
void prop_dialog(GttProject *proj);

/* options.c */

void options_dialog(void);


/* log.c */

void log_proj(GttProject *proj);
void log_start(void);
void log_exit(void);
void log_endofday(void);


/* app.c */

extern GttProject *cur_proj;
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
extern char *config_command, *config_command_null, *config_logfile_name,
	*config_logfile_str, *config_logfile_stop;
extern int config_logfile_use, config_logfile_min_secs;

void update_status_bar(void);
void cur_proj_set(GttProject *p);

void app_new(int argc, char *argv[], const char *geometry_string);


/* clist.c */

extern int clist_header_width_set;

GtkWidget *create_clist(void);
void setup_clist(void);
void clist_add(GttProject *p);
void clist_insert(GttProject *p, gint pos);
void clist_remove(GttProject *p);
void clist_update_label(GttProject *p);
void clist_update_title(GttProject *p);
void clist_update_desc(GttProject *p);


/* main.c */

void unlock_gtt(void);
const char *gtt_gettext(const char *s);

#define gtt_sure_string(x) ((x)?(x):"")


#endif /* __GTT_H__ */
