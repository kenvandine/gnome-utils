#include <config.h>
#include <gnome.h>

#include <gdk/gdkx.h>

typedef enum {
  DECIMAL_8BIT, 
  DECIMAL_16BIT,
  HEX_8BIT, 
  HEX_16BIT,
  FLOAT
} FormatType;

typedef struct 
{
  char name[256];
  int r,g,b;
  int num;
  float diff;
  int ref;
} RGBColor;

char *rgb_txt[] = { "/usr/X11R6/lib/X11/rgb.txt",
                    "/usr/X11/lib/X11/rgb.txt",
                    "/usr/openwin/lib/X11/rgb.txt",
                    "/usr/lpp/X11/lib/X11/rgb.txt",
                    "/usr/lib/X11/rgb.txt",
                    NULL };

char *favorites_rgb_txt[] = { ".gcolorsel_rgb.txt", 
			      /* We add home path in main */
			      NULL };

/* Menus */

static void menu_about_cb     (GtkWidget *widget, gpointer data);
static void menu_format_cb    (GtkWidget *widget, gpointer data);
static void menu_search_in_cb (GtkWidget *widget, gpointer data);
static void menu_update_cb    (GtkWidget *widget, gpointer data);
static void menu_on_drop_cb   (GtkWidget *widget, gpointer data);
static void menu_exit_cb      (GtkWidget *widget, gpointer data);
static void menu_grab_cb      (GtkWidget *widget, gpointer data);
static void menu_add_favorites_cb    (GtkWidget *widget, gpointer data);
static void menu_remove_favorites_cb (GtkWidget *widget, gpointer data);
static void menu_rename_cb           (GtkWidget *widget, gpointer data);

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_MENU_ABOUT_ITEM(menu_about_cb,NULL),
	GNOMEUIINFO_END
};
  
