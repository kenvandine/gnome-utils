#include "config.h"

#include <gnome.h>
#include <gdk/gdkx.h>

#include "widget-color-list.h"
#include "widget-color-search.h"
#include "widget-editable-label.h"
#include "utils.h"

#define CONFIG_TYPE_COLOR_SEARCH 1
#define CONFIG_TYPE_COLOR_LIST   2

#define PATH_FAVORITES ".gcolorsel_rgb.txt"

char *path_rgb_txt[] = { "/usr/X11R6/lib/X11/rgb.txt",
			 "/usr/X11/lib/X11/rgb.txt",
			 "/usr/openwin/lib/X11/rgb.txt",
			 "/usr/lpp/X11/lib/X11/rgb.txt",
			 "/usr/lib/X11/rgb.txt",
			 NULL };

static void menu_new_cb       (GtkWidget *widget, gpointer data);
static void menu_open_cb      (GtkWidget *widget, gpointer data);
static void menu_exit_cb      (GtkWidget *widget, gpointer data);

static void menu_grab_cb      (GtkWidget *widget, gpointer data);

static void menu_about_cb     (GtkWidget *widget, gpointer data);

static void menu_popup_remove (GtkWidget *widget, gpointer data);
static void menu_popup_rename (GtkWidget *widget, gpointer data);

static void menu_close_cb     (GtkWidget *widget, gpointer data);
static void menu_save_cb      (GtkWidget *widget, gpointer data);
static void menu_saveas_cb    (GtkWidget *widget, gpointer data);
static void menu_revert_cb    (GtkWidget *widget, gpointer data);

static void menu_saveas_search_cb    (GtkWidget *widget, gpointer data);

static void menu_popup_select_all    (GtkWidget *widget, gpointer data);
static void menu_popup_select_none   (GtkWidget *widget, gpointer data);
static void menu_popup_select_invert (GtkWidget *widget, gpointer data);

static void color_list_popup_cb      (GtkWidget *widget, gpointer data);

/* Menus */
static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_MENU_ABOUT_ITEM(menu_about_cb, NULL),
	GNOMEUIINFO_END
};
  
