/* logview-utils.c
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 551 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 */

#define _XOPEN_SOURCE
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <glib.h>

#include "logview-utils.h"

static gint 
days_compare (gconstpointer a, gconstpointer b)
{
  const Day *day1 = a, *day2 = b;
  return (g_date_compare (day1->date, day2->date));
}

static GDate *
string_get_date (char *line)
{
  GDate *date;
  struct tm tp;
  char *cp;

  if (line == NULL || line[0] == 0)
    return NULL;

  cp = strptime (line, "%b %d", &tp);
  if (cp == NULL) {
    cp = strptime (line, "%F", &tp);
    if (cp == NULL) {
      return NULL;
    }
  }

  date = g_date_new_dmy (tp.tm_mday, tp.tm_mon+1, 70);
  return date;
}

static gchar *
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