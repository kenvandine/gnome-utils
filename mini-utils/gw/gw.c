/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   gw: Graphical version of "w", shows users on the system.
 *    
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

#include <string.h>

#define APPNAME "gw"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#ifndef VERSION
#define VERSION "0.0.0"
#endif

/* gnome-config section for actions on w entries */
#define ACTIONS_SECTION "Actions"

/* Used to store pointers to entries in CList 
   in the preferences dialog */
#define NAME_ENTRY_KEY "Name_Entry"
#define COMMAND_ENTRY_KEY "Command_Entry"
#define PROPERTY_BOX_KEY "PropertyBox_Key"

/*************************
  Types
  **************************/

typedef struct _Action Action;

struct _Action {
  gchar * key;      /* displayable name and config key */
  gchar * format;   /* Format for command, with escape sequences
                       for data drawn from "w" display.
                       So far:
                       %u     user name
                       %t     tty   */
};


/****************************
  Function prototypes
  ******************************/

static void prepare_app();
static void popup_about();

static gint delete_event_cb(GtkWidget * w, gpointer data);
static void about_cb(GtkWidget * w, gpointer data);
static void save_cb(GtkWidget * w, gpointer data);

static void preferences_cb(GtkWidget *w, gpointer data);

static void reset_list(GtkCList * list);


static void new_action_in_prefs(GtkCList * list, gchar * name, 
                                gchar * command);
static void add_action_cb(GtkButton * button, gpointer clist);
static void delete_action_cb(GtkButton * button, gpointer clist);
static void clist_selected_cb(GtkCList * clist, gint row, 
                              gint col, GdkEventButton * event,
                              gpointer add_button);
static void clist_unselected_cb(GtkCList * clist, gint row,
                                gint col, GdkEventButton * event);
static void name_changed_cb(GtkEntry * entry, GtkCList * clist);
static void command_changed_cb(GtkEntry * entry, GtkCList * clist);
static void apply_prefs_cb(GnomePropertyBox * pb, gint page, GtkCList * data);

static void load_actions(); 
static void clear_actions();
static void save_actions(); 
static void prepend_action(const gchar * key, const gchar * format);

/***********************************
  Globals
  ***********************************/

/* This is bad, it shouldn't be hardcoded;
   but CList won't let you change it after the list
   is created. The alternative is to recreate the 
   CList each time, I guess; may do that later. */
static gint num_columns = 7;

static GtkWidget * app;

static GList * actions = NULL;

/*******************************
  Main
  *******************************/

int main ( int argc, char ** argv )
{
  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, 0, argc, argv, 0, 0);

  load_actions();

  prepare_app();

  gtk_main();

  exit(EXIT_SUCCESS);
}

/**************************************
  Set up the GUI
  ******************************/

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
   N_("Save"), N_("Write information to disk"), 
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
#define MENU_HELP_POS 2
  GNOMEUIINFO_SUBTREE (N_("Help"), help_menu),
  GNOMEUIINFO_END
};

static void prepare_app()
{
  GtkWidget * app_box;
  GtkWidget * clist;
  GtkWidget * reset_button;

  app = gnome_app_new( APPNAME, _("User Listing") ); 

  gnome_app_create_menus(GNOME_APP(app), main_menu);

  gtk_menu_item_right_justify(GTK_MENU_ITEM(main_menu[MENU_HELP_POS].widget));

  app_box = gtk_vbox_new ( FALSE, GNOME_PAD );
  gtk_container_border_width(GTK_CONTAINER(app_box), GNOME_PAD);

  gnome_app_set_contents ( GNOME_APP(app), app_box );

  gtk_signal_connect ( GTK_OBJECT(app), "destroy",
                       GTK_SIGNAL_FUNC(gtk_main_quit),
                       0 );

  gtk_signal_connect ( GTK_OBJECT (app), "delete_event",
                       GTK_SIGNAL_FUNC (delete_event_cb),
                       NULL );
 
  reset_button = gtk_button_new_with_label(_("Update Information"));
  gtk_box_pack_start(GTK_BOX(app_box), reset_button, 
                     FALSE, FALSE, GNOME_PAD_SMALL);

  clist = gtk_clist_new(num_columns);
  gtk_clist_set_border(GTK_CLIST(clist), GTK_SHADOW_OUT);
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_BROWSE);
  gtk_clist_set_policy(GTK_CLIST(clist), GTK_POLICY_AUTOMATIC,
                       GTK_POLICY_AUTOMATIC);
  gtk_clist_column_titles_show(GTK_CLIST(clist));

  gtk_box_pack_start(GTK_BOX(app_box), clist, 
                     TRUE, TRUE, GNOME_PAD);


  gtk_signal_connect_object(GTK_OBJECT(reset_button), "clicked",
                            GTK_SIGNAL_FUNC(reset_list), GTK_OBJECT(clist));

  reset_list(GTK_CLIST(clist));

  gtk_widget_show_all(app);
}