static GnomeUIInfo file_menu[] = {
        GNOMEUIINFO_MENU_NEW_ITEM(N_("New"), NULL, menu_new_cb, NULL),
        GNOMEUIINFO_MENU_OPEN_ITEM(menu_open_cb, NULL),

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM(menu_exit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo app_menu[] = {
        GNOMEUIINFO_MENU_FILE_TREE(file_menu),
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
        GNOMEUIINFO_END
};

/* Notebook Popup */

static GnomeUIInfo popup_menu_notebook_color_list[] = {
        GNOMEUIINFO_MENU_SAVE_ITEM (menu_save_cb, NULL),
	GNOMEUIINFO_MENU_SAVE_AS_ITEM (menu_saveas_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_REVERT_ITEM (menu_revert_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_CLOSE_ITEM (menu_close_cb, NULL),
	
	GNOMEUIINFO_END
};

#define POPUP_NOTEBOOK_POS 2

static GnomeUIInfo popup_menu_notebook_color_search_in[] = {
        GNOMEUIINFO_SEPARATOR,
        GNOMEUIINFO_ITEM_NONE (N_("Select All"), 
			       NULL, menu_popup_select_all),
        GNOMEUIINFO_ITEM_NONE (N_("UnSelect All"), 
			       NULL, menu_popup_select_none),
        GNOMEUIINFO_ITEM_NONE (N_("Invert"), 
			       NULL, menu_popup_select_invert),
	GNOMEUIINFO_END
};

static GnomeUIInfo popup_menu_notebook_color_search[] = {
	GNOMEUIINFO_MENU_SAVE_AS_ITEM (menu_saveas_search_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_SUBTREE (N_("Search in"), NULL),
        GNOMEUIINFO_END
};

/* Color List Popup */

#define POPUP_COLOR_LIST_POS 0

static GnomeUIInfo popup_menu_color_list_insert[] = {
        GNOMEUIINFO_SEPARATOR,
        GNOMEUIINFO_ITEM_NONE (N_("New"), 
			       NULL, color_list_popup_cb),
	GNOMEUIINFO_END
};

static GnomeUIInfo popup_menu_color_list[] = {
        GNOMEUIINFO_SUBTREE (N_("Insert to"), NULL),
        GNOMEUIINFO_SEPARATOR,
        GNOMEUIINFO_ITEM_STOCK (N_("Remove"), NULL, menu_popup_remove,
			  GNOME_STOCK_PIXMAP_REMOVE),
        GNOMEUIINFO_ITEM_NONE  (N_("Rename"), NULL, menu_popup_rename),
	GNOMEUIINFO_END
};

/* Color Search Popup */

#define POPUP_COLOR_SEARCH_POS 0

static GnomeUIInfo popup_menu_color_search[] = {
        GNOMEUIINFO_SUBTREE (N_("Insert to"), NULL),
	GNOMEUIINFO_END
};

/* Toolbar */
static GnomeUIInfo app_toolbar[] = {
        GNOMEUIINFO_ITEM_STOCK (N_("Exit"), N_("Exit the program"), 
				menu_exit_cb, GNOME_STOCK_PIXMAP_EXIT),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_STOCK (N_("Grab"), N_("Grab a color on the screen"),
				menu_grab_cb, GNOME_STOCK_PIXMAP_JUMP_TO),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_STOCK (N_("About"), N_("About this application"),
				menu_about_cb, GNOME_STOCK_PIXMAP_ABOUT),
				
	GNOMEUIINFO_END
};

static void grab_key_press_cb (GtkWidget *widget, GdkEventKey *event, 
			       gpointer data);
static void grab_button_press_cb (GtkWidget *widget, 
				  GdkEventButton *event, gpointer data);

static void file_selection_open_ok_cb (GtkWidget *widget, GtkWidget *fs);

GtkWidget  *app_create (GtkWidget *nb);
static gint app_delete_event_cb (GtkWidget *widget,
				 GdkEventAny *event, gpointer *data);
static void app_destroy_cb (GtkWidget *widget, gpointer data);

GnomeUIInfo *construct_popup (GtkNotebook *nb, int type, 
			      GnomeUIInfo *merge, gpointer cb);

void color_search_search_update (GtkWidget *widget, gpointer data);

char *notebook_get_label_text (GtkNotebook *nb, GtkWidget *child);
GtkWidget *notebook_create_color_search (GtkNotebook *nb, char *page);
GtkWidget *notebook_create_color_list   (GtkNotebook *nb, char *page,
					 char *filename, gboolean search_in,
					 gboolean switch_to,
					 gboolean check_opened);

void notebook_save_layout (GtkNotebook *nb);
gboolean notebook_save_files (GtkNotebook *nb);
gboolean notebook_save_one_file (GtkNotebook *nb, ColorList *cl);
void notebook_load_layout (GtkNotebook *nb);
GtkWidget *notebook_switch_to_color_search (GtkNotebook *nb);
int notebook_search_color_list (GtkNotebook *nb, char *filename);

gint editable_label_button_press (GtkWidget *widget, GdkEventButton *event, 
				  GtkWidget *child);
gboolean editable_label_text_changed (EditableLabel *el, const char *newtext);

void color_list_button_press (GtkWidget *widget, GdkEventButton *event,
			      gpointer data);
void color_list_select_row (GtkWidget *widget, gint row, gint col,
			    GdkEvent *event, GtkWidget *nb);

void create_config (void);

GtkWidget *app;

/*************** Menu ****************/

/* New */

static void 
menu_new_cb (GtkWidget *widget, gpointer data)
{
  notebook_create_color_list (GTK_NOTEBOOK (data), "Unknown", 
			      NULL, TRUE, TRUE, FALSE);
}

/* Open */

static void
file_selection_open_ok_cb (GtkWidget *widget, GtkWidget *fs)
{
  char *file;
  GtkWidget *nb;

  file = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  if (g_file_test (file, G_FILE_TEST_ISFILE)) {
    nb = gtk_object_get_data (GTK_OBJECT (fs), "notebook");

    /* Todo : check if file is not open */

    if (!notebook_create_color_list (GTK_NOTEBOOK (nb), 
			 g_basename (file), file, FALSE, TRUE, TRUE)) {
      return;
    }

    gtk_widget_destroy (fs);
  }
}

static void 
menu_open_cb (GtkWidget *widget, gpointer data)
{
  static GtkWidget *fs = NULL;

  if (fs) {
    gtk_widget_show_now (fs);
    gdk_window_raise (fs->window);
    return;
  }
  
  fs = gtk_file_selection_new (_("Open file"));
  gtk_object_set_data (GTK_OBJECT (fs), "notebook", data);
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (fs));

  gtk_signal_connect (GTK_OBJECT (fs), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &fs);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fs)->ok_button),
	       "clicked", GTK_SIGNAL_FUNC (file_selection_open_ok_cb), fs);

  gtk_signal_connect_object (GTK_OBJECT (
			       GTK_FILE_SELECTION (fs)->cancel_button), 
			     "clicked", 
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     GTK_OBJECT (fs));

  gtk_widget_show (fs);
}

/* About */

static void 
menu_about_cb (GtkWidget *widget, gpointer data)
{  
    static GtkWidget *about = NULL;

    gchar *authors[] = {
	"Tim P. Gerla",
	"Eric Brayeur",
	NULL
    };

    if (about) {
      gtk_widget_show_now (about);
      gdk_window_raise (about->window);
      return;
    }
		
    about = gnome_about_new (
	_("Gnome Color Browser"),
	VERSION,
	"(C) 1997-98 Tim P. Gerla", 
	(const gchar**)authors,
	_("Small utility to browse available X11 colors."),
	NULL);                                

    gtk_signal_connect (GTK_OBJECT (about), "destroy",
			GTK_SIGNAL_FUNC (gtk_widget_destroyed), &about);

    gtk_widget_show(about);
}

/* Exit */

static void 
menu_exit_cb (GtkWidget *widget, gpointer data)
{
  if (! notebook_save_files (GTK_NOTEBOOK (data)))
    return;

  notebook_save_layout (GTK_NOTEBOOK (data));

  gtk_main_quit ();  
}

/* Grab */

static void
grab_key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_Escape) {
  
    /* Ungrab */
    
    gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				   grab_key_press_cb, data);
    gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				   grab_button_press_cb, data);
    gtk_grab_remove (GTK_WIDGET(data));
    gdk_pointer_ungrab (GDK_CURRENT_TIME); 
    gdk_keyboard_ungrab (GDK_CURRENT_TIME);   
  }
}

static void
grab_button_press_cb (GtkWidget *widget, 
		      GdkEventButton *event, gpointer data)
{
  GtkWidget *cs;
  int red, green, blue;
  
  GdkImage *img;
  GdkWindow *win;

  /* Ungrab */
  
  gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				 grab_key_press_cb, data);
  gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				 grab_button_press_cb, data);
  gtk_grab_remove (GTK_WIDGET (data));
  gdk_pointer_ungrab (GDK_CURRENT_TIME); 
  gdk_keyboard_ungrab (GDK_CURRENT_TIME); 

  /* Get Root window */

  win = gdk_window_foreign_new (GDK_ROOT_WINDOW ());
  g_return_if_fail (win != NULL);  
  
  /* Get RGB */
  
  img = gdk_image_get (win, event->x_root, event->y_root, 1, 1);
  g_return_if_fail (img != NULL);

  pixel_to_rgb (gdk_window_get_colormap (win), 
		gdk_image_get_pixel (img, 0, 0),
		&red, &green, &blue);
 
  /* Done */           
           
  gdk_image_destroy (img);           
  gdk_window_unref (win);

  /* Search */

  cs = notebook_switch_to_color_search (GTK_NOTEBOOK (data));
  if (!cs) return;

  color_search_search (COLOR_SEARCH (cs), red, green, blue);
}

