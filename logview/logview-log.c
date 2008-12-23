/* logview-log.c
 *
 * Copyright (C) 1998 Cesar Miquel <miquel@df.uba.ar>
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

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include "logview-log.h"
#include "logview-utils.h"

G_DEFINE_TYPE (LogviewLog, logview_log, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_LOG, LogviewLogPrivate))

enum {
  LOG_CHANGED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

struct _LogviewLogPrivate {
  /* file and monitor */
  GFile *file;
  GFileMonitor *mon;

  /* stats about the file */
  GTimeVal file_time;
  goffset file_size;
  char *display_name;

  /* lines and relative days */
  GSList *days;
  GPtrArray *lines;
  guint lines_no;

  /* stream poiting to the log */
  GDataInputStream *stream;
  gboolean has_new_lines;
};

typedef struct {
  LogviewLog *log;
  GError *err;
  LogviewCreateCallback callback;
  gpointer user_data;
} LoadJob;

typedef struct {
  LogviewLog *log;
  GError *err;
  const char **lines;
  LogviewNewLinesCallback callback;
  gpointer user_data;
} NewLinesJob;

static void
do_finalize (GObject *obj)
{
  LogviewLog *log = LOGVIEW_LOG (obj);
  char ** lines;

  if (log->priv->stream) {
    g_object_unref (log->priv->stream);
    log->priv->stream = NULL;
  }

  if (log->priv->file) {
    g_object_unref (log->priv->file);
    log->priv->file = NULL;
  }

  if (log->priv->mon) {
    g_object_unref (log->priv->mon);
    log->priv->mon = NULL;
  }

  if (log->priv->days) {
    g_slist_foreach (log->priv->days,
                     (GFunc) logview_utils_day_free, NULL);
    g_slist_free (log->priv->days);
    log->priv->days = NULL;
  }

  if (log->priv->lines) {
    lines = (char **) g_ptr_array_free (log->priv->lines, FALSE);
    g_strfreev (lines);
    log->priv->lines = NULL;
  }

  G_OBJECT_CLASS (logview_log_parent_class)->finalize (obj);
}

