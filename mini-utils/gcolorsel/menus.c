#include "config.h"

#include "menus.h"
#include "gcolorsel.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "view-color-generic.h"
#include "view-color-edit.h"
#include "gcolorsel.h"
#include "session.h"
#include "utils.h"
#include "idle.h"
#include "dialogs.h"

#include "gnome.h"

static GList *prop_list = NULL;  /* List of GnomePropertyBox */

/* New */
static void new_doc_cb  (GtkWidget *widget);
static void new_view_cb (GtkWidget *widget);

/* Close */
static void close_doc_cb (GtkWidget *widget);
static void close_view_cb (GtkWidget *widget);

/* File */
static void open_cb    (GtkWidget *widget);
static void save_cb    (GtkWidget *widget);
static void save_as_cb (GtkWidget *widget);
static void revert_cb  (GtkWidget *widget);
static void exit_cb    (GtkWidget *widget);

/* edit */
static void new_color_cb  (GtkWidget *widget);
static void remove_cb     (GtkWidget *widget);
static void edit_cb       (GtkWidget *widget);
static void properties_cb (GtkWidget *widget);

/* Help */
static void about_cb   (GtkWidget *widget);

/* Other */
static void grab_cb    (GtkWidget *widget);

GnomeUIInfo new_menu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM     (N_("New Document"), 
				 N_("Create a new document"),
				 new_doc_cb, NULL),
  GNOMEUIINFO_MENU_NEW_ITEM     (N_("New View"),
				 N_("Create a new view for a document"),
				 new_view_cb, NULL),
  GNOMEUIINFO_END
};

GnomeUIInfo close_menu[] = {
  GNOMEUIINFO_ITEM_STOCK       (N_("Close document"),
				N_("Close the current document"),
				close_doc_cb, GNOME_STOCK_MENU_CLOSE),
  GNOMEUIINFO_ITEM_STOCK        (N_("Close view"),
				N_("Close the current view"),
				close_view_cb, GNOME_STOCK_MENU_CLOSE),
  GNOMEUIINFO_END
};

GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_NEW_SUBTREE  (new_menu),

  GNOMEUIINFO_SEPARATOR,
  
  GNOMEUIINFO_MENU_OPEN_ITEM    (open_cb,    NULL),
  GNOMEUIINFO_MENU_SAVE_ITEM    (save_cb,    NULL),
  GNOMEUIINFO_MENU_SAVE_AS_ITEM (save_as_cb, NULL),
  GNOMEUIINFO_MENU_REVERT_ITEM  (revert_cb,  NULL),
  
  GNOMEUIINFO_SEPARATOR,

  GNOMEUIINFO_SUBTREE_STOCK     (N_("Close"), close_menu,
				 GNOME_STOCK_MENU_CLOSE),

  GNOMEUIINFO_SEPARATOR,
  
  GNOMEUIINFO_MENU_EXIT_ITEM    (exit_cb,    NULL),
  
  GNOMEUIINFO_END
};

GnomeUIInfo edit_menu[] = {
  GNOMEUIINFO_ITEM_STOCK (N_("New color"), NULL,
			  new_color_cb, GNOME_STOCK_PIXMAP_ADD),
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

/********************************* New ******************************/

static void
new_doc_cb (GtkWidget *widget)
{
  MDIColorFile *file;

  file = MDI_COLOR_FILE (mdi_color_file_new ());
  mdi_color_generic_set_name (MDI_COLOR_GENERIC (file), _("New"));
  
  gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (file));
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (file));

  msg_flash (mdi, _("Document created ..."));
}

static void
new_view_cb (GtkWidget *widget)
{
  dialog_new_view ();
}

static gint
save_fail (char *filename)
{
  GtkWidget *dia;
  char *str;

  str = g_strconcat (_("GColorsel was unable to save the file :"),
		     "\n\n",
		     filename,
		     "\n\n",
		     _("Make sure that the path you provided exists,\nand that you have the appropriate write permissions."), 
		     "\n\n",
		     _("Do you want to retry ?"),
		     NULL);
  dia = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,

			       GNOME_STOCK_BUTTON_YES, 
			       GNOME_STOCK_BUTTON_NO,
			       GNOME_STOCK_BUTTON_CANCEL,
			       NULL);
  g_free (str);

  return gnome_dialog_run_and_close (GNOME_DIALOG (dia));
}

