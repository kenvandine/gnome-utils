#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gnome.h>
#include "gnome-find.h"

static void initialize_app(char * start_dir);

static void new_cb (GtkWidget *widget, void *data);
static void open_cb (GtkWidget *widget, void *data);
static void save_cb (GtkWidget *widget, void *data);
static void save_as_cb (GtkWidget *widget, void *data);
static void exit_cb (GtkWidget *widget, void *data);
static void preferences_cb (GtkWidget *widget, void *data);
static void add_spec_cb (GtkWidget *widget, void *data);
static void remove_spec_cb (GtkWidget *widget, void *data);
static void toggle_advanced_cb (GtkWidget *widget, void *data);
static void start_search_cb (GtkWidget *widget, void *data);

static void about_cb (GtkWidget *widget, void *data);


/* app points to our toplevel window */
GtkWidget *app;

/* File entry in the top level window */
GtkWidget *file_entry;

GtkWidget *search_subdir_check;
GtkWidget *follow_links_check;
GtkWidget *follow_mountpoints_check;

/* Advanced options on right now? */
int advanced_state = 0;




static GnomeUIInfo file_menu [] = {

  GNOMEUIINFO_ITEM_STOCK (N_("New search"), NULL, new_cb, GNOME_STOCK_MENU_NEW),
  GNOMEUIINFO_ITEM_STOCK (N_("Open search spec"), NULL, open_cb, GNOME_STOCK_MENU_OPEN),
  GNOMEUIINFO_ITEM_STOCK (N_("Save search spec"), NULL, save_cb, GNOME_STOCK_MENU_SAVE),
  GNOMEUIINFO_ITEM_STOCK (N_("Save search spec as..."), NULL, save_as_cb, GNOME_STOCK_MENU_SAVE_AS),
  
  GNOMEUIINFO_SEPARATOR,

  GNOMEUIINFO_ITEM_STOCK (N_("Exit"), NULL, exit_cb, GNOME_STOCK_MENU_EXIT),
  GNOMEUIINFO_END
};

static GnomeUIInfo edit_menu [] = {

  GNOMEUIINFO_ITEM_STOCK (N_("Preferences..."), NULL, preferences_cb, GNOME_STOCK_MENU_NEW),
  GNOMEUIINFO_END
};

static GnomeUIInfo search_menu [] = {
  
  GNOMEUIINFO_ITEM_STOCK (N_("Add condition"), N_("Add a search condition"), add_spec_cb, GNOME_STOCK_MENU_DOWN),
  GNOMEUIINFO_ITEM_STOCK (N_("Remove condition"), N_("Remove last search condition"), remove_spec_cb, GNOME_STOCK_MENU_UP),
  
  {GNOME_APP_UI_TOGGLEITEM, N_("Show advanced options"), 
   N_("Show advanced options"), toggle_advanced_cb, 0, NULL, 
   GNOME_APP_PIXMAP_NONE, NULL, 0,0, NULL},
  
  GNOMEUIINFO_SEPARATOR,
  
  GNOMEUIINFO_ITEM_STOCK (N_("Start search"), NULL, start_search_cb, 
			  GNOME_STOCK_MENU_SEARCH),
  GNOMEUIINFO_END
};


static GnomeUIInfo help_menu [] = {
  GNOMEUIINFO_ITEM_STOCK (N_("About Gnome Find..."), NULL, about_cb, 
			  GNOME_STOCK_MENU_ABOUT),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("gnome-find"),
  GNOMEUIINFO_END
};

	
static GnomeUIInfo main_menu [] = {
  GNOMEUIINFO_SUBTREE (N_("File"), &file_menu),
  GNOMEUIINFO_SUBTREE (N_("Edit"), &edit_menu),
  GNOMEUIINFO_SUBTREE (N_("Search"), &search_menu),
  GNOMEUIINFO_SUBTREE (N_("Help"), &help_menu),
  GNOMEUIINFO_END
};