static void 
menu_grab_cb (GtkWidget *widget, gpointer data)
{
  GdkCursor *cursor;
  
  gtk_grab_add (GTK_WIDGET (data));

  cursor = gdk_cursor_new (GDK_CROSS_REVERSE);

  gdk_pointer_grab (GTK_WIDGET (data)->window, FALSE, 
       		    GDK_BUTTON_PRESS_MASK,
   		    NULL, cursor, GDK_CURRENT_TIME);
  		    
  gdk_cursor_destroy (cursor);

  gdk_keyboard_grab (GTK_WIDGET(data)->window, FALSE, GDK_CURRENT_TIME);

  gtk_signal_connect (GTK_OBJECT (data), "button_press_event",
  	              GTK_SIGNAL_FUNC (grab_button_press_cb), data);

  gtk_signal_connect (GTK_OBJECT (data), "key_press_event",
  	              GTK_SIGNAL_FUNC (grab_key_press_cb), data);
}

/* Notebook : close */

static void 
menu_close_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget *child = GTK_WIDGET (data);
  GtkWidget *cl;
  int page;
  int ret;
  
  cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (child));
  g_assert (cl != NULL);
  g_assert (IS_COLOR_LIST (cl));
  
  ret = notebook_save_one_file (GTK_NOTEBOOK (child->parent), COLOR_LIST (cl));    
  if (!ret) return;

  page = gtk_notebook_page_num (GTK_NOTEBOOK (child->parent), child);
  gtk_notebook_remove_page (GTK_NOTEBOOK (child->parent), page);
}

/* Save */

static void 
menu_save_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget *sw = GTK_WIDGET (data);
  GtkWidget *cl;
  GtkWidget *dialog;
  char *str;
  char *filename;

  cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (sw));
  g_assert (cl != NULL);
  g_assert (IS_COLOR_LIST (cl));

  filename = gtk_object_get_data (GTK_OBJECT (cl), "file_name");
  g_assert (filename != NULL);

  if (color_list_save (COLOR_LIST (cl), filename, GNOME_APP (app))) {
    str = g_strdup_printf (_("Could not save to file '%s'"), filename);
    dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR, 
				    GNOME_STOCK_BUTTON_OK, NULL);
    gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
    g_free (str);
  } else
    color_list_set_modified (COLOR_LIST (cl), FALSE);
}

/* SaveAS */

static void
file_selection_save_as_ok_cb (GtkWidget *widget, GtkWidget *fs)
{
  GtkWidget *cl;
  GtkWidget *dialog;
  char *file;
  char *str;
  int ret;

  cl = gtk_object_get_data (GTK_OBJECT (fs), "color_list");

  file = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  /* Does the file already exist ? */

  if (g_file_exists (file)) {
    str = g_strdup_printf (_("'%s' already exists, ecrase it ?"), file);
    dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_QUESTION,
				    GNOME_STOCK_BUTTON_YES,
				    GNOME_STOCK_BUTTON_NO, NULL);
    g_free (str);
    gnome_dialog_set_default (GNOME_DIALOG (dialog), 1);
    ret = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
    if (ret != 0) return;
  }

  /* Todo : Check if file is not open  ... */

  ret = color_list_save (COLOR_LIST (cl), file, GNOME_APP (app));
  if (ret) { /* Error */
    str = g_strdup_printf (_("Could not save to file '%s'"), file);
    dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,
				    GNOME_STOCK_BUTTON_OK, NULL);
    g_free (str);
    gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

    return;
  }

  g_free (gtk_object_get_data (GTK_OBJECT (cl), "file_name"));
  gtk_object_set_data (GTK_OBJECT (cl), "file_name", g_strdup (file));
    
  gtk_widget_destroy (fs);
  gtk_main_quit ();
}

static void 
menu_saveas_cb (GtkWidget *widget, gpointer data)
{  
  static GtkWidget *fs;
  GtkWidget *nb;
  GtkWidget *label;
  GtkWidget *cl;

  nb = GTK_WIDGET (data)->parent;

  fs = gtk_file_selection_new (_("Save file"));
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (fs));

  gtk_signal_connect (GTK_OBJECT (fs), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fs)->ok_button),
	    "clicked", GTK_SIGNAL_FUNC (file_selection_save_as_ok_cb), fs);

  gtk_signal_connect_object (GTK_OBJECT (
			     GTK_FILE_SELECTION (fs)->cancel_button), 
			     "clicked", 
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     GTK_OBJECT (fs));

  if (GTK_IS_SCROLLED_WINDOW (data)) {
    label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (nb), data);
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (fs), 
				 EDITABLE_LABEL (label)->text);    

    cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (data));
  } else 
    cl = COLOR_SEARCH (data)->cl;

  gtk_object_set_data (GTK_OBJECT (fs), "color_list", cl);

  gtk_window_set_modal (GTK_WINDOW (fs), TRUE);
  gtk_widget_show (fs);

  gtk_main ();
  printf ("Ok\n");
}

/* Revert */

