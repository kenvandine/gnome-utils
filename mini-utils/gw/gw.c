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

/***********************************
  Globals
  ***********************************/

/* This is bad, it shouldn't be hardcoded;
   but CList won't let you change it after the list
   is created. The alternative is to recreate the 
   CList each time, I guess; may do that later. */
static gint num_columns = 7;

static GtkWidget * app;

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
  g_warning("Preferences not implemented yet\n");
}


/*********************************************
  Non-GUI
  ***********************/








