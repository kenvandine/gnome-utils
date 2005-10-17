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

#include <gtk/gtkmessagedialog.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

static gboolean queue_err_messages = FALSE;
static GList *msg_queue_main = NULL, *msg_queue_sec = NULL;
extern gboolean restoration_complete;
const char *C_monthname[12] =
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
/* space separated list of locales to check, must be space separated, no newlines */
/* FIXME: This should be checked for at configure time or something to that effect */
char *all_locales = "af_ZA ar_SA bg_BG ca_ES cs_CZ da_DK de_AT de_BE de_CH de_DE de_LU el_GR en_AU en_CA en_DK en_GB en_IE en_US es_ES et_EE eu_ES fi_FI fo_FO fr_BE fr_CA fr_CH fr_FR fr_LU ga_IE gl_ES gv_GB hr_HR hu_HU in_ID is_IS it_CH it_IT iw_IL kl_GL kw_GB lt_LT lv_LV mk_MK nl_BE nl_NL no_NO pl_PL pt_BR pt_PT ro_RO ru_RU.KOI8-R ru_RU ru_UA sk_SK sl_SI sr_YU sv_FI sv_SE th_TH tr_TR uk_UA";

static void
MakeErrorDialog (GtkWidget *window, const char *main, char *secondary)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
						     GTK_DIALOG_MODAL,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
						     main,
						     secondary);
	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/* ----------------------------------------------------------------------
   NAME:        ShowErrMessage
   DESCRIPTION: Print an error message. It will eventually open a 
   window and display the message.
   ---------------------------------------------------------------------- */

void
ShowErrMessage (GtkWidget *window, char *main, char *secondary)
{
    if (queue_err_messages) {
        msg_queue_main = g_list_append (msg_queue_main, g_strdup (main));
        msg_queue_sec = g_list_append (msg_queue_sec, g_strdup (secondary));
    } else
        MakeErrorDialog (window, main, secondary);
}

void
QueueErrMessages (gboolean do_queue)
{
	queue_err_messages = do_queue;
}

void
ShowQueuedErrMessages (void)
{
	if (msg_queue_main != NULL) {
		gboolean title_created = FALSE;
		GList *li, *li_sec;
		GString *gs = g_string_new (NULL);
		GString *gs_sec = g_string_new (NULL);

		for (li = msg_queue_main, li_sec=msg_queue_sec; li != NULL; li = li->next) {
			char *msg = li->data;
			char *sec = li_sec->data;

			li->data = NULL;
			li_sec->data = NULL;

			if (!title_created) {
				g_string_append (gs, msg);
				title_created = TRUE;
			}
			
			g_string_append (gs_sec, sec);
			if (li->next != NULL)
				g_string_append (gs_sec, "\n");

			g_free (msg);
			g_free (sec);
			li_sec = li_sec->next;
		}
		g_list_free (msg_queue_main);
		g_list_free (msg_queue_sec);
		msg_queue_main = NULL;
		msg_queue_sec = NULL;

		MakeErrorDialog (NULL, gs->str, gs_sec->str);

		g_string_free (gs, TRUE);
		g_string_free (gs_sec, TRUE);
	}
}

char *
LocaleToUTF8 (const char *in)
{
	/* FIXME: we could guess the language from the line how we guessed
	 * the month and can we get this to utf8 correctly even if it's not in
	 * the current encoding */
	char *out = g_locale_to_utf8 (in, -1, NULL, NULL, NULL);
	if (out == NULL) {
		if (g_utf8_validate (in, -1, NULL))
			return g_strdup (in);
		else
			return g_strdup ("?");
	} else {
		return out;
	}
}

int
IsLeapYear (int year)
{
   if ((1900 + year) % 4 == 0)
      return TRUE;
   else
      return FALSE;
}

int
days_are_equal (time_t day1, time_t day2)
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

int
string_get_month (const char *str)
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

time_t
string_get_date (char *line)
{
   struct tm date;
   char *token;
   int i;

   token = strtok (line, " ");
   if (!token)
       return -1;
   
   i = string_get_month (token);

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