static GnomeUIInfo main_toolbar [] = {
  GNOMEUIINFO_ITEM_STOCK (N_("New"),  N_("Clear the search window"), 
			  new_cb, GNOME_STOCK_PIXMAP_NEW),

  GNOMEUIINFO_SEPARATOR,
  
  GNOMEUIINFO_ITEM_STOCK (N_("Add"), N_("Add a search condition"), 
			  add_spec_cb, GNOME_STOCK_PIXMAP_DOWN),
  GNOMEUIINFO_ITEM_STOCK (N_("Remove"), N_("Remove last search condition"), remove_spec_cb, GNOME_STOCK_PIXMAP_UP),
    {GNOME_APP_UI_TOGGLEITEM, N_("Advanced"), N_("Show advanced options"), 
   toggle_advanced_cb, (gpointer) 1, NULL, GNOME_APP_PIXMAP_NONE, 
   GNOME_STOCK_PIXMAP_MULTIPLE, 0, 0, NULL},

  GNOMEUIINFO_SEPARATOR,

  GNOMEUIINFO_ITEM_STOCK (N_("Search"), N_("Start the current search"), 
			  start_search_cb, GNOME_STOCK_PIXMAP_SEARCH),

  GNOMEUIINFO_END
};

int
main (int argc, char *argv[])
{
  poptContext ctx;
  char *dir = NULL;
  char *spec = NULL;
  
  struct poptOption prog_options[] = {
    {"dir", 'd', POPT_ARG_STRING, NULL, 0, NULL, "Directory to start searching in."},
    {"spec", 's', POPT_ARG_STRING, NULL, 0, NULL, "Search specification to use."},
    POPT_AUTOHELP
    {NULL, '\0', 0, NULL}
  };
  prog_options[0].arg =  &dir;
  prog_options[1].arg =  &spec;
  
  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);
  
  /*
   * gnome_init() is always called at the beginning of a program.  it
   * takes care of initializing both Gtk and GNOME.  It also parses
   * the command-line arguments.
   */
  gnome_init_with_popt_table ("gnome-find", VERSION,
			      argc, argv, prog_options, 0, NULL);
  
  if (NULL != spec) {
    puts("Got spec.");
  } 
  
  if (NULL != dir) {
    puts("Got dir.");
  } else {
    dir = ".";
  }
  
  
  /*
   * prepare_app() makes all the gtk calls necessary to set up a
   * minimal Gnome application; It's based on the hello world example
   * from the Gtk+ tutorial
   */
  initialize_app (dir);
  
  gtk_main ();
  
  return 0;
}

static void
initialize_app(char *start_dir)
{
  GtkWidget *main_vbox;
  GtkWidget *dir_hbox;
  GtkWidget *dir_label;
  GtkWidget *table;

  GtkWidget *hseparator;
  GtkWidget *check_table;

  
  GtkWidget *button;
  
  app = gnome_app_new("gnome-find", _("Find Files"));
  
  dir_label = gtk_label_new(_("Directory:"));
    
  file_entry = gnome_file_entry_new("direntry", _("Directory to Search"));
  gnome_file_entry_set_directory(GNOME_FILE_ENTRY(file_entry), TRUE);
  gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(file_entry), start_dir);

  dir_hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(dir_hbox), dir_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dir_hbox), file_entry, TRUE, TRUE, 0);

  search_subdir_check = gtk_check_button_new_with_label
    ("Search subdirectories");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_subdir_check), 
			      TRUE);

  follow_links_check = gtk_check_button_new_with_label("Follow links");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(follow_links_check), 
			      FALSE);

  follow_mountpoints_check = gtk_check_button_new_with_label
    ("Follow mountpoints");
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(follow_mountpoints_check), 
			      FALSE);

  check_table = gtk_table_new(2, 2, FALSE);

  gtk_table_attach_defaults (GTK_TABLE(check_table), search_subdir_check, 
			     0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE(check_table), follow_links_check, 
			     1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE(check_table), follow_mountpoints_check, 
			     0, 1, 1, 2);
  

  hseparator = gtk_hseparator_new();
  


  main_vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);
  gtk_box_pack_start(GTK_BOX(main_vbox), dir_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(main_vbox), check_table, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(main_vbox), hseparator, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(main_vbox), init_spec_vbox(advanced_state), 
		     FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT(app), "delete_event",
		      GTK_SIGNAL_FUNC (exit_cb),
		      NULL);

  gnome_app_create_menus (GNOME_APP(app), main_menu);
  gnome_app_create_toolbar (GNOME_APP(app), main_toolbar);
  
  /*
   * We make a button, bind the 'clicked' signal to hello and setting it
   * to be the content of the main window
   */
  gnome_app_set_contents(GNOME_APP(app), main_vbox);

  gtk_window_set_policy (GTK_WINDOW(app), FALSE, FALSE, TRUE);
  
  gtk_widget_show (dir_label);
  gtk_widget_show (file_entry);
  gtk_widget_show (dir_hbox);
  gtk_widget_show (search_subdir_check);
  gtk_widget_show (check_table);
  gtk_widget_show (hseparator);
  gtk_widget_show (main_vbox);
  gtk_widget_show (app);
}




