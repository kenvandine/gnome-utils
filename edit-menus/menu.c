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

#include "menu.h"

static Menu * menu_new(gchar * name, gchar * icon, 
		       gpointer format_data)
{
  Menu * m;
  m = g_malloc(sizeof(Menu));

  MENU_NAME(m) = g_strdup(name);
  MENU_ICON(m) = g_strdup(icon);
  MENU_FORMAT(m) = format_data;

  MENU_PIXMAP(m) = NULL;
  MENU_TREEITEM(m) = NULL;
  MENU_PARENT(m) = NULL;

  MENU_IN_DESTROY(m) = FALSE;
  MENU_CHANGEABLE(m) = FALSE;

  return m;
}

Menu * menu_command_new (gchar * name, 
			 gchar * icon, 
			 gchar * command,
			 gpointer format_data)
{
  Menu * m;

  m = menu_new(name, icon, format_data);

  MENU_TYPE(m) = Command;
  MENU_COMMAND_COMMAND(m) = g_strdup(command);

  g_assert( IS_MENU_COMMAND(m) );

  return m;
}

Menu * menu_folder_new (gchar * name, 
			gchar * icon, 
			GSList * contents,
			gpointer format_data)
{
  Menu * m;

  m = menu_new(name, icon, format_data);

  MENU_TYPE(m) = Folder;
  MENU_FOLDER_CONTENTS(m) = contents;
  
  g_assert( IS_MENU_FOLDER(m) );

  return m;
}

Menu * menu_separator_new(gpointer format_data)
{
  Menu * m;

  m = menu_new("-----------", NULL, NULL);

  MENU_TYPE(m) = Separator;

  g_assert( IS_MENU_SEPARATOR(m) );

  return m;
}

Menu * menu_special_new(gchar * name, 
			gchar * icon,
			gpointer format_data)
{
  Menu * m;

  m = menu_new(name, icon, format_data);

  MENU_TYPE(m) = Special;

  g_assert( IS_MENU_SPECIAL(m) );

  return m;
}

void menu_destroy_one ( gpointer menu, gpointer data )
{
  g_return_if_fail ( menu != NULL );
  menu_destroy((Menu *)menu);
}

void menu_destroy (Menu * m)
{
  g_return_if_fail ( m != 0 );

  MENU_IN_DESTROY(m) = TRUE;

  if ( MENU_PARENT(m) ) {
    menu_folder_remove(MENU_PARENT(m), m);
  }

  g_free ( MENU_NAME(m) );
  g_free ( MENU_ICON(m) );

  /* The pixmap and treeitem should be destroyed with
     their respective containers, once we unref them */
  if (MENU_PIXMAP(m)) gtk_widget_unref(MENU_PIXMAP(m));
  if (MENU_TREEITEM(m)) gtk_widget_unref(MENU_TREEITEM(m));

  if ( IS_MENU_FOLDER(m) ) {
    g_slist_foreach ( MENU_FOLDER_CONTENTS(m),
		      menu_destroy_one, NULL );

    g_slist_free ( MENU_FOLDER_CONTENTS(m) );
  }
  
  else if ( IS_MENU_COMMAND(m) ) {
    g_free ( MENU_COMMAND_COMMAND(m) );
  }
  
  else if ( IS_MENU_SEPARATOR(m) || IS_MENU_SPECIAL(m) ) {
    /* do nothing for now */
  }  
  else {
    g_assert_not_reached();
  }
  
  g_free( MENU_FORMAT(m) );

  g_free (m);
}

void menu_folder_add (Menu * folder, Menu * add_me)
{
  g_return_if_fail ( IS_MENU_FOLDER(folder) );
  g_return_if_fail ( MENU_PARENT(add_me) == NULL );
  g_return_if_fail ( ! MENU_IN_DESTROY(folder) );
  
  MENU_PARENT(add_me) = folder;

  MENU_FOLDER_CONTENTS(folder) = 
    g_slist_append( MENU_FOLDER_CONTENTS(folder),
		    add_me );
}

