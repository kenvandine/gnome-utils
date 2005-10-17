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

const char *error_main = N_("One file or more could not be opened");
const char *month[12] =
{N_("January"), N_("February"), N_("March"), N_("April"), N_("May"),
 N_("June"), N_("July"), N_("August"), N_("September"), N_("October"),
 N_("November"), N_("December")};

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
   char buff[1024];
   char **token;
   char *found_space;
   GnomeVFSHandle *handle;
   GnomeVFSResult result;
   GnomeVFSFileSize size;
   GnomeVFSFileInfo info;

   /* Read first line and check that it is text */
   result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);
   if (result != GNOME_VFS_OK) {
	   return FALSE;
   }

   result = gnome_vfs_get_file_info_from_handle (handle, &info, GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
   if (result != GNOME_VFS_OK || info.type != GNOME_VFS_FILE_TYPE_REGULAR) {
	   gnome_vfs_close (handle);
	   return FALSE;
   }

   result = gnome_vfs_read (handle, buff, sizeof(buff), &size);
   gnome_vfs_close (handle);
   if (result != GNOME_VFS_OK)
	   return FALSE;
   
   found_space = g_strstr_len (buff, 1024, " ");
   if (found_space == NULL) {
	   if (show_error) {
		   g_snprintf (buff, sizeof (buff), _("%s not a log file."), filename);
		   ShowErrMessage (NULL, error_main, buff);
	   }
	   return FALSE;
   }
   
   /* It looks like a log file... */
   return TRUE;
}

/* log line manipulations */

char *
logline_get_date_string (LogLine *line)
{
   char buf[1024];
   char *utf8;
   GDate *date;

   if (line->month >= 0 && line->month < 12) {

       date = g_date_new_dmy (line->date, line->month + 1, 2000);

       if (!g_date_valid (date)) {
           utf8 = g_strdup(_("Invalid date"));
           return utf8;
       }

       /* Translators: Make sure this is only Month and Day format, year
        * will be bogus here */
       if (g_date_strftime (buf, sizeof (buf), _("%B %e"), date) == 0) {
           /* If we fail just use the US format */
           utf8 = g_strdup_printf ("%s %d", _(month[(int) line->month]), 
                                   line->date);
       } else {
           utf8 = LocaleToUTF8 (buf);
       }
   } else {
       utf8 = g_strdup_printf ("?%d? %d", (int) line->month, line->date);
   }
   
   g_date_free (date);

   return utf8;
}

static LogLine *
logline_new_from_string (gchar *buff, gboolean has_date)
{
   char *token;
   char scratch[1024];
   int i;
   int len;
   char message[MAX_WIDTH];
   char hostname[MAX_HOSTNAME_WIDTH];
   char process[MAX_PROC_WIDTH];
   LogLine *line;
   
   line = g_new (LogLine, 1);
   if (line == NULL) {
       ShowErrMessage (NULL, error_main, _("Not enough memory!\n"));
       return NULL;
   }

   /* create defaults */
   line->month = -1; line->date = -1;
   line->hour = -1; line->min = -1; line->sec = -1;
   if (buff[0] == 0) {
       line->message=NULL;       
       return (line);
   }

   strncpy (message, buff, MAX_WIDTH);
   len = MIN (MAX_WIDTH-1, strlen (buff));
   message[len] = '\0';  

   if (!has_date) {
       line->message = g_strdup (message);
       return (line);
   }

   strcpy (hostname, "");
   strcpy (process, "");

   /* FIXME : stop using strtok, it's unrecommended. */
   token = strtok (message, " ");
   if (token == NULL) return (line);

   i = string_get_month (token);
   if (i == 12)
       return (line);
   line->month = i;	 

   token = strtok (NULL, " ");
   if (token != NULL)
       line->date = (char) atoi (token);
   else
       return (line);

   token = strtok (NULL, ":");
   if (token != NULL)
       line->hour = (char) atoi (token);
   else
       return (line);

   token = strtok (NULL, ":");
   if (token != NULL)
      line->min = (char) atoi (token);
   else
       return (line);

   token = strtok (NULL, " ");
   if (token != NULL)
       line->sec = (char) atoi (token);
   else
       return (line);

   token = strtok (NULL, " ");
   if (token != NULL) {
      strncpy (hostname, token, MAX_HOSTNAME_WIDTH);
      hostname[MAX_HOSTNAME_WIDTH-1] = '\0';
   } else
       return (line);

   token = strtok (NULL, ":\n");
   if (token != NULL)
       strncpy (scratch, token, 254);
   else
       strncpy (scratch, " ", 254);
   scratch[254] = '\0';
   token = strtok (NULL, "\n");

   if (token == NULL)
   {
      strncpy (process, "", MAX_PROC_WIDTH);
      i = 0;
      while (scratch[i] == ' ')
	 i++;
      strncpy (message, &scratch[i], MAX_WIDTH);
      message [MAX_WIDTH-1] = '\0';
   } else
   {
      strncpy (process, scratch, MAX_PROC_WIDTH);
      process [MAX_PROC_WIDTH-1] = '\0';
      while (*token == ' ')
	 token++;
      strncpy (message, token, MAX_WIDTH);
      message [MAX_WIDTH-1] = '\0';
   }

   line->message = g_strdup (message);
   line->hostname = g_strdup (hostname);
   line->process = g_strdup (process);
   return (line);
}

