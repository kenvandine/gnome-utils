
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

#include <config.h>
#ifdef __CYGWIN__
#define timezonevar
#endif
#include <gnome.h>
#include <locale.h>
#include <errno.h>
#include "logview.h"
#include "logrtns.h"
#include <libgnomevfs/gnome-vfs.h>

/*
 * -------------------
 * Function prototypes 
 * -------------------
 */

int match_line_in_db (LogLine *line, GList *db);
void UpdateLastPage (Log *log);

extern GList *regexp_db;
extern GList *actions_db;

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

GnomeVFSFileSize GetLogSize (Log *log)
{
	GnomeVFSFileInfo info;
	GnomeVFSResult result;

	result = gnome_vfs_get_file_info (log->name, &info, GNOME_VFS_FILE_INFO_DEFAULT);
	return (info.size);
}
	

/* ----------------------------------------------------------------------
   NAME:          OpenLogFile
   DESCRIPTION:   Open a log file and read several pages.
   ---------------------------------------------------------------------- */

Log *
OpenLogFile (char *filename)
{
   Log *tlog;
   LogLine *line;
   char *buffer;
   char **buffer_lines;
   int i;
   GnomeVFSResult result;
   GnomeVFSFileSize size, read_bytes;

   /* Check that the file exists and is readable and is a logfile */
   if (!isLogFile (filename))
	   return NULL;

   /* Alloc memory for log structure */
   tlog = g_new0 (Log, 1);
   if (tlog == NULL) {
	   ShowErrMessage (_("Not enough memory!\n"));
	   return NULL;
   }

   /* Open log files */
   strncpy (tlog->name, filename, sizeof (tlog->name)-1);
   tlog->name[sizeof (tlog->name) - 1] = '\0';

   result = gnome_vfs_open (&(tlog->handle), filename, GNOME_VFS_OPEN_READ);
   if (result != GNOME_VFS_OK) {
	   ShowErrMessage (_("Unable to open logfile!\n"));
	   return NULL;
   }

   size = GetLogSize (tlog);
   buffer = g_malloc (size);
   if (gnome_vfs_read (tlog->handle, buffer, size, &read_bytes) != GNOME_VFS_OK)
	   return NULL;
   buffer_lines = g_strsplit (buffer, "\n", -1);

   /* count the lines */
   for (i=0; buffer_lines[i+1] != NULL; i++);
   tlog->total_lines = i;

   tlog->lines = g_malloc (sizeof (*(tlog->lines)) * tlog->total_lines);
   for (i=0; buffer_lines[i+1] != NULL; i++) {	   
	   line = g_malloc (sizeof(**(tlog->lines)));
	   if (!line) {
		   ShowErrMessage ("Unable to malloc for lines[i]\n");
		   return NULL;
	   }

	   ParseLine (buffer_lines[i], line);
	   (tlog->lines)[i] = line;
   }   

   /* Read log stats */
   ReadLogStats (tlog, buffer_lines);
   g_strfreev (buffer_lines);

   gnome_vfs_seek (tlog->handle, 0L, GNOME_VFS_SEEK_END);
   result = gnome_vfs_tell (tlog->handle, &(tlog->filesize));

   if (!tlog->filesize)
	   printf ("Empty file! \n");

   /* initialize date headers hash table */
   tlog->date_headers = g_hash_table_new_full (NULL, NULL, NULL, 
					       (GDestroyNotify) gtk_tree_path_free);
   tlog->first_time = TRUE;
   return tlog;

}

/* ----------------------------------------------------------------------
   NAME:          isLogFile
   DESCRIPTION:   Check that the given file is indeed a logfile and 
   that it is readable.
   ---------------------------------------------------------------------- */
