#include <config.h>

#include "gcolorsel.h"
#include "menus.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "mdi-color-virtual-rgb.h"
#include "mdi-color-virtual-monitor.h"
#include "view-color-grid.h"
#include "view-color-list.h"
#include "view-color-edit.h"
#include "session.h"
#include "utils.h"

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>

#ifdef HAVE_GNOME_APPLET
#include <applet-widget.h>
#endif

GnomeMDI *mdi = NULL;
GtkWidget *event_widget = NULL;
prefs_t prefs;

/* First view is considered as the default view ... */
views_t views_tab[] = { {N_("List"), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS, 
			 (views_new)view_color_list_new, 
			 view_color_list_get_type,
			 "List view description TODO" },
			{N_("Grid"), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
			 (views_new)view_color_grid_new, 
			 view_color_grid_get_type,
			 "Grid view description TODO" },
			{N_("Edit"), GTK_POLICY_NEVER, GTK_POLICY_NEVER,
			 (views_new)view_color_edit_new, 
			 view_color_edit_get_type,
			 "Edit view description TODO" },
			
			{ NULL, 0, 0, NULL, 0 } };

docs_t docs_tab[] = { 
  { N_("File"),       mdi_color_file_get_type,            TRUE,  FALSE,
  "File document description TODO" },
  { N_("Search RGB"), mdi_color_virtual_rgb_get_type,     TRUE,  TRUE,  
  "Search RGB document description TODO" },
  { N_("Monitor"),    mdi_color_virtual_monitor_get_type, FALSE, TRUE,  
  "Monitor document description TODO" },
  { N_("Concat"),    mdi_color_virtual_get_type,         TRUE,  TRUE,  
  "Concat document description TODO" },
  { NULL, NULL, FALSE },
};

static GtkTargetEntry app_drop_targets[] = {
  { "application/x-color", 0 }
};

views_t *
get_views_from_type (GtkType type)
{
  int i = 0;

  while (views_tab[i].name) {
    if (views_tab[i].type () == type) return &views_tab[i];
    i++;
  }

  return NULL;
}

static void
call_get_type (void)
{
  int i = 0;

  while (views_tab[i].name) {
    views_tab[i].type ();
    i++;
  }

  i = 0;
  while (docs_tab[i].name) {
    docs_tab[i].type ();
    i++;
  }
}

static gint 
mdi_remove_child (GnomeMDI *mdi, MDIColorGeneric *mcg)
{
  GtkWidget *dia;
  int ret;
  char *str;

  if (mcg->other_views) return TRUE;

  if ((IS_MDI_COLOR_FILE (mcg)) && (mcg->modified)) {

    str = g_strdup_printf (_("'%s' document has been modified; do you wish to save it ?"), mcg->name);
    
    dia = gnome_message_box_new (str, GNOME_MESSAGE_BOX_QUESTION, 
				 GNOME_STOCK_BUTTON_YES, 
				 GNOME_STOCK_BUTTON_NO,
				 GNOME_STOCK_BUTTON_CANCEL, NULL);    
    g_free (str);
      
    ret = gnome_dialog_run_and_close (GNOME_DIALOG (dia));
    if (ret == 2) return FALSE;

    if (!ret)     
      while (1) {
	ret = save_file (MDI_COLOR_FILE (mcg));

	/* We need to save the filename in the config ...
	   because session is saved before this function */
	if (ret == 0) {
	  char *prefix = prefix = g_strdup_printf ("/gcolorsel/%d/", mcg->key);
	  gnome_config_push_prefix (prefix);
	  MDI_COLOR_GENERIC_GET_CLASS (mcg)->save (mcg); 
	  gnome_config_pop_prefix ();
	  gnome_config_sync ();
	}
	if (ret == -1) return FALSE;
	if (ret != 1) break;
      }
  }

  return TRUE;
}

