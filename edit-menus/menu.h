
#ifndef MENU_H
#define MENU_H

#include <gtk/gtk.h>

/* Base class - not for direct use */
typedef struct {
  gchar * name;
  gchar * icon;
  GtkWidget * pixmap; /* GnomePixmap */
  GtkWidget * treeitem; /* GtkTreeItem ??? needed??? */
  gpointer format_data; /* Possibly created by parser,
			   passed through to saver unchanged,
			   unless it's g_freed 
			   Not sure if I need this; parsers 
			   and savers could communicate via
			   global variables instead. */
  gpointer parent;       /* a Menu * */
  guint changeable : 1; /* Don't allow special wm menus, etc.
			   to be changed. FIXME this flag
			   isn't implemented in the source. */
  guint in_destroy : 1; /* being destroyed - don't remove 
			   children */
} MenuItem;

/* Object that's actually used */
/* "Special" is for window manager commands like "restart" 
   Not implemented yet in other parts of source*/
typedef struct {
  enum {Command, Folder, Separator, Special} type;
  union {
    struct {
      MenuItem menuitem;
      GSList * contents;
    } mf;    
    struct {
      MenuItem menuitem;
      gchar * command;
    } mc;
    MenuItem mi; 
  } d;
} Menu;

/* Use it via these macros */

#define IS_MENU_COMMAND(x) ( ((Menu *)(x))->type == Command )
#define IS_MENU_FOLDER(x)    ( ((Menu *)(x))->type == Folder )
#define IS_MENU_SEPARATOR(x) ( ((Menu *)(x))->type == Separator )
#define IS_MENU_SPECIAL(x) ( ((Menu *)(x))->type == Special )

#define MENU_TYPE(x) ( ((Menu *)(x))->type )
#define MENU_NAME(x) ( ((Menu *)(x))->d.mi.name )
#define MENU_ICON(x) ( ((Menu *)(x))->d.mi.icon )
#define MENU_PIXMAP(x) ( ((Menu *)(x))->d.mi.pixmap )
#define MENU_TREEITEM(x) ( ((Menu *)(x))->d.mi.treeitem )
#define MENU_FORMAT(x) ( ((Menu *)(x))->d.mi.format_data )
#define MENU_CHANGEABLE(x)  ( ((Menu *)(x))->d.mi.changeable )
#define MENU_IN_DESTROY(x)  ( ((Menu *)(x))->d.mi.in_destroy )
#define MENU_PARENT(x) ( (Menu *) (((Menu *)(x))->d.mi.parent) )

#define MENU_COMMAND_COMMAND(x)  ( ((Menu *)(x))->d.mc.command )
#define MENU_FOLDER_CONTENTS(x)  ( ((Menu *)(x))->d.mf.contents )



Menu * menu_command_new (gchar * name, 
			 gchar * icon, 
			 gchar * command,
			 gpointer format_data);
Menu * menu_folder_new (gchar * name, 
			gchar * icon, 
			GSList * contents,
			gpointer format_data);
Menu * menu_separator_new(gpointer format_data);
Menu * menu_special_new(gchar * name, 
			gchar * icon,
			gpointer format_data);

void menu_destroy (Menu * m);

/* should be called append, fixme someday */
void menu_folder_add (Menu * folder, Menu * add_me);
void menu_folder_remove (Menu * folder, Menu * remove_me);
void menu_folder_prepend (Menu * folder, Menu * add_me);
void menu_folder_add_after (Menu * folder, Menu * add_me, 
			    Menu * after_me);

void menu_folder_foreach (Menu * folder, GFunc func,
			  gpointer data);
void menu_folder_foreach1 ( Menu * folder, 
			    void (*func)(Menu *) );

void menu_debug_print(Menu * m);

#endif




