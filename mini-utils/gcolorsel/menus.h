#ifndef MENUS_H
#define MENUS_H

#include "gnome.h"
#include "mdi-color-file.h"

extern GnomeUIInfo main_menu [];
extern GnomeUIInfo toolbar [];

void menu_view_do_popup (GdkEventButton *event);

gint save_file (MDIColorFile *mcf);

#endif