/* log functions */

/* ----------------------------------------------------------------------
   NAME:          log_read_stats
   DESCRIPTION:   Read the log and get some statistics from it. Read
   all dates which have a log entry to create calendar.
   All dates are given with respect to the 1/1/1970
   and are then corrected to the correct year once we
   reach the end.
   buffer_lines must NOT be freed.
   ---------------------------------------------------------------------- */

static void
log_read_stats (Log *log, gchar **buffer_lines)
{
   gchar *buffer;
   int offsetyear, lastyear, thisyear;
   int nl, i;
   time_t curdate, newdate, correction;
   DateMark *curmark;
   struct stat filestat;
   struct tm *tmptm, footm;
   GnomeVFSResult result;
   GnomeVFSFileInfo *info;

   log->lstats.startdate = 0;
   log->lstats.enddate = 0;
   log->lstats.firstmark = NULL;
   log->lstats.mtime = 0;
   log->lstats.size = 0;

   info = gnome_vfs_file_info_new ();
   result = gnome_vfs_get_file_info (log->name, info, GNOME_VFS_FILE_INFO_DEFAULT);
   log->lstats.mtime = info->mtime;
   log->lstats.size = info->size;

   /* Make sure we have at least a starting date. */
	 if ((log->lines)[0]->month == -1)
		 return;

   nl = 0;
   curdate = -1;
   while ((curdate < 0) && (buffer_lines[nl]!=NULL))
   {
	   curdate = string_get_date (buffer_lines[nl]);	  
	   nl++;
   }
   log->lstats.startdate = curdate;

   /* Start building the list */

   offsetyear = 0;
   curmark = malloc (sizeof (DateMark));
   if (curmark == NULL) {
	   ShowErrMessage (NULL, error_main, _("Out of memory"));
	   exit (0);
   }
   curmark->time = curdate;
   curmark->year = offsetyear;
   curmark->next = NULL;
   curmark->prev = NULL;
   curmark->offset = info->size;
   curmark->ln = nl;
   log->lstats.firstmark = curmark;

   /* Count days appearing in the log */

   i = 0;
   for (i=0; buffer_lines[i]!=NULL; i++) {
	   newdate = string_get_date (buffer_lines[i]);

	   if (newdate < 0)
		   continue;
	   if (days_are_equal (newdate, curdate))
		   continue;
	   
	   curmark->next = malloc (sizeof (DateMark));
	   if (curmark->next == NULL) {
		   ShowErrMessage (NULL, error_main, _("Out of memory"));
		   exit (0);
	   }

	   curmark->next->prev = curmark;
	   curmark = curmark->next;
	   curmark->time = newdate;
	   if (newdate < curdate)	/* The newdate is next year */
		   offsetyear++;
	   curmark->year = offsetyear;
	   curmark->next = NULL;
	   curmark->ln = nl;
	   curdate = newdate;
   }
   log->lstats.numlines = i;
   log->lstats.enddate = curdate;

   /* Correct years now. We assume that the last date on the log
      is the date last accessed */

   curdate = info->mtime;
   tmptm = localtime (&curdate);
   thisyear = tmptm->tm_year;
   lastyear = offsetyear;
   footm.tm_sec = footm.tm_mon = 0;
   footm.tm_hour = footm.tm_min = 0;
   footm.tm_isdst = 0;
   footm.tm_mday = 1;
   curmark = log->lstats.firstmark;

   gnome_vfs_file_info_unref (info);

   while (curmark != NULL) {
	   footm.tm_year = (curmark->year - lastyear) + thisyear;
	   correction = mktime (&footm);
	   if (IsLeapYear (curmark->year - lastyear + thisyear))
		   curmark->time += 24 * 60 * 60;		/*  Add one day */
	   
#if defined(__NetBSD__) || defined(__FreeBSD__)
	   curmark->time += correction - tmptm->tm_gmtoff;
#else
	   curmark->time += correction - timezone;
#endif
	   
	   memcpy (&curmark->fulldate, localtime (&curmark->time), sizeof (struct tm));
	   
	   if (curmark->next != NULL)
		   curmark = curmark->next;
	   else {
		   log->lstats.lastmark = curmark;
		   break;
	   }
   }
   
   log->curmark = log->lstats.lastmark;

   /* Correct start and end year in lstats. */
   footm.tm_year = thisyear;
   correction = mktime (&footm);
   if (IsLeapYear (thisyear))
	   log->lstats.enddate += 24 * 60 * 60;	/*  Add one day */
   
#if defined(__NetBSD__) || defined(__FreeBSD__)
   log->lstats.enddate += correction - tmptm->tm_gmtoff;
#else
   log->lstats.enddate += correction - timezone;
#endif
   
   footm.tm_year = thisyear - lastyear;
   correction = mktime (&footm);
   if (IsLeapYear (thisyear - lastyear))
	   log->lstats.startdate += 24 * 60 * 60;	/*  Add one day */

#if defined(__NetBSD__) || defined(__FreeBSD__)
   log->lstats.startdate += correction - tmptm->tm_gmtoff;
#else
   log->lstats.startdate += correction - timezone;
#endif

   return;
}

