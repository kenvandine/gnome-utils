/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-prefs.c - logview user preferences handling
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
 * Copyright (C) 2004  Vincent Noel
 * Copyright (C) 2006  Emmanuele Bassi
 * Copyright (C) 2008  Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "logview-prefs.h"

#define LOGVIEW_DEFAULT_HEIGHT 400
#define LOGVIEW_DEFAULT_WIDTH 600

/* logview settings */
#define GCONF_DIR 		"/apps/gnome-system-log"
#define GCONF_WIDTH_KEY 	GCONF_DIR "/width"
#define GCONF_HEIGHT_KEY 	GCONF_DIR "/height"
#define GCONF_LOGFILE 		GCONF_DIR "/logfile"
#define GCONF_LOGFILES 		GCONF_DIR "/logfiles"
#define GCONF_FONTSIZE_KEY 	GCONF_DIR "/fontsize"

/* desktop-wide settings */
#define GCONF_MONOSPACE_FONT_NAME "/desktop/gnome/interface/monospace_font_name"
#define GCONF_MENUS_HAVE_TEAROFF  "/desktop/gnome/interface/menus_have_tearoff"

static LogviewPrefs *singleton = NULL;

enum {
  SYSTEM_FONT_CHANGED,
  HAVE_TEAROFF_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static char *default_logfiles[] = {
  "/var/log/sys.log",
#ifndef ON_SUN_OS
  "/var/log/messages",
  "/var/log/secure",
  "/var/log/maillog",
  "/var/log/cron",
  "/var/log/Xorg.0.log",
  "/var/log/XFree86.0.log",
  "/var/log/auth.log",
  "/var/log/cups/error_log",
#else
  "/var/adm/messages",
  "/var/adm/sulog",
  "/var/log/authlog",
  "/var/log/brlog",
  "/var/log/postrun.log",
  "/var/log/scrollkeeper.log",
  "/var/log/snmpd.log",
  "/var/log/sysidconfig.log",
  "/var/log/swupas/swupas.log",
  "/var/log/swupas/swupas.error.log",
#endif
  NULL
};

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_PREFS, LogviewPrefsPrivate))

struct _LogviewPrefsPrivate {
  GConfClient *client;

  guint size_store_timeout;
};

typedef struct {
  int width;
  int height;
} WindowSize;

G_DEFINE_TYPE (LogviewPrefs, logview_prefs, G_TYPE_OBJECT);

static void
do_finalize (GObject *obj)
{
  LogviewPrefs *prefs = LOGVIEW_PREFS (obj);

  g_object_unref (prefs->priv->client);

  G_OBJECT_CLASS (logview_prefs_parent_class)->finalize (obj);
}

static void
logview_prefs_class_init (LogviewPrefsClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = do_finalize;

  signals[SYSTEM_FONT_CHANGED] = g_signal_new ("system-font-changed",
                                               G_OBJECT_CLASS_TYPE (oclass),
                                               G_SIGNAL_RUN_LAST,
                                               G_STRUCT_OFFSET (LogviewPrefsClass, system_font_changed),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__STRING,
                                               G_TYPE_NONE, 1,
                                               G_TYPE_STRING);
  signals[HAVE_TEAROFF_CHANGED] = g_signal_new ("have-tearoff-changed",
                                                G_OBJECT_CLASS_TYPE (oclass),
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (LogviewPrefsClass, have_tearoff_changed),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__BOOLEAN,
                                                G_TYPE_NONE, 1,
                                                G_TYPE_BOOLEAN);

  g_type_class_add_private (klass, sizeof (LogviewPrefsPrivate));
}

static void
have_tearoff_changed_cb (GConfClient *client,
                         guint id,
                         GConfEntry *entry,
                         gpointer data)
{
  LogviewPrefs *prefs = data;

  if (entry->value && (entry->value->type == GCONF_VALUE_BOOL)) {
    gboolean add_tearoffs;

    add_tearoffs = gconf_value_get_bool (entry->value);
    g_signal_emit (prefs, signals[HAVE_TEAROFF_CHANGED], 0, add_tearoffs, NULL);
  }
}

static void
monospace_font_changed_cb (GConfClient *client,
                           guint id,
                           GConfEntry *entry,
                           gpointer data)
{
  LogviewPrefs *prefs = data;

  if (entry->value && (entry->value->type == GCONF_VALUE_STRING)) {
    const gchar *monospace_font_name;

    monospace_font_name = gconf_value_get_string (entry->value);
    g_signal_emit (prefs, signals[SYSTEM_FONT_CHANGED], 0, monospace_font_name, NULL);
  }
}

