/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
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

#ifndef __LOG_LIST_H__
#define __LOG_LIST_H__

#define LOG_LIST_TYPE		  (loglist_get_type ())
#define LOG_LIST(obj)		  (GTK_CHECK_CAST ((obj), LOG_LIST_TYPE, LogList))
#define LOG_LIST_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), LOG_LIST_TYPE, LogListClass))
#define LOG_IS_LIST(obj)	  (GTK_CHECK_TYPE ((obj), LOG_LIST_TYPE))
#define LOG_IS_LIST_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), LOG_LIST_TYPE))
#define LOG_LIST_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), LOG_LIST_TYPE, LogListClass))

typedef struct _LogList LogList;
typedef struct _LogListClass LogListClass;

struct _LogList
{	
    GtkTreeView parent_instance;
    GtkListStore *model;
    gpointer logview;
};

struct _LogListClass
{
	GtkTreeViewClass parent_class;
};

GtkWidget *loglist_new (void);
void loglist_connect (LogList *list, LogviewWindow *window);
void loglist_add_log (LogList *list, Log *log);
void loglist_select_log (LogList *list, Log *log);
void loglist_unbold_log (LogList *list, Log *log);
void loglist_bold_log (LogList *list, Log *log);
void loglist_remove_log (LogList *list, Log *log);

#endif /* __LOG_LIST_H__ */
