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

#include <gnome.h>

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

typedef enum {
	FIND_OPTION_END, /*end the option templates list*/
	FIND_OPTION_CHECKBOX_TRUE, /*if the user checks this use the option*/
	FIND_OPTION_CHECKBOX_FALSE, /* if the user checks it don't use the
				       option*/
	FIND_OPTION_TEXT,
	FIND_OPTION_NUMBER,
	FIND_OPTION_TIME,
	FIND_OPTION_GREP
} FindOptionType;

typedef struct _FindOptionTemplate FindOptionTemplate;
struct _FindOptionTemplate {
	FindOptionType type;
	gchar *option; /*the option string to pass to find or whatever*/
	gchar *desc; /*description*/
};

typedef struct _FindOption FindOption;
struct _FindOption {
	/*the index of the template this uses*/
	gint templ;

	/*these guys aren't in a union so that they aren't lost when we
	  change templates*/

	/* true false data */
	gint bool;

	/* this is a char string of the data */
	gchar text[256];

	/* number data */
	gint number;

	/* the time data */
	gchar time[50];
};
	


#endif