Log *
log_open (char *filename, gboolean show_error)
{
   Log *tlog;
   LogLine *line;
   char *buffer;
   char **buffer_lines;
   char *display_name=NULL;
   int i;
   GError *error;
   GnomeVFSResult result;
   int size;
   
   if (file_exist (filename, show_error) == FALSE)
	   return NULL;

   if (file_is_zipped (filename)) {
	   display_name = filename;
	   filename = g_strdup_printf ("%s#gzip:", display_name);
   }   

   /* Check that the file is readable and is a logfile */
   if (!file_is_log (filename, TRUE))
       return NULL;

   result = gnome_vfs_read_entire_file (filename, &size, &buffer);
   if (result != GNOME_VFS_OK) {
	   ShowErrMessage (NULL, error_main, _("Unable to open logfile!\n"));
	   return NULL;
   }

   /* Alloc memory for log structure */
   tlog = g_new0 (Log, 1);
   if (tlog == NULL) {
	   ShowErrMessage (NULL, error_main, _("Not enough memory!\n"));
	   return NULL;
   }
   tlog->name = g_strdup (filename);
   tlog->display_name = display_name;
   tlog->has_date = TRUE;
   if (display_name)
	   g_free (filename);

   buffer_lines = g_strsplit_set (buffer, "\n\r", -1);
   g_free (buffer);

   /* count the lines */
   for (i=0; buffer_lines[i+1] != NULL; i++);
   tlog->total_lines = i;
   tlog->lines = g_new (LogLine*, tlog->total_lines);
   tlog->displayed_lines = 0;
   
   for (i=0; i < tlog->total_lines; i++) {
	   (tlog->lines)[i] = logline_new_from_string (buffer_lines[i], tlog->has_date);
       if ((tlog->lines)[i]->month == -1 && tlog->has_date)
           tlog->has_date = FALSE;
   }

   log_read_stats (tlog, buffer_lines);
   g_strfreev (buffer_lines);

   /* initialize date headers hash table */
   tlog->date_headers = g_hash_table_new_full (NULL, NULL, NULL, 
                                               (GDestroyNotify) g_free);
   tlog->first_time = TRUE;
   
   /* Check for older versions of the log */
   tlog->versions = 0;
   tlog->current_version = 0;
   tlog->parent_log = NULL;
   tlog->mon_offset = size;
   for (i=1; i<5; i++) {
       gchar *older_name;
       older_name = g_strdup_printf ("%s.%d", filename, i);
       tlog->older_logs[i] = log_open (older_name, FALSE);
       g_free (older_name);
       if (tlog->older_logs[i] != NULL) {
           tlog->older_logs[i]->parent_log = tlog;
           tlog->older_logs[i]->current_version = i;
           tlog->versions++;
       }
       else
           break;
   }

   return tlog;
}

static Log*
log_add_lines (Log *log, gchar *buffer)
{
  char **buffer_lines;
  int i, j, new_total_lines;
  LogLine **new_lines;

  g_return_if_fail (log != NULL);
  g_return_if_fail (buffer != NULL);

  buffer_lines = g_strsplit (buffer, "\n", -1);
  
  for (i=0; buffer_lines[i+1] != NULL; i++);
  new_total_lines = log->total_lines + i;
  new_lines = g_new (LogLine *, new_total_lines);
  
  for (i=0; i< log->total_lines; i++)
    new_lines [i] = (log->lines)[i];
  
  for (i=0; buffer_lines[i+1]!=NULL; i++) {
    j = log->total_lines + i;
    new_lines [j] = logline_new_from_string (buffer_lines[i], log->has_date);
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
       if (log->has_date) {
           g_free ((log->lines)[i] -> hostname);
           g_free ((log->lines)[i] -> process);
       }
       g_free ((log->lines)[i]);           
   }
   
   if (log->has_date) {
       for (i = 0; i < 32; i++)
           if (log->expand_paths[i])
               gtk_tree_path_free (log->expand_paths[i]);
       g_hash_table_destroy (log->date_headers);
   }

   gtk_tree_path_free (log->current_path);
   log->current_path = NULL;

   g_free (log);
   return;
}
