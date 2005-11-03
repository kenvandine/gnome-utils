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

#ifndef __LOG_MISC_H__
#define __LOG_MISC_H__

void error_dialog_show (GtkWidget *window, char *main, char *secondary);
void error_dialog_queue (gboolean do_queue);
void error_dialog_show_queued (void);

char *locale_to_utf8 (const char *in);
GDate *string_get_date (char *line);
int string_get_month (const char *str);
char *date_get_string (GDate *date);

#endif /* __LOG_MISC_H__ */