static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_MENU_EXIT_ITEM(menu_exit_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo format_menu2[] = {
        GNOMEUIINFO_RADIOITEM_DATA(N_("Decimal (8 bit)"), NULL, 
				   menu_format_cb, GINT_TO_POINTER(0), NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Decimal (16 bit)"), NULL, 
				   menu_format_cb, GINT_TO_POINTER(1), NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Hex (8 bit)"), NULL,  
				   menu_format_cb, GINT_TO_POINTER(2), NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Hex (16 bit)"), NULL, 
				   menu_format_cb, GINT_TO_POINTER(3), NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Float"), NULL, 
				   menu_format_cb, GINT_TO_POINTER(4), NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo format_menu[] = {
        GNOMEUIINFO_RADIOLIST(format_menu2),
        GNOMEUIINFO_END
};

static GnomeUIInfo update_menu2[] = {
        GNOMEUIINFO_RADIOITEM_DATA(N_("Continuous (Slow)"), NULL, 
				   menu_update_cb, 
				   GINT_TO_POINTER(GTK_UPDATE_CONTINUOUS), 
				   NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Delayed"), NULL, 
				   menu_update_cb, 
				   GINT_TO_POINTER(GTK_UPDATE_DELAYED), 
				   NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Discontinuous"), NULL,  
				   menu_update_cb, 
				   GINT_TO_POINTER(GTK_UPDATE_DISCONTINUOUS), 
				   NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo update_menu[] = {
        GNOMEUIINFO_RADIOLIST(update_menu2),
        GNOMEUIINFO_END
};

static GnomeUIInfo search_in_menu[] = {
        GNOMEUIINFO_TOGGLEITEM_DATA(N_("Simple"), NULL, 
				    menu_search_in_cb, 
				    GINT_TO_POINTER(0), NULL),
        GNOMEUIINFO_TOGGLEITEM_DATA(N_("Favorites"), NULL, 
				    menu_search_in_cb, 
				    GINT_TO_POINTER(1), NULL),
        GNOMEUIINFO_END
};

static GnomeUIInfo on_drop_menu2[] = {
        GNOMEUIINFO_RADIOITEM_DATA(N_("Search it"), NULL,
				   menu_on_drop_cb, 
				   GINT_TO_POINTER(0), NULL),
        GNOMEUIINFO_RADIOITEM_DATA(N_("Add it to favorites"), NULL,
				   menu_on_drop_cb, 
				   GINT_TO_POINTER(1), NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo on_drop_menu[] = {
        GNOMEUIINFO_RADIOLIST(on_drop_menu2),
        GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
        GNOMEUIINFO_SUBTREE (N_("Format"), format_menu),
	GNOMEUIINFO_SUBTREE (N_("Search update"), update_menu),
	GNOMEUIINFO_SUBTREE (N_("Search in"), search_in_menu),
	GNOMEUIINFO_SUBTREE (N_("On drop"), on_drop_menu),
	GNOMEUIINFO_END
};
 
static GnomeUIInfo main_menu[] = {
        GNOMEUIINFO_MENU_FILE_TREE(file_menu),
	GNOMEUIINFO_MENU_SETTINGS_TREE(settings_menu),
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
        GNOMEUIINFO_END
};

/* Popup */

static GnomeUIInfo popup_menu_add[] = {
        GNOMEUIINFO_ITEM_STOCK (N_("Add to favorites"), NULL,    
			        menu_add_favorites_cb, 
				GNOME_STOCK_PIXMAP_ADD),
	GNOMEUIINFO_END
};

static GnomeUIInfo popup_menu_remove[] = {
        GNOMEUIINFO_ITEM_STOCK (N_("Remove"), NULL,
			        menu_remove_favorites_cb, 
				GNOME_STOCK_PIXMAP_REMOVE),
        GNOMEUIINFO_ITEM_NONE (N_("Rename"), NULL, menu_rename_cb),
	GNOMEUIINFO_END
};

/* Toolbar */

#define TOOLBAR_ADD_POS    4
#define TOOLBAR_REMOVE_POS 5

static GnomeUIInfo main_toolbar[] = {
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

GtkWidget *window;
GtkWidget *notebook;
GtkWidget *clist;
GtkWidget *clist_found;
GtkWidget *clist_favorites;

GdkGC *gc;
GdkColor black;


static void       window_delete_event_cb (GtkWidget *widget, gpointer *data);

static GtkWidget *clist_create          (gboolean with_diff,
					 GnomeUIInfo *popup);
static int        clist_load_rgb        (GtkCList *clist, GtkWidget *progress,
					 char *rgb_txt[]);
static int        clist_save_rgb        (GtkCList *clist, char *filename);
static gint       clist_add_color       (GtkCList *clist, RGBColor *col, 
					 FormatType type);
static void       clist_find_color      (GtkCList *source, GtkCList *dest,
					 gint red, gint green, 
					 gint blue, gint tolerance);
static void       clist_set_sort_column (GtkCList *clist, gint column);
static void       clist_click_column_cb (GtkCList *clist, gint column);
static void       clist_select_row_cb   (GtkCList *clist, gint row, gint col,
	                                 GdkEvent *event, gpointer data);
static void       clist_button_press_cb  (GtkCList *clist, 
					  GdkEventButton *event,
					  GtkWidget *menu);
static void       clist_change_format   (GtkCList *clist, int type);
static void       clist_get_adjustment  (GtkCList *clist, 
					 GtkAdjustment **adj_red, 
					 GtkAdjustment **adj_green, 
					 GtkAdjustment **adj_blue,
					 GtkAdjustment **adj_tolerance);
static void       clist_get_range       (GtkCList *clist, GtkRange 
					 **range_red, GtkRange **range_blue,
					 GtkRange **range_green, 
					 GtkRange **range_tolerance);

static void 	  format_color          (FormatType type, 
					 char *str, int r, int g, int b);
static void       pixel_to_rgb          (GdkColormap *cmap, guint32 pixel, 
					 gint *red, gint *green, gint *blue);

static void       preview_update        (GtkCList *clist);
static void       range_update          (GtkCList *clist, 
					 int red, int green, int blue);
static void       range_value_changed_cb(GtkWidget *widget, gpointer data);

static void       grab_key_press_cb    (GtkWidget *widget,
					GdkEventKey *event, gpointer data);
static void       grab_button_press_cb (GtkWidget *widget, 
					GdkEventButton *event, 
					GtkWidget *data);

static void clist_drag_begin_cb      (GtkWidget *widget,
				      GdkDragContext *context, 
				      GtkCList *clist);
static void clist_drag_data_get_cb   (GtkWidget *widget,
				      GdkDragContext *context, 
				      GtkSelectionData *selection_data,
				      guint info, guint time, GtkCList *clist);

static void preview_button_press_cb  (GtkWidget *widget, 
				      GdkEventButton *event,
				      GtkWidget *menu);
static void preview_drag_begin_cb    (GtkWidget *widget,
				      GdkDragContext *context, 
				      GtkCList *clist);		        
static void preview_drag_data_get_cb (GtkWidget *widget,
				      GdkDragContext *context, 
				      GtkSelectionData *selection_data,
				      guint info, guint time, GtkCList *clist);

static void window_drop_data_cb      (GtkWidget *widget, 
				      GdkDragContext *context,
				      gint x, gint y, GtkSelectionData *data,
				      guint info, guint32 time, gpointer d);

static const GtkTargetEntry drag_targets[] = {
  { "application/x-color", 0 }
};

static const GtkTargetEntry drop_targets[] = {
  { "application/x-color", 0 }
};
		           
static void 
window_delete_event_cb (GtkWidget *widget, gpointer *data)
{
  GtkWidget *dialog;

  /* Save favorites */
  
  if (clist_save_rgb (GTK_CLIST (clist_favorites), favorites_rgb_txt[0])) {
    dialog = gnome_message_box_new(_("Error, cannot save favorites !"), 
				   "error",
				   GNOME_STOCK_BUTTON_OK, NULL);
    gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
  }

  if (gtk_main_level ()) 
    gtk_main_quit();
  
  exit (0);
}

static GtkWidget *
range_create (gchar *title, gfloat max, gfloat val, GtkWidget **prange,
	      GtkSignalFunc cb)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *range;
  GtkObject *adj;

  adj = gtk_adjustment_new (val, 0.0, max, 1.0, 1.0, 1.0);

  gtk_signal_connect (GTK_OBJECT (adj), "value_changed", cb, NULL);

  hbox = gtk_hbox_new (TRUE, 2);

  label = gtk_label_new (title);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

  range = gtk_hscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_value_pos (GTK_SCALE (range), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (range), 0);

  gtk_box_pack_start_defaults(GTK_BOX(hbox), range);

  if (prange) *prange = range;

  return hbox;
}

static GtkWidget *
search_tab_create ()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *range_vbox;
  GtkWidget *frame;
  GtkWidget *preview;
  GtkWidget *sw;

  GtkWidget *range_red;
  GtkWidget *range_blue;
  GtkWidget *range_green;
  GtkWidget *range_tolerance;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);

  range_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), range_vbox,
		      FALSE, FALSE, 2);

  /* Create entries */
  
  gtk_box_pack_start_defaults (GTK_BOX(range_vbox),
	      range_create (_("Red :"), 256.0, 0.0, &range_red,
			    range_value_changed_cb));

  gtk_box_pack_start_defaults (GTK_BOX(range_vbox),
	      range_create (_("Green : "), 256.0, 0.0, &range_green,
			    range_value_changed_cb));

  gtk_box_pack_start_defaults (GTK_BOX(range_vbox),
              range_create (_("Blue : "), 256.0, 0.0, &range_blue,
			    range_value_changed_cb));

  gtk_box_pack_start_defaults (GTK_BOX(range_vbox),
              range_create (_("Tolerance (%) : "), 101.0, 10.0, 
			    &range_tolerance, range_value_changed_cb));
 
  /* Create preview */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(hbox), frame, TRUE, TRUE, 2);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);    
  gtk_preview_set_expand (GTK_PREVIEW (preview), TRUE);  
  gtk_container_add (GTK_CONTAINER (frame), preview);

  gtk_drag_source_set (preview, GDK_BUTTON1_MASK,
		       drag_targets, 1, 
		       GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (preview), "button_press_event",
		      GTK_SIGNAL_FUNC (preview_button_press_cb), 
		      gnome_popup_menu_new (popup_menu_add));
  
  gtk_signal_connect (GTK_OBJECT (preview), "drag_data_get",
		      GTK_SIGNAL_FUNC (preview_drag_data_get_cb), preview);
  
  gtk_signal_connect (GTK_OBJECT (preview), "drag_begin",
		      GTK_SIGNAL_FUNC (preview_drag_begin_cb), preview);
  
  /* Create color list */
  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), sw);

  clist_found = clist_create (TRUE, popup_menu_add);
  gtk_container_add(GTK_CONTAINER(sw), clist_found);
  
  gtk_object_set_data(GTK_OBJECT(clist_found), "range_red", range_red);
  gtk_object_set_data(GTK_OBJECT(clist_found), "range_green", range_green);
  gtk_object_set_data(GTK_OBJECT(clist_found), "range_blue", range_blue);
  gtk_object_set_data(GTK_OBJECT(clist_found), "range_tolerance", 
		      range_tolerance);
  
  gtk_object_set_data(GTK_OBJECT(clist_found), "preview", preview);    

  /* Allow update */
  gtk_object_set_data (GTK_OBJECT (clist_found), "do_update",     
                       GINT_TO_POINTER(TRUE));

  return vbox;
}