static gboolean
size_store_timeout_cb (gpointer data)
{
  WindowSize *size = data;
  LogviewPrefs *prefs = logview_prefs_get ();

  if (size->width > 0 && size->height > 0) {
    if (gconf_client_key_is_writable (prefs->priv->client, GCONF_WIDTH_KEY, NULL))
      gconf_client_set_int (prefs->priv->client,
                            GCONF_WIDTH_KEY,
                            size->width,
                            NULL);

    if (gconf_client_key_is_writable (prefs->priv->client, GCONF_HEIGHT_KEY, NULL))
      gconf_client_set_int (prefs->priv->client,
                            GCONF_HEIGHT_KEY,
                            size->height,
                            NULL);
  }

  /* reset the source id */
  prefs->priv->size_store_timeout = 0;

  g_free (size);

  return FALSE;
}

static gboolean
check_stored_logfiles (LogviewPrefs *prefs)
{
  GConfValue *val;
  gboolean retval = FALSE;

  val = gconf_client_get (prefs->priv->client,
                          GCONF_LOGFILES,
                          NULL);
  if (val) {
    gconf_value_free (val);
    retval = TRUE;
  }

  return retval;
}

/* adapted from sysklogd sources */
static GSList*
parse_syslog ()
{
  char *logfile = NULL;
  char cbuf[BUFSIZ];
  char *cline, *p;
  FILE *cf;
  GSList *logfiles = NULL;

  if ((cf = fopen ("/etc/syslog.conf", "r")) == NULL) {
    return NULL;
  }

  cline = cbuf;
  while (fgets (cline, sizeof (cbuf) - (cline - cbuf), cf) != NULL) {
    gchar **list;
    gint i;

    for (p = cline; g_ascii_isspace (*p); ++p);
    if (*p == '\0' || *p == '#' || *p == '\n')
      continue;

    list = g_strsplit_set (p, ", -\t()\n", 0);

    for (i = 0; list[i]; ++i) {
      if (*list[i] == '/' &&
          g_slist_find_custom (logfiles, list[i],
                               (GCompareFunc) g_ascii_strcasecmp) == NULL)
      {
        logfiles = g_slist_insert (logfiles,
                                   g_strdup (list[i]), 0);
      }
    }

    g_strfreev (list);
  }

  fclose (cf);

  return logfiles;
}

static void
logview_prefs_fill_defaults (LogviewPrefs *prefs)
{
  GSList *logs;
  int i;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  /* insert in the registry both the default items and the files
   * specified in syslog.conf.
   */

  logs = parse_syslog ();

  for (i = 0; default_logfiles[i]; i++) {
    if (g_slist_find_custom (logs, default_logfiles[i], (GCompareFunc) g_ascii_strcasecmp) == NULL)
      logs = g_slist_insert (logs, g_strdup (default_logfiles[i]), 0);
  }

  gconf_client_set_list (prefs->priv->client,
                         GCONF_LOGFILES,
                         GCONF_VALUE_STRING,
                         logs, NULL);

  /* the string list is copied */
  g_slist_foreach (logs, (GFunc) g_free, NULL);
  g_slist_free (logs);
}

static void
logview_prefs_init (LogviewPrefs *self)
{
  gboolean stored_logs;
  LogviewPrefsPrivate *priv;

  priv = self->priv = GET_PRIVATE (self);

  priv->client = gconf_client_get_default ();
  priv->size_store_timeout = 0;

  stored_logs = check_stored_logfiles (self);
  if (!stored_logs) {
    /* if there's no stored logs, either it's the first start or GConf has
     * been corrupted. re-fill the registry with sensible defaults anyway.
     */
    logview_prefs_fill_defaults (self);
  }

  gconf_client_notify_add (priv->client,
                           GCONF_MONOSPACE_FONT_NAME,
                           (GConfClientNotifyFunc) monospace_font_changed_cb,
                           self, NULL, NULL);
  gconf_client_notify_add (priv->client,
                           GCONF_MENUS_HAVE_TEAROFF,
                           (GConfClientNotifyFunc) have_tearoff_changed_cb,
                           self, NULL, NULL);
}

/* public methods */

LogviewPrefs *
logview_prefs_get ()
{
  if (!singleton)
    singleton = g_object_new (LOGVIEW_TYPE_PREFS, NULL);

  return singleton;
}

void
logview_prefs_store_window_size (LogviewPrefs *prefs,
                                 int width, int height)
{
  /* we want to be smart here: since we will get a lot of configure events
   * while resizing, we schedule the real GConf storage in a timeout.
   */
  WindowSize *size;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  size = g_new0 (WindowSize, 1);
  size->width = width;
  size->height = height;

  if (prefs->priv->size_store_timeout != 0) {
    /* reschedule the timeout */
    g_source_remove (prefs->priv->size_store_timeout);
    prefs->priv->size_store_timeout = 0;
  }

  prefs->priv->size_store_timeout = g_timeout_add (200,
                                                   size_store_timeout_cb,
                                                   size);
}

