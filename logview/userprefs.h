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

#ifndef __LOG_PREFS_H__
#define __LOG_PREFS_H__

typedef struct
{
	gchar *logfile;
	GSList *logs;
	int width, height, fontsize;
} UserPrefs;

gboolean prefs_get_have_tearoff (void);
gchar *prefs_get_monospace (void);
gchar *prefs_get_active_log (void);
GSList *prefs_get_logs (void);
int prefs_get_width (void);
int prefs_get_height (void);
void prefs_store_log (gchar *name);
void prefs_store_active_log (gchar *name);
void prefs_store_fontsize (int fontsize);
void prefs_store_window_size (GtkWidget *window);
void prefs_save (void);
void prefs_free_loglist ();
void prefs_init (int argc, char *argv[]);

#endif /* __LOG_PREFS_H__ */

