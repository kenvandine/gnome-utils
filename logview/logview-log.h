/* logview-log.h
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

/* logview-log.h */

#ifndef _LOGVIEW_LOG
#define _LOGVIEW_LOG

#include <glib-object.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_LOG logview_log_get_type()
#define LOGVIEW_LOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_LOG, LogviewLog))
#define LOGVIEW_LOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_LOG, LogviewLogClass))
#define LOGVIEW_IS_LOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_LOG))
#define LOGVIEW_IS_LOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_LOG))
#define LOGVIEW_LOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_LOG, LogviewLogClass))
  
typedef struct _LogviewLog LogviewLog;
typedef struct _LogviewLogClass LogviewLogClass;
typedef struct _LogviewLogPrivate LogviewLogPrivate;

typedef void (* LogviewCreateCallback) (LogviewLog *log,
                                        GError *error,
                                        gpointer user_data);

#define LOGVIEW_ERROR_QUARK g_quark_from_static_string ("logview-error")

typedef enum {
  LOGVIEW_ERROR_FAILED,
  LOGVIEW_ERROR_NOT_A_LOG
} LogviewErrorEnum;

struct _LogviewLog {
  GObject parent;
  LogviewLogPrivate *priv;
};

struct _LogviewLogClass {
  GObjectClass parent_class;
};

GType logview_log_get_type (void);
LogviewLog* logview_log_new (char *filename, GError **error);

G_END_DECLS

#endif /* _LOGVIEW_LOG */