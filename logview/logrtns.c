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

#ifdef __CYGWIN__
#define timezonevar
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include "logview.h"
#include "logrtns.h"
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include "misc.h"

char *error_main = N_("One file or more could not be opened");
enum {
    MONTH = 0, DAY,
    HOUR, MIN, SEC,
    HOSTNAME, PROCESS, MESSAGE
};

static LogStats *log_stats_new (char *filename, gboolean show_error);

/* File checking */

static gboolean
file_exist (char *filename, gboolean show_error)
{
   GnomeVFSHandle *handle;
   GnomeVFSResult result;
   char buff[1024];

   result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);
   if (result != GNOME_VFS_OK) {
	   if (show_error) {
		   switch (result) {
		   case GNOME_VFS_ERROR_ACCESS_DENIED:
		   case GNOME_VFS_ERROR_NOT_PERMITTED:
			   g_snprintf (buff, sizeof (buff),
				       _("%s is not user readable. "
					 "Either run the program as root or ask the sysadmin to "
					 "change the permissions on the file.\n"), filename);
			   break;
		   case GNOME_VFS_ERROR_TOO_BIG:
			   g_snprintf (buff, sizeof (buff), _("%s is too big."), filename);
			   break;
		   default:
			   g_snprintf (buff, sizeof (buff),
				       _("%s could not be opened."), filename);
		   }
		   ShowErrMessage (NULL, error_main, buff);
	   }
	   return FALSE;
   }

   gnome_vfs_close (handle);
   return TRUE;
}

static gboolean
file_is_zipped (char *filename)
{
	char *mime_type;

	mime_type = gnome_vfs_get_mime_type (filename);

	if (strcmp (mime_type, "application/x-gzip")==0 ||
	    strcmp (mime_type, "application/x-zip")==0 ||
	    strcmp (mime_type, "application/zip")==0)
		return TRUE;
	else
		return FALSE;
}

gboolean
file_is_log (char *filename, gboolean show_error)
{
    LogStats *stats;
    stats = log_stats_new (filename, show_error);
    if (stats==NULL)
        return FALSE;
    else {
        g_free (stats);
        return TRUE;
    }
}

/* log line manipulations */


static LogLine *
logline_new_from_string (gchar *message, gboolean has_date)
{
   char **splits;
   int i, field;
   LogLine *line;
   
   line = g_new (LogLine, 1);
   if (line == NULL)
       return NULL;

   if (message[0] == 0) {
       line->message=NULL;       
       return (line);
   }

   if (!has_date) {
       line->message = g_strdup (message);
       return (line);
   }

   /* Seperation of a log line into 8 fields */
   /* Some of them can be empty */
   splits = g_strsplit_set (message, " :\n", 9);

   i = 0; field= 0;
   while (splits[i]!=NULL) {

       if (g_str_equal (splits[i], ""))
           i++;

       switch (field) {
       case MONTH:
           /*           line->month = string_get_month (splits[i]);
           if (line->month==12) {
               line->month == -1;
               line->message = g_strdup (message);
               }*/
           break;
       case DAY:
           /*           line->date = (int) atoi (splits[i]);*/
           break;
       case HOUR:
           line->hour = (int) atoi (splits[i]);
           break;
       case MIN:
           line->min = (int) atoi (splits[i]);
           break;
       case SEC:
           line->sec = (int) atoi (splits[i]);
           break;
       case HOSTNAME:
           line->hostname = g_strdup (splits[i]);
           break;
       case PROCESS:
           line->process = g_strdup (splits[i]);
           break;
       case MESSAGE:
           line->message = g_strdup (splits[i]);
           break;
       default:
           break;
       }
       i++;
       field++;
   }

   g_strfreev (splits);
   return (line);
}

/* log functions */

gint 
days_compare (gconstpointer a, gconstpointer b)
{
    const Day *day1 = a, *day2 = b;
    return (g_date_compare (day1->date, day2->date));
}

gchar *
string_get_date_string (gchar *line)
{
    gchar **split, *date_string;
    gchar *month=NULL, *day=NULL;
    int i=0;

    split = g_strsplit (line, " ", 4);
    while ((day == NULL || month == NULL) && split[i]!=NULL) {
        if (g_str_equal (split[i], "")) {
            i++;
            continue;
        }

        if (month == NULL) {
            month = split[i++];
            continue;
        }

        if (day == NULL)
            day = split[i];
        i++;
    }

    if (i==3)
        date_string = g_strconcat (month, "  ", day, NULL);
    else
        date_string = g_strconcat (month, " ", day, NULL);
    g_strfreev (split);
    return (date_string);
}