int main(int argc, char **argv)
{
    GtkWidget *vbox;
    GtkWidget *label, *sw;
    GtkWidget *progress, *progress_label;

    gint i, on_drop;
    FormatType format;
    GtkUpdateType update;
        
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, GNOMELOCALEDIR);
    textdomain(PACKAGE);
#endif
      
    gnome_init("gcolorsel", VERSION, argc, argv);

    favorites_rgb_txt[0] = gnome_util_prepend_user_home (favorites_rgb_txt[0]);

    window = gnome_app_new("gcolorsel", _("Gnome Color Browser"));

    gtk_widget_set_usize(window, 320, 360);
    gtk_window_set_title(GTK_WINDOW(window), _("Gnome Color Browser"));
    gtk_window_set_wmclass(GTK_WINDOW(window), "main_window","gcolorsel");
    gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
    gtk_signal_connect( GTK_OBJECT(window), "delete_event",
			GTK_SIGNAL_FUNC(window_delete_event_cb), NULL);

    gnome_app_create_menus(GNOME_APP (window), main_menu);
    gnome_app_create_toolbar(GNOME_APP (window), main_toolbar);

    gtk_widget_realize(window);

    gtk_drag_dest_set (window, GTK_DEST_DEFAULT_ALL,
		       drop_targets, 1,
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_signal_connect (GTK_OBJECT (window), "drag_data_received",
			GTK_SIGNAL_FUNC (window_drop_data_cb), window);

    gc = gdk_gc_new(window->window);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(vbox), 2);
    gnome_app_set_contents(GNOME_APP(window), vbox);

    /* Create notebook */

    notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

    /* Create color list */
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    
    clist = clist_create (FALSE, popup_menu_add);
    gtk_container_add(GTK_CONTAINER(sw), clist);

    label = gtk_label_new (_("Simple"));				    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), 
			      sw, label);

    /* Page 2 */
    label = gtk_label_new (_("Search"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), 
			      search_tab_create (), label);

    /* Page 3 */
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    
    clist_favorites = clist_create (FALSE, popup_menu_remove);
    gtk_container_add(GTK_CONTAINER(sw), clist_favorites);

    label = gtk_label_new (_("Favorites"));				    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), 
			      sw, label);

    /* Create progress bar */
    progress_label = gtk_label_new (_("Parsing Colors"));
    gtk_box_pack_start (GTK_BOX (vbox), progress_label, FALSE, FALSE, 0);
    
    progress = gtk_progress_bar_new();
    gtk_box_pack_start (GTK_BOX (vbox), progress, FALSE, FALSE, 0);

	
    gtk_widget_show_all(window);

    /* Load preferences */

    format = gnome_config_get_int_with_default ("/gcolorsel/Prefs/Format=0", 
						NULL);
    
    for (i=0; i<sizeof (format_menu2) / sizeof (GnomeUIInfo) - 1; i++) 
      if (GPOINTER_TO_INT (format_menu2[i].user_data) == format) {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(format_menu2[i].widget), TRUE);
      }

    update = gnome_config_get_int_with_default ("/gcolorsel/Prefs/Update=0", 
						NULL);

    for (i=0; i<sizeof (update_menu2) / sizeof (GnomeUIInfo) - 1; i++) 
      if (GPOINTER_TO_INT (update_menu2[i].user_data) == update) {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(update_menu2[i].widget), TRUE);
      }

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(search_in_menu[0].widget), gnome_config_get_int_with_default("/gcolorsel/Prefs/Search_in_simple=1", NULL));
					
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(search_in_menu[1].widget), gnome_config_get_int_with_default("/gcolorsel/Prefs/Search_in_favorites=0", NULL));

    on_drop = gnome_config_get_int_with_default ("/gcolorsel/Prefs/OnDrop=0", 
						NULL);
    
    for (i=0; i<sizeof (on_drop_menu2) / sizeof (GnomeUIInfo) - 1; i++) 
      if (GPOINTER_TO_INT (on_drop_menu2[i].user_data) == on_drop) {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(on_drop_menu2[i].widget), TRUE);
      }

	
    /* Load global rgb.txt */
    gtk_clist_freeze(GTK_CLIST(clist));
    if (clist_load_rgb(GTK_CLIST(clist), progress, rgb_txt)) {
      gtk_widget_show(gnome_message_box_new(
	 _("Error, cannot find the file 'rgb.txt' on your system!"), 
	 "error", GNOME_STOCK_BUTTON_OK, NULL));
    }
    gtk_clist_sort (GTK_CLIST(clist));    
    gtk_clist_thaw(GTK_CLIST(clist));

    /* Load favorites */
    gtk_label_set_text (GTK_LABEL (progress_label), "Parsing Favorites");

    gtk_clist_freeze(GTK_CLIST(clist_favorites));
    clist_load_rgb(GTK_CLIST(clist_favorites), progress, favorites_rgb_txt);
    gtk_clist_sort (GTK_CLIST(clist_favorites));    
    gtk_clist_thaw(GTK_CLIST(clist_favorites));

    /* Search color Black ... */
    range_value_changed_cb (NULL, NULL);
    
    gtk_widget_destroy (progress);
    gtk_widget_destroy (progress_label);
    
    gtk_main();

    return EXIT_SUCCESS;
}

static GtkWidget *
clist_create_label_arrow (GtkCList *clist, gint col, 
			  gchar *str, gboolean showit)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  gchar *key;

  hbox = gtk_hbox_new (FALSE, 2);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(hbox), arrow, FALSE, FALSE, 0);

  label = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_widget_show (hbox);
  gtk_widget_show (label);
  if (showit) gtk_widget_show (arrow);

  gtk_clist_set_column_widget (clist, col, hbox);

  key = g_strdup_printf ("arrow_%d", col);  
  gtk_object_set_data (GTK_OBJECT (clist), key, arrow);
  g_free (key);
  
  return hbox;
}

