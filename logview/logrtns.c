
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
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include "logview.h"
#include "logrtns.h"
#include <libgnomevfs/gnome-vfs-mime-utils.h>

/*
 * -------------------
 * Function prototypes 
 * -------------------
 */

static int isSameDay (time_t day1, time_t day2);
static void ReadLogStats (Log * log, gchar **buffer_lines);
static gboolean file_exist (char *filename, gboolean show_error);

extern GList *regexp_db;
extern GList *actions_db;
const char *error_main = N_("One file or more could not be opened");

/*
 * -------------------
 * Module variables 
 * -------------------
 */

const char *
C_monthname[12] =
{ "January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December" };

/* space separated list of locales to check, must be space separated, no
 * newlines */
/* FIXME: This should be checked for at configure time or something to
 * that effect */
char *all_locales = "af_ZA ar_SA bg_BG ca_ES cs_CZ da_DK de_AT de_BE de_CH de_DE de_LU el_GR en_AU en_CA en_DK en_GB en_IE en_US es_ES et_EE eu_ES fi_FI fo_FO fr_BE fr_CA fr_CH fr_FR fr_LU ga_IE gl_ES gv_GB hr_HR hu_HU in_ID is_IS it_CH it_IT iw_IL kl_GL kw_GB lt_LT lv_LV mk_MK nl_BE nl_NL no_NO pl_PL pt_BR pt_PT ro_RO ru_RU.KOI8-R ru_RU ru_UA sk_SK sl_SI sr_YU sv_FI sv_SE th_TH tr_TR uk_UA";

