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

#define MON_WINDOW_WIDTH           180
#define MON_WINDOW_HEIGHT          150

#define MON_MAX_NUM_LOGS         4   /* Max. num of logs to monitor       */
#define MON_MODE_VISIBLE         1   /* Displays log as it is monitored   */
#define MON_MODE_INVISIBLE       2   /* Monitors log without output       */
#define MON_MODE_ICONIFIED       3   /* Monitors log showing only an icon */

void MonitorMenu (GtkAction *action, GtkWidget *callback_data);
void mon_edit_actions (GtkWidget *widget, gpointer data);

static void close_monitor_options (GtkWidget * widget, gpointer client_data);
static void monitor_window_destroyed_cb (GtkWidget * widget, gpointer data);
static void go_monitor_log (GtkWidget * widget, gpointer client_data);
static void mon_remove_log (GtkWidget *widget, GtkWidget *foo);
static void mon_add_log (GtkWidget *widget, GtkWidget *foo);
static void mon_read_last_page (Log *log);
static void mon_read_new_lines (Log *log);
static void mon_format_line (char *buffer, int bufsize, LogLine *line);
static void mon_hide_app_checkbox (GtkWidget *widget, gpointer data);
static void mon_actions_checkbox (GtkWidget *widget, gpointer data);
static gint mon_check_logs (LogviewWindow *window);
static int mon_repaint (GtkWidget * widget, GdkEventExpose * event);

#endif /* __LOG_MONITOR_H__ */
