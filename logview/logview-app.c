/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-app.c - logview application singleton
 *
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* logview-app.c */

#include "logview-app.h"

#include "logview-manager.h"
#include "logview-window.h"
#include "logview-prefs.h"

struct _LogviewAppPrivate {
  LogviewPrefs *prefs;
  LogviewManager *manager;
  LogviewWindow *window;
};

enum {
  APP_QUIT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static LogviewApp *app_singleton = NULL;

G_DEFINE_TYPE (LogviewApp, logview_app, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_APP, LogviewAppPrivate))

static gboolean
main_window_delete_cb (GtkWidget *widget,
                       GdkEvent *event,
                       gpointer user_data)
{
  LogviewApp *app = user_data;

  g_signal_emit (app, signals[APP_QUIT], 0, NULL);

  return FALSE;
}

static gboolean
logview_app_set_window (LogviewApp *app)
{
  LogviewWindow *window;
  gboolean retval = FALSE;

  window = LOGVIEW_WINDOW (logview_window_new ());

  if (window) {
    app->priv->window = window;
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (main_window_delete_cb), app);
    retval = TRUE;
  }

  gtk_window_set_default_icon_name ("logviewer");

  return retval;
}

static void
do_finalize (GObject *obj)
{
  LogviewApp *app = LOGVIEW_APP (obj);

  g_object_unref (app->priv->manager);
  g_object_unref (app->priv->prefs);

  G_OBJECT_CLASS (logview_app_parent_class)->finalize (obj);
}

static void
logview_app_class_init (LogviewAppClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = do_finalize;

  signals[APP_QUIT] =
    g_signal_new ("app-quit",
                  G_OBJECT_CLASS_TYPE (oclass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LogviewAppClass, app_quit),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (LogviewAppPrivate));
}

static void
logview_app_init (LogviewApp *self)
{
  LogviewAppPrivate *priv = self->priv = GET_PRIVATE (self);

  priv->prefs = logview_prefs_get ();
  priv->manager = logview_manager_get ();
}

LogviewApp*
logview_app_get (void)
{
  if (!app_singleton) {
    app_singleton = g_object_new (LOGVIEW_TYPE_APP, NULL);

    if (!logview_app_set_window (app_singleton)) {
      g_object_unref (app_singleton);
      app_singleton = NULL;
    }
  }

  return app_singleton;
}

void
logview_app_initialize (LogviewApp *app, const char **log_files)
{
  LogviewAppPrivate *priv = app->priv;

  /* open regular logs and add each log passed as a parameter */

  if (log_files == NULL) {
    char *active_log;
    GSList *logs;

    active_log = logview_prefs_get_active_logfile (priv->prefs);
    logs = logview_prefs_get_stored_logfiles (priv->prefs);

    logview_manager_add_logs_from_names (priv->manager,
                                         logs, active_log);

    g_free (active_log);
    g_slist_foreach (logs, (GFunc) g_free, NULL);
    g_slist_free (logs);
  } else {
    gint i;

    for (i = 0; log_files[i]; i++)
      logview_manager_add_log_from_name (priv->manager, log_files[i], FALSE);
  }

  gtk_widget_show (GTK_WIDGET (priv->window));
}

#if 0
void
logview_show_error (const char *primary,
                    const char *secondary)
{
  GtkWidget *message_dialog;

  message_dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           "%s", primary);
  if (secondary) {
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message_dialog),
                                              "%s", secondary);
  };

  gtk_dialog_run (GTK_DIALOG (message_dialog));
  gtk_widget_destroy (message_dialog);
}

#endif