static void
logview_log_class_init (LogviewLogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = do_finalize;

  signals[LOG_CHANGED] = g_signal_new ("log-changed",
                                       G_OBJECT_CLASS_TYPE (object_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (LogviewLogClass, log_changed),
                                       NULL, NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (LogviewLogPrivate));
}

static void
logview_log_init (LogviewLog *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->lines = NULL;
  self->priv->lines_no = 0;
  self->priv->days = NULL;
  self->priv->file = NULL;
  self->priv->mon = NULL;
  self->priv->has_new_lines = FALSE;
}

static void
monitor_changed_cb (GFileMonitor *monitor,
                    GFile *file,
                    GFile *unused,
                    GFileMonitorEvent event,
                    gpointer user_data)
{
  LogviewLog *log = user_data;

  if (event == G_FILE_MONITOR_EVENT_CHANGED) {
    log->priv->has_new_lines = TRUE;
    g_signal_emit (log, signals[LOG_CHANGED], 0, NULL);
  }
  /* TODO: handle the case where the log is deleted? */
}

static void
setup_file_monitor (LogviewLog *log)
{
  GError *err = NULL;

  log->priv->mon = g_file_monitor (log->priv->file,
                                   0, NULL, &err);
  if (err) {
    /* it'd be strange to get this error at this point but whatever */
    g_warning ("Impossible to monitor the log file: the changes won't be notified");
    g_error_free (err);
    return;
  }
  
  /* set the rate to 1sec, as I guess it's not unusual to have more than
   * one line written consequently or in a short time, being a log file.
   */
  g_file_monitor_set_rate_limit (log->priv->mon, 1000);
  g_signal_connect (log->priv->mon, "changed",
                    G_CALLBACK (monitor_changed_cb), log);
}

static void
add_new_days_to_cache (LogviewLog *log, const char **new_lines)
{
  GSList *new_days, *l, *m, *last_cached;
  gboolean append = FALSE;

  new_days = log_read_dates (new_lines, log->priv->file_time.tv_sec);

  /* the days are stored in chronological order, so we compare the last cached
   * one with the new we got.
   */
  last_cached = g_slist_last (log->priv->days);

  if (!last_cached) {
    log->priv->days = new_days;
    return;
  }

  for (l = new_days; l; l = l->next) {
    if (days_compare (l->data, last_cached->data) > 0) {
      /* this day in the list is newer than the last one, append to
       * the cache.
       */
      log->priv->days = g_slist_concat (log->priv->days, l);
      append = TRUE;
      break;
    }
  }

  if (append) {
    /* we need to free the elements before the appended one */
    for (m = new_days; m != l; m = m->next) {
      logview_utils_day_free (m->data);
      g_slist_free_1 (m);
    }
  }
}

static gboolean
new_lines_job_done (gpointer data)
{
  NewLinesJob *job = data;

  if (job->err) {
    job->callback (job->log, NULL, job->err, job->user_data);
    g_error_free (job->err);
  } else {
    job->callback (job->log, job->lines, job->err, job->user_data);
  }

  /* drop the reference we acquired before */
  g_object_unref (job->log);  

  g_slice_free (NewLinesJob, job);

  return FALSE;
}

static gboolean
do_read_new_lines (GIOSchedulerJob *io_job,
                   GCancellable *cancellable,
                   gpointer user_data)
{
  /* this runs in a separate thread */
  NewLinesJob *job = user_data;
  LogviewLog *log = job->log;
  char *line;
  GError *err = NULL;
  GPtrArray *lines;

  g_assert (LOGVIEW_IS_LOG (log));
  g_assert (log->priv->stream != NULL);

  if (!log->priv->lines) {
    log->priv->lines = g_ptr_array_new ();
    g_ptr_array_add (log->priv->lines, NULL);
  }

  lines = log->priv->lines;

  /* remove the NULL-terminator */
  g_ptr_array_remove_index (lines, lines->len - 1);

  while ((line = g_data_input_stream_read_line (log->priv->stream, NULL,
                                                NULL, &err)) != NULL)
  {
    g_ptr_array_add (lines, (gpointer) line);
  }

  if (err) {
    job->err = err;
    goto out;
  }

  /* NULL-terminate the array again */
  g_ptr_array_add (lines, NULL);

  log->priv->has_new_lines = FALSE;

  /* we'll return only the new lines in the callback */
  line = g_ptr_array_index (lines, log->priv->lines_no);
  job->lines = (const char **) lines->pdata + log->priv->lines_no;

  /* save the new number of lines */
  log->priv->lines_no = (lines->len - 2);
  add_new_days_to_cache (log, job->lines);

out:
  g_io_scheduler_job_send_to_mainloop_async (io_job,
                                             new_lines_job_done,
                                             job, NULL);
}

static gboolean
log_load_done (gpointer user_data)
{
  LoadJob *job = user_data;

  if (job->err) {
    /* the callback will have NULL as log, and the error set */
    g_object_unref (job->log);
    job->callback (NULL, job->err, job->user_data);
    g_error_free (job->err);
  } else {
    job->callback (job->log, NULL, job->user_data);
  }

  setup_file_monitor (job->log);
  g_slice_free (LoadJob, job);

  return FALSE;
}

static gboolean
log_load (GIOSchedulerJob *io_job,
          GCancellable *cancellable,
          gpointer user_data)
{
  /* this runs in a separate i/o thread */
  LoadJob *job = user_data;
  LogviewLog *log = job->log;
  GFile *f = log->priv->file;
  GFileInfo *info;
  GFileInputStream *is;
  const char *content_type;
  char *buffer;
  GFileType type;
  GError *err = NULL;

  info = g_file_query_info (f,
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                            G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
                            G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                            G_FILE_ATTRIBUTE_STANDARD_SIZE ","
                            G_FILE_ATTRIBUTE_TIME_MODIFIED ",",
                            0, NULL, &err);
  if (err) {
    if (err->code == G_IO_ERROR_PERMISSION_DENIED) {
      /* TODO: PolicyKit integration */
    }
    job->err = err;
    goto out;
  }

  type = g_file_info_get_file_type (info);
  content_type = g_file_info_get_content_type (info);

  if (type != (G_FILE_TYPE_REGULAR || G_FILE_TYPE_SYMBOLIC_LINK) ||
      !g_content_type_is_a (content_type, "text/plain")) /* TODO: zipped logs */
  {
    err = g_error_new_literal (LOGVIEW_ERROR_QUARK, LOGVIEW_ERROR_NOT_A_LOG,
                               "The file is not a regular file or is not a text file");
    job->err = err;
    goto out;
  }

  log->priv->file_size = g_file_info_get_size (info);
  g_file_info_get_modification_time (info, &log->priv->file_time);
  log->priv->display_name = g_strdup (g_file_info_get_display_name (info));

  g_object_unref (info);

  /* initialize the stream */
  is = g_file_read (f, NULL, &err);

  if (err) {
    if (err->code == G_IO_ERROR_PERMISSION_DENIED) {
      /* TODO: PolicyKit integration */
    }
    job->err = err;
    goto out;
  }

  log->priv->stream = g_data_input_stream_new (G_INPUT_STREAM (is));

out:
  g_io_scheduler_job_send_to_mainloop_async (io_job,
                                             log_load_done,
                                             job, NULL);
  return FALSE;
}

static void
log_setup_load (LogviewLog *log, LogviewCreateCallback callback,
                gpointer user_data)
{
  LoadJob *job;

  job = g_slice_new0 (LoadJob);
  job->callback = callback;
  job->user_data = user_data;
  job->log = log;
  job->err = NULL;

  /* push the loading job into another thread */
  g_io_scheduler_push_job (log_load,
                           job,
                           NULL, 0, NULL);
}

/* public methods */

void
logview_log_read_new_lines (LogviewLog *log,
                            LogviewNewLinesCallback callback,
                            gpointer user_data)
{
  NewLinesJob *job;

  /* initialize the job struct with sensible values */
  job = g_slice_new0 (NewLinesJob);
  job->callback = callback;
  job->user_data = user_data;
  job->log = g_object_ref (log);
  job->err = NULL;
  job->lines = NULL;

  /* push the fetching job into another thread */
  g_io_scheduler_push_job (do_read_new_lines,
                           job,
                           NULL, 0, NULL);
}

void
logview_log_create (const char *filename, LogviewCreateCallback callback,
                    gpointer user_data)
{
  LogviewLog *log = g_object_new (LOGVIEW_TYPE_LOG, NULL);

  log->priv->file = g_file_new_for_path (filename);

  log_setup_load (log, callback, user_data);
}

const char *
logview_log_get_display_name (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->display_name;
}

gulong
logview_log_get_timestamp (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->file_time.tv_sec;
}

goffset
logview_log_get_file_size (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->file_size;
}

guint
logview_log_get_cached_lines_number (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->lines_no;
}

const char **
logview_log_get_cached_lines (LogviewLog *log)
{
  const char ** lines = NULL;

  g_assert (LOGVIEW_IS_LOG (log));

  if (log->priv->lines) {
    lines = (const char **) log->priv->lines->pdata;
  }

  return lines;
}

GSList *
logview_log_get_days_for_cached_lines (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->days;
}

gboolean
logview_log_has_new_lines (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->has_new_lines;
}