static void
drag_data_received (GtkWidget *widget, GdkDragContext *context,
		    gint x, gint y,
		    GtkSelectionData *selection_data, guint info,
		    guint time)
{
  guint16 *data = (guint16 *)selection_data->data;
  int r, g, b;
  
  if (data) {
    r = (data[0] * 255.0) / 0xffff;
    g = (data[1] * 255.0) / 0xffff;
    b = (data[2] * 255.0) / 0xffff;

    actions_drop (r, g, b);  
  }
}

static void
app_created (GnomeMDI *mdi, GnomeApp *app)
{
  GtkWidget *statusbar;

  gtk_widget_set_usize (GTK_WIDGET (app), 320, 410);

  statusbar = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_USER);
  gnome_app_set_statusbar (GNOME_APP (app), statusbar);
  gnome_app_install_menu_hints (app, gnome_mdi_get_menubar_info (app));

  gtk_widget_set_usize (GTK_WIDGET (gnome_appbar_get_progress (GNOME_APPBAR (statusbar))), 60, -1);

  gtk_signal_connect (GTK_OBJECT (app), "drag_data_received",
		      GTK_SIGNAL_FUNC (drag_data_received), NULL);

  gtk_drag_dest_set (GTK_WIDGET (app), 
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_DROP,
		     app_drop_targets, 1,
		     GDK_ACTION_COPY);
}

static gint 
selection_clear (GtkWidget         *widget,
		 GdkEventSelection *event,
		 gpointer data)
{
  char *str = gtk_object_get_data (GTK_OBJECT (event_widget), "col");

  if (str) {
    g_free (str);
    gtk_object_set_data (GTK_OBJECT (event_widget), "col", NULL);
  }

  return TRUE;
}

static void 
selection_get (GtkWidget        *widget, 
	       GtkSelectionData *selection_data,
	       guint             info,
	       guint             time_stamp,
	       gpointer          data)
{
  char *str;

  str = gtk_object_get_data (GTK_OBJECT (event_widget), "col");

  gtk_selection_data_set (selection_data, GDK_SELECTION_TYPE_STRING,
			  8, str, strlen (str) + 1);
}

static void
paste_fail ()
{
  GtkWidget *dialog;
  char *str;

  str = _("GColorsel was unable to paste colors from the clipboard.");

  dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,
				  GNOME_STOCK_BUTTON_OK, NULL);

  gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
}

static void
paste_bad_doc ()
{
  GtkWidget *dialog;
  char *str;

  str = _("Please select a file document before.");

  dialog = gnome_message_box_new (str, GNOME_MESSAGE_BOX_ERROR,
				  GNOME_STOCK_BUTTON_OK, NULL);

  gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
}

static void 
selection_received (GtkWidget        *widget,
		    GtkSelectionData *selection_data, 
		    gpointer          data)
{
  char *str, name[256];
  int r, g, b;
  GList *list = NULL, *l;
  GtkObject *col;
  GnomeMDIChild *child;
  GtkWidget *w;
  ViewColorGeneric *view;
  int pos;

  if (selection_data->length < 0) return;
  if (selection_data->type != GDK_SELECTION_TYPE_STRING) return;

  child = gnome_mdi_get_active_child (mdi);
  if ((! child) || 
      (! mdi_color_generic_can_do (MDI_COLOR_GENERIC (child), CHANGE_APPEND))) {
    paste_bad_doc ();
    return;
  }

  w = gnome_mdi_get_active_view (mdi);
  if (! w) return;

  view = gtk_object_get_data (GTK_OBJECT (w), "view_object");

  str = (char *)selection_data->data;

  while (str[0]) {
   
    if (sscanf(str, "%d %d %d\t\t%255[^\n]\n", &r, &g, &b, name) < 3) {
      paste_fail ();

      l = list;
      while (l) {
	gtk_object_destroy (GTK_OBJECT (l->data));
	l = g_list_next (l);
      }

      return; 
    }

    col = mdi_color_new ();
    MDI_COLOR (col)->r = r; MDI_COLOR (col)->g = g; MDI_COLOR (col)->b = b;
    MDI_COLOR (col)->name = g_strdup (name);

    list = g_list_prepend (list, col);
    
    while ((str[0]) && (str[0] != '\n')) str++;
    while (str[0] == '\n') str++;
  }

  pos = VIEW_COLOR_GENERIC_GET_CLASS (view)->get_insert_pos (view);
  mdi_color_generic_freeze (MDI_COLOR_GENERIC (child));

  l = list;
  if (l) 
    while (l) {
      mdi_color_generic_append (MDI_COLOR_GENERIC (child), l->data);
      mdi_color_generic_change_pos (MDI_COLOR_GENERIC (child), l->data, pos);
      l = g_list_next (l);
    }
    
  mdi_color_generic_thaw (MDI_COLOR_GENERIC (child));
    
  g_list_free (list);
    
  return;
}

