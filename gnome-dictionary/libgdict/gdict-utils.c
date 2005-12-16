/* gdict-utils.c - Utility functions for Gdict
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gdict-context-private.h"
#include "gdict-utils.h"

#ifdef GDICT_ENABLE_DEBUG
void
gdict_debug (const gchar *fmt,
             ...)
{
  va_list args;

  fprintf (stderr, "GDICT_DEBUG: ");
    
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
}
#endif

/* gdict_has_ipv6: checks for the existence of the IPv6 extensions; if
 * IPv6 support was not enabled, this function always return false
 */
gboolean
gdict_has_ipv6 (void)
{
#ifdef ENABLE_IPV6
  int s;

  s = socket (AF_INET6, SOCK_STREAM, 0);
  if (s != -1)
    {
      close(s);
      
      return TRUE;
    }
#endif

  return FALSE;
}

/* shows an error dialog making it transient for @parent */
static void
show_error_dialog (GtkWindow   *parent,
		   const gchar *message,
		   const gchar *detail)
{
  GtkWidget *dialog;
  
  dialog = gtk_message_dialog_new (parent,
  				   GTK_DIALOG_DESTROY_WITH_PARENT,
  				   GTK_MESSAGE_ERROR,
  				   GTK_BUTTONS_OK,
  				   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), "");
  
  if (detail)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
  					      "%s", detail);
  
  if (parent->group)
    gtk_window_group_add_window (parent->group, GTK_WINDOW (dialog));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

/* find the toplevel widget for @widget */
static GtkWindow *
get_toplevel_window (GtkWidget *widget)
{
  GtkWidget *toplevel;
  
  toplevel = gtk_widget_get_toplevel (widget);
  if (!GTK_WIDGET_TOPLEVEL (toplevel))
    return NULL;
  else
    return GTK_WINDOW (toplevel);
}

/**
 * gdict_show_error_dialog:
 * @widget: the widget that emits the error
 * @title: the primary error message
 * @message: the secondary error message or %NULL
 *
 * Creates and shows an error dialog bound to @widget.
 *
 * Since: 1.0
 */
void
gdict_show_error_dialog (GtkWidget   *widget,
			 const gchar *title,
			 const gchar *detail)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (title != NULL);
  
  show_error_dialog (get_toplevel_window (widget), title, detail);
}

/**
 * gdict_show_gerror_dialog:
 * @widget: the widget that emits the error
 * @title: the primary error message
 * @error: a #GError
 *
 * Creates and shows an error dialog bound to @widget, using @error
 * to fill the secondary text of the message dialog with the error
 * message.  Also takes care of freeing @error.
 *
 * Since: 1.0
 */
void
gdict_show_gerror_dialog (GtkWidget   *widget,
			  const gchar *title,
			  GError      *error)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (title != NULL);
  g_return_if_fail (error != NULL);
  
  show_error_dialog (get_toplevel_window (widget), title, error->message);
      
  g_error_free (error);
}