int
isLogFile (char *filename)
{
   char buff[1024];
   char **token;
   char *found_space;
   int i;
   GnomeVFSFileInfo info;
   GnomeVFSHandle *handle;
   GnomeVFSResult result;
   GnomeVFSFileSize size;

   result = gnome_vfs_get_file_info (filename, &info, GNOME_VFS_FILE_INFO_DEFAULT);

   /* Check that its a regular file       */
   if (info.type != GNOME_VFS_FILE_TYPE_REGULAR) {
      g_snprintf (buff, sizeof (buff),
		  _("%s is not a regular file."), filename);
      ShowErrMessage (buff);
      return FALSE;
   }

   /* File unreadable                     */
   if (!(info.permissions & GNOME_VFS_PERM_USER_READ)) {
      g_snprintf (buff, sizeof (buff),
		  _("%s is not user readable. "
      "Either run the program as root or ask the sysadmin to "
      "change the permissions on the file."), filename);
      ShowErrMessage (buff);
      return FALSE;
   }

   /* Read first line and check that it has the format
    * of a log file: Date ...    */
   result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);

   if (result != GNOME_VFS_OK)
   {
      /* FIXME: this error message is really quite poor
       * we should state why the open failed
       * ie. file too large etc etc..
       */
      g_snprintf (buff, sizeof (buff),
		  _("%s could not be opened."), filename);
      ShowErrMessage (buff);
      return FALSE;
   }

   switch (gnome_vfs_read (handle, buff, sizeof(buff), &size)) {
   case GNOME_VFS_ERROR_EOF :
	   gnome_vfs_close (handle);
	   return TRUE;
	   break;
   case GNOME_VFS_OK :
	   gnome_vfs_close (handle);
	   break;
   default:
	   gnome_vfs_close (handle);
	   return FALSE;
	   break;
   }

   found_space = g_strstr_len (buff, 1024, " ");
   if (found_space == NULL) {
	   g_snprintf (buff, sizeof (buff), _("%s not a log file."), filename);
	   ShowErrMessage (buff);
	   return FALSE;
   }
   
   token = g_strsplit (buff, " ", 1);
   i = get_month (token[0]);
   g_strfreev (token);
   if (i == 12) {
	   g_snprintf (buff, sizeof (buff), _("%s not a log file."), filename);
	   ShowErrMessage (buff);
	   return FALSE;
   }

   /* It looks like a log file... */
   return TRUE;
}


gchar **ReadLastPage (Log *log)
{
	GnomeVFSFileSize size, bytes_read;
	GnomeVFSResult result;
	gchar *buffer;

	g_return_val_if_fail (log, NULL);

	size = GetLogSize (log);
	g_return_val_if_fail (size > 0, NULL);

	buffer = g_malloc (size);

	result = gnome_vfs_seek (log->handle, GNOME_VFS_SEEK_START, 0L);
	result = gnome_vfs_read (log->handle, buffer, size, &bytes_read);

	log->filesize = size;
	result = gnome_vfs_tell (log->handle, &(log->mon_offset));

	if (bytes_read > 0) {
		gchar **buffer_lines;
		buffer_lines = g_strsplit (buffer, "\n", -1);
		g_free (buffer);
		return buffer_lines;
	}
	return NULL;
}

gchar **ReadNewLines (Log *log)
{
	GnomeVFSFileSize newsize, size, bytes_read;
	GnomeVFSResult result;
	gchar *buffer, **buffer_lines;
	
	g_return_val_if_fail (log, NULL);
	g_return_val_if_fail (log->mon_offset > 0, NULL);
	
	newsize = GetLogSize (log);

	buffer = g_malloc (newsize - log->filesize);

	g_return_val_if_fail (log, NULL);
	
	result = gnome_vfs_seek (log->handle, GNOME_VFS_SEEK_START, log->mon_offset);
	result = gnome_vfs_read (log->handle, buffer, newsize - log->filesize, &bytes_read);

	log->filesize = newsize;
	result = gnome_vfs_tell (log->handle, &(log->mon_offset));
	
	if (bytes_read > 0) {
		gchar **buffer_lines;
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
ParseLine (char *buff, LogLine * line)
{
   char *token;
   char scratch[1024];
   int i;

   /* just copy as a whole line to be the default */
   strncpy (line->message, buff, MAX_WIDTH);
   line->message[MAX_WIDTH-1] = '\0';

   token = strtok (buff, " ");
   if (token == NULL) return;
   /* This is not a good assumption I don't think, especially
    * if log is internationalized
    * -George
   if (strlen (token) != 3)
   {
      strncpy (line->message, buff, MAX_WIDTH);
      line->month = -1;
      line->date = -1;
      line->hour = -1;
      line->min = -1;
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "");
      return;
   }
   */
   i = get_month (token);

   if (i == 12)
   {
      line->month = -1;
      line->date = -1;
      line->hour = -1;
      line->min = -1;
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "");
      return;
   }
   line->month = i;

   token = strtok (NULL, " ");
   if (token != NULL)
      line->date = (char) atoi (token);
   else
   {
      line->date = -1;
      line->hour = -1;
      line->min = -1;
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "");
      strcpy (line->message, "");
      return;
   }

   token = strtok (NULL, ":");
   if (token != NULL)
      line->hour = (char) atoi (token);
   else
   {
      line->hour = -1;
      line->min = -1;
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "");
      strcpy (line->message, "");
      return;
   }

   token = strtok (NULL, ":");
   if (token != NULL)
      line->min = (char) atoi (token);
   else
   {
      line->min = -1;
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "-");
      strcpy (line->message, "");
      return;
   }

   token = strtok (NULL, " ");
   if (token != NULL)
      line->sec = (char) atoi (token);
   else
   {
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "-");
      strcpy (line->message, "");
      return;
   }

   token = strtok (NULL, " ");
   if (token != NULL) {
      strncpy (line->hostname, token, MAX_HOSTNAME_WIDTH);
      line->hostname[MAX_HOSTNAME_WIDTH-1] = '\0';
   } else {
      line->sec = -1;
      strcpy (line->hostname, "");
      strcpy (line->process, "-");
      strcpy (line->message, "");
      return;
   }

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

   /* Search current data base for string and attach descritpion */
   /* -------------------------
   if (regexp_db == NULL)
     return;
   if (match_line_in_db (line, regexp_db))
     {
       sprintf (scratch, _("Expression /%s/ matched."),
		 line->description->regexp);
       line->description->description = g_strdup(scratch);
     }
     ---------------------------- */
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

