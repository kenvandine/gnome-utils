/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   gshutdown: Popup dialog to shut down or reboot.
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
#include <string.h> /* strtok */

#define APPNAME "gshutdown"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#ifndef VERSION
#define VERSION "0.0.0"
#endif

/****************************
  Function prototypes
  ******************************/

static void popup_main_dialog(void);
static void prepare_easy_vbox(GtkWidget * dialog, GtkWidget * vbox);
static void prepare_advanced_vbox(GtkWidget * vbox);

static void dialog_clicked_cb(GnomeDialog * d, gint which, gpointer data);
static void runlevel_cb(GtkRadioButton * b, gint data);
static void help_cb(GtkButton * b, gpointer ignored);
static void apply_prefs_cb(GnomePropertyBox * pb, gint page, 
                           GtkEntry ** entries);
static void confirm_cb(GnomeDialog * d, gint which);
static void toggle_confirm_cb(GtkWidget * button, gpointer data);
static void revert_defaults_cb(GtkWidget * button, GtkEntry ** entries);

static void popup_preferences(void);
static void popup_about(void);
static void popup_not_in_path(const gchar * command);
static void popup_confirm();

static void do_shutdown();
static void run_command(gchar * command);

/*****************************
  Runlevel stuff.
  *****************************/

typedef enum {
  Halt, 
  SingleUser,
  Runlevel_2,
  Runlevel_3,
  Runlevel_4,
  Runlevel_5,
  Reboot
} Runlevel;

static gchar * config_keys[] = {
  "Halt",
  "SingleUser",
  "Runlevel_2",
  "Runlevel_3",
  "Runlevel_4",
  "Runlevel_5",
  "Reboot"
};

static gchar * human_readable[] = {
  N_("Shut Down"),
  N_("Single User Mode"),
  N_("Runlevel 2"),
  N_("Runlevel 3"),
  N_("Runlevel 4"),
  N_("Runlevel 5"),
  N_("Reboot")
};

/* These could be generated on the fly, except that it wouldn't
   be portable across human languages due to different syntax rules. */
static gchar * confirm_questions[] = {
  N_("Are you sure you want to shut down the computer?"),
  N_("Are you sure you want to switch to single user mode?"),
  N_("Are you sure you want to switch to runlevel 2?"),
  N_("Are you sure you want to switch to runlevel 3?"),
  N_("Are you sure you want to switch to runlevel 4?"),
  N_("Are you sure you want to switch to runlevel 5?"),
  N_("Are you sure you want to reboot the system?")
};


static gchar * default_runlevel_commands[] = {
  "shutdown -h now",
  "shutdown",
  "telinit 2",
  "telinit 3",
  "telinit 4",
  "telinit 5",
  "shutdown -r now"
};

static gchar * runlevel_commands[7];

static Runlevel requested_runlevel;

/* The runlevel radio buttons are on different notebook pages
   but in the same group, so only one is active at a time. */
static GSList * runlevel_radio_group;

/*******************************
  Main
  *******************************/

static gboolean confirm;
static gboolean confirm_button_state; /* button - may not reflect synced 
                                         setting. (Not enough data fields
                                         in property box callback). Also 
                                         used in confirm dialog. 
                                         */

int main ( int argc, char ** argv )
{
  int i;
  gchar * config_string;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, VERSION, argc, argv);

  i = 0;
  while (i < 7) {
    config_string = g_copy_strings("/"APPNAME"/Commands/", 
				   config_keys[i], "=", default_runlevel_commands[i],
				   NULL);
    runlevel_commands[i] = 
      gnome_config_get_string_with_default ( config_string,
					     NULL );
    g_free(config_string);
    ++i;
  }

  confirm = 
    gnome_config_get_bool_with_default("/"APPNAME"/General/Confirm=True", 
                                       NULL);

  popup_main_dialog();

  gtk_main();

  exit(EXIT_SUCCESS);
}

