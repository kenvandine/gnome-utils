#ifndef __MENUS_H__
#define __MENUS_H__

/* menus.c */

enum {
	MENU_MAIN,
	MENU_NUM
};

void get_menubar(GtkWidget **, GtkAcceleratorTable **, int);
void
menus_set_sensitive (char *path,
		     int   sensitive);
void
menus_set_state (char *path,
		 int   state);
void
menus_set_show_toggle (char *path,
		       int   state);
int menus_get_toggle_state(char *path);
void
menus_activate (char *path);

#endif