static void
load_fail (char *filename)
{
  GtkWidget *dia;
  char *str;

  str = g_strconcat (_("Gcolorsel was unable to load the file :"), 
		     "\n\n",
		     filename,
		     "\n\n",
		     _("Make sure that the path you provided exists,\nand that you have permissions for opening the file."), 
		     "\n\n",
		     _("May be this is not a palette file ..."),
		     NULL);
  dia = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,

			       GNOME_STOCK_BUTTON_OK, NULL);
  g_free (str);

  gnome_dialog_run_and_close (GNOME_DIALOG (dia));
}

/******************************* Open *******************************/

static void 
open_cb (GtkWidget *widget)
{
  GtkFileSelection *fs;
  gboolean cancel = TRUE, ret;
  MDIColorFile *file;
  char *filename;

  fs = GTK_FILE_SELECTION (gtk_file_selection_new (_("Open Palette")));
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
    
    file = MDI_COLOR_FILE (mdi_color_file_new ());
    mdi_color_file_set_filename (file, filename);
    mdi_color_generic_set_name (MDI_COLOR_GENERIC (file), 
				g_basename (filename));

    gtk_widget_destroy (GTK_WIDGET (fs));
    
    msg_push (mdi, _("Loading file, please wait ..."));
    mdi_set_sensitive (mdi, FALSE);
    idle_block (); /* without that gtk_flush can take a long long time ... */
    gtk_flush ();

    ret = mdi_color_file_load (file, mdi);

    msg_pop (mdi);
    mdi_set_sensitive (mdi, TRUE);    
    idle_unblock ();
    
    if (ret) {
      gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (file));
      gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (file));
    } else {
      load_fail (file->filename);
      gtk_object_unref (GTK_OBJECT (file));
    }
  } else
    gtk_widget_destroy (GTK_WIDGET (fs));
}

/**************************** Save ************************************/

static char *
save_as_get_filename (void)
{
  GtkFileSelection *fs;
  gboolean cancel = TRUE;
  char *tmp;

  fs = GTK_FILE_SELECTION (gtk_file_selection_new (_("Save Palette as")));
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

  if (cancel) 
    tmp = NULL;
  else 
    tmp = g_strdup (gtk_file_selection_get_filename (fs));

  gtk_widget_destroy (GTK_WIDGET (fs));

  return tmp;
}

/* Ok : 0 ; Cancel : -1 ; Retry : 1 ; No Retry : 2 */
gint
save_file (MDIColorFile *mcf)
{
  gboolean change_name = FALSE;
  gint ret;
  
  if (! mcf->filename) {
    if (!(mcf->filename = save_as_get_filename ())) return -1;    
    change_name = TRUE;    
  }
  
  if (! mdi_color_file_save (mcf)) {
    ret = save_fail (mcf->filename);
    
    if (change_name) {
      g_free (mcf->filename);
      mcf->filename = NULL;
    }

    if ((ret >= 0) && (ret <= 2)) {

      msg_flash (mdi, _("File not saved ..."));

      switch (ret) {
      case 0: /* Yes (retry) */ return 1;
      case 1: /* No (retry)  */ return 2;
      case 2: /* Cancel      */ return -1;
      }
    }
  }
  
  msg_flash (mdi, _("File saved ..."));
  
  return 0;
}

static void 
save_cb (GtkWidget *widget)
{
  MDIColorGeneric *mcg;

  mcg = MDI_COLOR_GENERIC (gnome_mdi_get_active_child (mdi));

  if (IS_MDI_COLOR_FILE (mcg)) 
    while (save_file (MDI_COLOR_FILE (mcg)) == 1);
  else 
    display_todo ();
}

static void 
save_as_cb (GtkWidget *widget)
{
  char *old;
  MDIColorGeneric *mcg;
  MDIColorFile *mcf;

  mcg = MDI_COLOR_GENERIC (gnome_mdi_get_active_child (mdi));

  if (IS_MDI_COLOR_FILE (mcg)) {
    mcf = MDI_COLOR_FILE (mcg);

    while (1) {

      old = mcf->filename;
      mcf->filename = NULL;
      
      if (save_file (mcf) != 1) break;
    }
  } else
    display_todo ();
}

/******************************* Revert *******************************/

static void 
revert_cb (GtkWidget *widget)
{
  display_todo ();
}

/***************************** Close **********************************/