static void popup_about()
{
  GtkWidget * ga;
  gchar * authors[] = { "Havoc Pennington <hp@pobox.com>",
                        NULL };

  ga = gnome_about_new (APPNAME,
                        VERSION, 
                        COPYRIGHT_NOTICE,
                        authors,
                        0,
                        0 );
  
  gtk_widget_show(ga);
}

/**********************************
  reset_list(), runs w and puts the info in the list.
  *******************************/

#define WHITESPACE " \t\r\n"


/* Want a function not a macro, so a and b are calculated only once */
static gint max(gint a, gint b)
{
  if ( a > b )
    return a;
  else return b;
}

static void reset_list(GtkCList * list)
{
  gchar * token;
  gint col;
  gchar * row_text[num_columns];
  static const gint bufsize = 255;
  gchar buffer[bufsize+1]; /* For a single line */
  FILE * f;
  gchar * returned;
  GdkFont * font;
  gint col_widths[num_columns];
  gint total_width;

  gtk_clist_freeze(list);  

  /* To compute column widths */
  font = gtk_widget_get_style(GTK_WIDGET(list))->font;
  col = 0; 
  while ( col < num_columns ) { col_widths[col] = 0; ++col; }

  /* Start getting "w" information */
  memset(buffer, '\0', sizeof(buffer));

  f = popen("w", "r");

  if ( f != NULL ) {

    /* The first line on Linux and Solaris is uname garbage. Skip it.
       (How portable this is, I have no idea.) */
    returned = fgets(buffer, bufsize, f);

    /* Grab the second line, and use it to create column titles. */
    returned = fgets(buffer, bufsize, f);

    if ( returned == NULL ) {
      /* FIXME error handling */
    }

    col = 0;
    token = strtok(buffer, WHITESPACE);
    while ( token ) {
      gtk_clist_set_column_title(list, col, token);
      ++col;
      token = strtok(NULL, WHITESPACE);
    }

    gtk_clist_clear(list); /* No errors so far, so clear the old stuff */

    /* Now parse each line of w's output and put each line in a row. */
    returned = fgets(buffer, bufsize, f);
    while ( returned != NULL ) {
      /* Have a line, parse it. */
      col = 0;
      token = strtok(buffer, WHITESPACE);
      while ( (col < num_columns) && (token != NULL) ) {
        row_text[col] = token;

        /* I hope this is fast enough. Save maximum width of anything
           in the column. Seems to be reasonably fast. */
        col_widths[col] = max(col_widths[col], gdk_string_width(font, token));

        token = strtok(NULL, WHITESPACE);
        ++col;
      }
          
      gtk_clist_append(list, row_text);

      returned = fgets(buffer, bufsize, f);
    } 

    fclose(f);
        
    /* Now set column widths to maximums we found, and sum
       total width, throwing in a little padding. */
    col = 0; total_width = 0;
    while ( col < num_columns ) {
      gtk_clist_set_column_width(list, col, col_widths[col] + 10);
      total_width += (col_widths[col] + 15);
      ++col;
    }

    /* set total widget size to the sum of all column widths, 
       with sanity check. */
    gtk_widget_set_usize(GTK_WIDGET(list), CLAMP(total_width, 0, 600), 400);
    /* Make column titles non-interactive. Seems to be needed on 
       each reset. */
    gtk_clist_column_titles_passive(list);

  }

  gtk_clist_thaw(list);
}

/************************************
  Callbacks 
  *********************************/
