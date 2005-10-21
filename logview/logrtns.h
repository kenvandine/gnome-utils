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

#ifndef __LOGRTNS_H__
#define __LOGRTNS_H__

#include <time.h>
#include <libgnomevfs/gnome-vfs.h>

typedef struct
{
    GDate *date;
    /* fulldate is only used in the status bar : needs updating */
    struct tm fulldate;
    long first_line, last_line; /* First and last line in the log */
    gboolean expand;
    GtkTreePath *path;
} Day;

typedef struct 
{
    GtkWidget *calendar;
    GList *days;
    gboolean first_pass;
} CalendarData;

typedef struct
{
    /* startdate and enddate are only used in info.c now
       find another way to do this.*/
    time_t mtime;
    GnomeVFSFileSize size;
} LogStats;

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
    GList *days;

	char *name;
	char *display_name;
	CalendarData *caldata;
	LogStats *lstats;
	gint selected_line_first;
	gint selected_line_last;
	gint total_lines; /* no of lines in the file */
    gint displayed_lines; /* no of lines displayed now */
	LogLine **lines; /* actual lines */
	gboolean first_time;
	GtkTreePath *current_path;
    GtkTreeModel *model;
	int versions;
	int current_version;
	/* older_logs[0] should not be used */
	Log *older_logs[5];
	Log *parent_log;
	
	/* Monitor info */
	GnomeVFSFileOffset mon_offset;
	GnomeVFSMonitorHandle *mon_handle;
    GnomeVFSHandle *mon_file_handle;
	gboolean monitored;

    gpointer window;
};

gboolean file_is_log (char *filename, gboolean show_error);
char *logline_get_date_string (LogLine *line);
Log *log_open (char *filename, gboolean show_error);
gboolean log_read_new_lines (Log *log);
void log_close (Log * log);

#endif /* __LOGRTNS_H__ */
