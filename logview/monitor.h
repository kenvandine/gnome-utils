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

#ifndef __LOG_MONITOR_H__
#define __LOG_MONITOR_H__

#define MONITORED_LINES 15

void monitor_stop (LogviewWindow *window);
GtkWidget *monitor_create_widget (LogviewWindow *window);
void go_monitor_log (LogviewWindow *window);
void mon_update_display (LogviewWindow *window);
static void mon_format_line (char *buffer, int bufsize, LogLine *line);
gboolean mon_check_logs (gpointer data);
/*void mon_check_logs (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri, 
		const gchar *info_uri, GnomeVFSMonitorEventType event_type, 
		gpointer data);*/

#endif /* __LOG_MONITOR_H__ */
