/*
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
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

#ifndef __LOGVIEW_LOGLIST_H__
#define __LOGVIEW_LOGLIST_H__

#define LOGVIEW_TYPE_LOGLIST logview_loglist_get_type()
#define LOGVIEW_LOGLIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_LOGLIST, LogviewLoglist))
#define LOGVIEW_LOGLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_LOGLIST, LogviewLogListClass))
#define LOGVIEW_IS_LOGLIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_LOGLIST))
#define LOGVIEW_IS_LOGLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_LOGLIST))
#define LOGVIEW_LOGLIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_LOGLIST, LogviewLoglistClass))

#include <gtk/gtk.h>

typedef struct _LogviewLoglist LogviewLoglist;
typedef struct _LogviewLoglistClass LogviewLoglistClass;
typedef struct _LogviewLoglistPrivate LogviewLogListPrivate;

struct _LogviewLoglist
{	
  GtkTreeView parent_instance;
  LogviewLoglistPrivate *priv;
};

struct _LogviewLoglistClass
{
	GtkTreeViewClass parent_class;
};

GType logview_loglist_get_type (void);

/* public methods */
GtkWidget * logview_loglist_new (void);


void loglist_connect (LogList *list, LogviewWindow *window);
void loglist_add_log (LogList *list, Log *log);
void loglist_remove_log (LogList *list, Log *log);
void loglist_select_log (LogList *list, Log *log);
void loglist_bold_log (LogList *list, Log *log);
void loglist_unbold_log (LogList *list, Log *log);

#endif /* __LOGVIEW_LOGLIST_H__ */