static int
get_month (const char *str)
{
	int i, j;
	static char *monthname[12] = { 0 };
	static GHashTable *locales_monthnames = NULL;
	static char **locales = NULL;

	for (i = 0; i < 12; i++) {
		if (g_strncasecmp (str, C_monthname[i], 3) == 0) {
			return i;
		}
	}

	for (i = 0; i < 12; i++) {
		if (monthname[i] == NULL) {
			struct tm tm = {0};
			char buf[256];

			tm.tm_mday = 1;
			tm.tm_year = 2000 /* bogus */;
			tm.tm_mon = i;

			/* Note: we don't want utf-8 here, we WANT the
			 * current locale! */
			if (strftime (buf, sizeof (buf), "%b", &tm) <= 0) {
				/* eek, just use C locale cuz we're screwed */
				monthname[i] = g_strndup (C_monthname[i], 3);
			} else {
				monthname[i] = g_strdup (buf);
			}
		}

		if (g_ascii_strcasecmp (str, monthname[i]) == 0) {
			return i;
		}
	}

	if (locales == NULL)
		locales = g_strsplit (all_locales, " ", 0);

	if (locales_monthnames == NULL)
		locales_monthnames = g_hash_table_new (g_str_hash, g_str_equal);

	/* Try all known locales */
	/* FIXME :
	 * This part of the function is very slow and should not be called
	 * on every line of a log, we should use it just once */

	for (j = 0; locales != NULL && locales[j] != NULL; j++) {
		for (i = 0; i < 12; i++) {
			char *key = g_strdup_printf ("%s %d", locales[j], i);
			char *name = g_hash_table_lookup (locales_monthnames, key);
			if (name == NULL) {
				char buf[256];
				char *old_locale = g_strdup (setlocale (LC_TIME, NULL));
				
				if (setlocale (LC_TIME, locales[j]) == NULL) {
					strcpy (buf, "");
				} else {
					struct tm tm = {0};

					tm.tm_mday = 1;
					tm.tm_year = 2000 /* bogus */;
					tm.tm_mon = i;


					if (strftime (buf, sizeof (buf), "%b",
						      &tm) <= 0) {
						strcpy (buf, "");
					}
				}

				if (old_locale != NULL) {
					setlocale (LC_TIME, old_locale);
					g_free (old_locale);
				}

				name = g_strdup (buf);
				g_hash_table_insert (locales_monthnames, g_strdup (key), name);
			}
			g_free (key);

			if (name != NULL &&
			    name[0] != '\0' &&
			    g_ascii_strcasecmp (str, name) == 0) {
				return i;
			}
		}
	}

	return 12;
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

/* ----------------------------------------------------------------------
   NAME:          OpenLogFile
   DESCRIPTION:   Open a log file and read several pages.
   ---------------------------------------------------------------------- */

Log *
OpenLogFile (char *filename, gboolean show_error)
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
   if (!isLogFile (filename, TRUE))
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

   buffer_lines = g_strsplit (buffer, "\n", -1);
   g_free (buffer);

   /* count the lines */
   for (i=0; buffer_lines[i+1] != NULL; i++);
   tlog->total_lines = i;

   tlog->lines = g_new (LogLine*, tlog->total_lines);
   for (i=0; buffer_lines[i+1]!=NULL; i++) {
       (tlog->lines)[i] = g_new (LogLine, 1);
       if ((tlog->lines)[i]==NULL) {
           ShowErrMessage (NULL, error_main, _("Not enough memory!\n"));
           return NULL;
       }
   }

	 	 
   for (i=0; buffer_lines[i+1] != NULL; i++) {
	   ParseLine (buffer_lines[i], (tlog->lines)[i], tlog->has_date);
       if ((tlog->lines)[i]->month == -1 && tlog->has_date)
           tlog->has_date = FALSE;
   }

   /* Read log stats */
   ReadLogStats (tlog, buffer_lines);
   g_strfreev (buffer_lines);

   /* initialize date headers hash table */
   tlog->date_headers = g_hash_table_new_full (NULL, NULL, NULL, 
					       (GDestroyNotify) gtk_tree_path_free);
   tlog->first_time = TRUE;

	 /* Check for older versions of the log */
	 tlog->versions = 0;
	 tlog->current_version = 0;
	 tlog->parent_log = NULL;
	 for (i=1; i<5; i++) {
		 gchar *older_name;
		 older_name = g_strdup_printf ("%s.%d", filename, i);
		 tlog->older_logs[i] = OpenLogFile (older_name, FALSE);
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
			   g_snprintf (buff, sizeof (buff), _("%s is too big."));
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

/* ----------------------------------------------------------------------
   NAME:          isLogFile
   DESCRIPTION:   Check that the given file is indeed a logfile and 
   that it is readable.
   ---------------------------------------------------------------------- */
int
isLogFile (char *filename, gboolean show_error)
{
   char buff[1024];
   char **token;
   char *found_space;
   GnomeVFSHandle *handle;
   GnomeVFSResult result;
   GnomeVFSFileSize size;

   /* Read first line and check that it is text */
   result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);
   if (result != GNOME_VFS_OK)
	   return FALSE;

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


gchar **ReadLastPage (Log *log)
{
	GnomeVFSResult result;
	gchar *buffer;
	int size;

	g_return_val_if_fail (log, NULL);

	result = gnome_vfs_read_entire_file (log->name, &size, &buffer);
	
	if (size > 0 && buffer != NULL) {
		gchar **buffer_lines;
		result = gnome_vfs_open (&(log->mon_file_handle), log->name, GNOME_VFS_OPEN_READ);
		log->mon_offset = size;
		buffer_lines = g_strsplit (buffer, "\n", -1);
		g_free (buffer);
		return buffer_lines;
	}
	return NULL;
}

gchar **ReadNewLines (Log *log)
{
	GnomeVFSFileSize bytes_read;
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	gchar *buffer;
	
	g_return_val_if_fail (log, NULL);
	g_return_val_if_fail (log->mon_file_handle, NULL);
	
	result = gnome_vfs_get_file_info_from_handle (log->mon_file_handle, &info, GNOME_VFS_FILE_INFO_DEFAULT);
	buffer = g_malloc (info.size - log->mon_offset);

	result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_START, log->mon_offset);
	result = gnome_vfs_read (log->mon_file_handle, buffer, info.size - log->mon_offset, &bytes_read);
	
	if (result == GNOME_VFS_OK && bytes_read > 0) {
		gchar **buffer_lines;
		log->mon_offset = info.size;
		result = gnome_vfs_tell (log->mon_file_handle, &(log->mon_offset));
		buffer_lines = g_strsplit (buffer, "\n", -1);
		g_free (buffer);
		return buffer_lines;
	}
}

/* ----------------------------------------------------------------------
   NAME:        ParseLine
   DESCRIPTION: Extract date and other info from the line. If any field 
   seems to be missing fill the others with -1 and "".
   ---------------------------------------------------------------------- */

void
ParseLine (char *buff, LogLine *line, gboolean has_date)
{
   char *token;
   char scratch[1024];
   int i;
   int len;

   /* create defaults */
   strncpy (line->message, buff, MAX_WIDTH);   
   len = MIN (MAX_WIDTH-1, strlen (buff));
   line->message[len] = '\0';   
   line->month = -1; line->date = -1;
   line->hour = -1; line->min = -1; line->sec = -1;
   strcpy (line->hostname, "");
   strcpy (line->process, "");

   if (!has_date)
       return;

   /* FIXME : stop using strtok, it's unrecommended. */
   token = strtok (line->message, " ");
   if (token == NULL) return;

   i = get_month (token);
   if (i == 12)
       return;
   line->month = i;	 

   token = strtok (NULL, " ");
   if (token != NULL)
       line->date = (char) atoi (token);
   else
       return;

   token = strtok (NULL, ":");
   if (token != NULL)
       line->hour = (char) atoi (token);
   else
       return;

   token = strtok (NULL, ":");
   if (token != NULL)
      line->min = (char) atoi (token);
   else
       return;

   token = strtok (NULL, " ");
   if (token != NULL)
       line->sec = (char) atoi (token);
   else
       return;

   token = strtok (NULL, " ");
   if (token != NULL) {
      strncpy (line->hostname, token, MAX_HOSTNAME_WIDTH);
      line->hostname[MAX_HOSTNAME_WIDTH-1] = '\0';
   } else
       return;

   token = strtok (NULL, ":\n");
   if (token != NULL)
       strncpy (scratch, token, 254);
   else
       strncpy (scratch, " ", 254);
   scratch[254] = '\0';
   token = strtok (NULL, "\n");

   if (token == NULL)
   {
      strncpy (line->process, "", MAX_PROC_WIDTH);
      i = 0;
      while (scratch[i] == ' ')
	 i++;
      strncpy (line->message, &scratch[i], MAX_WIDTH);
      line->message [MAX_WIDTH-1] = '\0';
   } else
   {
      strncpy (line->process, scratch, MAX_PROC_WIDTH);
      line->process [MAX_PROC_WIDTH-1] = '\0';
      while (*token == ' ')
	 token++;
      strncpy (line->message, token, MAX_WIDTH);
      line->message [MAX_WIDTH-1] = '\0';
   }

}

/* ----------------------------------------------------------------------
   NAME:          ReadLogStats
   DESCRIPTION:   Read the log and get some statistics from it. Read
   all dates which have a log entry to create calendar.
   All dates are given with respect to the 1/1/1970
   and are then corrected to the correct year once we
   reach the end.
   buffer_lines must NOT be freed.
   ---------------------------------------------------------------------- */

static void
ReadLogStats (Log *log, gchar **buffer_lines)
{
   gchar *buffer;
   int offsetyear, lastyear, thisyear;
   int nl, i;
   time_t curdate, newdate, correction;
   DateMark *curmark;
   struct stat filestat;
   struct tm *tmptm, footm;
   GnomeVFSResult result;
   GnomeVFSFileInfo info;

   /* Clear struct.      */
   log->lstats.startdate = 0;
   log->lstats.enddate = 0;
   log->lstats.firstmark = NULL;
   log->lstats.mtime = 0;
   log->lstats.size = 0;

   result = gnome_vfs_get_file_info (log->name, &info, GNOME_VFS_FILE_INFO_DEFAULT);
   log->lstats.mtime = info.mtime;
   log->lstats.size = info.size;

   /* Make sure we have at least a starting date. */
	 if ((log->lines)[0]->month == -1)
		 return;

   nl = 0;
   curdate = -1;
   while ((curdate < 0) && (buffer_lines[nl]!=NULL))
   {
	   curdate = GetDate (buffer_lines[nl]);	  
	   nl++;
   }
   log->lstats.startdate = curdate;

   /* Start building the list */

   offsetyear = 0;
   curmark = malloc (sizeof (DateMark));
   if (curmark == NULL) {
	   ShowErrMessage (NULL, error_main, _("ReadLogStats: out of memory"));
	   exit (0);
   }
   curmark->time = curdate;
   curmark->year = offsetyear;
   curmark->next = NULL;
   curmark->prev = NULL;
   curmark->offset = info.size;
   curmark->ln = nl;
   log->lstats.firstmark = curmark;

   /* Count days appearing in the log */

   i = 0;
   for (i=0; buffer_lines[i]!=NULL; i++) {
	   newdate = GetDate (buffer_lines[i]);

	   if (newdate < 0)
		   continue;
	   if (isSameDay (newdate, curdate) )
		   continue;
	   
	   curmark->next = malloc (sizeof (DateMark));
	   if (curmark->next == NULL) {
		   ShowErrMessage (NULL, error_main, _("ReadLogStats: out of memory"));
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

   curdate = info.mtime;
   tmptm = localtime (&curdate);
   thisyear = tmptm->tm_year;
   lastyear = offsetyear;
   footm.tm_sec = footm.tm_mon = 0;
   footm.tm_hour = footm.tm_min = 0;
   footm.tm_isdst = 0;
   footm.tm_mday = 1;
   curmark = log->lstats.firstmark;

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



/* ----------------------------------------------------------------------
   NAME:          isSameDay
   DESCRIPTION:   Determine if the given times are the same.
   ---------------------------------------------------------------------- */

static int
isSameDay (time_t day1, time_t day2)
{
   struct tm d1, d2, *foo;

   foo = localtime (&day1);
   memcpy (&d1, foo, sizeof (struct tm));

   foo = localtime (&day2);
   memcpy (&d2, foo, sizeof (struct tm));

   if (d1.tm_year != d2.tm_year)
      return FALSE;
   if (d1.tm_mon != d2.tm_mon)
      return FALSE;
   if (d1.tm_mday != d2.tm_mday)
      return FALSE;

   return TRUE;
}


/* ----------------------------------------------------------------------
   NAME:          GetDate
   DESCRIPTION:   Extract the date from the log line.
   ---------------------------------------------------------------------- */

time_t
GetDate (char *line)
{
   struct tm date;
   char *token;
   int i;

   token = strtok (line, " ");
   if (!token)
       return -1;
   
   i = get_month (token);

   if (i == 12)
      return -1;

   date.tm_mon = i;
   date.tm_year = 70;

   token = strtok (NULL, " ");
   if (token != NULL)
      date.tm_mday = atoi (token);
   else
      return -1;

   token = strtok (NULL, ":");
   if (token != NULL)
      date.tm_hour = atoi (token);
   else
      return -1;

   token = strtok (NULL, ":");
   if (token != NULL)
      date.tm_min = atoi (token);
   else
      return -1;

   token = strtok (NULL, " ");
   if (token != NULL)
      date.tm_sec = atoi (token);
   else
      return -1;

   date.tm_isdst = 0;

   return mktime (&date);

}

/* ----------------------------------------------------------------------
   NAME:          CloseLog
   DESCRIPTION:   Close log and free all memory used
   ---------------------------------------------------------------------- */

void
CloseLog (Log * log)
{
   gint i;

   g_return_if_fail (log);

	 /* Close archive logs if there's some */
	 for (i = 0; i < log->versions; i++)
		 CloseLog (log->older_logs[i]);

   /* Close file - this should not be needed */
   if (log->mon_file_handle != NULL) {
	   gnome_vfs_close (log->mon_file_handle);
	   log->mon_file_handle = NULL;
   }

   /* Free all used memory */
   for (i = 0; i < log->total_lines; i++)
      g_free ((log->lines)[i]);

   for (i = 0; log->expand_paths[i]; ++i)
       gtk_tree_path_free (log->expand_paths[i]);

   gtk_tree_path_free (log->current_path);
   g_hash_table_destroy (log->date_headers);

   g_free (log);
   return;

}

/* ----------------------------------------------------------------------
   NAME:	WasModified
   DESCRIPTION:	Returns true if modified flag in log changed. It also
                changes the modified flag.
   ---------------------------------------------------------------------- */

int
WasModified (Log *log)
{
	GnomeVFSResult result;
	GnomeVFSFileInfo info;

	result = gnome_vfs_get_file_info (log->name, 
					  &(info),
					  GNOME_VFS_FILE_INFO_DEFAULT);
	if (info.mtime != log->lstats.mtime) {
		log->lstats.mtime = info.mtime;
		return TRUE;
	} else
		return FALSE;
}
