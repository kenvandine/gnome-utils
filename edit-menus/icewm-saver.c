/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   edit-menus: Gnome app for editing window manager, panel menus.
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>,
 *    this file based on icewm parser (C) Marko Macek.
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

#include "saver.h"
#include "icewm.h"

static gint recursion_level;
static gchar * user_menu = 0;

static gboolean is_restart(Menu * m)
{
  return ( (MENU_FORMAT(m) != 0) &&
	   (strcmp(MENU_FORMAT(m), RESTART_NOT_PROG) == 0) );
}

static gchar * save_word(gchar * dest, gchar * src)
{
  if ( src == NULL ) {
    g_warning("Null source in save_word\n");
    return dest;
  }

  while (*src) {
    *dest = *src;
    ++dest;
    ++src;
  }
  return dest;
}

static gchar * save_argument(gchar * dest, gchar * src)
{
  *dest = '"'; ++dest;
  dest = save_word(dest, src);
  *dest = '"'; ++dest;
  return dest;
}

static gchar * save_spaces(gchar * dest, gint num)
{
  while ( num > 0 ) {
    *dest = ' ';
    ++dest;
    --num;
  }
  return dest;
}

static gchar * save_prog(gchar * dest, Menu * m)
{
  g_return_val_if_fail( IS_MENU_COMMAND(m), NULL );

  dest = save_spaces(dest, recursion_level);

  if ( is_restart(m) ) {
    dest = save_word(dest, "restart");
  }
  else {
    dest = save_word(dest, "prog");
  }

  *dest = ' '; ++dest;

  if ( MENU_NAME(m) ) {
    dest = save_argument (dest, MENU_NAME(m));
  }
  else {
    g_warning("Menu has no name!\n");
    dest = save_argument (dest, "edit-menus-error:menu-without-name");
  }
  *dest = ' '; ++dest;

  if ( MENU_ICON(m) ) {
    dest = save_word(dest, MENU_ICON(m));
  }
  else {
    dest = save_word(dest, "-");
  }
  
  *dest = ' '; ++dest;

  if ( MENU_COMMAND_COMMAND(m) ) {
    dest = save_argument(dest, MENU_COMMAND_COMMAND(m));
  }
  else {
    g_warning("No command for menu item %s\n", MENU_NAME(m));
  }

  dest = save_word(dest, "\n");

  return dest;
}

static gchar * save_separator(gchar * dest)
{
  dest = save_spaces(dest, recursion_level);
  
  dest = save_word(dest, "separator\n");

  return dest;
}

static gchar * save_menu(gchar * dest, Menu * m)
{
  GSList * contents;
  Menu * submenu;
  gboolean at_toplevel;
  
  g_return_val_if_fail ( IS_MENU_FOLDER(m), NULL );

  if ( strcmp (MENU_NAME(m), ICEWM_TOPLEVEL_MENU) == 0 )
    at_toplevel = TRUE;
  else at_toplevel = FALSE;

  if (!at_toplevel) {

    dest = save_spaces(dest, recursion_level);
    ++recursion_level;
    
    dest = save_word(dest, "menu");
    
    *dest = ' '; ++dest;
    
    if ( MENU_NAME(m) ) {
      dest = save_argument(dest, MENU_NAME(m));
    }
    else {
      g_warning("Menu has no name!\n");
    }

    *dest = ' '; ++dest;

    /* The parser saves the "folder" keyword as
       format_data. Not sure icewm allows arbitrary
       icons for folders. */
    if ( MENU_ICON(m) ) {
      dest = save_word(dest, MENU_ICON(m));
    }
    else if ( MENU_FORMAT(m) ) {
      dest = save_word(dest, MENU_FORMAT(m) );
    }
    else {
      dest = save_word(dest, "-");
    }
    
    dest = save_word(dest, " {\n");

  } /* if (! at_toplevel) */
  
  contents = MENU_FOLDER_CONTENTS(m);
  
  while ( contents ) {

    submenu = (Menu *)contents->data;

    if ( IS_MENU_FOLDER(submenu) ) {      
      dest = save_menu(dest, submenu);
    }
    else if ( IS_MENU_COMMAND(submenu) ) {
      dest = save_prog(dest, submenu);
    }
    else if ( IS_MENU_SEPARATOR(submenu) ) {
      dest = save_separator(dest);
    }
    else {
      g_warning("Unknown Menu struct!\n");
      g_assert_not_reached();
    }
    
    contents = g_slist_next(contents);
  }

  if (!at_toplevel) {
    --recursion_level;
    dest = save_spaces(dest, recursion_level);
    dest = save_word(dest, "}\n");
  }

  return dest;  
}

gchar * make_user_menu(Menu * m)
{
  gchar * end;

  /* does nothing on first invocation of course */
  g_free (user_menu);

  user_menu = g_malloc(100000); /* fixme fixme fixme !!! */

  recursion_level = 0;

  end = save_menu(user_menu, m);
  *end = '\0';

  if ( recursion_level != 0 ) {
    g_warning("Possible problem: recursion level ended at %d\n",
	      recursion_level );
  }

  return user_menu;
}

void write_user_menu(gchar * nondefault_filename)
{
  

}
