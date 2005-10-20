
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

#define MAX_WIDTH                240
#define MAX_HOSTNAME_WIDTH       257	/* Need authoritative answer on this value. */
#define MAX_PROC_WIDTH           60
#define MAX_VERSIONS             5

/* Function Prototypes */

#define LOGVIEW_TYPE_WINDOW		  (logview_window_get_type ())
#define LOGVIEW_WINDOW(obj)		  (GTK_CHECK_CAST ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindow))
#define LOGVIEW_WINDOW_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))
#define LOGVIEW_IS_WINDOW(obj)	  (GTK_CHECK_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_IS_WINDOW_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_WINDOW_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))

typedef struct _LogviewWindow LogviewWindow;
typedef struct _LogviewWindowClass LogviewWindowClass;

struct _LogviewWindow {
	GtkWindow parent_instance;

	GtkWidget *view;		
	GtkWidget *statusbar;
	GtkUIManager *ui_manager;

	GtkWidget *calendar;
	gboolean calendar_visible;

	gboolean loginfovisible;

	GtkWidget *find_bar;
	GtkWidget *find_entry;
	GtkWidget *find_next;
	GtkWidget *find_prev;
	gchar *find_string;

	GtkWidget *loglist;
	GtkWidget *treeview;
	GtkWidget *sidebar; 
	gboolean sidebar_visible;
	GtkWidget *version_bar;
	GtkWidget *version_selector;
    GtkWidget *progressbar;
    
    GList *logs;
	Log *curlog;

	GtkClipboard *clipboard;
	
	int original_fontsize, fontsize;
};

struct _LogviewWindowClass {
	GtkWindowClass parent_class;
};

void logview_set_window_title (LogviewWindow *window);

#endif /* __LOGVIEW_H__ */
