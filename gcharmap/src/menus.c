/*
 *  Gnome Character Map
 *  menus.c - Menus for the main window
 *
 *  Copyright (C) Hongli Lai
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MENUS_C_
#define _MENUS_C_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "menus.h"
#include "callbacks.h"

GnomeUIInfo characters_menu[] =
{
    GNOMEUIINFO_ITEM_STOCK (N_("_Browse..."),
      			    N_("Insert character(s) by choosing character codes."), 
			    cb_insert_char_click,
			    GNOME_STOCK_MENU_SEARCH),
    GNOMEUIINFO_MENU_EXIT_ITEM (cb_exit_click, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo edit_menu[] =
{
    GNOMEUIINFO_MENU_CUT_ITEM (cb_cut_click, NULL),
    GNOMEUIINFO_MENU_COPY_ITEM (cb_copy_click, NULL),
    GNOMEUIINFO_MENU_PASTE_ITEM (cb_paste_click, NULL),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLEAR_ITEM (cb_clear_click, NULL),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_SELECT_ALL_ITEM (cb_select_all_click, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo help_menu[] =
{
    GNOMEUIINFO_HELP ("gnome-character-map"),
    GNOMEUIINFO_MENU_ABOUT_ITEM (cb_about_click, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo menubar[] =
{	
    { GNOME_APP_UI_SUBTREE, N_("Ch_aracters"), NULL,
      characters_menu, NULL, NULL, (GnomeUIPixmapType) 0,
      NULL, 0, (GdkModifierType) 0, NULL },
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_HELP_TREE(help_menu),
    GNOMEUIINFO_END
};

GnomeUIInfo toolbar[] = {
    /*GNOMEUIINFO_ITEM(N_("Insert"), N_("Insert character(s) by choosing character codes"),
      cb_insert_char_click, NULL),
    GNOMEUIINFO_SEPARATOR,*/
    GNOMEUIINFO_ITEM_STOCK(N_("Cut"), N_("Cut the selection"),
      cb_cut_click, GTK_STOCK_CUT),
    GNOMEUIINFO_ITEM_STOCK(N_("Copy"), N_("Copy the selection"),
      cb_copy_click, GTK_STOCK_COPY),
    GNOMEUIINFO_ITEM_STOCK(N_("Paste"), N_("Paste the clipboard"),
      cb_paste_click, GTK_STOCK_PASTE),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
};


#endif /* _MENUS_C_ */
