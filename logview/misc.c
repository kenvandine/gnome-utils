
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
#include <gnome.h>

#include "logview.h"

extern ConfigData *cfg;
extern GtkWidget *window;

static gboolean queue_err_messages = FALSE;
static GList *msg_queue = NULL;


static void
MakeErrorDialog (const char *msg)
{
	GtkWidget *msgbox;
	msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_ERROR,
					GNOME_STOCK_BUTTON_OK, NULL);
	if (window != NULL)
		gnome_dialog_set_parent (GNOME_DIALOG (msgbox), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW(msgbox), TRUE); 
	gtk_widget_show (msgbox);
}

/* ----------------------------------------------------------------------
   NAME:        ShowErrMessage
   DESCRIPTION: Print an error message. It will eventually open a 
   window and display the message.
   ---------------------------------------------------------------------- */

void
ShowErrMessage (const char *msg)
{
  DB (printf(_("Error: [%s]\n"), msg));

  if (queue_err_messages) {
	  msg_queue = g_list_append (msg_queue, g_strdup (msg));
  } else {
	  MakeErrorDialog (msg);
  }
}

void
QueueErrMessages (gboolean do_queue)
{
	queue_err_messages = do_queue;
}

void
ShowQueuedErrMessages (void)
{
	if (msg_queue != NULL) {
		GList *li;
		GString *gs = g_string_new (NULL);

		for (li = msg_queue; li != NULL; li = li->next) {
			char *msg = li->data;
			li->data = NULL;

			g_string_append (gs, msg);

			if (li->next != NULL)
				g_string_append (gs, "\n");

			g_free (msg);
		}
		g_list_free (msg_queue);
		msg_queue = NULL;

		MakeErrorDialog (gs->str);

		g_string_free (gs, TRUE);
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
