/* logview-manager.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 551 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/* logview-manager.c */

#include "logview-manager.h"

enum {
  LOG_ADDED,
  LOG_ADD_ERROR,
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

static LogviewManager *singleton = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (LogviewManager, logview_manager, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_MANAGER, LogviewManagerPrivate))

typedef struct {
  LogviewManager *manager;
  char *filename;
  gboolean set_active;
} CreateCBData;

struct _LogviewManagerPrivate {
  GList *logs;
  LogviewLog *active_log;
};

static void
logview_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (logview_manager_parent_class)->finalize (object);
}

static void
logview_manager_class_init (LogviewManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = logview_manager_finalize;

  signals[LOG_ADDED] = g_signal_new ("log-added",
                                     G_OBJECT_CLASS_TYPE (object_class),
                                     G_SIGNAL_RUN_LAST,
                                     G_STRUCT_OFFSET (LogviewManagerClass, log_added),
                                     NULL, NULL,
                                     g_cclosure_marshal_VOID__OBJECT,
                                     G_TYPE_NONE, 1,
                                     LOGVIEW_TYPE_LOG);

  signals[LOG_ADD_ERROR] = g_signal_new ("log-add-error",
                                         G_OBJECT_CLASS_TYPE (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (LogviewManagerClass, log_add_error),
                                         NULL, NULL,
                                         g_cclosure_marshal_VOID__STRING,
                                         G_TYPE_NONE, 1,
                                         G_TYPE_STRING);

  signals[ACTIVE_CHANGED] = g_signal_new ("active-changed",
                                          G_OBJECT_CLASS_TYPE (object_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (LogviewManagerClass, active_changed),
                                          NULL, NULL,
                                          g_cclosure_marshal_VOID__OBJECT,
                                          G_TYPE_NONE, 1,
                                          LOGVIEW_TYPE_LOG);

  g_type_class_add_private (klass, sizeof (LogviewManagerPrivate));
}

static void
logview_manager_init (LogviewManager *self)
{
  self->priv = GET_PRIVATE (self);

  priv->active_log = NULL;
  priv->logs = NULL;
}

static void
create_log_cb (LogviewLog *log,
               GError *error,
               gpointer user_data)
{
  CreateCBData *data;

  if (log) {
    /* creation went well, store the log and notify */
    g_slist_append (data->manager->priv->logs, log);

    g_signal_emit (data->manager, signals[LOG_ADDED], 0, log, NULL);

    if (data->set_active) {
      logview_manager_set_active (data->manager, log);
    }
  } else {
    /* notify the error */
    g_signal_emit (data->manager, signals[LOG_ADD_ERROR], 0, data->filename, NULL);
  }

  g_free (data->filename);
  g_slice_free (CreateCBData, data);
}

/* public methods */

LogviewManager*
logview_manager_get (void)
{
  if (!singleton) {
    singleton = g_object_new (LOGVIEW_TYPE_MANAGER, NULL);
  }

  return singleton;
}

void
logview_manager_set_active_log (LogviewManager *manager,
                                LogviewLog *log)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  if (manager->priv->active_log) {
    g_object_unref (manager->priv->active_log);
  }

  manager->priv->active_log = g_object_ref (log);
  g_signal_emit (manager, signals[ACTIVE_CHANGED], 0, log, NULL);
}

void
logview_manager_get_active_log (LogviewManager *manager)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  return (manager->priv->active_log != NULL) ?
    g_object_ref (manager->priv->active_log) :
    NULL;
}

void
logview_manager_add_logs_from_names (LogviewManager *manager,
                                     GSList *names,
                                     const char *active)
{
  GSList *l;

  g_assert (LOGVIEW_IS_MANAGER (manager));

  for (l = names; l; l = l->next) {
    logview_manager_add_log_from_name (l->data, (g_ascii_strcasecmp (active, l->data) == 0));
  }
}

void
logview_manager_add_log_from_name (LogviewManager *manager,
                                   const char *filename, gboolean set_active)
{
  CreateCBData *data = g_slice_new0 (CreateCBData);

  g_assert (LOGVIEW_IS_MANAGER (manager));

  if (set_active == FALSE) {
    /* if it's the first log being added, set it as active anyway */
    set_active = (manager->priv->logs == NULL);
  }

  data->filename = g_strdup (filename);
  data->manager = manager;
  data->set_active = set_active;

  logview_log_create (filename, create_log_cb, data);
}

int
logview_manager_get_log_count (LogviewManager *manager)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  return g_slist_length (manager->priv->logs);
}