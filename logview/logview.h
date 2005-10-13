
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

#include <time.h>
#include <libgnomevfs/gnome-vfs.h>

#define MAX_WIDTH                240
#define MAX_HOSTNAME_WIDTH       257	/* Need authoritative answer on this value. */
#define MAX_PROC_WIDTH           60
#define MAX_VERSIONS             5

/* the following is a simple hashing function that will convert a
 * given date into an integer value that can be used as a key for
 * the date_headers hash table */
#define DATEHASH(month, day)     GUINT_TO_POINTER (month * 31 + day)

/*
 *    ,----------.
 *    | Typedefs |
 *    `----------'
 */

struct __datemark
{
  time_t time;		/* date           */
  struct tm fulldate;
  long offset;		/* offset in file */
  char year;		/* to correct for logfiles with many years */
  long ln;		/* Line # from begining of file */
  struct __datemark *next, *prev;
};

typedef struct __datemark DateMark;

typedef struct
{
  time_t startdate, enddate;
  time_t mtime;
  long numlines;
  long size;
  DateMark *firstmark, *lastmark;
}
LogStats;

typedef struct {
  GtkWidget *calendar;
  DateMark *curmonthmark;
  gboolean first_pass;
} CalendarData;

typedef struct
{
    char *message;
    char *process;
    char *hostname;
    signed char month;
    signed char date;
    signed char hour;
    signed char min;
    signed char sec;
} LogLine;

typedef struct _log Log;
struct _log
{
	DateMark *curmark;
	char *name;
	char *display_name;
	CalendarData *caldata;
	LogStats lstats;
	gint selected_line_first;
	gint selected_line_last;
	gint total_lines; /* no of lines in the file */
    gint displayed_lines; /* no of lines displayed now */
	LogLine **lines; /* actual lines */
	gboolean first_time;
	gboolean has_date;
	GtkTreePath *current_path;
	GtkTreePath *expand_paths[33];
    GtkTreeModel *model;
	gboolean expand[33];
	int versions;
	int current_version;
	/* older_logs[0] should not be used */
	Log *older_logs[5];
	Log *parent_log;
	GHashTable *date_headers; /* stores paths to date headers */
	
	/* Monitor info */
	GnomeVFSFileOffset mon_offset;
	GnomeVFSMonitorHandle *mon_handle;
    GnomeVFSHandle *mon_file_handle;
	gboolean monitored;

    gpointer window;
};

/*
 *    ,---------------------.
 *    | Function Prototypes |
 *    `---------------------'
 */

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
GtkTreePath *loglist_find_logname (LogviewWindow *logview, gchar *logname);


#endif /* __LOGVIEW_H__ */