static GtkWidget *
clist_create(gboolean with_diff, GnomeUIInfo *popup)
{
    GdkColormap *colormap;
    GtkWidget *clist;

    g_assert (popup != NULL);
        
    clist = gtk_clist_new(with_diff ? 4 : 3);;
    gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_EXTENDED);

    gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_IN);

    gtk_clist_set_row_height(GTK_CLIST(clist), 18);

    clist_create_label_arrow (GTK_CLIST(clist), 0, _("Color"), FALSE);
    gtk_clist_set_column_width(GTK_CLIST(clist), 0, 52);

    clist_create_label_arrow (GTK_CLIST(clist), 1, _("Value"), FALSE);
    gtk_clist_set_column_width(GTK_CLIST(clist), 1, 72);

    clist_create_label_arrow (GTK_CLIST(clist), 2, _("Name"), !with_diff);
    gtk_clist_set_column_width(GTK_CLIST(clist), 2, 80);

    if (with_diff) {
      gtk_clist_set_column_width(GTK_CLIST(clist), 3, 15);
      clist_create_label_arrow (GTK_CLIST(clist), 3, _("Match"), TRUE);

      clist_set_sort_column (GTK_CLIST(clist), 3);
    } else 
      clist_set_sort_column (GTK_CLIST(clist), 2);

    gtk_clist_set_auto_sort (GTK_CLIST(clist), TRUE);
    gtk_clist_column_titles_show(GTK_CLIST(clist));
    gtk_clist_column_titles_active (GTK_CLIST (clist));
    
    gtk_signal_connect (GTK_OBJECT (clist), "click_column",
        		GTK_SIGNAL_FUNC (clist_click_column_cb), NULL);
        		
    gtk_signal_connect (GTK_OBJECT (clist), "select_row",
    			GTK_SIGNAL_FUNC (clist_select_row_cb), NULL);        		

    gtk_drag_source_set (clist, GDK_BUTTON1_MASK,
          	         drag_targets, 1, 
    		         GDK_ACTION_COPY);
    
    gtk_signal_connect (GTK_OBJECT (clist), "drag_data_get",
    			GTK_SIGNAL_FUNC (clist_drag_data_get_cb), clist);
    			
    gtk_signal_connect (GTK_OBJECT (clist), "drag_begin",
    			GTK_SIGNAL_FUNC (clist_drag_begin_cb), clist);

    gtk_clist_set_use_drag_icons (GTK_CLIST (clist), FALSE);

    /* Set popup menu */

    gtk_signal_connect (GTK_OBJECT (clist), "button_press_event",
			GTK_SIGNAL_FUNC (clist_button_press_cb), 
			gnome_popup_menu_new (popup));

    colormap = gtk_widget_get_colormap(clist);
    gdk_color_parse("black", &black);
    gdk_color_alloc(colormap, &black);
	
    gtk_object_set_data(GTK_OBJECT(clist), "colormap", colormap);

    return clist;
}

static void 
color_destroy_notify_cb (gpointer data)
{
  RGBColor *c = data;

  g_assert (c != NULL);

  if (--(c->ref)) return;

  g_free (data);
}

static gint
clist_add_color (GtkCList *clist, RGBColor *c, FormatType type)
{
    gchar *string[4];
    gint row;
    GdkPixmap *pixmap;
    GdkColor color;

    g_assert (clist != NULL);
    g_assert (c != NULL);
    g_assert (c->name != NULL);

    c->ref++;

    color.red = c->r * 255;
    color.green = c->g * 255;
    color.blue = c->b * 255;
   
    gdk_color_alloc(gtk_object_get_data(GTK_OBJECT(clist), "colormap"), &color);
       
    pixmap = gdk_pixmap_new(window->window,
			    48, 16, gtk_widget_get_visual(window)->depth);
	
    gdk_gc_set_foreground(gc, &color);
    gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, 47, 15);
    gdk_gc_set_foreground(gc, &black);
    gdk_draw_rectangle(pixmap, gc, FALSE, 0, 0, 47, 15);

    string[0] = NULL;
    string[1] = g_malloc(64);
    string[2] = c->name;
    string[3] = g_malloc(10);
	
    format_color(type, string[1], c->r, c->g, c->b);
    sprintf (string[3], "%d", (int)c->diff);
	
    row = gtk_clist_append(clist, string);
    gtk_clist_set_pixmap (clist, row, 0, pixmap, NULL);
    gtk_clist_set_row_data_full (clist, row, c, 
				 color_destroy_notify_cb);
		
    g_free(string[1]);
    g_free(string[3]);
    
    return row;
}

/* grabbed from ee */
#define GTK_FLUSH \
    while (gtk_events_pending()) \
            gtk_main_iteration();

static int
clist_load_rgb(GtkCList *clist, GtkWidget *progress, char *files[])
{
    gchar tmp[256];
    gint t,i=0;
    FILE *file = NULL;
    glong flen;
    gint iter;
    FormatType type;

    g_assert (clist != NULL);
    g_assert (progress != NULL);
    g_assert (files != NULL);
	
    while(files[i] != NULL) {
        if ((file = fopen(files[i], "r")) != NULL)
            break;
        i++;
    }

    if(!file) return -1;

    type = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (clist),"format"));

    fseek(file, 0, SEEK_END);
    flen = ftell(file);
    fseek(file, 0, SEEK_SET);

    gtk_progress_bar_update(GTK_PROGRESS_BAR(progress), 0);

    for(iter = 0;;)
    {
	int or=257, og=257, ob=257;
	gfloat p;
	RGBColor *color;
		
	fgets(tmp, 255, file);

	if(feof(file))
	    break;

	color = g_new(RGBColor, 1);
	color->ref = 0;
	t = sscanf(tmp, "%d %d %d\t\t%[a-zA-Z0-9 ]\n",
		   &color->r,
		   &color->g,
		   &color->b,
		   color->name);

	if(t==4 && (color->r != or || color->g != og || color->b != ob))
	{
	    color->num = iter;

	    clist_add_color (clist, color, type);

	    or = color->r;
	    og = color->g;
	    ob = color->b;
	
   	    if (! (iter % 5)) {
  		p = (gfloat)ftell(file)/(gfloat)flen;
		gtk_progress_bar_update(GTK_PROGRESS_BAR(progress), p); 
		GTK_FLUSH;
   	    }
   	    
   	    iter++;
   	}
    }

    return 0;
}

static int        
clist_save_rgb (GtkCList *clist, char *filename)
{
  FILE *file;
  GList *row_list;
  GtkCListRow *row;
  RGBColor *color;
  gchar tmp[256];

  g_assert (clist != NULL);
  g_assert (filename != NULL);

  file = fopen (filename, "w");
  if (!file) return -1;
  
  row_list = clist->row_list;

  fputs ("! gcolorsel's favorites\n", file);

  while (row_list) {
    row = row_list->data;
    g_assert (row != NULL);

    color = row->data; 
    g_assert (color != NULL);

    sprintf (tmp, "%d %d %d\t\t%s\n", 
	     color->r, color->g, color->b, color->name);

    fputs (tmp, file);   

    row_list = row_list->next;
  }

  fclose (file);

  return 0;
}