static void popup_main_dialog()
{
  GtkWidget * d;
  GtkWidget * notebook;
  GtkWidget * easy, * advanced; /* notebook pages */

  d = gnome_dialog_new( _("Shutdown or Reboot"), GNOME_STOCK_BUTTON_OK,
			GNOME_STOCK_BUTTON_CANCEL, NULL );
  gnome_dialog_set_close( GNOME_DIALOG(d), TRUE );
  /*  gnome_dialog_set_modal( GNOME_DIALOG(d) ); */

  notebook = gtk_notebook_new();

  easy = gtk_vbox_new(FALSE, GNOME_PAD);
  prepare_easy_vbox(d, easy);
  gtk_container_border_width(GTK_CONTAINER(easy), GNOME_PAD);

  advanced = gtk_vbox_new(FALSE, GNOME_PAD);
  prepare_advanced_vbox(advanced);
  gtk_container_border_width(GTK_CONTAINER(advanced), GNOME_PAD);

  gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook),
				 easy,
				 gtk_label_new(_("Easy")),
				 gtk_label_new(_("Easy")) );

  gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook),
				 advanced,
				 gtk_label_new(_("Advanced")),
				 gtk_label_new(_("Advanced")) );

  gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
				
  gtk_container_add( GTK_CONTAINER(GNOME_DIALOG(d)->vbox), notebook );

  gtk_signal_connect ( GTK_OBJECT(d), "clicked",
                       GTK_SIGNAL_FUNC(dialog_clicked_cb),
                       NULL );
 
  gtk_widget_show_all(d);
}

static void prepare_easy_vbox(GtkWidget * dialog, GtkWidget * vbox)
{
  GtkWidget * button, * label, * warning_hbox, * help_box;
  GtkWidget * warning_pixmap = NULL;
  gchar * s;

  warning_hbox = gtk_hbox_new(TRUE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX (vbox), 
		      warning_hbox, TRUE, TRUE, GNOME_PAD);

  /* Fixme, move these messagebox pixmaps into defines */
  s = gnome_pixmap_file("gnome-warning.png");
  if (s) warning_pixmap = gnome_pixmap_new_from_file(s);

  if (warning_pixmap) {
    gtk_box_pack_start (GTK_BOX (warning_hbox), 
                        warning_pixmap, TRUE, TRUE, GNOME_PAD);
  }
  
  label = gtk_label_new(_("Click OK to shutdown or reboot. You will lose\n"
                          "any unsaved information in open applications."));
  gtk_box_pack_end (GTK_BOX (warning_hbox), 
                    label, TRUE, TRUE, 0);

  button = gtk_radio_button_new_with_label(NULL, human_readable[Reboot]);

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  requested_runlevel = Reboot;

  gtk_box_pack_start (GTK_BOX (vbox), 
                      button, TRUE, TRUE, GNOME_PAD);
  
  /* Hacky int-to-pointer cast */
  gtk_signal_connect ( GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(runlevel_cb),
                       (gpointer)Reboot );

  runlevel_radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (runlevel_radio_group, 
                                            human_readable[Halt]);
  runlevel_radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  gtk_box_pack_start (GTK_BOX (vbox), 
                      button, TRUE, TRUE, GNOME_PAD);

  gtk_signal_connect ( GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(runlevel_cb),
                       (gpointer)Halt );

  help_box = gtk_hbox_new(FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX (vbox), 
                      help_box, TRUE, TRUE, 0);

  button = gnome_stock_button(GNOME_STOCK_BUTTON_HELP);
  gtk_box_pack_end(GTK_BOX(help_box), button, FALSE, FALSE, 0);
  
  gtk_signal_connect( GTK_OBJECT(button), "clicked",
                      GTK_SIGNAL_FUNC(help_cb),
                      NULL );
}

static void prepare_advanced_vbox(GtkWidget * vbox)
{
  GtkWidget * button;
  GtkWidget * box;
  int i;

  box = gtk_vbox_new(TRUE, GNOME_PAD_SMALL);
  gtk_container_border_width(GTK_CONTAINER(box), GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0); 
 
  i = SingleUser;
  while ( i < Reboot ) {
    button = gtk_radio_button_new_with_label (runlevel_radio_group, 
                                              human_readable[i]);
    runlevel_radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
    
    gtk_signal_connect ( GTK_OBJECT(button), "clicked",
                         GTK_SIGNAL_FUNC(runlevel_cb),
                         (gpointer)i );
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0); 
    ++i;
  }

  box = gtk_hbox_new(FALSE, GNOME_PAD);
  gtk_container_border_width(GTK_CONTAINER(box), GNOME_PAD);
  gtk_box_pack_end(GTK_BOX(vbox), box, FALSE, FALSE, GNOME_PAD); 

  /* Fixme these should be stock buttons */

  button = gnome_stock_or_ordinary_button("Preferences");
  gtk_box_pack_end(GTK_BOX(box), button, FALSE, FALSE, GNOME_PAD); 
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(popup_preferences), NULL);

  button = gnome_stock_or_ordinary_button("About");
  gtk_box_pack_end(GTK_BOX(box), button, FALSE, FALSE, GNOME_PAD); 
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(popup_about), NULL);
}