static void 
menu_revert_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget *sw = GTK_WIDGET (data);
  GtkWidget *nb;
  GtkWidget *cl_orig;  
  GtkWidget *cl = NULL;
  GtkWidget *dialog;
  GtkWidget *label;
  char *str;
  char *filename;
  int ret;
  int page;

  cl_orig = scrolled_window_get_child (GTK_SCROLLED_WINDOW (sw));
  g_assert (cl_orig != NULL);
  g_assert (IS_COLOR_LIST (cl_orig));

  filename = gtk_object_get_data (GTK_OBJECT (cl_orig), "file_name");
  g_assert (filename != NULL);

  str = g_strdup_printf (_("Are you sure you wish revert all changes ?\n(%s)"),
			 filename);
  
  dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_QUESTION, 
				  GNOME_STOCK_BUTTON_YES, 
				  GNOME_STOCK_BUTTON_NO,
				  NULL);
  g_free (str);

  gnome_dialog_set_default (GNOME_DIALOG (dialog), 1);
  ret = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

  if (ret == 0) {
    nb = GTK_WIDGET (data)->parent;
    label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (nb), data);

    if (! (cl = notebook_create_color_list (GTK_NOTEBOOK (nb), 
	    EDITABLE_LABEL (label)->text, filename, FALSE, TRUE, FALSE))){
      
      str = g_strdup_printf (_("Could not read file '%s'"), filename);
      dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,
				      GNOME_STOCK_BUTTON_OK, NULL);
      g_free (str);
      gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
    } else {
      gtk_object_set_data (GTK_OBJECT (cl), "search_in",  
           gtk_object_get_data (GTK_OBJECT (cl_orig), "search_in"));
           
      page = gtk_notebook_page_num (GTK_NOTEBOOK (nb), GTK_WIDGET (data));
      gtk_notebook_remove_page (GTK_NOTEBOOK (nb), page);
      gtk_notebook_reorder_child (GTK_NOTEBOOK (nb), cl->parent, page);

      color_list_set_modified (COLOR_LIST (cl), FALSE);
    }
  }
}

/* Notebook : save as search result */

static void
menu_saveas_search_cb (GtkWidget *widget, gpointer data)
{
  printf ("Save as ...\n");

}

/* Notebook : search : toggle */

static void
menu_popup_select_all (GtkWidget *widget, gpointer data)
{
  GnomeUIInfo *info;
  int i = 0;

  info = gtk_object_get_data (GTK_OBJECT (data), "menu_info");

  while (info[i].type != GNOME_APP_UI_ENDOFINFO) {
    if (GTK_IS_CHECK_MENU_ITEM (info[i].widget)) 
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (info[i].widget),
				      TRUE);

    i++;
  }
}

static void
menu_popup_select_none (GtkWidget *widget, gpointer data)
{
  GnomeUIInfo *info;
  int i = 0;

  info = gtk_object_get_data (GTK_OBJECT (data), "menu_info");

  while (info[i].type != GNOME_APP_UI_ENDOFINFO) {
    if (GTK_IS_CHECK_MENU_ITEM (info[i].widget)) 
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (info[i].widget),
				      FALSE);

    i++;
  }
}

static void
menu_popup_select_invert (GtkWidget *widget, gpointer data)
{
  GnomeUIInfo *info;
  int i = 0;

  info = gtk_object_get_data (GTK_OBJECT (data), "menu_info");

  while (info[i].type != GNOME_APP_UI_ENDOFINFO) {
    if (GTK_IS_CHECK_MENU_ITEM (info[i].widget)) 
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (info[i].widget),
		     ! GTK_CHECK_MENU_ITEM (info[i].widget)->active);

    i++;
  }
}

static void
menu_popup_remove (GtkWidget *widget, gpointer data)
{ 
  GtkWidget *cl;

  cl = gtk_object_get_data (GTK_OBJECT (data), "popup_clist");

  if (GTK_CLIST (cl)->selection) {
    color_list_set_modified (COLOR_LIST (cl), TRUE);

    gtk_clist_freeze (GTK_CLIST (cl));

    while (GTK_CLIST (cl)->selection) 
      gtk_clist_remove (GTK_CLIST (cl), 
			GPOINTER_TO_INT (GTK_CLIST (cl)->selection->data));

    gtk_clist_thaw (GTK_CLIST (cl));
  }
}

static void
menu_popup_rename_end (gchar *str, gpointer data)
{
  ColorListData *col;
  int row;

  if (!str) return;

  row = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (data), 
					      "row_clicked"));

  col = gtk_clist_get_row_data (GTK_CLIST (data), row);
  g_assert (col != NULL);

  g_free (col->name);
  col->name = str;

  gtk_clist_set_text (GTK_CLIST (data), row, 2, str);

  color_list_set_modified (COLOR_LIST (data), TRUE);
}

static void
menu_popup_rename (GtkWidget *widget, gpointer data)
{
  ColorListData *col;
  GtkWidget *dialog;
  GtkWidget *cl;
  int row;
  char *str;

  cl = gtk_object_get_data (GTK_OBJECT (data), "popup_clist");
  row = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (cl), 
					      "row_clicked"));

  col = gtk_clist_get_row_data (GTK_CLIST (cl), row);
  g_assert (col != NULL);

  str = g_strdup_printf (_("Enter new name for '%s'"), col->name);

  dialog = gnome_request_dialog (FALSE, str, col->name, 250,
				 menu_popup_rename_end, cl, 
				 NULL);
  
  g_free (str);

  gnome_dialog_run (GNOME_DIALOG (dialog));
}

/*************** App ****************/

static gint
app_delete_event_cb (GtkWidget *widget, GdkEventAny *event, gpointer *data)
{
  if (! notebook_save_files (GTK_NOTEBOOK (data)))
    return TRUE;

  notebook_save_layout (GTK_NOTEBOOK (data));

  return FALSE;
}

static void
app_destroy_cb (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}

GtkWidget *
app_create (GtkWidget *nb)
{
  GtkWidget *app;
  GtkWidget *status;

  app = gnome_app_new ("gcolorsel", _("Gnome Color Browser"));

  gtk_widget_set_usize (app, 320, 360);

  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
		      GTK_SIGNAL_FUNC (app_delete_event_cb), nb);
  gtk_signal_connect (GTK_OBJECT (app), "destroy",
		      GTK_SIGNAL_FUNC (app_destroy_cb), nb);

  gnome_app_create_menus_with_data (GNOME_APP (app), app_menu, nb);
  gnome_app_create_toolbar_with_data (GNOME_APP (app), app_toolbar, nb);
  
  status = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_ALWAYS);
  
  gnome_app_set_statusbar (GNOME_APP (app), status);

  return app;
}

/*************** EditableLabel ****************/