/* log_read_dates
   Read all dates which have a log entry to create calendar.
   All dates are given with respect to the 1/1/1970
   and are then corrected to the correct year once we
   reach the end.
*/
GList *
log_read_dates (gchar **buffer_lines, time_t current)
{
   int offsetyear = 0, current_year;
   GList *days = NULL, *days_copy;
   GDate *date, *newdate;
   struct tm *tmptm;
   gchar *date_string;
   Day *day;
   int i;

   tmptm = localtime (&current);
   current_year = tmptm->tm_year + 1900;

   for (i=0; buffer_lines[i]==NULL; i++);

   /* Start building the list */

   date = string_get_date (buffer_lines[i]);
   if ((date==NULL)|| !g_date_valid (date))
       return NULL;

   g_date_set_year (date, current_year);
   day = g_new (Day, 1);
   days = g_list_append (days, day);

   day->date = date;
   day->first_line = i;
   date_string = string_get_date_string (buffer_lines[i]);

   /* Count days appearing in the log */

   for (i=day->first_line; buffer_lines[i] != NULL; i++) {

       if (g_str_has_prefix (buffer_lines[i], date_string))
           continue;
       g_free (date_string);
       date_string = string_get_date_string (buffer_lines[i]);

       newdate = string_get_date (buffer_lines[i]);
       if (newdate == NULL)
           continue;
       
       day->last_line = i-1;
       
       /* Append the day to the list */	
       g_date_set_year (newdate, current_year + offsetyear);	
       if (g_date_compare (newdate, date) < 1) {
		   offsetyear++; /* newdate is next year */
           g_date_add_years (newdate, 1);
       }

       date = newdate;
       day = g_new (Day, 1);
       days = g_list_append (days, day);

	   day->date = date;
	   day->first_line = i;
   }
   day->last_line = i-1;
   days = g_list_first (days);
   g_free (date_string);

   /* Correct years now. We assume that the last date on the log
      is the date last accessed */

   for (days_copy = days; days_copy != NULL; days_copy = g_list_next (days_copy)) {       
       day = days_copy -> data;
       g_date_subtract_years (day->date, offsetyear);
   }
   
   /* Sort the days in chronological order */
   days = g_list_sort (days, days_compare);

   return (days);
}

/* 
   log_stats_new
   Read the log and get some statistics from it. 
   Returns NULL if the file is not a log.
*/

static LogStats *
log_stats_new (char *filename, gboolean show_error)
{
   GnomeVFSResult result;
   GnomeVFSFileInfo *info;
   GnomeVFSHandle *handle;
   GnomeVFSFileSize size;
   LogStats *lstats;
   char buff[1024];
   char *found_space;

   /* Read first line and check that it is text */
   result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);
   if (result != GNOME_VFS_OK) {
	   return NULL;
   }

   info = gnome_vfs_file_info_new ();
   result = gnome_vfs_get_file_info_from_handle (handle, info, GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
   if (result != GNOME_VFS_OK || info->type != GNOME_VFS_FILE_TYPE_REGULAR) {
       gnome_vfs_file_info_unref (info);
	   gnome_vfs_close (handle);
	   return NULL;
   }

   result = gnome_vfs_read (handle, buff, sizeof(buff), &size);
   gnome_vfs_close (handle);
   if (result != GNOME_VFS_OK) {
       gnome_vfs_file_info_unref (info);
	   return NULL;
   }
   
   found_space = g_strstr_len (buff, 1024, " ");
   if (found_space == NULL) {
	   if (show_error) {
		   g_snprintf (buff, sizeof (buff), _("%s not a log file."), filename);
		   ShowErrMessage (NULL, error_main, buff);
	   }
       gnome_vfs_file_info_unref (info);
	   return NULL;
   }
   
   lstats = g_new (LogStats, 1);   
   lstats->mtime = info->mtime;
   lstats->size = info->size;
   gnome_vfs_file_info_unref (info);

   return (lstats);
}

