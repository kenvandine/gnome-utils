
#ifndef SAVER_H
#define SAVER_H

#include <glib.h>
#include "menu.h"

/* The opposite of the parser */
/* Can't be used for system menu, for now */

/* Returns a representation of file(s) written suitable
   for showing the user in a text box, so the user can
   verify it looks good. Stores its own representation
   in global variables or whatever, if it's different
   from the returned gchar * */

/* FIXME returning such a representation won't make
   sense if the "file" written is a bunch of gnome_config
   settings or equivalent. */

gchar * make_user_menu(Menu * menu);

/* Free the gchar * returned by make_user_menu(),
   and actually write the files. */

void write_user_menu(gchar * nondefault_filename);

/* FIXME this API is really not thought out well */

#endif