static void
new_cb (GtkWidget *widget, void *data)
{
  puts("New.");
  return;
}

static void
open_cb (GtkWidget *widget, void *data)
{
  puts("Open.");
  return;
}

static void
save_cb (GtkWidget *widget, void *data)
{
  puts("Save.");
  return;
}

static void
save_as_cb (GtkWidget *widget, void *data)
{
  puts("Save as.");
  return;
}


static void
exit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}


static int preferences_active = 0;
static int requested_state = 0;
static GtkWidget *property_box;

static void
apply_preferences_cb (GtkWidget *widget, gint page_num, void *data)
{
  puts("Applying.");
}

static void
close_preferences_cb (GtkWidget *widget, GdkEvent* event, void *data)
{
  preferences_active = 0;
}

static void
preferences_cb (GtkWidget *widget, void *data)
{
  GtkWidget *box_label;
  GtkWidget *vbox;
  GtkWidget *radio1;
  GtkWidget *radio2;
  GtkWidget *radio3;
  
  /* FIXME: Race condition */
  if (0 == preferences_active) {
    preferences_active=1;

    radio1 = gtk_radio_button_new_with_label_from_widget(NULL,"Basic");
    gtk_widget_show(radio1);
    radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1),"Dynamic");
    gtk_widget_show(radio2);
    radio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1),"Advanced");
    gtk_widget_show(radio3);
    
    box_label = gtk_label_new("Interface");
    gtk_widget_show(box_label);
    
    vbox = gtk_vbox_new(FALSE,5);
    gtk_box_pack_start(GTK_BOX(vbox),radio1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),radio2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),radio3, FALSE, FALSE, 0);
    gtk_widget_show(vbox);
    
    property_box = gnome_property_box_new();
    gnome_property_box_append_page(GNOME_PROPERTY_BOX(property_box),
				   vbox,
				   box_label);

    gtk_signal_connect(GTK_OBJECT(property_box),"apply",apply_preferences_cb, NULL);
    gtk_signal_connect(GTK_OBJECT(property_box),"delete_event",close_preferences_cb, NULL);
    gtk_widget_show(property_box);
  }

  return;
}




static void
add_spec_cb (GtkWidget *widget, void *data)
{
  spec_vbox_add_spec(advanced_state);
  return;
}

static void
remove_spec_cb (GtkWidget *widget, void *data)
{
  spec_vbox_remove_spec();
  return;
}



static void
toggle_advanced_cb (GtkWidget *widget, void *data)
{
  static int operating = 0;
  int toolbar_p = (int) data;

  if (!operating) {
    operating = 1;
    advanced_state = !advanced_state;
    if (toolbar_p) {
      gtk_check_menu_item_set_state
	(GTK_CHECK_MENU_ITEM(search_menu[2].widget),
	 advanced_state);
    } else {
      gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(main_toolbar[4].widget), 
	 advanced_state);
    }

    if (advanced_state) {
      gtk_widget_show (follow_links_check);
      gtk_widget_show (follow_mountpoints_check);
    } else {
      gtk_widget_hide (follow_links_check);
      gtk_widget_hide (follow_mountpoints_check);
    }

    spec_vbox_add_spec(advanced_state);
    
    operating = 0;
  }
  

  return;
}

static void
start_search_cb (GtkWidget *widget, void *data)
{
  puts("Start search.");
  return;
}



static void
about_cb (GtkWidget *widget, void *data)
{
	GtkWidget *about;
	const gchar *authors[] = {
/* Here should be your names */
		"Maciej Stachowiak",
		NULL
	};
	
	about = gnome_about_new ( _("GNOME Find"), VERSION,
				  /* copyright notice */
				  _("(C) 1998 the Free Software Foundation"),
				  authors,
				  /* another comments */
				  _("GNOME Find is a fast, flexible file "
				    "search tool."),
				  NULL);
	gtk_widget_show (about);
	
	return;
}
