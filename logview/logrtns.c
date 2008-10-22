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
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>

#include "logview.h"
#include "logrtns.h"
#include "misc.h"
#include "math.h"

char *error_main = N_("One file or more could not be opened");

static LogStats *log_stats_new (char *filename, gboolean show_error);

/* File checking */

static gboolean
file_exist (char *filename, gboolean show_error)
{
   GnomeVFSHandle *handle;
   GnomeVFSResult result;
   char *secondary = NULL;

   if (filename == NULL)
       return FALSE;

   result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);
   if (result != GNOME_VFS_OK) {
	   if (show_error) {
		   switch (result) {
		   case GNOME_VFS_ERROR_ACCESS_DENIED:
		   case GNOME_VFS_ERROR_NOT_PERMITTED:
			   secondary = g_strdup_printf (_("%s is not user readable. "
					 "Either run the program as root or ask the sysadmin to "
					 "change the permissions on the file.\n"), filename);
			   break;
		   case GNOME_VFS_ERROR_TOO_BIG:
			   secondary = g_strdup_printf (_("%s is too big."), filename);
			   break;
		   default:
			   secondary = g_strdup_printf (_("%s could not be opened."), filename);
               break;
		   }
		   error_dialog_show (NULL, error_main, secondary);
           g_free (secondary);
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

    if (filename == NULL)
        return FALSE;

	mime_type = gnome_vfs_get_mime_type (filename);
    if (mime_type == NULL)
        return FALSE;

	if (strcmp (mime_type, "application/x-gzip")==0 ||
	    strcmp (mime_type, "application/x-zip")==0 ||
	    strcmp (mime_type, "application/zip")==0) {
        g_free (mime_type);
		return TRUE;
    } else {
        g_free (mime_type);
		return FALSE;
    }
}

gboolean
file_is_log (char *filename, gboolean show_error)
{
    LogStats *stats;

    if (filename == NULL)
        return FALSE;

    stats = log_stats_new (filename, show_error);
    if (stats==NULL)
        return FALSE;
    else {
        g_free (stats);
        return TRUE;
    }
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

    if (line == NULL || line[0] == 0)
        return NULL;

    split = g_strsplit (line, " ", 4);
    if (split == NULL)
        return NULL;

    while ((day == NULL || month == NULL) && split[i]!=NULL && i<4) {
        if (g_str_equal (split[i], "")) {
            i++;
            continue;
        }

        if (month == NULL) {
            month = split[i++];
            /* If the first field begins by a number, the date
               is given in yyyy-mm-dd format */
            if (!g_ascii_isalpha (month[0]))
                break;
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
GSList *
log_read_dates (gchar **buffer_lines, time_t current)
{
   int offsetyear = 0, current_year;
   GSList *days = NULL, *days_copy;
   GDate *date, *newdate;
   struct tm *tmptm;
   gchar *date_string;
   Day *day;
   gboolean done = FALSE;
   int i, n, rangemin, rangemax;

   if (buffer_lines == NULL)
       return NULL;

   n = g_strv_length (buffer_lines);

   tmptm = localtime (&current);
   current_year = tmptm->tm_year + 1900;

   for (i=0; buffer_lines[i]==NULL; i++);

   /* Start building the list */
   /* Scanning each line to see if the date changed is too slow, so we proceed
      in a recursive fashion */

   date = string_get_date (buffer_lines[i]);
   if ((date==NULL)|| !g_date_valid (date))
       return NULL;

   g_date_set_year (date, current_year);
   day = g_new (Day, 1);
   days = g_slist_append (days, day);

   day->date = date;
   day->first_line = i;
   day->last_line = -1;
   date_string = string_get_date_string (buffer_lines[i]);

   rangemin = 0;
   rangemax = n-1;

   while (!done) {
       
       i = n-1;
       while (day->last_line < 0) {

           if (g_str_has_prefix (buffer_lines[i], date_string)) {
               if (i == (n-1)) {
                   day->last_line = i;
                   done = TRUE;
                   break;
               } else {
                   if (!g_str_has_prefix (buffer_lines[i+1], date_string)) {
                       day->last_line = i;
                       break;
                   } else {
                       rangemin = i;
                       i = floor ( ((float) i + (float) rangemax)/2.);
                   }
               }
           } else {
               rangemax = i;
               i = floor (((float) rangemin + (float) i)/2.);               
           }

       }
       
       g_free (date_string);

       if (!done) {
         /* We need to find the first line now that has a date
            Logs can have some messages without dates ... */
         newdate = NULL;
         while (newdate == NULL && !done) {
           i++;
           date_string = string_get_date_string (buffer_lines[i]);
           if (date_string == NULL)
             if (i==n-1) {
               done = TRUE;
               break;
             } else
               continue;
           newdate = string_get_date (buffer_lines[i]);
           
           if (newdate == NULL && i==n-1)
             done = TRUE;
         }

         day->last_line = i-1;

         /* Append a day to the list */	
         if (newdate) {
           g_date_set_year (newdate, current_year + offsetyear);	
           if (g_date_compare (newdate, date) < 1) {
             offsetyear++; /* newdate is next year */
             g_date_add_years (newdate, 1);
           }
           
           date = newdate;
           day = g_new (Day, 1);
           days = g_slist_append (days, day);
           
           day->date = date;
           day->first_line = i;
           day->last_line = -1;
           rangemin = i;
           rangemax = n;
         }
       }
   }

   /* Correct years now. We assume that the last date on the log
      is the date last accessed */

   for (days_copy = days; days_copy != NULL; days_copy = g_slist_next (days_copy)) {       
       day = days_copy -> data;
       g_date_subtract_years (day->date, offsetyear);
   }
   
   /* Sort the days in chronological order */
   days = g_slist_sort (days, days_compare);

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
   LogStats *stats;
   char buff[1024];
   char *found_space;

   if (filename == NULL)
       return NULL;

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
       gnome_vfs_file_info_unref (info);
	   return NULL;
   }
   
   stats = g_new (LogStats, 1);   
   stats->file_time = info->mtime;
   stats->file_size = info->size;
   gnome_vfs_file_info_unref (info);

   return (stats);
}

Log *
log_open (char *filename, gboolean show_error)
{
   char *buffer, *zipped_name= NULL, *error_message;
   GnomeVFSResult result;
   LogStats *stats;
   gboolean opened = TRUE;
   int i, size;
   GList *days;
   Log *log;
   
   stats = log_stats_new (filename, show_error);
   if (stats == NULL) {
       if (file_is_zipped (filename)) {    
           zipped_name = g_strdup_printf ("%s#gzip:", filename);
           stats = log_stats_new (filename, show_error);
           if (stats == NULL) {
               opened = FALSE;
           }
       } else
           opened = FALSE;
   }
   
   if (opened == FALSE) {
       error_message = g_strdup_printf (_("%s is not a log file."), filename);
       goto error;
   }

   log = g_new0 (Log, 1);   
   if (log == NULL) {
       error_message = g_strdup (_("Not enough memory."));
       goto error;
   }

   if (!zipped_name) {
       log->name = g_strdup (filename);
   } else {
       log->name = zipped_name;
       log->display_name = g_strdup (filename);
   }

   result = gnome_vfs_read_entire_file (log->name, &size, &buffer);
   buffer[size-1] = 0;
   if (result != GNOME_VFS_OK) {
       error_message = g_strdup_printf (_("%s cannot be opened."), log->name);
       goto error;
   }

   if (g_get_charset (NULL) == FALSE) {
       char *buffer2;
       buffer2 = locale_to_utf8 (buffer);
       g_free (buffer);
       buffer = buffer2;
   }

   log->lines = g_strsplit (buffer, "\n", -1);
   g_free (buffer);   

   log->total_lines = g_strv_length (log->lines);
   log->displayed_lines = log->total_lines;
   log->first_time = TRUE;
   log->stats = stats;
   log->model = NULL;
   log->filter = NULL;
   log->mon_offset = 0;
   log->bold_rows_list = NULL;

   /* A log without dates will return NULL */
   log->days = log_read_dates (log->lines, log->stats->file_time);

   /* Check for older versions of the log */
   log->versions = 0;
   log->current_version = 0;
   log->parent_log = NULL;
   for (i=1; i<5; i++) {
       gchar *older_name;
       older_name = g_strdup_printf ("%s.%d", log->name, i);
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

error:
   if (error_message && show_error) {
       error_dialog_show (NULL, error_main, error_message);
   }
   
   g_free (error_message);
   
   return NULL;
}

static void
log_add_lines (Log *log, gchar *buffer)
{
  char *old_buffer, *new_buffer;

  g_assert (log != NULL);
  g_assert (buffer != NULL);

  old_buffer = g_strjoinv ("\n", log->lines);
  new_buffer = g_strconcat (old_buffer, "\n", buffer, NULL);
  g_free (old_buffer);
  
  g_strfreev (log->lines);
  log->lines = g_strsplit (new_buffer, "\n", -1);
  g_free (new_buffer);

  log->total_lines = g_strv_length (log->lines);
}

void log_stats_reload (Log *log)
{
  g_free (log->stats);
  log->stats = log_stats_new (log->name, TRUE);
}

/* log_read_new_lines */

gboolean 
log_read_new_lines (Log *log)
{
    GnomeVFSResult result;
    gchar *buffer;
    GnomeVFSFileSize newfilesize, read;
    guint64 size, newsize;
    
    g_return_val_if_fail (log!=NULL, FALSE);
    
    result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_END, 0L);
    result = gnome_vfs_tell (log->mon_file_handle, &newfilesize);
    size = log->mon_offset;
    newsize = newfilesize;

    if (newsize > size) {
      buffer = g_malloc (newsize - size);
      if (!buffer)
	return FALSE;

      result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_START, size);
      if (result != GNOME_VFS_OK)
	return FALSE;

      result = gnome_vfs_read (log->mon_file_handle, buffer, newsize-size, &read);
      if (result != GNOME_VFS_OK)
	return FALSE;

      buffer [newsize-size-1] = 0;
      log->mon_offset = newsize;      
      log_add_lines (log, buffer);
      g_free (buffer);
      
      log_stats_reload (log);
      return TRUE;
    }
    return FALSE;
}

/* 
   log_unbold is called by a g_timeout
   set in loglist_bold_log 
*/

gboolean
log_unbold (gpointer data)
{
    LogviewWindow *logview;
    LogList *list;
    Log *log = data;

    g_return_val_if_fail (log != NULL, FALSE);

    logview = log->window;

    /* If the log to unbold is not displayed, still wait */
    if (logview_get_active_log (logview) != log)
        return TRUE;

    list = logview_get_loglist (logview);
    loglist_unbold_log (list, log);
    return FALSE;
}

void
log_close (Log *log)
{
   gint i;
   Day *day;
   GSList *days;

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
   g_strfreev (log->lines);

   if (log->days != NULL) {
       for (days = log->days; days != NULL; days = g_slist_next (days)) {
           day = days->data;
           g_date_free (day->date);
           gtk_tree_path_free (day->path);
           g_free (day);
       }
       g_slist_free (log->days);
       log->days = NULL;
   }
   
   g_free (log->stats);

   if (log->selected_range.first)
     gtk_tree_path_free (log->selected_range.first);
   if (log->selected_range.last)
     gtk_tree_path_free (log->selected_range.last);
   if (log->visible_first)
     gtk_tree_path_free (log->visible_first);

   g_free (log);
   return;
}

gchar *
log_extract_filename (Log *log)
{
  GnomeVFSURI *uri;
  gchar *filename;

  uri = gnome_vfs_uri_new (log->name);
  filename = gnome_vfs_uri_extract_short_name (uri);
  gnome_vfs_uri_unref (uri);

  return filename;
}

gchar *
log_extract_dirname (Log *log)
{
  GnomeVFSURI *uri;
  gchar *dirname;

  uri = gnome_vfs_uri_new (log->name);
  dirname = gnome_vfs_uri_extract_dirname (uri);
  gnome_vfs_uri_unref (uri);

  return dirname;
}