void
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
   GnomeVFSFileOffset pos;
   GnomeVFSFileSize size;

   /* Clear struct.      */
   log->lstats.startdate = 0;
   log->lstats.enddate = 0;
   log->lstats.firstmark = NULL;
   log->lstats.mtime = 0;
   log->lstats.size = 0;

   if (log->handle == NULL)
      return;

   result = gnome_vfs_get_file_info_from_handle (log->handle, &info, GNOME_VFS_FILE_INFO_DEFAULT);
   log->lstats.mtime = info.mtime;
   log->lstats.size = info.size;

   /* Make sure we have at least a starting date. */
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
      ShowErrMessage (_("ReadLogStats: out of memory"));
      exit (0);
   }
   log->lstats.firstmark = curmark;
   curmark->time = curdate;
   curmark->year = offsetyear;
   curmark->next = NULL;
   curmark->prev = NULL;
   curmark->offset = info.size;
   curmark->ln = nl;

   /* Count days appearing in the log */

   i = 0;
   for (i=0; buffer_lines[i]!=NULL; i++) {
	   newdate = GetDate (buffer_lines[nl]);

	   if (newdate < 0)
		   continue;
	   if (isSameDay (newdate, curdate) )
		   continue;
	   
	   curmark->next = malloc (sizeof (DateMark));
	   if (curmark->next == NULL) {
		   ShowErrMessage (_("ReadLogStats: out of memory"));
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
	   
	   fprintf (stderr, "%s\n", ctime (&curmark->time)); 
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
   log->lstats.numlines = nl;

   return;
}

/* ----------------------------------------------------------------------
   NAME:	UpdateLogStats
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

void
UpdateLogStats( Log *log )
{
   gchar *buffer;
   gchar **buffer_lines;
   int i, thisyear, nl, newbytes;
   time_t curdate, newdate;
   DateMark *curmark;
   struct tm *tmptm;
   GnomeVFSFileSize filesize;
   GnomeVFSFileSize read_bytes;
   GnomeVFSResult result;

   if (log == NULL)
     return;

   curmark = log->lstats.lastmark;
   curdate = log->lstats.enddate;
   tmptm = localtime (&curdate);
   thisyear = tmptm->tm_year;

   /* Count the new bytes that were added to the file since last time */
   result = gnome_vfs_seek (log->handle, GNOME_VFS_SEEK_END, 0L);
   filesize = gnome_vfs_tell (log->handle, &(filesize));
   newbytes = filesize - curmark->offset;

   /* Read these new bytes */
   gnome_vfs_seek (log->handle, curmark->offset, GNOME_VFS_SEEK_START);   
   buffer = g_malloc (newbytes);
   result = gnome_vfs_read (log->handle, buffer, newbytes, &read_bytes);
   buffer_lines = g_strsplit (buffer, "\n", -1);
   
   /* Go back to where we were */
   gnome_vfs_seek (log->handle, log->filesize, 0L);

   nl=0;
   for (i=1; buffer_lines[i]!=NULL; i++) {
	   nl++;
	   newdate = GetDate (buffer_lines[i]);
	   
	   if (newdate < 0 || isSameDay (newdate, curdate) )
		   continue;
	   curmark->next = malloc (sizeof (DateMark));
	   if (curmark->next == NULL) {
		   ShowErrMessage (_("ReadLogStats: out of memory"));
		   exit (0);
	   }
	   curmark->next->prev = curmark;
	   curmark = curmark->next;
	   curmark->time = newdate;
	   if (newdate < curdate)	/* The newdate is next year */
		   thisyear++;
	   curmark->year = thisyear;
	   curmark->next = NULL;
	   curmark->offset = filesize;
	   curdate = newdate;
   }
   
   log->lstats.enddate = curdate;
   log->lstats.lastmark = curmark;
   log->lstats.numlines += nl;

   g_strfreev (buffer_lines);
   g_free (buffer);

   return;
}



