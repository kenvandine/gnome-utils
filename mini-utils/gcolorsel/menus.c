#include "menu.h"
#include "gcolorsel.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "widget-color-list.h"
#include "widget-color-grid.h"
#include "gcolorsel.h"

#include "config.h"
#include "gnome.h"

static void open_cb    (GtkWidget *widget);
static void save_cb    (GtkWidget *widget);
static void save_as_cb (GtkWidget *widget);
static void revert_cb  (GtkWidget *widget);
static void close_cb   (GtkWidget *widget);
static void exit_cb    (GtkWidget *widget);

static void about_cb   (GtkWidget *widget);

GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_OPEN_ITEM    (open_cb,    NULL),
  GNOMEUIINFO_MENU_SAVE_ITEM    (save_cb,    NULL),
  GNOMEUIINFO_MENU_SAVE_AS_ITEM (save_as_cb, NULL),
  GNOMEUIINFO_MENU_REVERT_ITEM  (revert_cb,  NULL),
  
  GNOMEUIINFO_SEPARATOR,
  
  GNOMEUIINFO_MENU_CLOSE_ITEM   (close_cb,   NULL),
  GNOMEUIINFO_MENU_EXIT_ITEM    (exit_cb,    NULL),
  
  GNOMEUIINFO_END
};

GnomeUIInfo help_menu[] = {
  GNOMEUIINFO_MENU_ABOUT_ITEM   (about_cb,   NULL),
  GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE    (file_menu),
  GNOMEUIINFO_MENU_HELP_TREE    (help_menu),
  GNOMEUIINFO_END
};

static void
file_selection_ok_cb (GtkWidget *widget, gboolean *cancel)
{
  *cancel = FALSE;

  gtk_main_quit ();
}

static gint
file_selection_delete_event_cb (GtkWidget *widget)
{
  gtk_main_quit ();
  
  return TRUE;
}

static void 
open_cb (GtkWidget *widget)
{
  GtkFileSelection *fs;
  gboolean cancel = TRUE;
  GnomeMDIChild *child;
  char *filename;

  fs = GTK_FILE_SELECTION (gtk_file_selection_new (_("Open file")));
  gtk_file_selection_hide_fileop_buttons (fs);
  gtk_window_set_modal (GTK_WINDOW (fs), TRUE);

  gtk_signal_connect (GTK_OBJECT (fs->ok_button), "clicked", 
		      GTK_SIGNAL_FUNC (file_selection_ok_cb), &cancel);
  gtk_signal_connect (GTK_OBJECT (fs->cancel_button), "clicked", 
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  gtk_signal_connect (GTK_OBJECT (fs), "delete_event",
		      GTK_SIGNAL_FUNC (file_selection_delete_event_cb), NULL);
  
  gtk_widget_show (GTK_WIDGET (fs));
  gtk_main ();

  if (!cancel) {
    filename = gtk_file_selection_get_filename (fs);
    
    child = GNOME_MDI_CHILD (mdi_color_file_new (filename));
    if (child) {
      gnome_mdi_add_child (mdi, child);
      gnome_mdi_add_view (mdi, child);
      mdi_color_file_load (MDI_COLOR_FILE (child));
    } else 
      gnome_app_error (mdi->active_window, _("Can not open file !"));     
  }

  gtk_widget_destroy (GTK_WIDGET (fs));
}

static void 
save_cb (GtkWidget *widget)
{

}

static void 
save_as_cb (GtkWidget *widget)
{

}

static void 
revert_cb (GtkWidget *widget)
{

}

static void 
close_cb (GtkWidget *widget)
{
  GnomeMDIChild *child = gnome_mdi_get_active_child (mdi);
  GtkWidget     *view  = gnome_mdi_get_active_view  (mdi);

  if ((child->views->data == view) && (!child->views->next)) {
    printf ("1\n");
    gnome_mdi_remove_child (mdi, child, FALSE);
  } else {
    printf ("2\n");
    gnome_mdi_remove_view (mdi, view, FALSE);
  }
}

static void 
exit_cb (GtkWidget *widget)
{
  gtk_main_quit ();
}

static void
about_cb (GtkWidget *widget)
{
  static GtkWidget *about = NULL;
  
  gchar *authors[] = {
    "Tim P. Gerla",
    "Eric Brayeur (eb2@ibelgique.com)",
    NULL
  };

  if (about) {
    gtk_widget_show_now (about);
    gdk_window_raise (about->window);
    return;
  }
  
  about = gnome_about_new (_("Gnome Color Browser"), VERSION,
			   "(C) 1997-98 Tim P. Gerla", 
			   (const gchar**)authors,
			   _("Small utility to browse available X11 colors."),
			   NULL);                                
  
  gtk_signal_connect (GTK_OBJECT (about), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &about);
  
  gtk_widget_show(about);
}

/*********************************************************/

static void remove_cb  (GtkWidget *widget, gpointer data);

GnomeUIInfo edit_menu[] = {
  GNOMEUIINFO_ITEM_STOCK (N_("Remove"), NULL, 
			  remove_cb, GNOME_STOCK_PIXMAP_REMOVE),
  GNOMEUIINFO_END
};

GnomeUIInfo mdi_menu[] = {
  GNOMEUIINFO_MENU_EDIT_TREE    (edit_menu),
  GNOMEUIINFO_END
};

static void remove_cb (GtkWidget *widget, gpointer data)
{
  MDIColorGenericCol *col;
  GList *list, *freezed = NULL;
  GtkWidget *view;

  view = gtk_object_get_data (GTK_OBJECT (mdi->active_view), "data_widget");

  if (GTK_IS_CLIST (view)) {
    GtkCList *clist = GTK_CLIST (view);
    list = clist->selection;

    while (list) {  
      col = gtk_clist_get_row_data (clist, GPOINTER_TO_INT (list->data));
      
      col = mdi_color_generic_get_owner (col);
      
      mdi_color_generic_freeze (col->owner);
      mdi_color_generic_remove (col->owner, col);
      
      freezed = g_list_append (freezed, col->owner);
      
      list = g_list_next (list);
    }   
  } 

  else

    if (IS_COLOR_GRID (view)) {      
      ColorGrid *cg = COLOR_GRID (view);
      ColorGridCol *c;
      list = cg->selected;

      while (list) {
	c = list->data;
	col = mdi_color_generic_get_owner (c->col);

	mdi_color_generic_freeze (col->owner);
	mdi_color_generic_remove (col->owner, col);
      
	freezed = g_list_append (freezed, col->owner);

	list = g_list_next (list);
      }
    }

    else      
      g_assert_not_reached ();

  list = freezed;
  while (list) {
    mdi_color_generic_thaw (freezed->data);
    list = g_list_next (list);
  }    

  g_list_free (freezed);
}