/**************************** Preferences *******************************/

void prefs_load ()
{
  gnome_config_push_prefix ("/gcolorsel/gcolosel/");

  prefs.save_session = gnome_config_get_bool ("SaveSession=TRUE");
  prefs.display_doc  = gnome_config_get_bool ("DisplayDoc=FALSE");

  prefs.on_drop      = gnome_config_get_int ("OnDrop=-0");
  prefs.on_grab      = gnome_config_get_int ("OnGrab=-0");
  prefs.on_views     = gnome_config_get_int ("OnViews=-0");
  prefs.on_previews  = gnome_config_get_int ("OnPreviews=-0");

  prefs.on_drop2     = gnome_config_get_int ("OnDrop2=-0");
  prefs.on_grab2     = gnome_config_get_int ("OnGrab2=-0");
  prefs.on_views2    = gnome_config_get_int ("OnViews2=-0");
  prefs.on_previews2 = gnome_config_get_int ("OnPreviews2=-0");

  prefs.tab_pos = gnome_config_get_int ("TabPos=-1");
  if (prefs.tab_pos == -1) prefs.tab_pos = GTK_POS_TOP;
  mdi->tab_pos = prefs.tab_pos;

  prefs.mdi_mode = gnome_config_get_int ("MDIMode=-1");
  if (prefs.mdi_mode == -1) prefs.mdi_mode = GNOME_MDI_NOTEBOOK;
  gnome_mdi_set_mode (mdi, prefs.mdi_mode);  

  gnome_config_pop_prefix ();
}

void prefs_save ()
{
  gnome_config_push_prefix ("/gcolorsel/gcolosel/");

  gnome_config_set_bool ("SaveSession", prefs.save_session);
  gnome_config_set_bool ("DisplayDoc", prefs.display_doc);

  gnome_config_set_int ("OnDrop", prefs.on_drop);
  gnome_config_set_int ("OnGrab", prefs.on_grab);
  gnome_config_set_int ("OnViews", prefs.on_views);
  gnome_config_set_int ("OnPreviews", prefs.on_previews);

  gnome_config_set_int ("OnDrop2", prefs.on_drop2);
  gnome_config_set_int ("OnGrab2", prefs.on_grab2);
  gnome_config_set_int ("OnViews2", prefs.on_views2);
  gnome_config_set_int ("OnPreviews2", prefs.on_previews2);

  gnome_config_set_int ("TabPos", prefs.tab_pos);
  gnome_config_set_int ("MDIMode", prefs.mdi_mode);

  gnome_config_pop_prefix ();
  gnome_config_sync ();
}

static void 
set_menu ()
{
  if (prefs.display_doc) {
    gnome_mdi_set_menubar_template (mdi, main_menu_with_doc);
    gnome_mdi_set_child_list_path (mdi, GNOME_MENU_FILES_PATH);
  } else {
    gnome_mdi_set_menubar_template (mdi, main_menu);
    gnome_mdi_set_child_list_path (mdi, NULL);
  }
}

/***************************** Action ****************************************/

static MDIColorGeneric *
search_mcg_from_key (int key)
{
  GList *list = mdi->children;
  MDIColorGeneric *mcg;

  while (list) {
    mcg = list->data;

    if (mcg->key == key) return mcg;

    list = g_list_next (list);
  }

  return NULL;
}