static void popup_preferences() 
{
  GtkWidget * box, * label, * button, * table;
  GtkWidget * d;
  static GtkWidget * entries[7];
  Runlevel i;
  gint row = 0;
  gint col = 0;

  d = gnome_property_box_new();

  table = gtk_table_new(8, 2, TRUE);
  gtk_table_set_col_spacings(GTK_TABLE(table), GNOME_PAD * 2);
  gtk_container_border_width(GTK_CONTAINER(table), GNOME_PAD);

  i = Halt;
  while ( i <= Reboot ) {
    if ( i == Runlevel_4 ) {
      /* There are two columns of entries, 4 in one, 3 in the other */
      ++col;
      row = 0;
    }

    label = gtk_label_new(human_readable[i]);
    gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, 
                     0, 0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);
    ++row;

    entries[i] = gtk_entry_new();
    gtk_widget_set_usize(entries[i], 200, 20); /* Ugh, if PropertyBox changes
                                                  size this will break */
    gtk_table_attach(GTK_TABLE(table), entries[i], col, col+1, row, row+1, 
                     0, 0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);
    ++row;

    gtk_entry_set_text(GTK_ENTRY(entries[i]), runlevel_commands[i]);

    gtk_signal_connect_object( GTK_OBJECT(entries[i]), "changed",
                               GTK_SIGNAL_FUNC(gnome_property_box_changed),
                               GTK_OBJECT(d) );
    ++i;
  }

  /* Put in a "revert to defaults" button */
  button = gtk_button_new_with_label(_("Revert to Defaults"));
  /* Attach to the last of 8 table slots in col. 2 */
  gtk_table_attach(GTK_TABLE(table), button, col, col+1,row+1, row+2, 
                   0, 0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);

  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
                     GTK_SIGNAL_FUNC(revert_defaults_cb), entries);


  gnome_property_box_append_page(GNOME_PROPERTY_BOX(d), table,
                                 gtk_label_new(_("Commands")) );

  box = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_border_width(GTK_CONTAINER(box), GNOME_PAD);

  button = gtk_check_button_new_with_label(_("Confirm before shutdown?"));
  confirm_button_state = confirm;
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), confirm_button_state);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(toggle_confirm_cb), NULL);
  gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
                             GTK_SIGNAL_FUNC(gnome_property_box_changed),
                             GTK_OBJECT(d) );

  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, GNOME_PAD);

  gnome_property_box_append_page(GNOME_PROPERTY_BOX(d), box,
                                 gtk_label_new(_("General")) );
  
  gtk_signal_connect( GTK_OBJECT(d), "apply",
                      GTK_SIGNAL_FUNC(apply_prefs_cb), entries );

  gtk_widget_show_all(d);
}

static void popup_not_in_path(const gchar * command)
{
  GtkWidget * dialog;
  gchar * message;

  message = g_copy_strings("The command \"", command, "\"\n" 
			   "could not be found.\n\n" 
			   "Most likely it's because you are "
			   "not authorized to use it.\n"
			   "This command is necessary to "
			   "shutdown, reboot, or change runlevels.", NULL);

  dialog = gnome_message_box_new(message, GNOME_MESSAGE_BOX_ERROR, 
				 GNOME_STOCK_BUTTON_OK, NULL);

  gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
		     GTK_SIGNAL_FUNC(gtk_main_quit), 
		     NULL);

  gtk_widget_show(dialog);

  g_free(message);
}

static void popup_about()
{
  GtkWidget * ga;
  static const char * authors[] = { "Havoc Pennington <hp@pobox.com>",
                                    NULL };

  ga = gnome_about_new (APPNAME,
                        VERSION, 
                        COPYRIGHT_NOTICE,
                        authors,
                        0,
                        0 );
  
  gtk_widget_show(ga);
}

