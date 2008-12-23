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

#include <glib.h>
#include <gio/gio.h>

#include "logview-log.h"

G_DEFINE_TYPE (LogviewLog, logview_log, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_LOG, LogviewLogPrivate))

struct _LogviewLogPrivate {
  GFile *file;

  /* stats about the file */
  GTimeVal *file_time;
  goffset file_size;

  /* real content, array of lines */
  char **lines;
};

typedef struct {
  LogviewLog *log;
  GError *err;

  LogviewCreateCallback callback;
  gpointer user_data;
} LoadJob;

static void
logview_log_class_init (LogviewLogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (LogviewLogPrivate));
}

static void
logview_log_init (LogviewLog *self)
{
  self->priv = GET_PRIVATE (self);
}

static gboolean
log_load_done (gpointer user_data)
{
  LoadJob *job = user_data;

  if (job->err) {
    /* the callback will have NULL as log, and the error set */
    g_object_unref (job->log);
    job->callback (NULL, job->err, job->user_data);
  }

  job->callback (job->log, NULL, job->user_data);

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
  g_file_info_get_modification_time (info, log->priv->file_time);

  g_object_unref (info);

  /* try now to read the file */
  g_file_load_contents (f, NULL, &buffer,
                        NULL, NULL, &err);
  if (err) {
    if (err->code == G_IO_ERROR_PERMISSION_DENIED) {
      /* TODO: PolicyKit integration */
    }
    job->err = err;
    goto out;
  }

  /* split into an array of lines */
  log->priv->lines = g_strsplit (buffer, "\n", -1);
  g_free (buffer);

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

void
logview_log_create (const char *filename, LogviewCreateCallback callback,
                    gpointer user_data)
{
  LogviewLog *log = g_object_new (LOGVIEW_TYPE_LOG, NULL);

  log->priv->file = g_file_new_for_path (filename);

  log_setup_load (log, callback, user_data);
}