void menu_folder_remove (Menu * folder, Menu * remove_me)
{
  g_return_if_fail ( IS_MENU_FOLDER(folder) );

  if ( ! MENU_IN_DESTROY(folder) ) {
    MENU_FOLDER_CONTENTS(folder) = 
      g_slist_remove ( MENU_FOLDER_CONTENTS(folder),
		       remove_me );
  }

  MENU_PARENT(remove_me) = NULL;
}

void menu_folder_prepend (Menu * folder, Menu * add_me)
{
  g_return_if_fail ( IS_MENU_FOLDER(folder) );
  g_return_if_fail ( MENU_PARENT(add_me) == NULL );
  g_return_if_fail ( ! MENU_IN_DESTROY(folder) );
  
  MENU_PARENT(add_me) = folder;

  MENU_FOLDER_CONTENTS(folder) = 
    g_slist_prepend( MENU_FOLDER_CONTENTS(folder),
		    add_me );
}

void menu_folder_add_after (Menu * folder, Menu * add_me, 
			    Menu * after_me)
{
  GSList * where;

  g_return_if_fail ( IS_MENU_FOLDER(folder) );
  g_return_if_fail ( ! MENU_IN_DESTROY(folder) );
  g_return_if_fail ( add_me != NULL );
  g_return_if_fail ( after_me != NULL );

  where = g_slist_find(MENU_FOLDER_CONTENTS(folder), after_me);
  
  if ( where == NULL ) {
    g_warning("Attempt to insert after nonexistent folder!");
    return;
  }

  MENU_PARENT(add_me) = folder;
 
  /* after_me should be at 0, so we put our menu at 1 */
  g_slist_insert(where, add_me, 1);
}

void menu_folder_foreach (Menu * folder, GFunc func, 
				 gpointer data)
{
  GSList * contents;
  Menu * m;

  g_return_if_fail ( IS_MENU_FOLDER(folder) );

  contents = MENU_FOLDER_CONTENTS(folder);
  
  while ( contents ) {

    m = (Menu *)contents->data;

    if ( IS_MENU_FOLDER(m) ) {
      g_print("Entering subfolder in foreach\n");
      menu_folder_foreach(m, func, data);
      g_print("Leaving subfolder\n");
    }

    (* func)(m, data);
    
    contents = g_slist_next(contents);
  }
}

void menu_folder_foreach1 ( Menu * folder, 
				   void (* func)(Menu *) )
{
  GSList * contents;
  Menu * m;

  g_return_if_fail ( IS_MENU_FOLDER(folder) );

  contents = MENU_FOLDER_CONTENTS(folder);
  
  while ( contents ) {

    m = (Menu *)contents->data;

    if ( IS_MENU_FOLDER(m) ) {
      g_print("Entering subfolder in foreach1\n");
      menu_folder_foreach1(m, func);
      g_print("Leaving subfolder\n");
    }

    (* func)(m);
    
    contents = g_slist_next(contents);
  }
}

static void menu_debug_print_one(Menu * m)
{
  g_print("Menu: \n ");
  g_print("Name: %s\n ", MENU_NAME(m));
  g_print("Icon: %s\n ", MENU_ICON(m));
  if ( IS_MENU_COMMAND(m) ) {
    g_print("Command: %s\n", MENU_COMMAND_COMMAND(m));
  }
  else if ( IS_MENU_FOLDER(m) ) {
    g_print("Folder. Contents above.\n");
  }
  else if ( IS_MENU_SEPARATOR(m) ) {
    g_print("-------------Separator\n");
  }
  else if ( IS_MENU_SPECIAL(m) ) {
    g_print("Special window manager menu\n");
  }
  else {
    g_print("**** Unknown type.\n");
  }
}

void menu_debug_print(Menu * m)
{
  if ( IS_MENU_FOLDER(m) ) {
    menu_folder_foreach1(m, menu_debug_print_one);
  }
  else {
    menu_debug_print_one(m);
  }
}


