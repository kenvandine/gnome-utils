/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   edit-menus: Gnome app for editing window manager, panel menus.
 *   This file is a C port of the icewm parser, (C) Marko Macek,
 *    the icewm author.
 *   C port Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>

#include <gnome.h>
 
#include "parser.h"
#include "icewm.h"

#ifndef O_TEXT      /* Not sure what this is, took it from icewm */
#define O_TEXT 0
#endif

Menu * main_menu;
Menu * programs_menu;

/* I've pretty much borrowed this wholesale from icewm itself,
   de-C++-ifying and switching from its YMenu class to my Menu 
   struct. */

/* I've had to comment this heavily to make it understandable to me,
   hope this doesn't offend anyone's aesthetics. */

/* parse_argument can get a quoted string, parse_word just gets 
   alphanumerics with no whitespace */
gchar * parse_argument(gchar *dest, gint maxLen, gchar *p) {
  gchar *d;
  int len = 0;
  int in_str = 0;

  /* This happened anyway by default, but I'm making it explicit */
  g_return_val_if_fail ( p != 0, 0 );  

  /* Strip leading whitespace */
  while (*p && (*p == ' ' || *p == '\t')) {
    p++;
  }

  d = dest;
  len = 0;

  /* while ( not end of string AND not max arg length AND
     (in a quoted region OR (not in white space, end of line)) ) */
  while (*p && len < maxLen &&
	 (in_str || (*p != ' ' && *p != '\t' && *p != '\n')))
    {
      /* If \ continues quoted region on next line */
      if (in_str && *p == '\\' && p[1]) {
	p++;
	*d++ = *p++;
	len++;
      }
      /* If we're entering/exiting a quoted region */
      else if (*p == '"') {
	in_str = !in_str;
	p++;
      } 
      /* Nothing special, just read in the data */
      else {
	*d++ = *p++;
	len++;
      }
    }
  /* Null terminate */
  *d = 0;
  
  /* Return where we left off */
  return p;
}


gchar * parse_word(gchar *word, gint maxlen, gchar *p) 
{
  while ( *p && (*p == ' ' || *p == '\t' 
		 || *p == '\r' || *p == '\n') ) {
    p++;
  }

  while (*p && isalnum(*p) && maxlen > 1) {
    *word++ = *p++;
    maxlen--;
  }

  *word = 0;
  ++word;
  return p;
}


gchar *parse_prog(char * data, Menu * menu, gpointer format_data)
{
  gchar *p = data;
  gchar name[64];
  gchar icon_a[128];
  gchar * icon; /* temporary hack, needed pointer not array fixme */
  Menu * m;

  const gint command_max = 512;
  gchar command[512];
  gint command_length; 

  icon = &icon_a[0];

  g_return_val_if_fail(menu != 0, 0);
 
  /* We are right after "prog" keyword at the 
     start of this function 
     Expecting a name, followed by an icon (or - for none),
     followed by a command line */
 
  /* Get name */
  p = parse_argument(name, sizeof(name), p);
  g_return_val_if_fail ( p != 0, 0 );
  
  /* Get icon filename */
  p = parse_argument(icon, sizeof(icon_a), p);
  g_return_val_if_fail ( p != 0, 0 );
            
  /* Get command line */

  command[0] = '\0'; /* make strlen(command) == 0 */

  /* Strip leading whitespace */
  while (*p && (*p == ' ' || *p == '\t')) {
    p++;
  }

  command_length = 0;
  while (*p) {
    
    if ( command_length == command_max ) {
      command[command_max-1] = '\0';
      g_warning("Command line too long, buffer overflow.\n"
		"Line begins: %s\n", command);
      /* finish line off and break */
      while ( (*p != '\n') && *p ) {
	++p;
      }
      break;
    }
     
    /* check newline */
    if (*p == '\n') {
      if ( strlen(command) == 0 ) {
	g_warning("No command line for menu item %s\n", name);
	return ++p;
      }
      command[command_length] = '\0';
      break;
    }
    
    command[command_length] = *p;
    ++p;
    ++command_length;
  }

  /* Make the menu item */

  if ( strcmp("-", icon) == 0 ) icon = NULL;
  m = menu_command_new ( name, icon, command, format_data );
  menu_folder_add(menu, m);

  return p;
}

/* Main parser loop is here in this function. 
   All the parse_ functions return a pointer to where
   they left off. */

