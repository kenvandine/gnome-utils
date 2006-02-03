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
#include <time.h>
#include <string.h>
#include <stdlib.h>

static gboolean queue_err_messages = FALSE;
static GSList *msg_queue_main = NULL, *msg_queue_sec = NULL;
const char *month[12] =
{N_("January"), N_("February"), N_("March"), N_("April"), N_("May"),
 N_("June"), N_("July"), N_("August"), N_("September"), N_("October"),
 N_("November"), N_("December")};

static void
error_dialog_run (GtkWidget *window, const char *main, char *secondary)
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

void
error_dialog_show (GtkWidget *window, char *main, char *secondary)
{
    if (queue_err_messages) {
        msg_queue_main = g_slist_append (msg_queue_main, g_strdup (main));
        msg_queue_sec = g_slist_append (msg_queue_sec, g_strdup (secondary));
    } else
        error_dialog_run (window, main, secondary);
}

void
error_dialog_queue (gboolean do_queue)
{
	queue_err_messages = do_queue;
}

void
error_dialog_show_queued (void)
{
	if (msg_queue_main != NULL) {
		gboolean title_created = FALSE;
		GSList *li, *li_sec;
		GString *gs = g_string_new (NULL);
		GString *gs_sec = g_string_new (NULL);

		for (li = msg_queue_main, li_sec=msg_queue_sec; li != NULL; li = g_slist_next (li)) {
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
		g_slist_free (msg_queue_main);
		g_slist_free (msg_queue_sec);
		msg_queue_main = NULL;
		msg_queue_sec = NULL;

		error_dialog_run (NULL, gs->str, gs_sec->str);

		g_string_free (gs, TRUE);
		g_string_free (gs_sec, TRUE);
	}
}

char *
locale_to_utf8 (const char *in)
{
  char *out;
  
  if (g_utf8_validate (in, -1, NULL))
	out = g_strdup (in);
  else {
	out = g_locale_to_utf8 (in, -1, NULL, NULL, NULL);
	if (out == NULL)
	  out = g_strdup ("?");
  }

  return out;
}

GDate *
string_get_date (char *line)
{
    GDate *date;
    struct tm tp;
    int cp;
    
    if (line == NULL || line[0] == 0)
        return NULL;

    cp = strptime (line, "%b %d", &tp);
    if (cp == 0) {
        cp = strptime (line, "%F", &tp);
        if (cp == 0) {
            return NULL;
        }
    }

    date = g_date_new_dmy (tp.tm_mday, tp.tm_mon+1, 70);
    return date;
}

char *
date_get_string (GDate *date)
{
   char buf[512];
   char *utf8;

   if (date == NULL || !g_date_valid (date)) {
       utf8 = g_strdup(_("Invalid date"));
       return utf8;
   }
   
   /* Translators: Only date format, time will be bogus */
   if (g_date_strftime (buf, sizeof (buf), _("%x"), date) == 0) {
       int m = g_date_get_month (date);
       int d = g_date_get_day (date);
       /* If we fail just use the US format */
       utf8 = g_strdup_printf ("%s %d", _(month[(int) m-1]), d);
   } else
       utf8 = locale_to_utf8 (buf);
   
   return utf8;
}

void
widget_set_font (GtkWidget   *widget,
		 const gchar *font_name)
{
	PangoFontDescription *font_desc;

	g_return_if_fail (GTK_IS_WIDGET (widget));

	font_desc = pango_font_description_from_string (font_name);
	if (font_desc) {
		gtk_widget_modify_font (widget, font_desc);
		pango_font_description_free (font_desc);
	}
	else
		gtk_widget_modify_font (widget, NULL);
}
