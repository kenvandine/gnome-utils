/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   edit-menus: Gnome app for editing window manager, panel menus.
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <config.h>
#include <gnome.h>

#include "menuentrywiz.h"
#include "menu.h"
#include "parser.h"
#include "saver.h"
#include "run_command.h"

#define APPNAME "edit-menus"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#define TREEITEM_ROOT_KEY "tree_item_root" /* FIXME is this used? */

#ifndef VERSION
#define VERSION "0.0.0"
#endif

/****************************
  Function prototypes
  ******************************/

static void prepare_app();
static void prepare_tree(GtkWidget * box);
static void prepare_menudata();

static gint delete_event_cb(GtkWidget * w, gpointer data);
static void about_cb(GtkWidget * w, gpointer data);
static void save_cb(GtkWidget * w, gpointer data);
static void cut_cb(GtkWidget * w, gpointer data);
static void paste_cb(GtkWidget * w, gpointer data);
static void new_cb(GtkWidget * w, gpointer data);
static void edit_cb(GtkWidget *w, gpointer data);
static void preferences_cb(GtkWidget *w, gpointer data);
static void test_cb(GtkWidget *w, gpointer data);

static void tree_selection_changed_cb(GtkTree * tree);

static void menu_entry_cb(MenuEntryWiz *, gchar *, 
			  gchar *, gchar *);
static void new_what_cb(GnomeDialog * dialog, gint button, 
                        gpointer data);
static void test_menuitem_cb(GtkWidget * mi, gpointer data);
static void quit_for_sure_cb(GnomeDialog * d, gint button);

static GtkWidget * tree_from_menu (Menu * m,
				   GtkTree * root); 
static GtkWidget * gtk_menu_from_menu (Menu * m,
				       GtkMenu * root);
static GtkWidget * tree_from_menu_with_pos (Menu * m,
					    GtkTree * root,
					    gint pos);

static void popup_test(Menu * m);
static void popup_new_what();
static void popup_nothing_selected();
static void popup_no_name();
static void popup_clipboard_empty();
static void popup_quit_for_sure();
static void popup_ok(gchar * message);

static void relabel_tree_item(Menu * m);
static void init_tree_item(GtkWidget * ti, GtkWidget * root, 
			   Menu * m);
static void edit_this_menu(Menu * m);
static void make_menu_changes(Menu * m, gchar * name, gchar * icon,
			      gchar * command);
static void remove_from_tree(Menu * m);
static void add_to_tree_at_selection(Menu * m);

/*****************************
  Globals
  *****************************/

GtkWidget * app;

Menu * menu_data;

GtkTreeItem * selected_tree_item; 
Menu * selected_menu;

Menu * clipboard;

gboolean changed_since_saved;

/*******************************
  Main
  *******************************/

int main ( int argc, char ** argv )
{
  /* What does this do, anyway? */
  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, 0, argc, argv, 0, 0);

  prepare_app();

  gtk_widget_show(app);

  gtk_main();

  exit(EXIT_SUCCESS);
}

/***************************************
  GnomeUIInfo arrays and other such stuff
  *****************************************/

/* Are the menu tool tips supposed to do anything? status bar? */
/* Why not stock whole entries, except the callback and tool tip?
   e.g. GNOMEUIINFO_SAVE("Save menu to disk", save_cb) */

