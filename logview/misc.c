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

#include <gtk/gtkmessagedialog.h>

static gboolean queue_err_messages = FALSE;
static GList *msg_queue_main = NULL, *msg_queue_sec = NULL;
extern gboolean restoration_complete;

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

/* ----------------------------------------------------------------------
   NAME:          IsLeapYear
   DESCRIPTION:   Return TRUE if year is a leap year.
   ---------------------------------------------------------------------- */
int
IsLeapYear (int year)
{
   if ((1900 + year) % 4 == 0)
      return TRUE;
   else
      return FALSE;

}