static void
actions_fail ()
{
  GtkWidget *dia;
  char *str;

  str = _("I can't execute this action. Check that :\n\n1. You have associated an action with this event.\n2. The associated document still exits.\n3. The type of the associated document.\n\nGo to preferences, in the edit menu.");

  dia = gnome_message_box_new (str, GNOME_MESSAGE_BOX_WARNING,
			       GNOME_STOCK_BUTTON_OK, NULL);

  gtk_widget_show (GTK_WIDGET (dia));
//  gnome_dialog_run_and_close (GNOME_DIALOG (dia)); 
  /* don't work ... */
}

static void
actions (int r, int g, int b, char *name, 
	 MDIColor *col,
	 actions_t type, int key)
{
  MDIColorGeneric *mcg;

  switch (type) {
  case ACTIONS_NOTHING:
    actions_fail ();
    break;

  case ACTIONS_APPEND:
    mcg = search_mcg_from_key (key);

    if ((mcg) && (mdi_color_generic_can_do (mcg, CHANGE_APPEND))) {
      if (col)
	mdi_color_generic_append_new (mcg, col->r, col->g, col->b, col->name);
      else
	mdi_color_generic_append_new (mcg, r, g, b, name);
    } else
      actions_fail ();

    break;

  case ACTIONS_EDIT:
    menu_edit (col);
    break;

  case ACTIONS_SEARCH:
    mcg = search_mcg_from_key (key);

    if ((mcg) && (IS_MDI_COLOR_VIRTUAL (mcg))) {
      GList *list;

      if (! col)
	mdi_color_virtual_rgb_set (MDI_COLOR_VIRTUAL_RGB (mcg), 
				   r, g, b, 
				   MDI_COLOR_VIRTUAL_RGB (mcg)->t);
      else
	mdi_color_virtual_rgb_set (MDI_COLOR_VIRTUAL_RGB (mcg), 
				   col->r, col->g, col->b, 
				   MDI_COLOR_VIRTUAL_RGB (mcg)->t);
      
      list = GNOME_MDI_CHILD (mcg)->views;
      g_assert (list != NULL);

      gnome_mdi_set_active_view (mdi, GTK_WIDGET (list->data));
    } else 
      actions_fail ();

    break;

  default:
    g_assert_not_reached ();
  }
}

void 
actions_drop (int r, int g, int b)
{
  actions (r, g, b, "Dropped", NULL, prefs.on_drop, prefs.on_drop2);
}

void 
actions_grab (int r, int g, int b)
{
  actions (r, g, b, "Grabbed", NULL, prefs.on_grab, prefs.on_grab2);
}

void 
actions_previews (int r, int g, int b)
{
  actions (r, g, b, "New", NULL, prefs.on_previews, prefs.on_previews2);
}

void 
actions_views (MDIColor *col)
{
  actions (0, 0, 0, NULL, col, prefs.on_views, prefs.on_views2);
}

/*********************** Applet **************************/

#ifdef HAVE_GNOME_APPLET

typedef struct data_t {
  GtkWidget *applet;
  
  GtkWidget *dialog;
  GtkWidget *view;
  
  GtkWidget *button;
  GtkWidget *arrow;
  
  PanelOrientType orient;
  int size;
} data_t;

static void
applet_button_clicked (GtkWidget *widget, data_t *data)
{
  GtkAllocation dsize;
  gint x1, y1, x2, y2, x, y;
  
  if (GTK_WIDGET_VISIBLE (data->dialog))
    gtk_widget_hide (data->dialog);
  else {
    
    dsize = data->dialog->allocation;   
    gdk_window_get_origin (data->applet->window, &x1, &y1);
    x2 = x1 + data->applet->allocation.width;
    y2 = y1 + data->applet->allocation.height;
    if (data->orient == ORIENT_UP)
      y = MAX (y1 - dsize.height, 0);
    else if (data->orient == ORIENT_DOWN)
      y = MIN (y2, gdk_screen_height () - dsize.height);
    else
      y = CLAMP (y1, 0, gdk_screen_height () - dsize.height);
    if (data->orient == ORIENT_LEFT)
      x = MAX (x1 - dsize.width, 0);   
    else if (data->orient == ORIENT_RIGHT)
      x = MIN (x2, gdk_screen_width () - dsize.width);
    else
      x = CLAMP (x1, 0, gdk_screen_width () - dsize.width);
    gtk_widget_set_uposition (data->dialog, x, y);        
    
    gtk_widget_show_all (data->dialog);    
    gdk_window_raise (data->dialog->window);  
  }    
}