GnomeUIInfo *
construct_popup (GtkNotebook *nb, int type, GnomeUIInfo *merge, gpointer cb)
{
  GnomeUIInfo *info;
  GtkNotebookPage *page;
  GtkWidget *cl;  
  GList *list;
  int size = 0, i = 0;

  while (merge[size].type != GNOME_APP_UI_ENDOFINFO) size++;

  list = nb->children;

  info = g_new0 (GnomeUIInfo, g_list_length (list) + size);
  
  while (list) {
    page = list->data;

    if (GTK_IS_SCROLLED_WINDOW (page->child)) {
      cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (page->child));
      if (IS_COLOR_LIST (cl)) {
	info[i].type  = type;
	info[i].label = g_strdup (EDITABLE_LABEL (page->tab_label)->text);
	info[i].pixmap_type = GNOME_APP_PIXMAP_NONE;
	info[i].moreinfo = cb;
	
	i++;
      }
    }
    
    list = list->next;
  }

  memcpy (&info[i], merge, size * sizeof (GnomeUIInfo));

  return info;
}

gint
editable_label_button_press (GtkWidget *widget, GdkEventButton *event, 
			     GtkWidget *child)
{
  GnomeUIInfo *info;
  GtkWidget *label;
  GtkWidget *menu;
  GtkWidget *cl;
  GtkNotebookPage *nb_page;
  GList *list;
  int i, page;

  page = gtk_notebook_page_num (GTK_NOTEBOOK (child->parent), child);
  gtk_notebook_set_page (GTK_NOTEBOOK (child->parent), page);

  if ((event->type != GDK_BUTTON_PRESS)||(event->button != 3)) return FALSE;

  label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (child->parent), child);
  g_assert (label != NULL);

  if ((event->window == widget->window) || IS_EDITABLE_LABEL (label)) {

    if (IS_COLOR_SEARCH (child)) {

      info = construct_popup (GTK_NOTEBOOK (child->parent), 
			      GNOME_APP_UI_TOGGLEITEM, 
			      popup_menu_notebook_color_search_in, NULL);      

      popup_menu_notebook_color_search[POPUP_NOTEBOOK_POS].moreinfo = info;

      menu = gnome_popup_menu_new (popup_menu_notebook_color_search);

      /* Set toggle item ... */

      list = GTK_NOTEBOOK (child->parent)->children;
      i = 0;

      while (list) {
	nb_page = list->data;
	if (GTK_IS_SCROLLED_WINDOW (nb_page->child)) {
	  cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (nb_page->child));
	  if (IS_COLOR_LIST (cl)) {

	      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (info[i].widget), GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (cl), "search_in")));
	  
	    i++;
	  }
	}

	list = list->next;
      }

      gtk_object_set_data (GTK_OBJECT (child), "menu_info", info);
      gnome_popup_menu_do_popup_modal (menu, NULL, NULL, event, child);

      /* Get toggle item ... */

      list = GTK_NOTEBOOK (child->parent)->children;
      i = 0;

      while (list) {
	nb_page = list->data;
	if (GTK_IS_SCROLLED_WINDOW (nb_page->child)) {
	  cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (nb_page->child));
	  if (IS_COLOR_LIST (cl)) {

	      gtk_object_set_data (GTK_OBJECT (cl), "search_in", GINT_TO_POINTER (GTK_CHECK_MENU_ITEM (info[i].widget)->active));
	  
	    i++;
	  }
	}

	list = list->next;
      }

      gtk_widget_destroy (menu);
      g_free (info);
      
    } else {

      g_assert (GTK_IS_SCROLLED_WINDOW (child));

      cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (child));
      g_assert (IS_COLOR_LIST (cl));
    
      menu = gnome_popup_menu_new (popup_menu_notebook_color_list);
      gnome_popup_menu_do_popup (menu, NULL, NULL, event, child);
    }
  }
    
  return FALSE;
}

gboolean 
editable_label_text_changed (EditableLabel *el, const char *newtext)
{  
  if (!newtext) return FALSE;
  while (newtext[0] == ' ') newtext++;
  
  return newtext[0];  
}

/*************** ColorList ****************/

void
color_list_select_row (GtkWidget *widget, gint row, gint col,
		       GdkEvent *event, GtkWidget *nb)
{
  GtkWidget *cs;
  ColorListData *color;

  if ((!event)||(event && event->type != GDK_2BUTTON_PRESS))
    return;

  cs = notebook_switch_to_color_search (GTK_NOTEBOOK (nb));
  if (!cs) return;

  color = gtk_clist_get_row_data (GTK_CLIST (widget), row);
  if (!color) return;

  cs = notebook_switch_to_color_search (GTK_NOTEBOOK (nb));
  if (!cs) return;

  color_search_search (COLOR_SEARCH (cs), color->r, color->g, color->b);
}

void 
color_list_popup_cb (GtkWidget *widget, gpointer data)
{  
  GList *list;
  ColorListData *col;
  GnomeUIInfo *info;
  GtkWidget *cl_source;
  GtkWidget *cl_dest;
  GtkWidget *sw;
  int i = 0, j = 0;

  cl_source = gtk_object_get_data (GTK_OBJECT (data), "popup_clist");

  info = gtk_object_get_data (GTK_OBJECT (data), "menu_info");
  while (widget != info [i].widget) i++;

  for (j=0; j<=i; j++)
    if (IS_COLOR_SEARCH (gtk_notebook_get_nth_page (GTK_NOTEBOOK (data), j)))
	i++;

  sw = gtk_notebook_get_nth_page (GTK_NOTEBOOK (data), i);
  if (!sw) {
    /* Create new ... */
    printf ("To new ...\n");
    cl_dest = notebook_create_color_list (GTK_NOTEBOOK (data), "Unknown",
					  NULL, TRUE, FALSE, FALSE);
  } else
    cl_dest = scrolled_window_get_child (GTK_SCROLLED_WINDOW (sw));

  gtk_clist_freeze (GTK_CLIST (cl_dest));

  list = GTK_CLIST (cl_source)->selection;
  while (list) {
    col = gtk_clist_get_row_data (GTK_CLIST (cl_source), 
				  GPOINTER_TO_INT (list->data));
    
    color_list_append (COLOR_LIST (cl_dest), col->r, col->g, col->b, 
		       col->num, col->name);

    list = list->next;
  }

  gtk_clist_thaw (GTK_CLIST (cl_dest));
  gtk_clist_sort (GTK_CLIST (cl_dest));

  color_list_set_modified (COLOR_LIST (cl_dest), TRUE);
}

