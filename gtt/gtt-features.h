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

#ifndef __FEATURES_H__
#define __FEATURES_H__


/*
 * ALWAYS_SHOW_TOGGLE
 * If defined, the gtk_check_menu_item_set_show_toggle will be called with
 * allways set to TRUE.
 * If undefined, the function will never be called.
 * I included this, because the factory will leave this flag FALSE in
 * gtk-0.99.0, but the check is drawn like there should allways be
 * the indicator.
 */
#undef ALLWAYS_SHOW_TOGGLE


/*
 * GNOME_USE_MSGBOX
 * If defined and GNOME support is included, GNOME message boxes will be
 * used. If undefined, my own message boxes will be used, which I designed
 * more to reflect the applications global look (and are transients)
 * If GNOME support is disabled, this define has no effect.
 */
#define GNOME_USE_MSGBOX


/*
 * GNOME_USE_APP
 * If defined and GNOME support is included, GNOME App will be used.
 * 
 * This is still experimetal.
 */
#define GNOME_USE_APP


/*
 * GNOME_USE_MENU_INFO
 * If define, GnomeMenuInfo (used by GnomeApp) is used. This is experimental
 * and will be stripped pretty soon.
 */
#undef GNOME_USE_MENU_INFO


/*
 * GNOME_CHANGE_TOOLBAR
 * GtkToolbar doesn't support changing icons yet.
 * Defining GNOME_CHNAGE_TOOLBAR enables an experimental workaround.
 * I refcomment not to use it. If you want better toolbar support, disable
 * GNOME_USE_APP
 */
#undef GNOME_CHANGE_TOOLBAR



/*
 * EXTENDED_TOOLBAR
 * I only define this when DEBUG defined also. If defined, I add two more
 * buttons to the toolbar, one of which is the quit button, which I'm using
 * very often, when debugging.
 */
#ifdef DEBUG
# define EXTENDED_TOOLBAR
#endif


/*
 * DIALOG_USE_ACCEL
 * If defined, my dialogs will install an accelerator table, using "ENTER"
 * for the "OK" button and "ESC" for "Cancel" (or "OK", if no cancel).
 * This doesn't work right now.
 */
#undef DIALOG_USE_ACCEL



/*
 * Some dependencies
 */

#if !HAS_GNOME
#undef GNOME_USE_MSGBOX
#undef GNOME_USE_APP
#endif

#ifndef GNOME_USE_APP
#undef GNOME_USE_MENU_INFO
#undef GNOME_CHANGE_TOOLBAR
#endif



#endif
