#include "menus.h"
#include "gcolorsel.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "view-color-generic.h"
#include "view-color-edit.h"
#include "gcolorsel.h"
#include "session.h"

#include "config.h"
#include "gnome.h"

static GList *prop_list = NULL;  /* List of GnomePropertyBox */

/* File */
static void open_cb    (GtkWidget *widget);
static void save_cb    (GtkWidget *widget);
static void save_as_cb (GtkWidget *widget);
static void revert_cb  (GtkWidget *widget);
static void close_cb   (GtkWidget *widget);
static void exit_cb    (GtkWidget *widget);

/* edit */
static void remove_cb     (GtkWidget *widget, gpointer data);
static void edit_cb       (GtkWidget *widget, gpointer data);
static void properties_cb (GtkWidget *widget, gpointer data);


/* Help */
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

GnomeUIInfo edit_menu[] = {
  GNOMEUIINFO_ITEM_STOCK (N_("New color"), NULL,
			  NULL, GNOME_STOCK_PIXMAP_ADD),
  GNOMEUIINFO_ITEM_STOCK (N_("Remove selected colors"), NULL, 
			  remove_cb, GNOME_STOCK_PIXMAP_REMOVE),
  GNOMEUIINFO_ITEM_NONE (N_("Edit selected color..."), NULL, edit_cb),
  
  GNOMEUIINFO_SEPARATOR,

  GNOMEUIINFO_MENU_PROPERTIES_ITEM (properties_cb, NULL),
			 
			     
  GNOMEUIINFO_END
};