void
color_list_button_press (GtkWidget *widget, GdkEventButton *event,
			 gpointer data)
{
  GnomeUIInfo *info, *uiinfo;
  GtkWidget *menu;
  GtkCListRow *r;
  int row, col, pos;
  int result;

  if (event->button != 3) return;
  if (!gtk_clist_get_selection_info (GTK_CLIST (widget), event->x, 
				     event->y, &row, &col))
    return;

  r = g_list_nth (GTK_CLIST (widget)->row_list, row)->data;
  g_assert (r != NULL);

  if (r->state != GTK_STATE_SELECTED) {
    gtk_clist_freeze (GTK_CLIST (widget));
    gtk_clist_unselect_all (GTK_CLIST (widget));
    GTK_CLIST (widget)->focus_row = row;
    gtk_clist_select_row (GTK_CLIST (widget), row, col);
    gtk_clist_thaw (GTK_CLIST (widget));

    r->state = GTK_STATE_SELECTED;
  }

  gtk_object_set_data (GTK_OBJECT (widget), "row_clicked", 
		       GINT_TO_POINTER (row));

  /* Construct inser to menu ... */

  info = construct_popup (GTK_NOTEBOOK (data), 
			  GNOME_APP_UI_ITEM, 
			  popup_menu_color_list_insert, color_list_popup_cb);
  
  pos = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), 
					      "uiinfo_pos"));
  uiinfo = gtk_object_get_data (GTK_OBJECT (widget), "uiinfo");

  uiinfo[pos].moreinfo = info;
  
  menu = gnome_popup_menu_new (uiinfo);
  
  gtk_object_set_data (GTK_OBJECT (data), "menu_info", info);
  gtk_object_set_data (GTK_OBJECT (data), "popup_clist", widget);
  result = gnome_popup_menu_do_popup_modal (menu, NULL, NULL, 
					    event, data);
}

/*************** ColorSearch **************/

void
color_search_search_update (GtkWidget *widget, gpointer data)
{
  GList *list;
  GtkWidget *cl;
  GtkNotebookPage *page;

  gtk_clist_freeze (GTK_CLIST (COLOR_SEARCH (widget)->cl));  
  gtk_clist_clear (GTK_CLIST (COLOR_SEARCH (widget)->cl));

  list = GTK_NOTEBOOK (data)->children;

  while (list) {
    page = list->data;
    if (GTK_IS_SCROLLED_WINDOW (page->child)) {
      cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (page->child));
      if (IS_COLOR_LIST (cl)) {
	
	if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT (cl), "search_in")))
	  color_search_search_in (COLOR_SEARCH (widget), GTK_CLIST (cl));
      }
    }    
     
    list = list->next;
  }

  gtk_clist_sort (GTK_CLIST (COLOR_SEARCH (widget)->cl));
  gtk_clist_thaw (GTK_CLIST (COLOR_SEARCH (widget)->cl));
}

/*************** NoteBook ****************/

char *
notebook_get_label_text (GtkNotebook *nb, GtkWidget *child)
{
  GtkWidget *label;

  label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (nb), child);
  return EDITABLE_LABEL (label)->text;
}

int
notebook_search_color_list (GtkNotebook *nb, char *filename)
{  
  GList *list_child;
  GtkNotebookPage *page;
  GtkWidget *cl;
  int pos = 0;
  char *file;
  
  g_assert (nb != NULL);
  g_assert (filename != NULL);

  list_child = nb->children;

  while (list_child) {
    page = list_child->data;

    if (GTK_IS_SCROLLED_WINDOW (page->child)) {
      cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (page->child));
      g_assert (IS_COLOR_LIST (cl));

      file = gtk_object_get_data (GTK_OBJECT (cl), "file_name");
      
      if ((file) && (! strcmp (file, filename)))
	return pos;
    }

    pos++; list_child = list_child->next;
  }

  return -1;
}

GtkWidget *
notebook_switch_to_color_search (GtkNotebook *nb)
{
  GList *list_child;
  GtkNotebookPage *page;
  int page_num;

  list_child = nb->children;

  while (list_child) {
    page = list_child->data;
    if (IS_COLOR_SEARCH (page->child)) break;
    list_child = list_child->next;
  }

  if (!list_child) return NULL;

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (nb), page->child);
  if (page_num == -1) return NULL;
  gtk_notebook_set_page (GTK_NOTEBOOK (nb), page_num);

  return page->child;
}

