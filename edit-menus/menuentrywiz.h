
#ifndef MENUENTRYWIZ_H
#define MENUENTRYWIZ_H

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MENU_ENTRY_WIZ(obj) GTK_CHECK_CAST (obj, menu_entry_wiz_get_type(), MenuEntryWiz)
#define MENU_ENTRY_WIZ_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, menu_entry_wiz_get_type(), MenuEntryWizClass)
#define IS_MENU_ENTRY_WIZ(obj) GTK_CHECK_TYPE (obj, menu_entry_wiz_get_type())

  typedef struct _MenuEntryWiz MenuEntryWiz;
  typedef struct _MenuEntryWizClass MenuEntryWizClass;
  
  struct _MenuEntryWiz
  {
    GnomePropertyBox property_box;

    /* It is just so ugly to have all these implementation details
       here. Makes you want to write C++ */
    /* Or maybe it's ugly to have this many details :) */
    /* Can possibly eliminate the gchar * and just store 
       in the entries, for example. */
    /* FIXME ah, I saw the solution somewhere. Make a 
       MenuEntryWizInfo struct in the .c file. */
    gchar * icon;
    gchar * name;
    gchar * command;

    GtkWidget * icon_entry;
    GtkWidget * name_entry;
    GtkWidget * command_entry;

    GtkWidget * icon_pixmap;
    GtkWidget * ip_container;
    GtkWidget * icon_container;
    GtkWidget * command_container;
    
    GtkWidget * advanced_container;
    GtkWidget * test_command_button;
  };
  
  struct _MenuEntryWizClass
  {
    GnomePropertyBoxClass parent_class;    

    /* Returns this, name, icon filename, command line 
       Called each time the user applies changes */
    void (* menu_entry)(MenuEntryWiz *, gchar *, gchar *, gchar *); 
  };
  
  guint menu_entry_wiz_get_type (void);
  
  GtkWidget * menu_entry_wiz_new(gboolean ask_for_icon,
				 gboolean ask_for_command);

  /* NULL for anything with no defaults */
  GtkWidget * 
    menu_entry_wiz_new_with_defaults(gboolean ask_for_icon,
				     gboolean ask_for_command,
				     gchar * name,
				     gchar * icon,
				     gchar * command);

  /* Eventually I want to have a "simple" 
     tab that offers a list of preconstructed menu items 
     that came with the distribution, 
     in addition to the "advanced" tab
     which is the currently-implemented widget.
     For non-dist purposes you might want only advanced,
     for other purposes only simple. 

  
  GtkWidget * menu_entry_wiz_advanced(gboolean ask_for_icon);
  GtkWidget * menu_entry_wiz_simple();
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