Log *
log_open (char *filename, gboolean show_error)
{
   Log *log;
   LogLine *line;
   char *buffer;
   char **buffer_lines;
   char *display_name = NULL;
   LogStats *stats;
   int i, j;
   GList *days;
   Day *day;
   GError *error;
   GnomeVFSResult result;
   int size;
   gboolean has_dates;
   
   if (file_exist (filename, show_error) == FALSE)
	   return NULL;

   if (file_is_zipped (filename)) {
	   display_name = filename;
	   filename = g_strdup_printf ("%s#gzip:", display_name);
   }   

   stats = log_stats_new (filename, show_error);
   if (stats == NULL)
       return NULL;

   result = gnome_vfs_read_entire_file (filename, &size, &buffer);
   if (result != GNOME_VFS_OK) {
	   ShowErrMessage (NULL, error_main, _("Unable to open logfile!\n"));
	   return NULL;
   }

   log = g_new0 (Log, 1);   
   if (log == NULL) {
	   ShowErrMessage (NULL, error_main, _("Not enough memory!\n"));
	   return NULL;
   }
   log->name = g_strdup (filename);
   log->display_name = display_name;
   if (display_name)
	   g_free (filename);

   buffer_lines = g_strsplit_set (buffer, "\n\r", -1);
   g_free (buffer);

   for (i=0; buffer_lines[i+1] != NULL; i++);
   log->total_lines = i;
   log->displayed_lines = 0;
   log->first_time = TRUE;
   log->lstats = stats;

   /* Read the dates */
   /* A log without dates will return NULL */
   log->days = log_read_dates (buffer_lines, log->lstats->mtime);
   has_dates = (log->days != NULL);

   /* Parse the lines */
   log->lines = g_new (LogLine*, log->total_lines);
   j = 0;
   for (days = log->days; days != NULL; days = g_list_next (days)) {
       day = days->data;
       for (i = day->first_line; i <= day->last_line; i++) {
           (log->lines)[j] = logline_new_from_string (buffer_lines[i], has_dates);
           if ((log->lines)[j] == NULL) {
               ShowErrMessage (NULL, error_main, _("Not enough memory!\n"));
               return;
           }
           j++;
       }
   }
   g_strfreev (buffer_lines);

   /* Check for older versions of the log */
   log->versions = 0;
   log->current_version = 0;
   log->parent_log = NULL;
   log->mon_offset = size;
   for (i=1; i<5; i++) {
       gchar *older_name;
       older_name = g_strdup_printf ("%s.%d", filename, i);
       log->older_logs[i] = log_open (older_name, FALSE);
       g_free (older_name);
       if (log->older_logs[i] != NULL) {
           log->older_logs[i]->parent_log = log;
           log->older_logs[i]->current_version = i;
           log->versions++;
       }
       else
           break;
   }

   return log;
}

static Log*
log_add_lines (Log *log, gchar *buffer)
{
  char **buffer_lines;
  int i, j, new_total_lines;
  LogLine **new_lines;
  Day *day;

  g_return_if_fail (log != NULL);
  g_return_if_fail (buffer != NULL);

  buffer_lines = g_strsplit (buffer, "\n", -1);
  
  for (i=0; buffer_lines[i+1] != NULL; i++);
  new_total_lines = log->total_lines + i;
  new_lines = g_new (LogLine *, new_total_lines);
  
  for (i=0; i< log->total_lines; i++)
    new_lines [i] = (log->lines)[i];
  
  /* FIXME : we should be smarter than this for day changes */
  day = g_list_last (log->days)->data;
  for (i=0; buffer_lines[i+1]!=NULL; i++) {
    j = log->total_lines + i;
    new_lines [j] = logline_new_from_string (buffer_lines[i], (log->days != NULL));
  }
  g_strfreev (buffer_lines);
  
  /* Now store the new lines in the log */
  g_free (log->lines);
  log->total_lines = new_total_lines;
  log->lines = new_lines;
  
  return log;
}

/* log_read_new_lines */

gboolean 
log_read_new_lines (Log *log)
{
	GnomeVFSResult result;
	gchar *buffer;
    GnomeVFSFileSize newsize, read;
    GnomeVFSFileOffset size;

	g_return_val_if_fail (log!=NULL, FALSE);
    
    result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_END, 0L);
	result = gnome_vfs_tell (log->mon_file_handle, &newsize);
    size = log->mon_offset;
    
	if (newsize > log->mon_offset) {
        buffer = g_malloc (newsize-size);
        result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_START, size);
        result = gnome_vfs_read (log->mon_file_handle, buffer, newsize-size, &read);    
        log->mon_offset = newsize;
        
        log_add_lines (log, buffer);
        g_free (buffer);
        return TRUE;
	}
	return FALSE;
}

void
log_close (Log *log)
{
   gint i;
   Day *day;
   GList *days;

   g_return_if_fail (log);
   
   /* Close archive logs if there's some */
   for (i = 0; i < log->versions; i++)
       log_close (log->older_logs[i]);

   /* Close file - this should not be needed */
   if (log->mon_file_handle != NULL) {
	   gnome_vfs_close (log->mon_file_handle);
	   log->mon_file_handle = NULL;
   }

   g_object_unref (log->model);

   /* Free all used memory */
   for (i = 1; i < log->total_lines; i++) {
       g_free ((log->lines)[i] -> message);
       
       if (log->days != NULL) {
           g_free ((log->lines)[i] -> hostname);
           g_free ((log->lines)[i] -> process);
       }
       g_free ((log->lines)[i]);           
   }

   if (log->days != NULL) {
       for (days = log->days; days != NULL; days = g_list_next (days)) {
           day = days->data;
           g_date_free (day->date);
           gtk_tree_path_free (day->path);
           g_free (day);
       }
       g_list_free (log->days);
       log->days = NULL;
   }
   
   g_free (log->lstats);
   gtk_tree_path_free (log->current_path);
   log->current_path = NULL;

   g_free (log);
   return;
}
