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

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#include "run_command.h"

void run_command(gchar * command)
{
  pid_t new_pid;
  
  new_pid = fork();

  switch (new_pid) {
  case -1 :
    g_warning("Command execution failed: fork failed");
    break;
  case 0 : 
    _exit(system(command));
    break;
  default:
    break;
  }
}