/* ----------------------------------------------------------------------
   NAME:          isSameDay
   DESCRIPTION:   Determine if the given times are the same.
   ---------------------------------------------------------------------- */

int
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

   /* Close file */
   if (log->handle != NULL) {
	   gnome_vfs_close (log->handle);
	   log->handle = NULL;
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

	result = gnome_vfs_get_file_info_from_handle (log->handle, 
						      &(info),
						      GNOME_VFS_FILE_INFO_DEFAULT);
	if (info.mtime != log->lstats.mtime) {
		log->lstats.mtime = info.mtime;
		return TRUE;
	} else
		return FALSE;
}

/******************************************************************************/
/* Unmaintained functions that may be deleted */
/******************************************************************************/

#ifdef FIXME

#define LINES_P_PAGE             10
#define NUM_PAGES                5 
#define R_BUF_SIZE               1024	/* Size of read buffer */

/* ----------------------------------------------------------------------
   NAME:        ReadNPagesUp
   DESCRIPTION: 
   ---------------------------------------------------------------------- */

int
ReadNPagesUp (Log * lg, Page * pg, int n)
{
   Page *cp;
   int i;

   /* Go to place on file. */
   fseek (lg->fp, pg->firstchpos, SEEK_SET);

   cp = pg->prev;
   /* Read n pages up.             */
   for (i = 0; i < n; i++)
   {
      ReadPageUp (lg, cp);
      lg->firstpg = cp;
      lg->lastpg = lg->lastpg->prev;
      if (cp->isfirstpage == TRUE)
	 return i+1;
      cp = cp->prev;
   }

   return n;
}

/* ----------------------------------------------------------------------
   NAME:        ReadNPagesDown
   DESCRIPTION: 
   ---------------------------------------------------------------------- */

int
ReadNPagesDown (Log * lg, Page * pg, int n)
{
   Page *cp;
   int i;

   /* Go to place on file. */
   fseek (lg->fp, pg->lastchpos, SEEK_SET);

   cp = pg->next;
   /* Read n pages down.   */
   for (i = 0; i < n; i++)
   {
      ReadPageDown (lg, cp, FALSE /* exec_actions */);
      lg->lastpg = cp;
      lg->firstpg = lg->firstpg->next;
      if (cp->islastpage == TRUE)
	 return i+1;
      cp = cp->next;
   }

   return n;
}

/* ----------------------------------------------------------------------
   NAME:        ReadPageUp
   DESCRIPTION: Reads a page from the log file.
   ---------------------------------------------------------------------- */

int
ReadPageUp (Log * lg, Page * pg)
{
   LogLine *line;
   FILE *fp;
   char *c, buffer[R_BUF_SIZE + 1];
   int ch;
   int ln;
   long int old_pos;

   fp = lg->fp;
   ln = LINES_P_PAGE - 1;

   pg->lastchpos = ftell (fp);
   pg->islastpage = FALSE;
   pg->isfirstpage = FALSE;
   pg->fl = 0;
   pg->ll = LINES_P_PAGE - 1;

   /* Tell if we are reading the last page */
   ch = fgetc (fp);
   if (ch == EOF)
     pg->islastpage = TRUE;
   ungetc (ch,fp);

   while (ln >= pg->fl)
   {
      c = buffer;
      if (fseek (fp, -1L, SEEK_CUR) < 0)
      {
	 pg->fl = ln;
	 pg->isfirstpage = TRUE;
	 break;
      }
      /* Go to end of previous line */
      ch = fgetc (fp);
      while (ch != '\n')
      {
	 if (fseek (fp, -2L, SEEK_CUR) < 0)
	 {
	    /*   ch = fgetc (fp); */
	    pg->isfirstpage = TRUE;
	    pg->fl = ln;
	    break;
	 }
	 ch = fgetc (fp);
      }
      ungetc (ch, fp);

      /* Read the line now. */
      old_pos = ftell (fp);
      if (pg->isfirstpage == FALSE)
	 fseek (fp, 1L, SEEK_CUR);
      fgets (buffer, R_BUF_SIZE, fp);

      /* Put cursor back where it was. */
      fseek (fp, old_pos, SEEK_SET);

      line = &pg->line[ln];
      ParseLine (buffer, line);
      ln--;
   }
   pg->firstchpos = ftell (fp);

   return pg->isfirstpage;
}