GnomeUIInfo help_menu[] = {
  GNOMEUIINFO_HELP ("gcolorsel"),
  GNOMEUIINFO_MENU_ABOUT_ITEM   (about_cb,   NULL),
  GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE    (file_menu),
  GNOMEUIINFO_MENU_EDIT_TREE    (edit_menu),
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
    
    child = GNOME_MDI_CHILD (mdi_color_file_new ());
    if (child) {
      mdi_color_file_set_filename (MDI_COLOR_FILE (child), filename);
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
  /* Save layout before ... */
  
  session_save (mdi);

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

/****************** Edit Menu ***************************************/

static void 
remove_cb (GtkWidget *widget, gpointer data)
{
  ViewColorGeneric *view;

  view = gtk_object_get_data (GTK_OBJECT (gnome_mdi_get_active_view (mdi)), 
					  "view_object");

  view_color_generic_remove_selected (view);
}

static void
edit_cb (GtkWidget *widget, gpointer data)
{
  mdi_color_generic_append_view_type (MDI_COLOR_GENERIC (gnome_mdi_get_active_child (mdi)), TYPE_VIEW_COLOR_EDIT);
  gnome_mdi_add_view (mdi, gnome_mdi_get_active_child (mdi));
}

/******* Properties ********/

static void
properties_apply_cb (GtkWidget *widget, int page, gpointer data)
{
  ViewColorGeneric *view = NULL, *view2;
  MDIColorGeneric *mcg = NULL, *mcg2;
  gpointer view_data, mcg_data, view2_data, mcg2_data;
  GList *list;
  GtkWidget *widget2;

  switch (page) {
  case 0 : 
    view      = gtk_object_get_data (GTK_OBJECT (widget), "prop-view");
    view_data = gtk_object_get_data (GTK_OBJECT (widget), "prop-view-data");
    VIEW_COLOR_GENERIC_GET_CLASS (view)->apply (view, view_data);
    break;

  case 1:        
    mcg       = gtk_object_get_data (GTK_OBJECT (widget), "prop-document");
    mcg_data  = gtk_object_get_data (GTK_OBJECT (widget), "prop-document-data");
    MDI_COLOR_GENERIC_GET_CLASS  (mcg)->apply  (mcg, mcg_data);
    break;

  default:
    return;
  }

  /* sync other properties */
  
  list = prop_list;
  while (list) {
    widget2 = list->data;

    if (widget2 != widget) {

      view2 = gtk_object_get_data (GTK_OBJECT (widget2), "prop-view");
      if (view2 == view) {
	view2_data = gtk_object_get_data (GTK_OBJECT (widget2), "prop-view-data");
	VIEW_COLOR_GENERIC_GET_CLASS (view2)->sync (view2, view2_data);
      }
      
      mcg2 = gtk_object_get_data (GTK_OBJECT (widget2), "prop-document");
      if (mcg2 == mcg) {
	mcg2_data = gtk_object_get_data (GTK_OBJECT (widget2), "prop-document-data");
	MDI_COLOR_GENERIC_GET_CLASS (mcg2)->sync (mcg2, mcg2_data);
      }

      /* Bugs ... when when view modified and doc sync ... */
      gnome_property_box_set_modified (GNOME_PROPERTY_BOX (widget2), FALSE);
    }      

    list = g_list_next (list);
  }
}

static void
properties_destroy_cb (GtkWidget *widget, gpointer data)
{
  ViewColorGeneric *view;
  MDIColorGeneric *mcg;
  gpointer view_data, mcg_data;

  printf ("Destroy\n");

  view      = gtk_object_get_data (GTK_OBJECT (widget), "prop-view");
  view_data = gtk_object_get_data (GTK_OBJECT (widget), "prop-view-data");
  mcg       = gtk_object_get_data (GTK_OBJECT (widget), "prop-document");
  mcg_data  = gtk_object_get_data (GTK_OBJECT (widget), "prop-document-data");

  VIEW_COLOR_GENERIC_GET_CLASS (view)->close (view, view_data);  
  MDI_COLOR_GENERIC_GET_CLASS  (mcg)->close  (mcg, mcg_data);

  prop_list = g_list_remove (prop_list, widget);
}

static void
properties_changed_cb (gpointer data)
{ 
  gnome_property_box_changed (GNOME_PROPERTY_BOX (data));
}

static void
properties_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget *property;
  ViewColorGeneric *view;
  GtkWidget *vbox;
  MDIColorGeneric *mcg;
  gpointer view_data, mcg_data;
  
  view = gtk_object_get_data (GTK_OBJECT (mdi->active_view), "view_object");
  mcg = VIEW_COLOR_GENERIC (view)->mcg;

  property = gnome_property_box_new ();

  gtk_signal_connect (GTK_OBJECT (property), "apply",
		      GTK_SIGNAL_FUNC (properties_apply_cb), NULL); 
  gtk_signal_connect (GTK_OBJECT (property), "destroy",
		      GTK_SIGNAL_FUNC (properties_destroy_cb), NULL);

  /* view properties */

  vbox = gtk_vbox_new (FALSE, GNOME_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

  view_data = VIEW_COLOR_GENERIC_GET_CLASS (view)->get_control (view, 
		    GTK_VBOX (vbox), properties_changed_cb, property);

  gnome_property_box_append_page (GNOME_PROPERTY_BOX (property), 
				  vbox, gtk_label_new ("View properties"));

  /* document properties */

  vbox = gtk_vbox_new (FALSE, GNOME_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

  mcg_data = MDI_COLOR_GENERIC_GET_CLASS (mcg)->get_control (mcg,
		    GTK_VBOX (vbox), properties_changed_cb, property);

  gnome_property_box_append_page (GNOME_PROPERTY_BOX (property), 
				  vbox, gtk_label_new ("Document properties"));

  gtk_object_set_data (GTK_OBJECT (property), "prop-view", view);
  gtk_object_set_data (GTK_OBJECT (property), "prop-view-data", view_data);
  gtk_object_set_data (GTK_OBJECT (property), "prop-document", mcg);
  gtk_object_set_data (GTK_OBJECT (property), "prop-document-data", mcg_data);

  VIEW_COLOR_GENERIC_GET_CLASS (view)->sync (view, view_data);
  MDI_COLOR_GENERIC_GET_CLASS  (mcg)->sync  (mcg, mcg_data);

  prop_list = g_list_append (prop_list, property);

  gtk_widget_show_all (property);
}

/*********************** View Popup Menu *******************************/

void
menu_view_do_popup (GdkEventButton *event)
{
  static GtkWidget *popup = NULL;

  if (!popup) 
    popup = gnome_popup_menu_new (edit_menu);

  gnome_popup_menu_do_popup (popup, NULL, NULL, event, edit_menu);
}

/************************* ToolBar *************************************/

GnomeUIInfo toolbar [] = {
  GNOMEUIINFO_ITEM_STOCK (N_("Exit"), N_("Exit the program"),
			  exit_cb, GNOME_STOCK_PIXMAP_EXIT),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK (N_("Grab"), N_("Grab a color on the screen"),
			  NULL, GNOME_STOCK_PIXMAP_JUMP_TO),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK (N_("About"), N_("About this application"),
			  about_cb, GNOME_STOCK_PIXMAP_ABOUT),
  GNOMEUIINFO_END
};