void
logview_prefs_get_stored_window_size (LogviewPrefs *prefs,
                                      int *width, int *height)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  *width = gconf_client_get_int (prefs->priv->client,
                                 GCONF_WIDTH_KEY,
                                 NULL);

  *height = gconf_client_get_int (prefs->priv->client,
                                  GCONF_HEIGHT_KEY,
                                  NULL);

  if ((*width == 0) ^ (*height == 0)) {
    /* if one of the two failed, return default for both */
    *width = LOGVIEW_DEFAULT_WIDTH;
    *height = LOGVIEW_DEFAULT_HEIGHT;
  }
}

char *
logview_prefs_get_monospace_font_name (LogviewPrefs *prefs)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  return (gconf_client_get_string (prefs->priv->client, GCONF_MONOSPACE_FONT_NAME, NULL));
}

gboolean
logview_prefs_get_have_tearoff (LogviewPrefs *prefs)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

	return (gconf_client_get_bool (prefs->priv->client, GCONF_MENUS_HAVE_TEAROFF, NULL));
}

/* the elements should be freed with g_free () */

GSList *
logview_prefs_get_stored_logfiles (LogviewPrefs *prefs)
{
  GSList *retval;
  GError *err = NULL;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  retval = gconf_client_get_list (prefs->priv->client,
                                  GCONF_LOGFILES,
                                  GCONF_VALUE_STRING,
                                  NULL);
  return retval;
}

void
logview_prefs_store_log (LogviewPrefs *prefs, GFile *file)
{
  GSList *stored_logs, *l;
  GFile *stored;
  gboolean found = FALSE;

  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (G_IS_FILE (file));

  stored_logs = logview_prefs_get_stored_logfiles (prefs);

  for (l = stored_logs; l; l = l->next) {
    stored = g_file_parse_name (l->data);
    if (g_file_equal (file, stored)) {
      found = TRUE;
    }

    g_object_unref (stored);

    if (found) {
      break;
    }
  }

  if (!found) {
    stored_logs = g_slist_prepend (stored_logs, g_file_get_parse_name (file));
    gconf_client_set_list (prefs->priv->client,
                           GCONF_LOGFILES,
                           GCONF_VALUE_STRING,
                           stored_logs,
                           NULL);
  }

  /* the string list is copied */
  g_slist_foreach (stored_logs, (GFunc) g_free, NULL);
  g_slist_free (stored_logs);
}

void
logview_prefs_remove_stored_log (LogviewPrefs *prefs, GFile *target)
{
  GSList *stored_logs, *l, *removed = NULL;
  GFile *stored;

  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (G_IS_FILE (target));

  stored_logs = logview_prefs_get_stored_logfiles (prefs);

  for (l = stored_logs; l; l = l->next) {
    stored = g_file_parse_name (l->data);
    if (g_file_equal (stored, target)) {
      removed = l;
      stored_logs = g_slist_remove_link (stored_logs, l);
    }

    g_object_unref (stored);

    if (removed) {
      break;
    }
  }

  if (removed) {
    gconf_client_set_list (prefs->priv->client,
                           GCONF_LOGFILES,
                           GCONF_VALUE_STRING,
                           stored_logs,
                           NULL);
  }

  /* the string list is copied */
  g_slist_foreach (stored_logs, (GFunc) g_free, NULL);
  g_slist_free (stored_logs);

  if (removed) {
    g_free (removed->data);
    g_slist_free (removed);
  }
}

void
logview_prefs_store_fontsize (LogviewPrefs *prefs, int fontsize)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (fontsize > 0);

  if (gconf_client_key_is_writable (prefs->priv->client, GCONF_FONTSIZE_KEY, NULL)) {
    gconf_client_set_int (prefs->priv->client,
                          GCONF_FONTSIZE_KEY,
                          fontsize, NULL);
  }
}

int
logview_prefs_get_stored_fontsize (LogviewPrefs *prefs)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

	return gconf_client_get_int (prefs->priv->client, GCONF_FONTSIZE_KEY, NULL);
}

void
logview_prefs_store_active_logfile (LogviewPrefs *prefs,
                                    const char *filename)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  if (gconf_client_key_is_writable (prefs->priv->client, GCONF_LOGFILE, NULL)) {
    gconf_client_set_string (prefs->priv->client,
                             GCONF_LOGFILE,
                             filename,
                             NULL);
  }
}

char *
logview_prefs_get_active_logfile (LogviewPrefs *prefs)
{
  char *filename;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  filename = gconf_client_get_string (prefs->priv->client, GCONF_LOGFILE, NULL);

  return filename;
}