static void 
format_color(FormatType type, gchar *str, gint r, gint g, gint b)
{
  g_assert (str != NULL);

  switch(type) {
  case DECIMAL_8BIT:
    g_snprintf(str, 64, "%d %d %d", r,g,b);
    return;
  case DECIMAL_16BIT:
    g_snprintf(str, 64, "%d %d %d", r*256,g*256,b*256);
    return;
  case HEX_8BIT:
    g_snprintf(str, 64, "#%02x%02x%02x", r,g,b);
    return;
  case HEX_16BIT:
    g_snprintf(str, 64, "#%04x%04x%04x", r*256,g*256,b*256);
    return;
  case FLOAT:
    g_snprintf(str, 64, "%1.4g %1.4g %1.4g", 
	       (float)r/256,
	       (float)g/256,
	       (float)b/256);
    return;
  default:
    g_assert_not_reached ();
  }
}

/* From gdk-pixbuf, gdk-pixbuf-drawable.c */       
void pixel_to_rgb (GdkColormap *cmap, guint32 pixel, 
		   gint *red, gint *green, gint *blue)
{
  GdkVisual *v;

  g_assert (cmap != NULL);
  g_assert (red != NULL);
  g_assert (green != NULL);
  g_assert (blue != NULL);

  v = gdk_colormap_get_visual (cmap);

  switch (v->type) {
    case GDK_VISUAL_STATIC_GRAY:
    case GDK_VISUAL_GRAYSCALE:
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_PSEUDO_COLOR:
      *red   = cmap->colors[pixel].red;
      *green = cmap->colors[pixel].green;
      *blue  = cmap->colors[pixel].blue;
      break;
    case GDK_VISUAL_TRUE_COLOR:
      *red   = ((pixel & v->red_mask)   << (32 - v->red_shift   - v->red_prec))   >> 24;
      *green = ((pixel & v->green_mask) << (32 - v->green_shift - v->green_prec)) >> 24;
      *blue  = ((pixel & v->blue_mask)  << (32 - v->blue_shift  - v->blue_prec))  >> 24;
                 
      break;
    case GDK_VISUAL_DIRECT_COLOR:
      *red   = cmap->colors[((pixel & v->red_mask)   << (32 - v->red_shift   - v->red_prec))   >> 24].red;
      *green = cmap->colors[((pixel & v->green_mask) << (32 - v->green_shift - v->green_prec)) >> 24].green; 
      *blue  = cmap->colors[((pixel & v->blue_mask)  << (32 - v->blue_shift  - v->blue_prec))  >> 24].blue;
      break;
  default:
    g_assert_not_reached ();
  }  
}

static void 
clist_change_format (GtkCList *clist, gint type)
{
  RGBColor *color;
  gint i;
  gchar tmp[64];  

  g_assert (clist != NULL);

  gtk_object_set_data (GTK_OBJECT (clist), "format",
		       GINT_TO_POINTER (type));
  
  for(i=0; i < GTK_CLIST (clist)->rows; i++) {
    color = gtk_clist_get_row_data(GTK_CLIST(clist), i);

    g_assert (color != NULL);

    format_color(type, tmp, color->r, color->g, color->b);
    gtk_clist_set_text(GTK_CLIST(clist), i, 1, tmp);
  }            
}

/********************* MENU CALLBACKS **********************/

static void 
menu_format_cb(GtkWidget *menuitem, gpointer data)
{
  gint type;

  type = GPOINTER_TO_INT(data);

  gnome_config_set_int ("/gcolorsel/Prefs/Format", type);
  gnome_config_sync ();
  
  clist_change_format (GTK_CLIST (clist), type);
  clist_change_format (GTK_CLIST (clist_found), type);
}

static void 
menu_update_cb(GtkWidget *menuitem, gpointer data)
{
  GtkRange *range_red, *range_green, *range_blue, *range_tolerance;
  gint type;

  type = GPOINTER_TO_INT(data);

  gnome_config_set_int ("/gcolorsel/Prefs/Update", type);
  gnome_config_sync ();

  clist_get_range (GTK_CLIST (clist_found), &range_red, &range_green,
		   &range_blue, &range_tolerance);

  gtk_range_set_update_policy (range_red, type);
  gtk_range_set_update_policy (range_blue, type);
  gtk_range_set_update_policy (range_green, type);
  gtk_range_set_update_policy (range_tolerance, type);
}

static void 
menu_search_in_cb (GtkWidget *widget, gpointer data)
{
  gnome_config_set_int ("/gcolorsel/Prefs/Search_in_simple", 
		      GTK_CHECK_MENU_ITEM (search_in_menu[0].widget)->active);

  gnome_config_set_int ("/gcolorsel/Prefs/Search_in_favorites", 
		      GTK_CHECK_MENU_ITEM (search_in_menu[1].widget)->active);

  gnome_config_sync ();

  range_value_changed_cb (NULL, NULL);
}

static void 
menu_on_drop_cb (GtkWidget *widget, gpointer data)
{
  gint type;

  type = GPOINTER_TO_INT(data);

  gnome_config_set_int ("/gcolorsel/Prefs/OnDrop", type);
  gnome_config_sync ();
}
 
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
          
    return;
}

static void 
menu_exit_cb (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();  
}

static void 
menu_grab_cb (GtkWidget *widget, gpointer data)
{
  GdkCursor *cursor;

  gtk_grab_add (window);

  cursor = gdk_cursor_new (GDK_CROSS_REVERSE);

  gdk_pointer_grab (window->window, FALSE, 
      		    GDK_BUTTON_PRESS_MASK,
  		    NULL, cursor, GDK_CURRENT_TIME);
  		    
  gdk_cursor_destroy (cursor);

  gdk_keyboard_grab (window->window, FALSE, GDK_CURRENT_TIME);

  gtk_signal_connect (GTK_OBJECT (window), "button_press_event",
  	              GTK_SIGNAL_FUNC (grab_button_press_cb), window);

  gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
  	              GTK_SIGNAL_FUNC (grab_key_press_cb), window);
}

