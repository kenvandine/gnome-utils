/*   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MENUS_H__
#define __MENUS_H__

#if 1


#else


/* menus.c */

enum {
	MENU_MAIN,
	MENU_POPUP,
	MENU_NUM
};

void get_menubar(GtkWidget **, GtkAcceleratorTable **, int);
void
menus_set_sensitive (char *path,
		     int   sensitive);
void
menus_set_state (char *path,
		 int   state);
void
menus_set_show_toggle (char *path,
		       int   state);
int menus_get_toggle_state(char *path);
int menus_get_sensitive_state(char *path);
void
menus_activate (char *path);


#endif /* 0 */

#endif