static GnomeUIInfo help_menu[] = {
  GNOMEUIINFO_HELP(APPNAME),
  {GNOME_APP_UI_ITEM, N_("About..."), 
   N_("Tell about this application"), 
   about_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo file_menu[] = {
  {GNOME_APP_UI_ITEM, 
   N_("Save"), N_("Write menu to disk"), 
   save_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 's', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_SEPARATOR,
  {GNOME_APP_UI_ITEM, N_("Exit"), 
   N_("Quit the application without saving"),
   delete_event_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'q', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo prefs_menu[] = {
  {GNOME_APP_UI_ITEM, N_("Preferences"), 
   N_("Change application preferences"),
   preferences_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF, 'p', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_SUBTREE (N_("File"), file_menu),
  GNOMEUIINFO_SUBTREE (N_("Preferences"), prefs_menu),
  GNOMEUIINFO_SUBTREE (N_("Help"), help_menu),
  GNOMEUIINFO_END
};

static GnomeUIInfo toolbar[] = {
  {GNOME_APP_UI_ITEM, N_("Cut"), N_("Remove this menu item"),
   cut_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
   GNOME_STOCK_PIXMAP_CUT, 'k', GDK_CONTROL_MASK, NULL },
  {GNOME_APP_UI_ITEM, N_("Paste"), 
   N_("Paste the last menu item you cut"),
   paste_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
   GNOME_STOCK_PIXMAP_PASTE, 'y', GDK_CONTROL_MASK, NULL },
  {GNOME_APP_UI_ITEM, N_("New"), 
   N_("Add a new menu item below current selection"), 
   new_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
   GNOME_STOCK_PIXMAP_NEW, 'n', GDK_CONTROL_MASK, NULL },
  /* Edit should have a stock pixmap, oh well, using properties */
  {GNOME_APP_UI_ITEM, N_("Edit"), 
   N_("Edit this menu item"), 
   edit_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
   GNOME_STOCK_PIXMAP_PROPERTIES, 'e', GDK_CONTROL_MASK, NULL },
  {GNOME_APP_UI_ITEM, N_("Test"), 
   N_("Create a test menu"), 
   test_cb, NULL, NULL, GNOME_APP_PIXMAP_NONE,
   NULL, 't', GDK_CONTROL_MASK, NULL },
  {GNOME_APP_UI_ITEM, N_("Exit"), 
   N_("Quit the application without saving"),
   delete_event_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_EXIT, 'q', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_END
};

/*******************************
  Set up the app GUI
  ********************************/

void prepare_app()
{
  GtkWidget * app_box;
  GtkWidget * tree_box, * frame;
  GtkWidget * scrolled_win;

  app = gnome_app_new(APPNAME, _("Menu Editor"));

  gnome_app_create_menus(GNOME_APP(app), main_menu);
  gnome_app_create_toolbar(GNOME_APP(app), toolbar);
  
  /******** Contents **************/

  app_box = gtk_vbox_new ( FALSE, GNOME_PAD );
  gtk_widget_show(app_box);

  gnome_app_set_contents ( GNOME_APP(app), app_box );

  gtk_signal_connect ( GTK_OBJECT(app), "destroy",
		       GTK_SIGNAL_FUNC(gtk_main_quit),
		       0 );

  gtk_signal_connect ( GTK_OBJECT (app), "delete_event",
		       GTK_SIGNAL_FUNC (delete_event_cb),
		       NULL );

  frame = gtk_frame_new(_("Menu Editing Window"));
  gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, 
                                  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize ( scrolled_win, 180, 400 );
  gtk_container_add ( GTK_CONTAINER(frame), scrolled_win );
  gtk_widget_show(scrolled_win);
  gtk_container_border_width(GTK_CONTAINER(scrolled_win), 
                             GNOME_PAD);
  gtk_box_pack_end( GTK_BOX(app_box), frame,
		    TRUE, TRUE, GNOME_PAD );

  gtk_widget_show(frame);

  tree_box = gtk_vbox_new ( FALSE, GNOME_PAD );
  gtk_container_add ( GTK_CONTAINER(scrolled_win), tree_box );

  gtk_widget_show(tree_box);

  clipboard = NULL;
  changed_since_saved = FALSE;

  prepare_menudata();

  prepare_tree(tree_box);    
}

void prepare_tree(GtkWidget * box)
{
  GtkWidget * tree_root;

  selected_tree_item = NULL;

  tree_root = gtk_tree_new();

  gtk_container_add( GTK_CONTAINER(box), tree_root );

  gtk_signal_connect_object 
    ( GTK_OBJECT(tree_root), 
      "selection_changed",
      GTK_SIGNAL_FUNC(tree_selection_changed_cb),
      GTK_OBJECT(tree_root) );

  gtk_widget_show(tree_root);

  tree_from_menu (menu_data, GTK_TREE(tree_root));
}

/***************************************
  Load in the menu data
  ***********************************/

static void prepare_menudata()
{
  selected_menu = NULL;
  menu_data = parse_combined_menu();
}


/**********************************************
  Menu to Tree, Menu to GtkMenu
  ******************************************/

#define APPEND_NOT_INSERT -1 

static GtkWidget * tree_from_menu (Menu * m,
				   GtkTree * root)
{
  return tree_from_menu_with_pos(m, root, APPEND_NOT_INSERT);
}

static GtkWidget * tree_from_menu_with_pos (Menu * m,
					    GtkTree * root,
					    gint pos)
{
  GtkWidget * subtree;
  GtkWidget * treeitem;
  GSList * contents;

  g_return_val_if_fail ( m != 0, NULL );
  g_return_val_if_fail ( root != 0, NULL );

  treeitem = gtk_tree_item_new_with_label( MENU_NAME(m) );
  init_tree_item(treeitem, GTK_WIDGET(root), m);
  
  gtk_widget_show(treeitem);
  
  if ( pos == APPEND_NOT_INSERT ) {
    gtk_tree_append ( GTK_TREE (root), treeitem );
  }
  else {
    gtk_tree_insert ( GTK_TREE (root), treeitem, pos );
  }

  if ( IS_MENU_FOLDER(m) ) {

    subtree = gtk_tree_new();
    
    gtk_tree_item_set_subtree ( GTK_TREE_ITEM(treeitem), subtree );

    gtk_widget_show(subtree);
    
    contents = MENU_FOLDER_CONTENTS(m);

    while ( contents ) {
      
      tree_from_menu ( (Menu *)(contents->data),
		       GTK_TREE(subtree) );
      
      contents = g_slist_next(contents);
    }
  }

  return treeitem;
}


static GtkWidget * gtk_menu_from_menu (Menu * m,
				       GtkMenu * root)
{
  GtkWidget * submenu;
  GtkWidget * menuitem;
  GSList * contents;

  g_return_val_if_fail ( m != 0, NULL );

  if ( IS_MENU_SEPARATOR(m) ) {
    /* I think this is a real opaque API.
       Anyway, creates a separator. */
    menuitem = gtk_menu_item_new();
  }
  else {
    menuitem = gtk_menu_item_new_with_label( MENU_NAME(m) );
  }

  gtk_widget_show(menuitem);
      
  if (root) {
    gtk_menu_append ( GTK_MENU (root), menuitem );
  }

  if ( IS_MENU_FOLDER(m) ) {

    submenu = gtk_menu_new();
    gtk_widget_show(submenu);

    gtk_menu_item_set_submenu ( GTK_MENU_ITEM(menuitem), submenu );
    
    contents = MENU_FOLDER_CONTENTS(m);

    while ( contents ) {
      
      gtk_menu_from_menu ( (Menu *)(contents->data),
			   GTK_MENU(submenu) );
      
      contents = g_slist_next(contents);
    }
  }
  else if ( IS_MENU_COMMAND(m) ){
    /* Connect command items to run a command */
    gtk_signal_connect ( GTK_OBJECT(menuitem), "activate",
                         GTK_SIGNAL_FUNC(test_menuitem_cb),
                         m );
  }

  /* Return value ignored by recursive calls */

  return menuitem;
}

/****************************************
  Tree/Menu callbacks
  **************************************/

static void tree_selection_changed_cb(GtkTree * tree)
{
  if (tree && GTK_TREE_SELECTION(tree)) {
    selected_tree_item = 
      GTK_TREE_ITEM(GTK_TREE_SELECTION(tree)->data);
  }
  else {
    selected_tree_item = NULL;
  }
  if (selected_tree_item) {
    selected_menu = 
      (Menu*)gtk_object_get_user_data(GTK_OBJECT(selected_tree_item));
  }
  else {
    selected_menu = NULL;
  }

  /* Both or neither */
  g_assert( (selected_tree_item && selected_menu) ||
            !(selected_tree_item || selected_menu) );
}


/*******************************************
  App GUI Callbacks
  **********************************************/

static gint delete_event_cb (GtkWidget *w, gpointer data)
{
  if (changed_since_saved) {
    popup_quit_for_sure();
  }
  else {
    gtk_widget_destroy(app);
  }
  return TRUE;
}

static void about_cb(GtkWidget * w, gpointer data)
{
  gchar * authors[] = { "Havoc Pennington <hp@pobox.com>",
			NULL };

  GtkWidget * ga;

  ga = gnome_about_new (APPNAME,
                        VERSION, 
                        COPYRIGHT_NOTICE,
                        authors,
                        0,
                        0 );

  gtk_widget_show(ga);
}

static void save_cb(GtkWidget *w, gpointer data)
{
  gchar * user_menu;
  user_menu = make_user_menu ( menu_data );

  g_print(_("Created this menu:\n"));
  g_print("%s", user_menu);

  changed_since_saved = FALSE;
}

static void cut_cb(GtkWidget *w, gpointer data)
{
  Menu * m;
  if (selected_tree_item && selected_menu) {
    m = selected_menu;
    remove_from_tree(m); /* changes selection, thus m needed */
    if (clipboard) {
      menu_destroy(clipboard);
    }
    clipboard = m;
  }
  else {
    popup_nothing_selected();
  } 
}

static void paste_cb(GtkWidget *w, gpointer data)
{
  if (selected_tree_item) {
    if (clipboard) {
      add_to_tree_at_selection(clipboard);
      clipboard = NULL;
    }
    else {
      popup_clipboard_empty();
    }
  }
  else {
    popup_nothing_selected();
  }      
}

static void new_cb(GtkWidget *w, gpointer data)
{
  if (!selected_menu) {
    popup_nothing_selected();
    return;
  }
  popup_new_what();
}

static void edit_cb(GtkWidget *w, gpointer data)
{
  if (!selected_menu) {
    popup_nothing_selected();
    return;
  }
  else {
    if ( IS_MENU_SEPARATOR(selected_menu) ) {
      popup_ok(_("You can't edit a separator"));
      return;
    }
    edit_this_menu(selected_menu);
  }
}

static void preferences_cb(GtkWidget *w, gpointer data)
{


}

static void test_cb(GtkWidget *w, gpointer data)
{
  popup_test(menu_data);
}

/*************************************
  Create popup dialogs
  *********************************/

static void popup_test(Menu * m)
{
  GtkWidget * menubar;
  GtkWidget * menuitem;
  GtkWidget * d;
  GtkWidget * frame;
  GtkWidget * menu_box;
  
  d = gnome_dialog_new(_("Testing menu"), 
                       GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_destroy(GNOME_DIALOG(d), TRUE);

  frame = gtk_frame_new(_("Working Test Menu"));
  gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD);
  gtk_box_pack_start ( GTK_BOX(GNOME_DIALOG(d)->vbox), frame, 
                       TRUE, TRUE, GNOME_PAD );
  
  menu_box = gtk_vbox_new ( FALSE, GNOME_PAD);
  gtk_container_border_width(GTK_CONTAINER(menu_box), 
                             GNOME_PAD);
  gtk_container_add ( GTK_CONTAINER(frame), menu_box );
  gtk_widget_show(menu_box);
  gtk_widget_show(frame);

  menubar = gtk_menu_bar_new();

  menuitem = gtk_menu_from_menu( menu_data, NULL );

  gtk_menu_bar_append(GTK_MENU_BAR(menubar), menuitem);

  gtk_container_add( GTK_CONTAINER(menu_box), menubar );

  gtk_widget_show(menubar);
  gtk_widget_show(d);
}

/* The order of the radio buttons in their group */
enum {
  separator_radio,
  folder_radio,
  command_radio
};

static void popup_new_what()
{
  GtkWidget * d;
  GtkWidget * button;
  GSList * group;
  GtkWidget * label;

  d = gnome_dialog_new(_("Menu item type"),
                       GNOME_STOCK_BUTTON_OK, NULL);
  gnome_dialog_set_destroy(GNOME_DIALOG(d), TRUE);
  gnome_dialog_set_modal(GNOME_DIALOG(d));
  
  label = gtk_label_new(_("What kind of menu item?"));
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG(d)->vbox), 
		      label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  button = gtk_radio_button_new_with_label(NULL, _("Command"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG(d)->vbox), 
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (group, _("Folder"));
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG(d)->vbox), 
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (group,
					    _("Separator"));
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG(d)->vbox), 
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* Get group to pass to the callback */
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  
  gtk_signal_connect ( GTK_OBJECT(d), "clicked",
                       GTK_SIGNAL_FUNC(new_what_cb),
                       group );

  gtk_widget_show(d);
}

static void popup_quit_for_sure()
{
  GtkWidget * mb;
  
  mb = gnome_message_box_new(_("You have unsaved changes. Quit?"),
                             GNOME_MESSAGE_BOX_QUESTION,
                             GNOME_STOCK_BUTTON_YES,
                             GNOME_STOCK_BUTTON_NO,
                             NULL);
  gnome_dialog_set_modal(GNOME_DIALOG(mb));
  gnome_dialog_set_default(GNOME_DIALOG(mb), 0);

  gtk_signal_connect( GTK_OBJECT(mb), "clicked",
                      GTK_SIGNAL_FUNC(quit_for_sure_cb),
                      NULL );

  gtk_widget_show(mb);
}

static void popup_ok(gchar * message)
{
  GtkWidget * mb;
  
  mb = gnome_message_box_new(message,
			     GNOME_MESSAGE_BOX_INFO,
			     GNOME_STOCK_BUTTON_OK,
			     NULL);
  gnome_dialog_set_modal(GNOME_DIALOG(mb));

  gtk_widget_show(mb);
}

static void popup_nothing_selected()
{
  popup_ok(_("You must select a menu item before "
		    "performing this operation."));
}

static void popup_no_name()
{
  popup_ok(_("You must name your menu item."));
}

static void popup_clipboard_empty()
{
  popup_ok(_("The clipboard is empty"));
}

/*****************************************
  Dialog callbacks
  ************************************/

/* Note that this has to work regardless of the selection,
   since it can be called on a new menu not yet in the tree */

static void menu_entry_cb(MenuEntryWiz * mew, gchar * name, 
                          gchar * icon, gchar * command)
{
  Menu * m = (Menu *)gtk_object_get_user_data(GTK_OBJECT(mew));
  make_menu_changes(m, name, icon, command);
  relabel_tree_item(m);
}

static void new_what_cb(GnomeDialog * dialog, gint button, 
                        gpointer data)
{
  GSList * group;
  int which;
  Menu * new_menu;

  group = (GSList *)data;

  /* Decide which radio item is selected */
  /* There really must be a better way, no? */
  which = 0;
  while (group) {
    if ( GTK_TOGGLE_BUTTON(group->data)->active ) {
      break;
    }
    group = g_slist_next(group);
    ++which;
  }

  switch (which) {
  case command_radio:
    new_menu = menu_command_new(_("New Command"), NULL, NULL, NULL);
    break;
  case folder_radio:
    new_menu = menu_folder_new(_("New Folder"), NULL, NULL, NULL);
    break;
  case separator_radio:
    new_menu = menu_separator_new(NULL);
    break;
  default:
    g_assert_not_reached();
    break;
  }

  add_to_tree_at_selection(new_menu);

  if ( IS_MENU_COMMAND(new_menu) || IS_MENU_FOLDER(new_menu) ) {
    edit_this_menu(new_menu);
  }
}

static void test_menuitem_cb(GtkWidget * mi, gpointer data)
{
  Menu * m = (Menu *) data;
  
  g_return_if_fail(IS_MENU_COMMAND(m));
  
  run_command(MENU_COMMAND_COMMAND(m));
}

static void quit_for_sure_cb(GnomeDialog * d, gint button)
{
  g_return_if_fail(GNOME_IS_MESSAGE_BOX(d));

  /* yes */
  if ( button == 0 ) {
    gtk_widget_destroy(app);    
  }
  /* no */
  else if ( button == 1 ) {
    ; 
  }
  else {
    g_assert_not_reached();
  }
}

/************************************************
  Utility functions
  ***************************************/

static void relabel_tree_item(Menu * m)
{
  GtkWidget * label;

  label = GTK_BIN(MENU_TREEITEM(m))->child;

  if ( label == NULL ) {
    /* If this happens it's a bug. */
    g_warning("Tree item has no child!");
    return;
  }

  if (MENU_NAME(m)) {
    gtk_label_set(GTK_LABEL(label), MENU_NAME(m));
  }
  else {
    /* if this happens it's a bug. fixme */
    gtk_label_set(GTK_LABEL(label), "***You didn't name this!***");
  }
}

static void init_tree_item(GtkWidget * treeitem, GtkWidget * root, 
                           Menu * m)
{
  MENU_TREEITEM(m) = treeitem;
  
  gtk_object_set_user_data ( GTK_OBJECT(treeitem), m );
  gtk_object_set_data ( GTK_OBJECT(treeitem), TREEITEM_ROOT_KEY,
			root );
  gtk_widget_ref(treeitem);
}

static void edit_this_menu(Menu * m)
{
  GtkWidget * mew;
  gchar * command;

  if ( IS_MENU_COMMAND(m) ) {
    command = MENU_COMMAND_COMMAND(m);
  }
  else {
    command = NULL;
  } 
  mew = 
    menu_entry_wiz_new_with_defaults
    ( TRUE,
      IS_MENU_COMMAND(m) ? TRUE : FALSE,
      MENU_NAME(m),
      MENU_ICON(m),
      command );

  gtk_object_set_user_data(GTK_OBJECT(mew), m);

  gtk_signal_connect ( GTK_OBJECT(mew), "menu_entry",
		       GTK_SIGNAL_FUNC(menu_entry_cb),
		       NULL );

  gtk_widget_show(mew);
}

static void make_menu_changes(Menu * m, gchar * name, gchar * icon,
                              gchar * command)
{
  if (name) {
    g_free(MENU_NAME(m));
    MENU_NAME(m) = g_strdup(name);
    changed_since_saved = TRUE;
  }
  else {
    /* A name is required. If there isn't one, that's bad. */
    popup_no_name();
    return;
  }
  
  g_free(MENU_ICON(m));
  if (icon) {
    MENU_ICON(m) = g_strdup(icon);
  }
  else MENU_ICON(m) = NULL;

  if (IS_MENU_COMMAND(m)) {
    g_free(MENU_COMMAND_COMMAND(m));
    if (command) {
      MENU_COMMAND_COMMAND(m) = g_strdup(command);
    }
    else MENU_COMMAND_COMMAND(m) = NULL;
  }
}

static void remove_from_tree(Menu * m)
{
  GtkContainer * c;
  GtkWidget * treeitem;

  changed_since_saved = TRUE;

  menu_folder_remove(MENU_PARENT(m), m);

  treeitem = MENU_TREEITEM(m);

  c = (GtkContainer *) 
    gtk_object_get_data(GTK_OBJECT(treeitem),
			TREEITEM_ROOT_KEY);

  if (c == NULL) {
    /* This is a bug if it happens */
    g_warning("Tree item with no root stored!\n");
  }

  /* This is intended to destroy this item and all its children,
     I hope it does. */
  gtk_container_remove( c, GTK_WIDGET(treeitem) );  
}

/* If a folder is selected, add at the beginning of the 
   folder. If an item is selected, add after that item. */

static void add_to_tree_at_selection(Menu * m)
{
  GtkWidget * root, * treeitem;
  gint pos;

  if (selected_tree_item == NULL) {
    popup_nothing_selected();
    return;
  }

  changed_since_saved = TRUE;

  if ( IS_MENU_FOLDER(selected_menu) ) {
    menu_folder_prepend( selected_menu, m );
    root = selected_tree_item->subtree;
    pos = 0;
  }
  else {
    menu_folder_add_after( MENU_PARENT(selected_menu),
			   m, selected_menu );
    root = GTK_TREE_ITEM(MENU_TREEITEM(MENU_PARENT(m)))->subtree;
    /* Get position of selected item, add 1 */
    pos = 
      1 + gtk_tree_child_position(GTK_TREE(root), 
				  GTK_WIDGET(selected_tree_item));
  }

  treeitem = tree_from_menu_with_pos (m, GTK_TREE(root), pos);
   
  gtk_widget_show(treeitem);
}


