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

#include "is_image.h"

#include <string.h>

/* This just checks filename extensions, then 
   we also check return value from imlib */

static const gchar * extensions [] = { ".xpm", ".png", ".ppm", 
				       ".pnm", ".jpeg", ".jpg", 
				       ".tiff", ".eim", ".gif", 
				       NULL };

gboolean is_image_file(const gchar * filename)
{
  int i = 0;

  while (extensions[i]) {
    if ( strstr (filename, extensions[i]) ) return TRUE;
    ++i;
  }
  return FALSE;
}
