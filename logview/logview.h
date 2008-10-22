
/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */

#ifndef __LOGVIEW_H__
#define __LOGVIEW_H__

#include "logrtns.h"

#define MAX_VERSIONS             5

#define LOGVIEW_TYPE_WINDOW		  (logview_window_get_type ())
#define LOGVIEW_WINDOW(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindow))
#define LOGVIEW_WINDOW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))
#define LOGVIEW_IS_WINDOW(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))

typedef struct _LogviewWindow LogviewWindow;
typedef struct _LogviewWindowClass LogviewWindowClass;

struct _LogviewWindow {
	GtkWindow parent_instance;

	GtkWidget *view;		
	GtkWidget *statusbar;
	GtkUIManager *ui_manager;

	GtkWidget *calendar;
	GtkWidget *find_bar;
	GtkWidget *loglist;
	GtkWidget *sidebar; 
	GtkWidget *version_bar;
	GtkWidget *version_selector;
        GtkWidget *hpaned;
    
        GSList *logs;
	Log *curlog;

	int original_fontsize, fontsize;
};

struct _LogviewWindowClass {
	GtkWindowClass parent_class;
};

#include "loglist.h"

Log *logview_get_active_log (LogviewWindow *logview);
LogList *logview_get_loglist (LogviewWindow *logview);
int logview_count_logs (LogviewWindow *logview);
void logview_select_log (LogviewWindow *logview, Log *log);
void logview_add_log_from_name (LogviewWindow *logview, gchar *file);
void logview_add_logs_from_names (LogviewWindow *logview, GSList *lognames, gchar *selected);
void logview_menus_set_state (LogviewWindow *logview);
void logview_set_window_title (LogviewWindow *window);
void logview_set_font (LogviewWindow *window, const gchar *fontname);
void logview_show_main_content (LogviewWindow *window);
GType logview_window_get_type (void);
GtkWidget *logview_window_new (void);

#endif /* __LOGVIEW_H__ */
