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

void CalendarMenu (LogviewWindow *window);
void calendar_month_changed (GtkWidget *widget, LogviewWindow *window);
CalendarData* init_calendar_data (LogviewWindow *window);
static void calendar_day_selected (GtkWidget *widget, LogviewWindow *window);
static void calendar_day_selected_double_click (GtkWidget *widget, LogviewWindow *window);
static void close_calendar (GtkWidget * widget, int arg, gpointer client_data);
static DateMark* find_prev_mark (CalendarData*);
static DateMark* find_next_mark (CalendarData*);
static DateMark* get_mark_from_month (CalendarData *data, gint month, gint year);
static DateMark *get_mark_from_date (CalendarData *, gint, gint, gint);

#endif /* __LOG_CALENDAR_H__ */