GtkWidget *
notebook_create_color_list (GtkNotebook *nb, char *page_label, 
			    char *filename, gboolean search_in, 
			    gboolean switch_to, gboolean check_opened)
{
  GtkWidget *cl;
  GtkWidget *sw;
  int page_num;
  GtkWidget *label;
  GtkWidget *dialog;
  int result;
  char *str;

  if ((check_opened)&&(filename)) { 
    page_num = notebook_search_color_list (nb, filename);
    if (page_num >= 0) {
      if (switch_to) 
	gtk_notebook_set_page (nb, page_num);

      str = g_strdup_printf ("'%s' is already opened", filename);
      dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_WARNING,
				      GNOME_STOCK_BUTTON_OK, NULL);
      g_free (str);
      gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

      sw = gtk_notebook_get_nth_page (nb, page_num);
      cl = scrolled_window_get_child (GTK_SCROLLED_WINDOW (sw));

      return cl;
    }      
  }

  cl = color_list_new ();

  gtk_object_set_data (GTK_OBJECT (cl), "uiinfo_pos",
		       GINT_TO_POINTER (POPUP_COLOR_LIST_POS));
  gtk_object_set_data (GTK_OBJECT (cl), "uiinfo", popup_menu_color_list);

  gtk_signal_connect (GTK_OBJECT (cl), "button_press_event",
		      GTK_SIGNAL_FUNC (color_list_button_press), nb);
  gtk_signal_connect (GTK_OBJECT (cl), "select_row",
		      GTK_SIGNAL_FUNC (color_list_select_row), nb);

  sw = gtk_scrolled_window_new (NULL, NULL);  
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (sw), cl);

  label = editable_label_new (page_label);
  gtk_signal_connect (GTK_OBJECT (label), "button_press_event", 
                      GTK_SIGNAL_FUNC (editable_label_button_press), sw);
  gtk_signal_connect (GTK_OBJECT (label), "text_changed",
                      GTK_SIGNAL_FUNC (editable_label_text_changed), sw); 
  	              
  gtk_notebook_append_page (GTK_NOTEBOOK (nb), sw, label);

  if (filename) {
  
    gtk_widget_realize (cl);
    result = color_list_load (COLOR_LIST (cl), filename, GNOME_APP (app));

    if (result) {
      str = g_strdup_printf (_("'%s' is not a palette file !"), filename);
      dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,
				      GNOME_STOCK_BUTTON_OK, NULL);
      
      gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
      
      g_free (str);
      
      gtk_notebook_remove_page (GTK_NOTEBOOK (nb), -1);
      
      return NULL;
    }
  }

  gtk_object_set_data (GTK_OBJECT (cl), "search_in", 
		       GINT_TO_POINTER ((search_in)));   
  gtk_object_set_data (GTK_OBJECT (cl), "file_name", g_strdup (filename));
  
  gtk_clist_columns_autosize (GTK_CLIST (cl));

  gtk_widget_show_all (sw);
  
  if (switch_to) 
    gtk_notebook_set_page (GTK_NOTEBOOK (nb), 
			   gtk_notebook_page_num (GTK_NOTEBOOK (nb), sw)); 

  return cl;
}

GtkWidget *
notebook_create_color_search (GtkNotebook *nb, char *page)
{
  GtkWidget *cs;
  GtkWidget *label;

  cs = color_search_new ();
  gtk_signal_connect (GTK_OBJECT (cs), "search_update",
		      GTK_SIGNAL_FUNC (color_search_search_update), nb);

  gtk_object_set_data (GTK_OBJECT (COLOR_SEARCH (cs)->cl), "uiinfo_pos",
		       GINT_TO_POINTER (POPUP_COLOR_LIST_POS));
  gtk_object_set_data (GTK_OBJECT (COLOR_SEARCH (cs)->cl), 
		       "uiinfo", popup_menu_color_search);

  gtk_signal_connect (GTK_OBJECT (COLOR_SEARCH (cs)->cl), "button_press_event",
		      GTK_SIGNAL_FUNC (color_list_button_press), nb);
  gtk_signal_connect (GTK_OBJECT (COLOR_SEARCH (cs)->cl), "select_row",
		      GTK_SIGNAL_FUNC (color_list_select_row), nb);

  label = editable_label_new (page);
  gtk_signal_connect (GTK_OBJECT (label), "button_press_event", 
                      GTK_SIGNAL_FUNC (editable_label_button_press), cs);
  gtk_signal_connect (GTK_OBJECT (label), "text_changed",
                      GTK_SIGNAL_FUNC (editable_label_text_changed), cs); 

  gtk_notebook_append_page (GTK_NOTEBOOK (nb), cs, label);

  gtk_widget_show_all (cs);

  return cs;
}

void  
notebook_save_layout (GtkNotebook *nb)
{
  GtkWidget *widget;
  GtkWidget *editable;
  GList *list_child;
  GtkNotebookPage *page;
  int i = 0;
  char *str;
  char *filename;

  list_child = nb->children;

  if (list_child)
    gnome_config_set_int ("/gcolorsel/Prefs/ActivePage", 
		      	  gtk_notebook_get_current_page (GTK_NOTEBOOK (nb)));

  while (list_child) {
    str = g_strdup_printf ("/gcolorsel/Page_%d/", i);
    gnome_config_push_prefix (str);
    g_free (str);

    page = list_child->data;
    editable = page->tab_label;
    g_assert (editable != NULL);

    if (IS_COLOR_SEARCH (page->child)) {
      gnome_config_set_int ("Type", CONFIG_TYPE_COLOR_SEARCH);
      gnome_config_set_string ("Name", EDITABLE_LABEL (editable)->text);
    
      i++;
    } else {

      widget = scrolled_window_get_child (GTK_SCROLLED_WINDOW (page->child));
      g_assert (IS_COLOR_LIST (widget));
      
      filename = gtk_object_get_data (GTK_OBJECT (widget), "file_name");

      if (filename) {
	gnome_config_set_int ("Type", CONFIG_TYPE_COLOR_LIST);
	gnome_config_set_string ("File", filename);
	gnome_config_set_string ("Name", EDITABLE_LABEL (editable)->text);
	
	gnome_config_set_bool ("SearchIn", 
         	GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), 
						      "search_in")));
    
	i++;
      }
    }

    gnome_config_pop_prefix ();
 
    list_child = list_child->next; 
  }  

  gnome_config_set_int ("/gcolorsel/Prefs/NbPage", i);
  
  gnome_config_sync ();
}