static void
applet_create_dialog (data_t *data)
{
  GtkWidget *frame, *box;
  MDIColorGeneric *mcg;
  char *buf;
  GList *list;
  gboolean load = FALSE;
  
  /* Dialog */
  
  data->dialog = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_usize (GTK_WIDGET (data->dialog), 300, 250);
  gtk_widget_set_app_paintable (GTK_WIDGET (data->dialog), TRUE);
  gtk_widget_size_request (data->dialog, NULL);
  gtk_widget_realize (data->dialog);
  
  /* Frame */
  
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (data->dialog), frame);

  /* Box */
  
  box = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
  gtk_container_add (GTK_CONTAINER (frame), box);

  /* Create document / Or use document if already opened */
    
  buf = gnome_util_prepend_user_home ("/.gcolorsel_colors");
    
  list = mdi->children;
  while (list) {
    if (IS_MDI_COLOR_FILE (list->data))
      if (! strcmp (MDI_COLOR_FILE (list->data)->filename, buf)) break;
    
      list = g_list_next (list);
  }
    
  if (list) 
    mcg = list->data;
  else {
    mcg = MDI_COLOR_GENERIC (mdi_color_file_new ());
    gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (mcg));
    
    mdi_color_file_set_filename (MDI_COLOR_FILE (mcg), buf, TRUE);
    mdi_color_generic_set_name (mcg, _("User Colors"));
      
    load = TRUE;
  }
    
  g_free (buf);
    
  /* Create view */
      
  data->view = mdi_color_generic_create_other_view (mcg);
  gtk_box_pack_start_defaults (GTK_BOX (box), data->view);
 
  /* Load document, if needed */
  
  if (load)
    mdi_color_file_load (MDI_COLOR_FILE (mcg), mdi);
      
  gnome_mdi_register (mdi, GTK_OBJECT (APPLET_WIDGET (data->applet)));      
}

static void
applet_change_pixel_size (AppletWidget *applet, int size, data_t *data)
{
  if ((data->orient == ORIENT_UP) || (data->orient == ORIENT_DOWN))
    gtk_widget_set_usize (data->button, 0, size);
  else
    gtk_widget_set_usize (data->button, size, 0); 
    
  data->size = size;    
}

static void
applet_change_orient (AppletWidget *applet, PanelOrientType o, data_t *data)
{
  GtkArrowType arrow_type = GTK_ARROW_UP;
  
  printf ("Change orient ...\n");
  
  data->orient = o;  
  
  switch (o) {
    case ORIENT_UP:
      arrow_type = GTK_ARROW_UP; break;    
    case ORIENT_DOWN:
      arrow_type = GTK_ARROW_DOWN; break;    
    case ORIENT_LEFT:
      arrow_type = GTK_ARROW_LEFT; break;    
    case ORIENT_RIGHT:  
      arrow_type = GTK_ARROW_RIGHT; break;    
  }
  
  gtk_arrow_set (GTK_ARROW (data->arrow), arrow_type, GTK_SHADOW_OUT);    
  
  applet_change_pixel_size (applet, data->size, data);
}

