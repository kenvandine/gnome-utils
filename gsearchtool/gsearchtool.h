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

#ifndef _GSEARCHTOOL_H_
#define _GSEARCHTOOL_H_


#include <gtk/gtk.h>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define PIPE_READ_BUFFER 4096
struct finfo {
	gchar file[256],dir[256];
	gboolean subdirs;
	gchar lastmod[50];
	gchar group[50];
	gchar owner[50];
	gchar extraopts[256];
	gboolean methodfind;
	gboolean methodlocate;
	gchar searchtext[256];
	gboolean searchgrep;
	gboolean searchfgrep;
	gboolean searchegrep;
	gchar applycmd[256];
	gboolean mount;
};

/*is there an event present*/
gboolean isevent(void);

/* search for a filename */
void search(GtkWidget * widget, gpointer data);

void browse(GtkWidget * widget, gpointer data);

/*
 * change data boolean to the value of the checkbox to keep the structure
 * allways up to date
 */
void changetb(GtkWidget * widget, gpointer data);

/*
 * change the text to what it is in the entry, so that the structure is
 * allways up to date
 */
void changeentry(GtkWidget * widget, gpointer data);

/* quit */
gint destroy(GtkWidget * widget, gpointer data);


#endif
