
#ifndef ICONBROWSE_H
#define ICONBROWSE_H

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ICON_BROWSE(obj) GTK_CHECK_CAST (obj, icon_browse_get_type(), IconBrowse)
#define ICON_BROWSE_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, icon_browse_get_type(), IconBrowseClass)
#define IS_ICON_BROWSE(obj) GTK_CHECK_TYPE (obj, icon_browse_get_type())

  typedef struct _IconBrowse IconBrowse;
  typedef struct _IconBrowseClass IconBrowseClass;
  
  struct _IconBrowse
  {
    GtkDialog dialog;

    gchar * last_clicked;
    GtkWidget * label;
    GSList * free_me;
    gboolean using_gnome_dir;
  };
  
  struct _IconBrowseClass
  {
    GtkDialogClass parent_class;    

    /* filename chosen is returned, or NULL if cancel was clicked or
       there was some error. */ 
    void (* icon_chosen)(IconBrowse *, gchar *);
  }; 

  guint icon_browse_get_type (void); 
  
  /* "what" is what you're prompting the user for: pixmaps, icons,
     buttons.  It appears in the phrase "Select ____" so you might
     want to include an article.  You can either browse a directory or
     browse a list of files or default to the standard Gnome 
     icon locations. 
     If you're using standard Gnome locations, the widget
     will not display the full path name, though it will return
     it. */

  /* FIXME the "what" interface isn't going to internationalize.
     Needs to be changed. It was dumb anyway.
     (this whole widget should probably use the iconlist
     widget, it's much nicer) */

  GtkWidget * icon_browse_new (const gchar * what);
  GtkWidget * icon_browse_new_from_dir (const gchar * what, 
					const gchar * dir);

  /* filenames/dirs lists are g_freed for you */
  GtkWidget * icon_browse_new_from_filenames (const gchar * what, 
					      GSList * filenames);
  GtkWidget * icon_browse_new_from_dirs (const gchar * what, 
					 GSList * dirs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

