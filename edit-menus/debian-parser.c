/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   edit-menus: Gnome app for editing window manager, panel menus.
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */


/* This file isn't written, feel free. :) */


#include "parser.h"

typedef struct {
  enum { need_x11, need_text, 
	 need_vc, need_wm } needs;
  gchar * section;
  gchar * title;
  gchar * command;
} debian_menu;      

#define MAX_SECTION 100
#define MAX_TITLE 100
#define MAX_COMMAND 300

#define NEED_X11_STR "x11"
#define NEED_TEXT_STR "text"
#define NEED_VC_STR   "vc"
#define NEED_WM_STR   "wm"

debian_menu * parse_entry(gchar * entry);

Menu * parse_system_menu(void)
{


}

Menu * parse_user_menu(void)
{


}

debian_menu * parse_entry(gchar * entry)
{
  debian_menu * dm;
  gchar needs[30];
  int scan_return;

  dm = g_malloc(sizeof(debian_menu));
  dm->section = g_malloc(MAX_SECTION);
  dm->title = g_malloc(MAX_TITLE);
  dm->command = g_malloc(MAX_COMMAND);

  /* fixme Add width limits */
  scan_return = 
    sscanf( entry, 
	    "needs=%s section=\"%s\" title=\"%s\" command=\"%s\"",
	    needs, dm->section, dm->title, dm->command);

  if ( scan_return != 4 ) {
    g_warning("Parse error on line %s\n", entry);
    return NULL;
  }
  
  if ( strcmp (needs, NEED_X11_STR) == 0 ) {
    dm->needs = need_x11;
  }  
  else if ( strcmp (needs, NEEDS_TEXT_STR) == 0 ) {
    dm->needs = need_text;
  }
  else if ( strcmp (needs, NEEDS_VC_STR) == 0 ) {
    dm->needs = need_vc;
  }
  else if ( strcmp (needs, NEEDS_WM_STR) == 0 ) {
    dm->needs = need_wm;
  }
  else {
    g_warning("Needs string not recognized: %s\n", needs);
  }

  /* Shrink strings as needed fixme*/

  return dm;
}