static void 
menu_add_favorites_cb (GtkWidget *widget, gpointer data)
{
  GList *l;
  GtkCList *list;
  RGBColor *color, *new;

  if (GTK_IS_CLIST (data)) {
    list = data;
    l = GTK_CLIST(list)->selection;

    gtk_clist_freeze (GTK_CLIST(clist_favorites));
    
    while (l) {
      
      color = gtk_clist_get_row_data (list, GPOINTER_TO_INT (l->data));
      g_assert (color != NULL);

      new = g_new (RGBColor, 1);
      *new = *color;
      new->ref = 0;
      
      clist_add_color (GTK_CLIST(clist_favorites), new, 
        GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),"format")));
   
      l=l->next;
    }

    gtk_clist_thaw (GTK_CLIST(clist_favorites));
  } else {
    g_assert (GTK_IS_PREVIEW (data));

    new = g_new (RGBColor, 1);
    strcpy (new->name, _("No Name"));
    new->r = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(data),"red"));
    new->g = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(data),"green"));
    new->b = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(data),"blue"));
    new->ref = 0;

    clist_add_color (GTK_CLIST(clist_favorites), new,
     GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),"format")));
  }

  range_value_changed_cb (NULL, NULL);
}

static void 
menu_remove_favorites_cb (GtkWidget *widget, gpointer data)
{
  GtkCList *list;
  GList *l;

  list = GTK_CLIST (clist_favorites);

  gtk_clist_freeze (list);

  l = GTK_CLIST(list)->selection;    
  while (l) {
    gtk_clist_remove (list, GPOINTER_TO_INT (l->data));
    
    l = GTK_CLIST(list)->selection;    
  }

  gtk_clist_thaw (list);
  
  range_value_changed_cb (NULL, NULL);
}

static void
menu_rename_end_cb (gchar *str, gpointer data)
{
  int row;
  RGBColor *color;

  if (!str) return;

  row = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (data),
					      "row_clicked"));
  color = gtk_clist_get_row_data (GTK_CLIST(data), row);
  g_assert (color != NULL);

  strcpy (color->name, str);

  gtk_clist_set_text (GTK_CLIST (data), row, 2, str);
  
  g_free (str);
}

static void
menu_rename_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  RGBColor  *color;
  gchar     *str;
  int       row;

  row = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (data), 
					      "row_clicked"));

  color = gtk_clist_get_row_data (GTK_CLIST(data), row);
  g_assert (color != NULL);

  str = g_strconcat (_("Enter new name for : "), color->name, NULL);

  dialog = gnome_request_dialog (FALSE, str, color->name, 250,
				 menu_rename_end_cb, data, 
				 GTK_WINDOW (window));

  g_free (str);

  gnome_dialog_run (GNOME_DIALOG (dialog));
}

/************************* GRAB COLOR *******************************/

static void
grab_key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_Escape) {
  
    /* Ungrab */
    
    gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				   grab_key_press_cb, data);
    gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				   grab_button_press_cb, data);
    gtk_grab_remove (window);
    gdk_pointer_ungrab (GDK_CURRENT_TIME); 
    gdk_keyboard_ungrab (GDK_CURRENT_TIME);   
  }
}

static void 
grab_button_press_cb (GtkWidget *widget, 
		      GdkEventButton *event, GtkWidget *data)
{
  int red, green, blue;
  
  GdkImage *img;
  GdkWindow *win;

  /* Ungrab */
  
  gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				 grab_key_press_cb, data);
  gtk_signal_disconnect_by_func (GTK_OBJECT (data), 
				 grab_button_press_cb, data);
  gtk_grab_remove (window);
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

  /* Set Range */

  range_update (GTK_CLIST (clist_found), red, green, blue);

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
}

static void
clist_select_row_cb (GtkCList *clist, gint row, gint col,
	             GdkEvent *event, gpointer data)
{
  RGBColor *color;
  
  if ((!event)||(event && event->type != GDK_2BUTTON_PRESS)) return;
  
  color = gtk_clist_get_row_data (GTK_CLIST (clist), row);
  if (!color) return;
    
  range_update (GTK_CLIST (clist_found), color->r, color->g, color->b);
  gtk_notebook_set_page (GTK_NOTEBOOK(notebook), 1);    
}	         

static void       
clist_button_press_cb  (GtkCList *clist, GdkEventButton *event, 
			GtkWidget *menu)
{
  GtkCListRow *r;
  gint row, col;

  g_assert (menu != NULL);

  if (event->button != 3) return;

  if (!gtk_clist_get_selection_info (clist, event->x, event->y, &row, &col))
    return;

  r = g_list_nth (clist->row_list, row)->data;
  g_assert (r != NULL);

  if (r->state != GTK_STATE_SELECTED) {
    gtk_clist_freeze (clist);
    gtk_clist_unselect_all (clist);
    clist->focus_row = row;
    gtk_clist_select_row (clist, row, col);
    gtk_clist_thaw (clist);
  }

  gtk_object_set_data (GTK_OBJECT (clist), "row_clicked", 
		       GINT_TO_POINTER (row));

  gnome_popup_menu_do_popup_modal (menu, NULL, NULL, event, clist);
}

/************************* DRAG AND DROP ****************************/

static void 
preview_button_press_cb  (GtkWidget *widget, GdkEventButton *event,
			  GtkWidget *menu)
{
  if (event->button != 3) return;

  gnome_popup_menu_do_popup_modal (menu, NULL, NULL, event, widget);
}

/* From gtk+, gtkcolorsel.c */
static GtkWidget *
drag_window_create (gint red, gint green, gint blue)
{
  GtkWidget *window;
  GdkColor bg;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  gtk_widget_set_usize (window, 48, 32);
  gtk_widget_realize (window);

  bg.red = (red / 255.0) * 0xffff;
  bg.green = (green / 255.0) * 0xffff;
  bg.blue = (blue / 255.0) * 0xffff;

  gdk_color_alloc (gtk_widget_get_colormap (window), &bg);
  gdk_window_set_background (window->window, &bg);

  return window;
}

static void 
preview_drag_begin_cb (GtkWidget *widget, 
		       GdkDragContext *context, GtkCList *clist)
{
  gint red, green, blue;

  red = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(widget), "red"));
  green = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(widget), "green"));
  blue = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(widget), "blue"));
    
  gtk_drag_set_icon_widget (context, 
			    drag_window_create (red, green, blue), -2, -2);
}

static void 
preview_drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, 
			  GtkSelectionData *selection_data, guint info,
			  guint time, GtkCList *clist)
{
  gint red, green, blue;
  guint16 vals[4];

  red = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(widget), "red"));
  green = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(widget), "green"));
  blue = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(widget), "blue"));

  vals[0] = (red / 255.0) * 0xffff;
  vals[1] = (green / 255.0) * 0xffff;
  vals[2] = (blue / 255.0) * 0xffff;
  vals[3] = 0xffff;
    
  gtk_selection_data_set (selection_data, 
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}	         

