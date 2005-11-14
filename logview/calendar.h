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

#ifndef __LOG_CALENDAR_H__
#define __LOG_CALENDAR_H__

#include "logrtns.h"

G_BEGIN_DECLS

#define CALENDAR_TYPE		  (calendar_get_type ())
#define CALENDAR(obj)		  (GTK_CHECK_CAST ((obj), CALENDAR_TYPE, Calendar))
#define CALENDAR_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), CALENDAR_TYPE, CalendarClass))
#define IS_CALENDAR(obj)	  (GTK_CHECK_TYPE ((obj), CALENDAR_TYPE))
#define IS_CALENDAR_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), CALENDAR_TYPE))
#define CALENDAR_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), CALENDAR_TYPE, CalendarClass))

typedef struct CalendarPriv CalendarPriv;
typedef struct Calendar
{	
	GtkCalendar parent_instance;
	CalendarPriv *priv;
	GList *days;
	gboolean first_pass;
} Calendar;

typedef struct CalendarClass
{
	GtkCalendarClass parent_class;
}CalendarClass;

GtkWidget *calendar_new (void);
void calendar_select_date (Calendar *calendar, GDate *date);
void calendar_init_data (Calendar *calendar, Log *log);
void calendar_connect (Calendar *calendar, LogviewWindow *window);

G_END_DECLS

#endif /* __LOG_CALENDAR_H__ */
