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
#define ALLWAYS_SHOW_TOGGLE


/*
 * GNOME_USE_MSGBOX
 * If defined and GNOME support is included, GNOME message boxes will be
 * used. If undefined, my own message boxes will be used, which I designed
 * more to reflect the applications global look (and are transients)
 * If GNOME support is disabled, this define has no effect.
 */
#undef GNOME_USE_MSGBOX


/*
 * GNOME_USE_TOOLBAR
 * If defines and GNOME support is included, GNOME toolbars will be used.
 * blah (see above)
 * 
 * WARNING: I'm not really supporting the GnomeToolbar at the moment.
 *          When the toolbar support of gnome gets better, I may continue
 *          to support it, but not now...
 */
#undef GNOME_USE_TOOLBAR


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
 * This does'nt work right now.
 */
#undef DIALOG_USE_ACCEL



#endif