static void 
clist_drag_begin_cb (GtkWidget *widget, 
		     GdkDragContext *context, GtkCList *clist)
{
  RGBColor *color;

  if (GTK_CLIST(widget)->click_cell.row < 0) {
    gtk_clist_select_row (clist, 0, 0);
    color = gtk_clist_get_row_data (GTK_CLIST(widget), 0);
  } else {
    gtk_clist_select_row (clist, GTK_CLIST(widget)->click_cell.row, 0);

    color = gtk_clist_get_row_data (GTK_CLIST(widget), 
				  GTK_CLIST(widget)->click_cell.row);
  }				
				  
  gtk_drag_set_icon_widget (context, 
			    drag_window_create (color->r, color->g, color->b),
			    -2, -2);
}

static void 
clist_drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, 
			GtkSelectionData *selection_data, guint info,
			guint time, GtkCList *clist)
{
    RGBColor *color;
    guint16 vals[4];

    color = gtk_clist_get_row_data (GTK_CLIST(widget), clist->click_cell.row);
    g_assert (color != NULL);

    vals[0] = ((gdouble)color->r / 255.0) * 0xffff;
    vals[1] = ((gdouble)color->g / 255.0) * 0xffff;
    vals[2] = ((gdouble)color->b / 255.0) * 0xffff;
    vals[3] = 0xffff;
    
    gtk_selection_data_set (selection_data, 
    		            gdk_atom_intern ("application/x-color", FALSE),
    		            16, (guchar *)vals, 8);
}	         

static void
window_drop_data_cb (GtkWidget *widget, GdkDragContext *context,
		    gint x, gint y, GtkSelectionData *data,
		    guint info, guint32 time, gpointer d)
{
  guint16 *vals;
  RGBColor *new;
  gint row;

  if ((data->length !=8) || (data->format != 16))
    return;

  vals = (guint16 *)data->data;

  if (GTK_CHECK_MENU_ITEM (on_drop_menu2[0].widget)->active) {
    /* Search it */

    range_update (GTK_CLIST(clist_found), 
		  (int)((vals[0] * 255.0) / 0xffff),
		  (int)((vals[1] * 255.0) / 0xffff),
		  (int)((vals[2] * 255.0) / 0xffff));
    
    gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
  } else {
    /* Add it to favorites */

    new = g_new (RGBColor, 1);
    strcpy (new->name, _("Dropped"));
    new->r = (vals[0] * 255.0) / 0xffff;
    new->g = (vals[1] * 255.0) / 0xffff;
    new->b = (vals[2] * 255.0) / 0xffff;
    new->ref = 0;

    row = clist_add_color (GTK_CLIST(clist_favorites), new,
		     GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT(clist_favorites), "format")));

    gtk_clist_unselect_all (GTK_CLIST(clist_favorites));
    gtk_clist_select_row (GTK_CLIST(clist_favorites), row, 0);
    gtk_clist_moveto (GTK_CLIST(clist_favorites), row, 0, 0, 0);
    gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 2);
  }
}

/******************************** SORT *************************************/

static gint 
clist_val_compare (GtkCList *clist, GtkCListRow *row1, GtkCListRow *row2)
{
  RGBColor *c1 = row1->data, *c2 = row2->data;
  int t1, t2;

  if (!c2) return (c1 != NULL);
  if (!c1) return -1;

  t1 = c1->r + c1->g + c1->b;
  t2 = c2->r + c2->g + c2->b;

  if (t1 < t2) return -1;
  if (t1 > t2) return 1;
  
  return 0;
}

static gint 
clist_col_compare (GtkCList *clist, GtkCListRow *row1, GtkCListRow *row2)
{
  RGBColor *c1 = row1->data, *c2 = row2->data;

  if (!c2) return (c1 != NULL);
  if (!c1) return -1;
  
  if (c1->num < c2->num) return -1;
  if (c1->num > c2->num) return 1;
  
  return 0;
}

static gint 
clist_str_compare (GtkCList *clist, GtkCListRow *row1, GtkCListRow *row2)
{
  RGBColor *c1 = row1->data, *c2 = row2->data;

  if (!c2) return (c1 != NULL);
  if (!c1) return -1;
  
  return g_strcasecmp (c1->name, c2->name);
}

static gint 
clist_diff_compare (GtkCList *clist, GtkCListRow *row1, GtkCListRow *row2)
{
  RGBColor *c1 = row1->data, *c2 = row2->data;

  if (!c2) return (c1 != NULL);
  if (!c1) return -1;
  
  if (c1->diff < c2->diff) return -1;
  if (c1->diff > c2->diff) return 1;
  
  return 0;  
}

static void
clist_set_sort_column (GtkCList *clist, gint column)
{
  g_assert (clist != NULL);

  switch (column) {
  case 0:
    gtk_clist_set_compare_func (clist,(GtkCListCompareFunc)clist_col_compare);
    break;
  case 1:
    gtk_clist_set_compare_func (clist,(GtkCListCompareFunc)clist_val_compare);
    break;
  case 2:    
    gtk_clist_set_compare_func (clist,(GtkCListCompareFunc)clist_str_compare);
    break;
  case 3:
    gtk_clist_set_compare_func (clist,(GtkCListCompareFunc)clist_diff_compare);
    break;
  default:
    g_assert_not_reached ();
 }
    
 gtk_clist_set_sort_column (clist, column);
 gtk_clist_sort (clist);
}

static void 
clist_click_column_cb (GtkCList *clist, gint column)
{
  GtkWidget *arrow;
  gchar *key;
  gint cur_col;

  g_assert (clist != NULL);

  cur_col = clist->sort_column;

  key = g_strdup_printf ("arrow_%d", cur_col);
  arrow = gtk_object_get_data (GTK_OBJECT (clist), key);
  g_assert (arrow != NULL);
  g_free (key);

  if (cur_col == column) {
    if (clist->sort_type == GTK_SORT_ASCENDING) {
      gtk_clist_set_sort_type (clist, GTK_SORT_DESCENDING);
      gtk_arrow_set (GTK_ARROW(arrow), GTK_ARROW_UP, GTK_SHADOW_OUT);
    } else {
      gtk_clist_set_sort_type (clist, GTK_SORT_ASCENDING);
      gtk_arrow_set (GTK_ARROW(arrow), GTK_ARROW_DOWN, GTK_SHADOW_IN);
    }

  } else {

    gtk_widget_hide (arrow);

    key = g_strdup_printf ("arrow_%d", column);
    arrow = gtk_object_get_data (GTK_OBJECT (clist), key);
    g_assert (arrow != NULL);
    g_free (key);
    
    gtk_arrow_set (GTK_ARROW(arrow), GTK_ARROW_DOWN, GTK_SHADOW_IN);
    gtk_widget_show (arrow);
    
    clist_set_sort_column (clist, column);
    gtk_clist_set_sort_type (clist, GTK_SORT_ASCENDING);
  }

  gtk_clist_sort (clist);
}