/* ----------------------------------------------------------------------
   NAME:        ReadPageDown
   DESCRIPTION: Reads new log lines 
   ---------------------------------------------------------------------- */

int
ReadPageDown (Log *log, LogLine ***inp_mon_lines, gboolean exec_actions)
{
	gchar *buffer;
	gint new_lines_read = 0;
	FILE *fp;
	LogLine *line;
	LogLine **new_mon_lines = NULL;	 
	GnomeVFSFileSize filesize;
	GnomeVFSFileSize size_read;
	GnomeVFSResult result;
	
	g_return_val_if_fail (log != NULL, FALSE);
	
	/* Count the new bytes that were added to the file since last time */
	result = gnome_vfs_seek (log->handle, GNOME_VFS_SEEK_END, 0L);
	filesize = gnome_vfs_tell (log->handle, &(filesize));
	result = gnome_vfs_seek (log->handle, log->filesize, 0L);
	
	if ((filesize - log->filesize) > 0) {
	
		buffer = g_malloc (filesize - log->filesize);
		result = gnome_vfs_read (log->handle, buffer, filesize - log->filesize, &size_read);
		
		if (size_read > 0) {
			gchar **buffer_lines;
			int i;
			
			buffer_lines = g_strsplit (buffer, "\n", -1);
			for (i=0; buffer_lines[i] != NULL; i++) {
				++new_lines_read;
				new_mon_lines = realloc (new_mon_lines,
							 sizeof(*(new_mon_lines)) * new_lines_read);
				
				if (!new_mon_lines) {
					ShowErrMessage ("Unable to realloc for new_mon_lines\n");
					return 0; 
				}
				
				line = malloc (sizeof(**(new_mon_lines)));
				if (!line) {
					ShowErrMessage ("Unable to malloc for line\n");
					return 0;
				}
				ParseLine (buffer, line);
				new_mon_lines[new_lines_read - 1] = line;
				
				if (exec_actions)
					exec_action_in_db (log, line, actions_db);
			}
			*inp_mon_lines = new_mon_lines;
		} else new_lines_read = 0;
	} else new_lines_read = 0;

	return new_lines_read;
}

/* ----------------------------------------------------------------------
   NAME:          MoveToMark
   DESCRIPTION:   From the current mark read NUM_PAGES/2 pages ahead and
                  NUM_PAGES/2 pages behind (if possible).
   ---------------------------------------------------------------------- */

void
MoveToMark (Log *log)
{
  DateMark *mark;
  FILE *fp;
  Page *middlepg;
  int pagesread, pagesdown, i;
  
  g_return_if_fail (log);
  
  mark = log->curmark;
  g_return_if_fail (mark);
  fp = log->fp;
  
  pagesdown = NUM_PAGES>>1;
  
  /* Read relative to middle page */
  middlepg = log->lastpg;
  for(i=0;i<pagesdown;i++)
    middlepg = middlepg->prev;
  
  /* Move file pointer to the current offset 
     into the file */
  if (mark->offset > 1)
    fseek(fp, mark->offset-1, SEEK_SET);
  else
    fseek(fp, mark->offset, SEEK_SET);
  ReadPageDown(log, middlepg, FALSE /* exec_actions */);
  if (middlepg->islastpage)
    {
      middlepg = log->lastpg;
      if (mark->offset > 1)
	fseek(fp, mark->offset-1, SEEK_SET);
      else
	fseek(fp, mark->offset, SEEK_SET);
      ReadPageDown(log, middlepg, FALSE /* exec_actions */);
      ReadNPagesUp(log, middlepg, NUM_PAGES-1);
      log->currentpg = middlepg;
      return;
    }
  
  if (middlepg->isfirstpage)
    {
      middlepg = log->firstpg;
      fseek(fp, mark->offset, SEEK_SET);
      ReadPageDown(log, middlepg, FALSE /* exec_actions */);
      ReadNPagesDown(log, middlepg, NUM_PAGES-1);
      log->currentpg = middlepg;
      return;
    }
  
  pagesread = ReadNPagesDown (log, middlepg, NUM_PAGES-1);
  ReadNPagesUp (log, middlepg, MAX (pagesdown, (NUM_PAGES-1-pagesread)));

  log->currentpg = middlepg;
   
}

/* ----------------------------------------------------------------------
   NAME:	UpdateLastPage
   DESCRIPTION:	Re-read the last log page.
   ---------------------------------------------------------------------- */

void
UpdateLastPage (Log *log)
{

  /* Check that the last page is in the list. */
  if (!log->lastpg->islastpage)
    return;


}
#endif
