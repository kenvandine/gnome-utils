
#ifndef PARSER_H
#define PARSER_H

#include "menu.h"

/* Not sure which of these will be used in the final thing */

/* Load any systemwide menus that apply, such as 
   system.fvwmrc, or /etc/X11/icewm/menu */
Menu * parse_system_menu();

/* Load any user menus that apply, such as .icewm/menu */
Menu * parse_user_menu();

/* Load both, merged in the way this wm would merge them. */
Menu * parse_combined_menu();

#endif
