/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net>
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

#ifndef __LOG_FINDBAR_H__
#define __LOG_FINDBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define LOGVIEW_FINDBAR_TYPE		  (logview_findbar_get_type ())
#define LOGVIEW_FINDBAR(obj)		  (GTK_CHECK_CAST ((obj), LOGVIEW_FINDBAR_TYPE, LogviewFindBar))
#define LOGVIEW_FINDBAR_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), LOGVIEW_FINDBAR_TYPE, LogviewFindBarClass))
#define LOGVIEW_IS_FINDBAR(obj)	          (GTK_CHECK_TYPE ((obj), LOGVIEW_FINDBAR_TYPE))
#define LOGVIEW_IS_FINDBAR_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), LOGVIEW_FINDBAR_TYPE))
#define LOGVIEW_FINDBAR_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), LOGVIEW_FINDBAR_TYPE, LogviewFindBarClass))

typedef struct LogviewFindBarPriv LogviewFindBarPriv;

typedef struct LogviewFindBar
{	
	GtkHBox parent_instance;
	LogviewFindBarPriv *priv;
}LogviewFindBar;

typedef struct LogviewFindBarClass
{
	GtkHBoxClass parent_class;
}LogviewFindBarClass;

GType logview_findbar_get_type (void);
GtkWidget *logview_findbar_new (void);
void logview_findbar_connect (LogviewFindBar *findbar, LogviewWindow *logview);
void logview_findbar_update_visibility (LogviewFindBar *findbar, LogviewWindow *logview);
void logview_findbar_grab_focus (LogviewFindBar *findbar);

G_END_DECLS

#endif /* __LOG_FINDBAR_H__ */