static gint delete_event_cb(GtkWidget * w, gpointer data)
{
  gtk_main_quit();
  return TRUE; /* hmm, look up what's supposed to be here. */
}

static void about_cb(GtkWidget * w, gpointer data)
{
  popup_about();
}

static void save_cb(GtkWidget * w, gpointer data)
{
  g_warning("Save not implemented yet\n");
}

static void preferences_cb(GtkWidget *w, gpointer data)
{
  GtkWidget * pb;
  GtkWidget * frame, * frame_vbox, * page_vbox, 
    * bottom_hbox, * entry_hbox;
  GtkWidget * samples_button, * add_button, * delete_button;
  GtkWidget * name_entry;
  GtkWidget * command_entry;
  GtkWidget * list;
  GtkWidget * explanation_label;
  gchar * titles[] = { _("Name"), _("Command Line") }; 
  GList * tmp; 
  Action * a;
  gchar * text[2];
  gint row;

  /* Create everything. */

  samples_button = gtk_button_new_with_label(_("Add some samples"));
  add_button = gtk_button_new_with_label(_("Add"));
  delete_button = gtk_button_new_with_label(_("Delete"));

  name_entry = gtk_entry_new();
  command_entry = gtk_entry_new();

  list = gtk_clist_new_with_titles(2, titles);
  gtk_clist_set_border(GTK_CLIST(list), GTK_SHADOW_OUT);
  gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_BROWSE);
  gtk_clist_set_policy(GTK_CLIST(list), GTK_POLICY_AUTOMATIC,
                       GTK_POLICY_AUTOMATIC);
  gtk_clist_set_column_width(GTK_CLIST(list), 0, 130); /* Otherwise it's 0 */
  gtk_clist_column_titles_show(GTK_CLIST(list));
  gtk_clist_column_titles_passive(GTK_CLIST(list));

  frame = gtk_frame_new(_("Command Editor"));
  gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD);

  frame_vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
  page_vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
  bottom_hbox = gtk_hbox_new(FALSE, GNOME_PAD);
  entry_hbox = gtk_hbox_new(FALSE, GNOME_PAD);

  explanation_label = 
    gtk_label_new(_("You can configure the commands on the popup menus.\n"
                    "Just enter the name you want to appear on the \n"
                    "menu, and the command to execute.\n"
                    "In the command, you can use %u to represent the\n"
                    "currently selected username, and %t for the tty."));

  pb = gnome_property_box_new();
  
  /* Some assembly required */

  gnome_property_box_append_page(GNOME_PROPERTY_BOX(pb), page_vbox,
                                 gtk_label_new(_("Menu items")));

  gtk_box_pack_start(GTK_BOX(page_vbox), frame, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_start(GTK_BOX(page_vbox), bottom_hbox, FALSE, FALSE, GNOME_PAD);

  gtk_box_pack_start(GTK_BOX(bottom_hbox), explanation_label, 
                     FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_end(GTK_BOX(bottom_hbox), samples_button, 
                   FALSE, FALSE, GNOME_PAD);

  gtk_container_add(GTK_CONTAINER(frame), frame_vbox);
  gtk_container_border_width(GTK_CONTAINER(frame_vbox), GNOME_PAD);

  gtk_box_pack_start(GTK_BOX(frame_vbox), entry_hbox, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_start(GTK_BOX(frame_vbox), list, TRUE, TRUE, GNOME_PAD_SMALL);


  gtk_box_pack_start(GTK_BOX(entry_hbox), gtk_label_new(_("Name")),
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(entry_hbox), name_entry, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_end(GTK_BOX(entry_hbox), delete_button, FALSE, FALSE, GNOME_PAD_SMALL);  
  gtk_box_pack_end(GTK_BOX(entry_hbox), add_button, FALSE, FALSE, GNOME_PAD_SMALL);  
  gtk_box_pack_end(GTK_BOX(entry_hbox), command_entry, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_end(GTK_BOX(entry_hbox), gtk_label_new(_("Command")),
                   FALSE, FALSE, 0);

  /* Put current actions in the list. */
  
  tmp = actions;
  while (tmp) {
    a = (Action *)tmp->data;
    text[0] = a->key; text[1] = a->format;
    gtk_clist_append(GTK_CLIST(list), text);
    tmp = tmp->next;
  }

  /* Unselect the whole CList */
  row = 0;
  while ( row < GTK_CLIST(list)->rows) {
    gtk_clist_unselect_row(GTK_CLIST(list), row, 0);
    ++row;
  }

  /* Since there's no selection, make the entries insensitive. */
  gtk_widget_set_sensitive(name_entry, FALSE);
  gtk_widget_set_sensitive(command_entry, FALSE);


  /* Connect signals to make something happen. */

  gtk_object_set_data(GTK_OBJECT(list), NAME_ENTRY_KEY, name_entry);
  gtk_object_set_data(GTK_OBJECT(list), COMMAND_ENTRY_KEY, command_entry);
  gtk_object_set_data(GTK_OBJECT(list), PROPERTY_BOX_KEY, pb);

  gtk_signal_connect(GTK_OBJECT(add_button), "clicked",
                     GTK_SIGNAL_FUNC(add_action_cb), list);

  gtk_signal_connect(GTK_OBJECT(delete_button), "clicked",
                     GTK_SIGNAL_FUNC(delete_action_cb), list);
    
  gtk_signal_connect(GTK_OBJECT(list), "select_row",
                     GTK_SIGNAL_FUNC(clist_selected_cb), NULL);

  gtk_signal_connect(GTK_OBJECT(list), "unselect_row",
                     GTK_SIGNAL_FUNC(clist_unselected_cb), NULL);

  gtk_signal_connect(GTK_OBJECT(pb), "apply",
                     GTK_SIGNAL_FUNC(apply_prefs_cb), list);

  gtk_signal_connect_object(GTK_OBJECT(name_entry), "changed",
                            GTK_SIGNAL_FUNC(gnome_property_box_changed),
                            GTK_OBJECT(pb));

  gtk_signal_connect_object(GTK_OBJECT(command_entry), "changed",
                            GTK_SIGNAL_FUNC(gnome_property_box_changed),
                            GTK_OBJECT(pb));

  /* Show */
  gtk_widget_show_all(pb);
}

/*********************************
  Preferences dialog callbacks and functions 
  ********************************/

/* I think this mess of set_data and callbacks might be cleaner
   if I just used some globals. */

static void new_action_in_prefs(GtkCList * list, gchar * name, gchar * command)
{
  gchar * text[2];
  gint new_row;
  GnomePropertyBox * pb;

  text[0] = name;
  text[1] = command;

  new_row = gtk_clist_append(list, text);

  gtk_clist_select_row(list, new_row, 0);

  pb = GNOME_PROPERTY_BOX(gtk_object_get_data(GTK_OBJECT(list), 
                                              PROPERTY_BOX_KEY));
  gnome_property_box_changed(pb);
}

static void add_action_cb(GtkButton * button, gpointer clist)
{ 
  new_action_in_prefs(GTK_CLIST(clist), "", "");
}

static void delete_action_cb(GtkButton * button, gpointer clist)
{
  GtkEntry * name_entry, * command_entry;
  GtkCList * c = GTK_CLIST(clist);
  GnomePropertyBox * pb;

  if ( c->selection != NULL ) {
    name_entry = gtk_object_get_data(GTK_OBJECT(clist), 
                                     NAME_ENTRY_KEY);
    command_entry = gtk_object_get_data(GTK_OBJECT(clist), 
                                        COMMAND_ENTRY_KEY);
    
    gtk_entry_set_text(name_entry, "");
    gtk_entry_set_text(command_entry, "");

    gtk_clist_remove(c, (gint)c->selection->data);

    pb = GNOME_PROPERTY_BOX(gtk_object_get_data(GTK_OBJECT(clist), 
                                                PROPERTY_BOX_KEY));
    gnome_property_box_changed(pb);
  }
}

/* Cheat a little; this dialog is so complex! */
static guint name_connection;
static guint command_connection;

static void clist_selected_cb(GtkCList * clist, gint row, 
                              gint col, GdkEventButton * event,
                              gpointer add_button)
{
  GtkEntry * name_entry, * command_entry;
  gchar * name, * command;

  name_entry = gtk_object_get_data(GTK_OBJECT(clist), 
                                   NAME_ENTRY_KEY);
  command_entry = gtk_object_get_data(GTK_OBJECT(clist), 
                                      COMMAND_ENTRY_KEY);

  gtk_clist_get_text(clist, row, 0, &name);
  gtk_clist_get_text(clist, row, 1, &command);

  gtk_entry_set_text(name_entry, name);
  gtk_entry_set_text(command_entry, command);

  name_connection = 
    gtk_signal_connect(GTK_OBJECT(name_entry), "changed",
                       GTK_SIGNAL_FUNC(name_changed_cb),
                       clist);
  command_connection = 
    gtk_signal_connect(GTK_OBJECT(command_entry), "changed",
                       GTK_SIGNAL_FUNC(command_changed_cb),
                       clist);

  gtk_widget_set_sensitive(GTK_WIDGET(name_entry), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(command_entry), TRUE);
}

static void name_changed_cb(GtkEntry * entry, GtkCList * clist)
{
  gchar * name;
  gint row;

  name = gtk_entry_get_text(entry);

  row = (gint) clist->selection->data;

  gtk_clist_set_text(clist, row, 0, name);
}

static void command_changed_cb(GtkEntry * entry, GtkCList * clist)
{
  gchar * command;
  gint row;

  command = gtk_entry_get_text(entry);
  
  row = (gint) clist->selection->data;

  gtk_clist_set_text(clist, row, 1, command);
}

static void clist_unselected_cb(GtkCList * clist, gint row,
                                gint col, GdkEventButton * event)
{
  GtkEntry * name_entry, * command_entry;

  name_entry = gtk_object_get_data(GTK_OBJECT(clist), 
                                   NAME_ENTRY_KEY);
  command_entry = gtk_object_get_data(GTK_OBJECT(clist), 
                                      COMMAND_ENTRY_KEY);
  
  gtk_signal_disconnect(GTK_OBJECT(name_entry), name_connection);
  gtk_signal_disconnect(GTK_OBJECT(command_entry), command_connection);

  if ( GTK_CLIST(clist)->selection == NULL ) {
    gtk_widget_set_sensitive(GTK_WIDGET(name_entry), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(command_entry), FALSE);
  }
}

static void apply_prefs_cb(GnomePropertyBox * pb, gint page, GtkCList * clist)
{
  gint row = 0;
  gchar * name, * command;

  if ( page != -1 ) return; /* Only act on global apply */

  clear_actions();

  while (row < clist->rows) {
    gtk_clist_get_text(clist, row, 0, &name);
    gtk_clist_get_text(clist, row, 1, &command);
    prepend_action(name, command);
    ++row;
  }

  save_actions();
}

/*********************************************
  Non-GUI
  ***********************/

static void load_actions()
{
  void * config_iterator;
  Action * a;
  gchar * name, * command;

  config_iterator = gnome_config_init_iterator("/"APPNAME"/"ACTIONS_SECTION);

  while ( (config_iterator = 
           gnome_config_iterator_next(config_iterator, 
                                      &name, &command)) != NULL ) {
    a = g_new(Action, 1);
    a->key = name; a->format = command;
    actions = g_list_prepend(actions, a);
  }
}

/* Just clears the loaded list, not gnome_config */
static void clear_actions()
{
  GList * tmp;
  Action * a;

  tmp = actions;

  while ( tmp ) {
    a = (Action *) tmp->data;
    g_free(a->key);
    g_free(a->format);
    g_free(a);
    tmp = tmp->next;
  }
  actions = NULL;
}

static void prepend_action(const gchar * key, const gchar * format)
{
  Action * a;

  a = g_new(Action, 1);
  a->key = g_strdup(key);
  a->format = g_strdup(format);

  actions = g_list_prepend(actions, a);
}

static void save_actions()
{
  Action * a;
  GList * tmp;

  gnome_config_clean_section("/"APPNAME"/"ACTIONS_SECTION);
  gnome_config_push_prefix("/"APPNAME"/"ACTIONS_SECTION"/");

  tmp = actions;

  while (tmp) {  
    a = (Action *)tmp->data;
    gnome_config_set_string(a->key, a->format);
    tmp = tmp->next;
  }

  gnome_config_sync();
  gnome_config_pop_prefix();
}