static GtkWidget *
applet_start_new_applet (const char *goad_id, 
			 const char **params, gint nparams)
{  
  data_t *data;

  data = g_new0 (data_t, 1);

  printf ("applet_start_new_applet : %s\n", goad_id);
  if (strcmp (goad_id, "gcolorsel_applet")) return NULL;

  data->applet = applet_widget_new ("hello_applet");
  if (! data->applet) 
    g_error("Can't create applet!\n");

  gtk_signal_connect (GTK_OBJECT (data->applet), "change_pixel_size",
                      GTK_SIGNAL_FUNC (applet_change_pixel_size), data);    
                      
  gtk_signal_connect (GTK_OBJECT (data->applet), "change_orient",
                      GTK_SIGNAL_FUNC (applet_change_orient), data);                      

  applet_create_dialog (data);    
  menu_configure_applet (APPLET_WIDGET (data->applet), 
      VIEW_COLOR_GENERIC (gtk_object_get_data (GTK_OBJECT (data->view), "view_object")));
                                                                          
  data->arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);    

  data->button = gtk_widget_new (GTK_TYPE_BUTTON,
                                 "visible", TRUE,
                                 "can_focus", FALSE,
                                 "child", data->arrow,
                                 "signal::clicked", applet_button_clicked, data,
                                 NULL);

  applet_widget_add (APPLET_WIDGET (data->applet), data->button);
  
  gtk_widget_show_all (data->button);
  gtk_widget_show (data->applet);

  applet_widget_send_draw (APPLET_WIDGET (data->applet), TRUE);

  return data->applet;
}

#endif

/**************************** Main *******************************/

int main (int argc, char *argv[])
{
  char *goad_id;
  gboolean applet_mode = FALSE;

  /* Initialize i18n */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);
  
  if (strstr(argv[0], "gcolorsel_applet")) 
    applet_mode = TRUE;
  
#ifdef HAVE_GNOME_APPLET
  if (applet_mode)
    applet_widget_init ("gcolorsel_applet", NULL, argc, argv, NULL, 0, NULL); 
#else
  if (applet_mode) 
    g_error ("This version of GColorsel doesn't support Gnome Applet !"); 
#endif

  if (! applet_mode)  
    gnome_init ("gcolorsel", VERSION, argc, argv);    

  gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-color-browser.png");
  glade_gnome_init ();
  
  /* Init GnomeMDI */
  mdi = GNOME_MDI (gnome_mdi_new ("gcolorsel", _("GColorsel")));

  gtk_signal_connect (GTK_OBJECT (mdi), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_signal_connect (GTK_OBJECT (mdi), "remove_child", 
		      GTK_SIGNAL_FUNC (mdi_remove_child), NULL);
  gtk_signal_connect (GTK_OBJECT (mdi), "app_created",
		      GTK_SIGNAL_FUNC (app_created), NULL);
		      
  /* Init menu/toolbar */
  set_menu ();
  gnome_mdi_set_toolbar_template (mdi, toolbar);		      
		  
  /* For clipboard */
  event_widget = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_selection_add_target (event_widget,
			    GDK_SELECTION_PRIMARY,
			    GDK_SELECTION_TYPE_STRING, 1);
  gtk_signal_connect (GTK_OBJECT (event_widget), "selection_clear_event",
		      GTK_SIGNAL_FUNC (selection_clear), NULL);
  gtk_signal_connect (GTK_OBJECT (event_widget), "selection_get",
		      GTK_SIGNAL_FUNC (selection_get), NULL);
  gtk_signal_connect (GTK_OBJECT (event_widget), "selection_received",
		      GTK_SIGNAL_FUNC (selection_received), NULL);
		      
  /* For gtk_type_from_name in session.c */
  call_get_type ();		      
  
  /* Load prefs */
  prefs_load ();

#ifdef HAVE_GNOME_APPLET

  if (applet_mode) {

    applet_factory_new ("gcolorsel_applet_factory", NULL, applet_start_new_applet);
    goad_id = (char *)goad_server_activation_id ();

    printf ("goad_id : %s\n", goad_id);

    applet_widget_gtk_main ();
    
    return 0;
  }
#endif

  /* Load old session if user want that */
  if (prefs.save_session) {
    if (! session_load (mdi)) session_create (mdi, TRUE);
  } else
    /* Else, construct a default session */
    session_create (mdi, FALSE);

  /* Load all file */
  session_load_data (mdi);

  gtk_main ();

  return 0;
}