/***************************** FIND COLOR *****************************/

static int 
color_get_diff (RGBColor *color, int red, int green, int blue)
{
  g_assert (color != NULL);

  return abs (color->r - red) + abs (color->g - green) 
                              + abs (color->b - blue);                      
}                 

static void 
clist_find_color (GtkCList *source, GtkCList *dest,
		  gint red, gint green, gint blue, gint tolerance)
{
  RGBColor *color;
  int diff;
  FormatType type;
  GList *row_list;
  GtkCListRow *row;

  g_assert (source != NULL);
  g_assert (dest != NULL);

  gtk_clist_freeze (dest);
  gtk_clist_set_auto_sort (GTK_CLIST(dest), FALSE);

  type = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (source), "format"));

  row_list = source->row_list;

  while (row_list) {
    row = row_list->data;
    g_assert (row != NULL);

    color = row->data; 
    g_assert (color != NULL);

    diff = color_get_diff (color, red, green, blue);

    if (((float)diff / (255.0 * 3.0)) * 100.0  <= (float)tolerance) {
	color->diff = diff;
	clist_add_color (dest, color, type);
    }    

    row_list = row_list->next;
  }

  gtk_clist_set_auto_sort (GTK_CLIST(dest), TRUE);
  gtk_clist_thaw (dest);
}

static void
clist_get_range (GtkCList *clist, GtkRange **range_red, GtkRange **range_green,
		 GtkRange **range_blue, GtkRange **range_tolerance)
{
  g_assert (clist != NULL);

  if (range_red)
    *range_red = GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_red"));

  if (range_blue)
    *range_blue = GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_blue"));

  if (range_green)
    *range_green = GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_green"));

  if (range_tolerance)
    *range_tolerance = GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_tolerance"));
}

static void
clist_get_adjustment (GtkCList *clist, GtkAdjustment **adj_red, 
		      GtkAdjustment **adj_green, GtkAdjustment **adj_blue,
		      GtkAdjustment **adj_tolerance)
{
  g_assert (clist != NULL);

  if (adj_red) {
    *adj_red = gtk_range_get_adjustment (GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_red")));
    g_assert (*adj_red != NULL);
  }

  if (adj_green) {
    *adj_green = gtk_range_get_adjustment (GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_green")));
    g_assert (*adj_green != NULL);
  }

  if (adj_blue) {
    *adj_blue = gtk_range_get_adjustment (GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_blue")));
    g_assert (*adj_blue != NULL);
  }

  if (adj_tolerance) {
    *adj_tolerance = gtk_range_get_adjustment (GTK_RANGE (gtk_object_get_data(GTK_OBJECT(clist), "range_tolerance")));
    g_assert (*adj_tolerance != NULL);
  }
}

static void 
preview_update (GtkCList *clist)
{
  GtkAdjustment *adj_red, *adj_blue, *adj_green;
  GtkWidget *preview;

  guchar *buf;
  gint x, y;

  g_assert (clist != NULL);

  clist_get_adjustment (clist, &adj_red, &adj_green, &adj_blue, NULL);

  preview = gtk_object_get_data (GTK_OBJECT (clist), "preview");
  g_assert (preview != NULL);

  gtk_object_set_data (GTK_OBJECT (preview), "red", 
		       GINT_TO_POINTER((int)adj_red->value));
  gtk_object_set_data (GTK_OBJECT (preview), "green", 
		       GINT_TO_POINTER ((int)adj_green->value));
  gtk_object_set_data (GTK_OBJECT (preview), "blue", 
		       GINT_TO_POINTER ((int)adj_blue->value));
    
  buf = g_new (guchar, 3 * preview->allocation.width);
  
  for (x = 0; x < preview->allocation.width; x++) {
    buf [x * 3] = adj_red->value;
    buf [x * 3 + 1] = adj_green->value;
    buf [x * 3 + 2] = adj_blue->value;
  }

  for (y=0; y<preview->allocation.height; y++)
     gtk_preview_draw_row (GTK_PREVIEW (preview), 
			   buf, 0, y, preview->allocation.width);;
     
  gtk_widget_draw (preview, NULL);

  g_free (buf);    
}

static void 
range_update (GtkCList *clist, int red, int green, int blue)
{
  GtkAdjustment *adj_red, *adj_green, *adj_blue;

  g_assert (clist != NULL);

  clist_get_adjustment (clist, &adj_red, &adj_green, &adj_blue, NULL);

  /* Do not update clist_found now ... */

  gtk_object_set_data (GTK_OBJECT (clist_found), "do_update", NULL);

  gtk_adjustment_set_value(adj_red, red);
  gtk_adjustment_set_value(adj_green, green);
  gtk_adjustment_set_value(adj_blue, blue);

  /* Now, we can */
  
  gtk_adjustment_value_changed (adj_red);

  gtk_object_set_data (GTK_OBJECT (clist_found), "do_update", 
                       GINT_TO_POINTER (TRUE));
                       
  gtk_adjustment_value_changed (adj_red);

  gtk_adjustment_set_value(adj_blue, blue);
}

static void 
range_value_changed_cb (GtkWidget *widget, gpointer data)
{
  GtkAdjustment *adj_red, *adj_green, *adj_blue, *adj_tolerance;

  if (! gtk_object_get_data (GTK_OBJECT (clist_found), "do_update"))
    return;

  clist_get_adjustment (GTK_CLIST (clist_found),
			&adj_red, &adj_green, &adj_blue, &adj_tolerance);

  preview_update (GTK_CLIST (clist_found));

  gtk_clist_clear (GTK_CLIST (clist_found));

  if (GTK_CHECK_MENU_ITEM (search_in_menu[0].widget)->active) {
    clist_find_color (GTK_CLIST (clist), GTK_CLIST (clist_found), 
		      adj_red->value, adj_green->value, 
		      adj_blue->value, adj_tolerance->value);
  }

  if (GTK_CHECK_MENU_ITEM (search_in_menu[1].widget)->active) {
    clist_find_color (GTK_CLIST (clist_favorites), GTK_CLIST (clist_found), 
		      adj_red->value, adj_green->value, 
		      adj_blue->value, adj_tolerance->value);
  }

  gtk_clist_sort (GTK_CLIST (clist_found));
}