static void 
close_view_cb (GtkWidget *widget)
{
  GnomeMDIChild *child = gnome_mdi_get_active_child (mdi);
  GtkWidget *view;

  if (!child) return;

  if ((child->views) && (! child->views->next)) { /* Only one view */
    GtkWidget *dia;
    char *str, *str2;
    int ret;

    str = g_strdup_printf (_("I'm going to close the last view of '%s' document.\n\nDo you want to close the document too ?"), MDI_COLOR_GENERIC (child)->name);

    if (MDI_COLOR_GENERIC (child)->other_views) {     
      str2 = g_strdup_printf (_("%s\n\nPlease note that %d virtual document use data from this document."), str, g_list_length (MDI_COLOR_GENERIC (child)->other_views));
      g_free (str); str = str2;
    }

    dia = gnome_message_box_new (str, GNOME_MESSAGE_BOX_QUESTION,
				 GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
				 GNOME_STOCK_BUTTON_CANCEL, NULL);
    g_free (str);

    ret = gnome_dialog_run_and_close (GNOME_DIALOG (dia));

    if (ret == 2) return;

    if (ret == 0) {
      gnome_mdi_remove_child (mdi, child, FALSE);
      return;
    }
  }

  if ((view = gnome_mdi_get_active_view  (mdi))) 
    gnome_mdi_remove_view (mdi, view, FALSE);
}

static void
close_doc_cb (GtkWidget *widget)
{
  GnomeMDIChild *child = gnome_mdi_get_active_child (mdi);
  char *str, *str2;
  GtkWidget *dia;
  int ret;

  if (!child) return;

  str = g_strdup_printf (_("I'm going to close the '%s' document."),
			 MDI_COLOR_GENERIC (child)->name);

  if (child->views) {
    str2 = g_strdup_printf (_("%s\n\nPlease note that %d views will be destroyed"), str, g_list_length (child->views));

    g_free (str); str = str2;

    if (MDI_COLOR_GENERIC (child)->other_views) {
      str2 = g_strdup_printf (_("%s and that %d virtual document use data from this document"), str, g_list_length (MDI_COLOR_GENERIC (child)->other_views));

      g_free (str); str = str2;
    } else {
      str2 = g_strconcat (str, ".", NULL);
      g_free (str); str = str2;
    }
  }

  str2 = g_strconcat (str, "\n\n", _("Do you want to continue ?"), NULL);
  g_free (str);

  dia = gnome_message_box_new (str2, GNOME_MESSAGE_BOX_QUESTION,
			       GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			       GNOME_STOCK_BUTTON_CANCEL, NULL);
  g_free (str2);
  
  ret = gnome_dialog_run_and_close (GNOME_DIALOG (dia));
  
  if (ret == 2) return;
  
  if (ret == 0) 
    gnome_mdi_remove_child (mdi, child, FALSE);  
}

static void 
exit_cb (GtkWidget *widget)
{  
  /* Save layout before ... */
  
  session_save (mdi);

  if (gnome_mdi_remove_all (mdi, FALSE))
    gtk_object_destroy (GTK_OBJECT (mdi));
  else  /* Cancel ... */
    return;
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
new_color_cb (GtkWidget *widget)
{
  display_todo ();
}

static void 
remove_cb (GtkWidget *widget)
{
  ViewColorGeneric *view;

  view = gtk_object_get_data (GTK_OBJECT (gnome_mdi_get_active_view (mdi)), 
					  "view_object");

  view_color_generic_remove_selected (view);
}

static void
edit_cb (GtkWidget *widget)
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
properties_cb (GtkWidget *widget)
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

static void 
grab_cb (GtkWidget *widget)
{
  display_todo ();
}

GnomeUIInfo toolbar [] = {
  GNOMEUIINFO_ITEM_STOCK (N_("Exit"), N_("Exit the program"),
			  exit_cb, GNOME_STOCK_PIXMAP_EXIT),
  GNOMEUIINFO_ITEM_STOCK (N_("Open"), N_("Open a palette"),
			  open_cb, GNOME_STOCK_PIXMAP_OPEN),
  GNOMEUIINFO_ITEM_STOCK (N_("Close"), N_("Close the current view"),
			  close_view_cb, GNOME_STOCK_PIXMAP_CLOSE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK (N_("Grab"), N_("Grab a color on the screen"),
			  grab_cb, GNOME_STOCK_PIXMAP_JUMP_TO),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK (N_("About"), N_("About this application"),
			  about_cb, GNOME_STOCK_PIXMAP_ABOUT),
  GNOMEUIINFO_END
};