gchar *parse_menus(char * data, Menu * menu) {
    gchar *p = data;
    Menu * m;
    gchar word[32];
    gchar name[64];
    gchar icon[128];

    g_return_val_if_fail(menu != 0, 0);
    
    while (p && *p) {
      
      /* Get the next non-white-space alphanumeric */
      p = parse_word(word, sizeof(word), p);

      if ( strcmp(word, "separator") == 0 ) {
	
	m = menu_separator_new(NULL);
	menu_folder_add(menu, m);
	
      } 
      
      else if ( strcmp(word, "prog") == 0 ) { 
	/* Parse a program line and return pointer
	   where it left off */
	p = parse_prog(p, menu, NULL);
	
      }

      else if ( strcmp(word, "restart") == 0) {
	/* If this item is a "restart" not a "prog",
	   put a special marker in the menu's format_data */	
	p = parse_prog(p, menu, g_strdup(RESTART_NOT_PROG));
      }
      
      else if ( strcmp(word, "menu") == 0 ) {
	
	p = parse_argument(name, sizeof(name), p);
	
	/* note that this doesn't get an icon pixmap file,
	   it gets an icewm builtin like "folder",
	   at least I think so */
	p = parse_argument(icon, sizeof(icon), p);
	
	/* This remains a little mysterious to me */
	if (p == 0)
	  return p;
	
	/* Get next non-white-space, should be {,
	   return null if it fails.*/
	p = parse_word(word, sizeof(word), p);
	if (*p != '{') {
	  g_warning("Expected {, got %s\n", p);
	  return 0;
	}
	p++; /* move past the { */

	/* Create this menu. Store icewm icon in format_data */
	m = menu_folder_new( name, NULL, NULL, g_strdup(icon) );
	menu_folder_add ( menu, m );

	/* Recursively call ourselves to parse menu contents */
	p = parse_menus(p, m);

	if ( MENU_FOLDER_CONTENTS(m) == NULL ) {
	  g_warning("Menu defined with no contents, deleting\n");
	  menu_folder_remove(menu, m);
	  menu_destroy(m);
	}
	/* Successfully parsed submenu, continue loop */
      } 
      /* If we're closing a menu, return. */
      else if (*p == '}') {
	p++;
	return p;
      } 
      /* We're either done or hosed somehow */
      else {
	g_print("Done parsing. Remainder of file " 
		"(should be empty): %s\n", p);
	return NULL;
      }
    } /* while loop */
    
    /* Return where we left off */
    return p;
}


void load_menus(const gchar *menuFile, gboolean programs) {
  int fd;
  struct stat sb;
  gchar * buf;

  g_return_if_fail ( menuFile != NULL );
  g_assert(main_menu != NULL);

  fd = open(menuFile, O_RDONLY | O_TEXT);

  if (fd == -1) {
    return;
  }
  
  if (fstat(fd, &sb) == -1)
    return ;

  buf = (gchar *)g_malloc(sb.st_size + 1);
  if (buf == 0)
    return;

  if (read(fd, buf, sb.st_size) != sb.st_size)
    return;

  buf[sb.st_size] = 0;
  close(fd);

  if (programs) {
    programs_menu = menu_folder_new("Programs", NULL, NULL, NULL);
    
    parse_menus(buf, programs_menu);

    menu_folder_add (main_menu, programs_menu);
  } 
  else {
    parse_menus(buf, main_menu);
  }
  
  g_free(buf);
}

Menu * parse_common(gchar * sysmenufile, gchar * sysprogramsfile,
		    gchar * usermenufile, gchar * userprogramsfile)
{
  main_menu = NULL;
  programs_menu = NULL;

  main_menu = menu_folder_new(ICEWM_TOPLEVEL_MENU, NULL, NULL, NULL);

  if ( usermenufile ) 
    load_menus(usermenufile, FALSE);

  if (MENU_FOLDER_CONTENTS(main_menu) == NULL) {
    if ( sysmenufile )
      load_menus(sysmenufile, FALSE);
  }

  if (userprogramsfile)
    load_menus(userprogramsfile, TRUE);

  if (programs_menu == NULL ) {
    if (sysprogramsfile) 
      load_menus(sysprogramsfile, TRUE);
  }

  return main_menu;
}

/* fixme, shouldn't be hardcoded */

#ifndef DEMO_MODE
#define SYS_MENU "/etc/X11/icewm/menu"
#else
#define SYS_MENU "./icewm-sample-menufile"
#endif
#define SYS_PROG "/etc/X11/icewm/programs"

Menu * parse_system_menu()
{
  return parse_common( SYS_MENU,
		       SYS_PROG,
		       NULL, NULL);
}

void get_home_menus(gchar * menubuf, gchar * progbuf)
{
  gchar * s;

  s = gnome_util_prepend_user_home(".icewm/menu");
  strncpy(menubuf, s, PATH_MAX);
  g_free(s);
  s = gnome_util_prepend_user_home(".icewm/programs");
  strncpy(progbuf, s, PATH_MAX);
  g_free(s);
}

Menu * parse_user_menu()
{
  gchar menubuf[PATH_MAX + 1];
  gchar progbuf[PATH_MAX + 1];

  get_home_menus(menubuf, progbuf);

  return parse_common(NULL, NULL, menubuf, progbuf);
}

Menu * parse_combined_menu()
{
  gchar menubuf[PATH_MAX + 1];
  gchar progbuf[PATH_MAX + 1];

  get_home_menus(menubuf, progbuf);

  return parse_common(SYS_MENU, SYS_PROG, menubuf, progbuf);
}
