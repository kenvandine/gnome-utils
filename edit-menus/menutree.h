
#ifndef MENUTREE_H
#define MENUTREE_H

#include <gnome.h>
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MENU_TREE(obj) GTK_CHECK_CAST (obj, menu_tree_get_type(), MenuTree)
#define MENU_TREE_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, menu_tree_get_type(), MenuTreeClass)
#define IS_MENU_TREE(obj) GTK_CHECK_TYPE (obj, menu_tree_get_type())

  typedef struct _MenuTree MenuTree;
  typedef struct _MenuTreeClass MenuTreeClass;
  
  struct _MenuTree
  {
    GtkVBox vbox;

    Menu * root_menu;
    GList * selection; /* list of Menu * */
  };
  
  struct _MenuTreeClass
  {
    GtkVBoxClass parent_class;    

    void (* selection_changed)(MenuTree *); 
  };
  
  guint menu_tree_get_type (void);

  GtkWidget * menu_tree_new(Menu * m);

  /* destroys old menu, if any */
  void menu_tree_set_root_menu(MenuTree * mt, Menu * m);


  /* At a default location, or according to selection */
  void menu_tree_insert_menu(MenuTree * mt, Menu * m);
  

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
