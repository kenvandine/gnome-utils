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

#ifndef __LOG_REPAINT_H__
#define __LOG_REPAINT_H__

int InitPages (LogviewWindow *window);
gboolean log_repaint (LogviewWindow *window);
void log_redrawdetail (LogviewWindow *window);
void DrawLogLines (LogviewWindow *window, Log *current_log);
void UpdateStatusArea (LogviewWindow *window);
void change_log (LogviewWindow *window, int direction);
void handle_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
void handle_row_activation_cb (GtkTreeView *treeview, GtkTreePath *path,
			       GtkTreeViewColumn *arg2, gpointer user_data);
void save_rows_to_expand (LogviewWindow *window);

#endif /* __LOG_REPAINT_H__ */