/* Return False if user choose to cancel exit/close process ...  */
gboolean 
notebook_save_one_file (GtkNotebook *nb, ColorList *cl)
{
  GtkWidget *dialog;
  char *filename;
  char *str;
  int ret;
  
  g_assert (nb != NULL);
  g_assert (cl != NULL);
  
  if (! cl->modified) return TRUE;
  
  filename = gtk_object_get_data (GTK_OBJECT (cl), "file_name");

  if (!filename) 
    filename = notebook_get_label_text (nb, GTK_WIDGET (cl)->parent);

    str = g_strdup_printf (_("'%s' has been modified. Do you wish to save it ?"), filename);
	
  dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_QUESTION,
		   		       GNOME_STOCK_BUTTON_YES,
				       GNOME_STOCK_BUTTON_NO,
			               GNOME_STOCK_BUTTON_CANCEL,
				       NULL);
  g_free (str);
  gnome_dialog_set_default (GNOME_DIALOG (dialog), 2);
	
  ret = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
  if (ret == 2) return FALSE;

  if (ret == 0) {
    if (! gtk_object_get_data (GTK_OBJECT (cl), "file_name")) 
      /* Quick hack ... */
      menu_saveas_cb (NULL, GTK_WIDGET (cl)->parent);
    else

     if (color_list_save (cl, filename, GNOME_APP (app))) {
       str = g_strdup_printf (_("Could not save to file '%s'"), filename);
       
       dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR, 
					    GNOME_STOCK_BUTTON_OK, 
					    GNOME_STOCK_BUTTON_CANCEL,
					    NULL);	    
       g_free (str);
       gnome_dialog_set_default (GNOME_DIALOG (dialog), 1);
	    
       ret = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
       if (ret == 1) return FALSE;
     }
  } else
    color_list_set_modified (cl, FALSE);  
    
  return TRUE;
}

/* Return False if user choose to cancel exit process ... */
gboolean
notebook_save_files (GtkNotebook *nb)
{
  GtkWidget *widget;
  GList *list_child;
  GtkNotebookPage *page;
  int ret;

  list_child = nb->children;

  while (list_child) {
    page = list_child->data;

    if (GTK_IS_SCROLLED_WINDOW (page->child)) {

      widget = scrolled_window_get_child (GTK_SCROLLED_WINDOW (page->child));
      g_assert (IS_COLOR_LIST (widget));

      ret = notebook_save_one_file (nb, COLOR_LIST (widget));
      if (!ret) return FALSE;
    }

    list_child = list_child->next; 
  }  

  return TRUE;
}

void
notebook_load_layout (GtkNotebook *nb)
{
  int n, i;
  char *str, *file, *name;
  gboolean search_in;

  n = gnome_config_get_int ("/gcolorsel/Prefs/NbPage=0");

  for (i=0; i < n; i++) {
    str = g_strdup_printf ("/gcolorsel/Page_%d/", i);
    gnome_config_push_prefix (str);
    g_free (str);

    name = gnome_config_get_string ("Name");

    switch (gnome_config_get_int ("Type")) {
    case CONFIG_TYPE_COLOR_LIST:
      file = gnome_config_get_string ("File");
      search_in = gnome_config_get_bool ("SearchIn");
      
      if (file) {
	if (g_file_test (file, G_FILE_TEST_ISFILE)) {
	  if ((name)&&(!name[0])) {
	    g_free (name);
	    name = NULL;
	  }
	  if (!name) name = g_strdup (g_basename (file));
	  notebook_create_color_list (nb, name, file, search_in, FALSE, TRUE);
        }	
	g_free (file);
      }
      
      if (name) g_free (name);
      
      break;

    case CONFIG_TYPE_COLOR_SEARCH:
      notebook_create_color_search (nb, name);
      break;
    }
      
    gnome_config_pop_prefix ();
  }  

  if (n) 
    gtk_notebook_set_page (nb, 
	     gnome_config_get_int ("/gcolorsel/Prefs/ActivePage=0"));
}

/*************** Config ****************/

void
create_config (void)
{ 
  int i = 0, j = 0;
  FILE *fp;
  char *str;
  char *path;

  if (gnome_config_get_int ("/gcolorsel/Prefs/NbPage=-1") != -1) 
    return;

  /* 1. Search rgb.txt */

  while (path_rgb_txt[i]) {
    if (g_file_test (path_rgb_txt[i], G_FILE_TEST_ISFILE)) {
      str = g_strdup_printf ("/gcolorsel/Page_%d/", j);
      gnome_config_push_prefix (str);
      g_free (str);
      gnome_config_set_int ("Type", CONFIG_TYPE_COLOR_LIST);
      gnome_config_set_string ("File", path_rgb_txt[i]);
      gnome_config_set_string ("Name", _("System"));
      gnome_config_set_bool ("SearchIn", TRUE);
      gnome_config_pop_prefix ();

      j++;

      break;
    }
    i++;
  }

  /* 2. Create Search page */  

  str = g_strdup_printf ("/gcolorsel/Page_%d/", j);
  gnome_config_push_prefix (str);
  g_free (str);
  gnome_config_set_int ("Type", CONFIG_TYPE_COLOR_SEARCH);
  gnome_config_set_string ("Name", _("Search"));
  gnome_config_pop_prefix ();

  j++;

  /* 3. Create favorites */

  path = gnome_util_prepend_user_home (PATH_FAVORITES);
  fp = fopen (path, "a");
  if (fp) {
    fclose (fp);
  
    str = g_strdup_printf ("/gcolorsel/Page_%d/", j);
    gnome_config_push_prefix (str);
    g_free (str);
    gnome_config_set_int ("Type", CONFIG_TYPE_COLOR_LIST);
    gnome_config_set_string ("File", path);
    gnome_config_set_string ("Name", _("Favorites"));
    gnome_config_set_bool ("SearchIn", FALSE);
    gnome_config_pop_prefix ();

    j++;
  } /* TODO : show message ? */

  g_free (path);

  gnome_config_set_int ("/gcolorsel/Prefs/NbPage", j);
  gnome_config_set_int ("/gcolorsel/Prefs/ActivePage", 0);
  gnome_config_sync ();
}

int main (int argc, char *argv[])
{
  GtkWidget *nb;

  gnome_init("gcolorsel", "", argc, argv);

  nb = gtk_notebook_new ();
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (nb), TRUE);

  app = app_create (nb);

  gtk_notebook_set_tab_border (GTK_NOTEBOOK (nb), 0);
  gnome_app_set_contents (GNOME_APP (app), nb);

  gtk_widget_realize (app);
  gtk_widget_show_all (app);
  while (gtk_events_pending ()) gtk_main_iteration ();

  create_config ();
  notebook_load_layout (GTK_NOTEBOOK (nb));


  gtk_main ();
    
  return EXIT_SUCCESS;
}