static void popup_confirm(void)
{
  GtkWidget * d, * button;
  gchar * message;
  
  message = g_copy_strings(confirm_questions[requested_runlevel],
                           "\nYou will lose any unsaved work.", NULL);

  d = gnome_message_box_new(message, GNOME_MESSAGE_BOX_QUESTION, 
                            GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
                            NULL);
  
  g_free(message);

  button = gtk_check_button_new_with_label(_("Don't ask next time."));
  confirm_button_state = !confirm; /* Will always be true */
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), confirm_button_state);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(toggle_confirm_cb), NULL);
  gtk_widget_show(button);

  gtk_box_pack_end(GTK_BOX(GNOME_DIALOG(d)->vbox), 
                   button, FALSE, FALSE, GNOME_PAD);

  /* Has to be or the runlevel requested
     could change in the background */
  gtk_window_set_modal(GTK_WINDOW(d),TRUE); 

  gtk_signal_connect(GTK_OBJECT(d), "clicked",
                     GTK_SIGNAL_FUNC(confirm_cb), NULL);

  gtk_widget_show(d);
}

/**********************************
  Callbacks
  *******************************/
static void revert_defaults_cb(GtkWidget * button, GtkEntry ** entries)
{
  Runlevel i;

  i = Halt;
  while ( i <= Reboot ) {
    gtk_entry_set_text(entries[i], default_runlevel_commands[i]);
    ++i;
  }
}

static void confirm_cb(GnomeDialog * d, gint which)
{
  if (confirm_button_state == TRUE) {
    /* Don't want to be asked next time. */
    confirm = !confirm_button_state; /* not necessary, just nicer */
    gnome_config_set_bool("/"APPNAME"/General/Confirm", confirm);
    gnome_config_sync();
  }

  if ( which == 0 ) { /* Yes */
    do_shutdown();
  }
  else if ( which == 1 ) { /* No */
    gtk_main_quit();
  }
}

static void apply_prefs_cb(GnomePropertyBox * pb, gint page, 
                           GtkEntry ** entries)
{
  gchar * config_path;
  Runlevel i;

  if ( page == 0 ) {
    i = Halt;
    while ( i <= Reboot ) {
      g_free(runlevel_commands[i]);
      runlevel_commands[i] = 
        g_strdup( gtk_entry_get_text(entries[i]) );

      config_path = g_copy_strings( "/", APPNAME, "/Commands/", 
                                    config_keys[i], NULL );
      gnome_config_set_string(config_path, runlevel_commands[i]);
      g_free(config_path);
      ++i;
    }
  }
  else if ( page == 1 ) {
    confirm = confirm_button_state;
    gnome_config_set_bool("/"APPNAME"/General/Confirm", confirm);
  }
  else if ( page == -1 ) { /* End of global apply */
    gnome_config_sync();
  }
  else {
    g_assert_not_reached();
  }
}

static void dialog_clicked_cb(GnomeDialog * d, gint which, gpointer data)
{
  if ( which == 0 ) { /* OK button */
    if ( confirm ) {
      popup_confirm();
      return;
    }
    else {
      do_shutdown();
    }
  }
  else if ( which == 1 ) { /* Cancel */
    gtk_main_quit();
  }
}

static void runlevel_cb(GtkRadioButton * b, gint data)
{
  requested_runlevel = data;
}

static void toggle_confirm_cb(GtkWidget * button, gpointer data)
{
  confirm_button_state = !confirm_button_state;
}

static void help_cb(GtkButton * b, gpointer ignored)
{
  gchar * path;

  path = gnome_help_file_find_file(APPNAME, "index.html");

  gnome_help_goto(NULL, path);

  g_free(path);
}

/*********************************************
  Non-GUI
  ***********************/
static void do_shutdown(void)
{
  gchar * command_name;
  
  command_name = g_strdup(runlevel_commands[requested_runlevel]);
  command_name = strtok(command_name, " \r\n\t");
  if ( gnome_is_program_in_path(command_name) ) {
    run_command(runlevel_commands[requested_runlevel]);
    g_free(command_name);
  }
  else {
    popup_not_in_path(command_name);
    g_free(command_name);
    return;
  }

  gtk_main_quit();
}

/********************************
  Stuff that should be in gnome-util and be an exec
  *******************************/

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

static void run_command(gchar * command)
{
  pid_t new_pid;
  
  new_pid = fork();

  switch (new_pid) {
  case -1 :
    g_warning("Command execution failed: fork failed");
    break;
  case 0 : 
    _exit(system(command));
    break;
  default:
    break;
  }
}

