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


#include <gnome.h>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif /* TM_IN_SYS_TIME */
#include <time.h>
#endif /* TIME_WITH_SYS_TIME */

#include "proj.h"


#ifdef DEBUG
#define APP_NAME "GTimeTracker DEBUG"
#else
#define APP_NAME "GTimeTracker"
#endif

#define XML_DATA_FILENAME "gtt-xml.gttml"

/* menus.h */

GtkWidget *menus_get_popup(void);
void menus_create(GnomeApp *app);
void menus_set_states(void);

GtkCheckMenuItem *menus_get_toggle_timer(void);


/* err.c */

void err_init(void);


/* timer.c */

void start_timer(void);
void stop_timer(void);
gboolean timer_is_running (void);


/* options.c */

void options_dialog(void);
extern int config_show_secs;
extern int config_show_statusbar;
extern int config_show_clist_titles;
extern int config_show_subprojects;
extern int config_show_tb_icons;
extern int config_show_tb_texts;
extern int config_show_tb_tips;
extern int config_show_tb_new;
extern int config_show_tb_file;
extern int config_show_tb_ccp;
extern int config_show_tb_journal;
extern int config_show_tb_prop;
extern int config_show_tb_timer;
extern int config_show_tb_pref;
extern int config_show_tb_help;
extern int config_show_tb_exit;


/* log.c */

void log_proj(GttProject *proj);
void log_start(void);
void log_exit(void);
void log_endofday(void);


/* app.c */

extern GttProject *cur_proj;
extern GtkWidget *glist, *window;
extern GtkWidget *status_bar;

extern char *config_command, *config_command_null, *config_logfile_name,
	*config_logfile_str, *config_logfile_stop;
extern int config_logfile_use, config_logfile_min_secs;

/* true if command line over-rides geometry */
extern gboolean geom_size_override;
extern gboolean geom_place_override;
extern char *first_proj_title;	/* command line flag */

void update_status_bar(void);
void cur_proj_set(GttProject *p);

void app_new(int argc, char *argv[], const char *geometry_string);


/* main.c */

/* save_all() will write out all state to files */
void save_all (void);
void unlock_gtt(void);
const char *gtt_gettext(const char *s);

#define gtt_sure_string(x) ((x)?(x):"")


#endif /* __GTT_H__ */
