/* GNOME Search Tool
 * Copyright (C) 1997 George Lebl.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _OUTDLG_H_
#define _OUTDLG_H_

#include <gtk/gtk.h>

#include "gsearchtool.h"


gboolean outdlg_makedlg(gchar name[]);

void outdlg_additem(gchar item[]);

void outdlg_showdlg();

void outdlg_closedlg(GtkWidget * widget, gpointer * data);

void outdlg_clearlist(GtkWidget * widget, gpointer * data);

#endif
