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
    long first_line, last_line; /* First and last line for this day in the log */
    gboolean expand;
    GtkTreePath *path;
} Day;

typedef struct
{
    time_t file_time;
    GnomeVFSFileSize file_size;
} LogStats;

typedef struct TreePathRange
{
  GtkTreePath *first;
  GtkTreePath *last;
}TreePathRange;

typedef struct _log Log;
struct _log
{
	char *name;
	char *display_name;
	LogStats *stats;

    GSList *days;
    gchar **lines;
	gint selected_line_first;
	gint selected_line_last;
	gint total_lines; /* no of lines in the file */
    gint displayed_lines; /* no of lines displayed now */

	/* Monitor info */
	GnomeVFSFileSize mon_offset;
	GnomeVFSMonitorHandle *mon_handle;
    GnomeVFSHandle *mon_file_handle;
	gboolean monitored;
    gboolean needs_refresh;

	gboolean first_time;
    GtkTreeModel *model;
    GtkTreeModelFilter *filter;
    GList *bold_rows_list;
	TreePathRange selected_range;
    GtkTreePath *visible_first;

	int versions;
	int current_version;
	/* older_logs[0] should not be used */
	Log *older_logs[5];
	Log *parent_log;
	
    gpointer window;
};

gboolean file_is_log (char *filename, gboolean show_error);
Log *log_open (char *filename, gboolean show_error);
gboolean log_read_new_lines (Log *log);
gboolean log_unbold (gpointer data);
void log_close (Log * log);
gchar *log_extract_filename (Log *log);
gchar *log_extract_dirname (Log *log);

#endif /* __LOGRTNS_H__ */
