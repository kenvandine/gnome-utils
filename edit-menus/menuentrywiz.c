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

#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "menuentrywiz.h"
#include "iconbrowse.h"
#include "is_image.h"
#include "run_command.h"

static void menu_entry_wiz_class_init (MenuEntryWizClass * mew_class);
static void menu_entry_wiz_init (MenuEntryWiz * mew);

static void destroy (GtkObject * mew);

static void set_icon(MenuEntryWiz * mew, gchar * filename );
static void set_icon_entry(MenuEntryWiz * e, gchar * filename );
static void set_icon_pixmap(MenuEntryWiz * mew, gchar * filename);
static void set_name(MenuEntryWiz * mew, gchar * name);
static void set_name_entry(MenuEntryWiz * mew, gchar * name);
static void set_command(MenuEntryWiz * mew, gchar * command);
static void set_command_entry(MenuEntryWiz * mew, gchar * command);

/* Attached to apply signal of property box, emits menu_entry
   signal */
static void emit_menu_entry (MenuEntryWiz * mew, gint page);

/* Callbacks from icon browser and file selection */
static void icon_chosen_callback(IconBrowse * ib, gchar * filename, 
				 MenuEntryWiz * mew);
static void file_selected_callback(GtkButton * whocares, 
				   GtkFileSelection * fs);

/* callbacks from text entries */
static void icon_changed_callback(GtkEntry * e, MenuEntryWiz * mew);
static void name_changed_callback(GtkEntry * e, MenuEntryWiz * mew);
static void command_changed_callback(GtkEntry * e, 
				     MenuEntryWiz * mew);

/* callback from "test command" button */
static void test_command(MenuEntryWiz * mew);

static void menu_entry_marshaller (GtkObject      *object,
				   GtkSignalFunc   func,
				   gpointer        func_data,
				   GtkArg         *params);

static void create_advanced(MenuEntryWiz * mew);

guint menu_entry_wiz_get_type (void)
{
  static guint mew_type = 0;

  if (!mew_type) {
    GtkTypeInfo mew_info = 
    {
      "MenuEntryWiz",
      sizeof (MenuEntryWiz),
      sizeof (MenuEntryWizClass),
      (GtkClassInitFunc) menu_entry_wiz_class_init,
      (GtkObjectInitFunc) menu_entry_wiz_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    mew_type = gtk_type_unique (gnome_property_box_get_type(), 
				&mew_info);
  }

  return mew_type;
}

enum {
  MENU_ENTRY_SIGNAL,
  LAST_SIGNAL
};

static gint mew_signals[LAST_SIGNAL] = { 0 };

static GtkDialogClass * parent_class = NULL;

static void menu_entry_wiz_class_init (MenuEntryWizClass * mew_class)
{
  GtkObjectClass * object_class;
  
  object_class = GTK_OBJECT_CLASS (mew_class);
  parent_class = gtk_type_class (gnome_property_box_get_type ());

  mew_signals[MENU_ENTRY_SIGNAL] = 
    gtk_signal_new ("menu_entry",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (MenuEntryWizClass, menu_entry),
		    menu_entry_marshaller,
		    GTK_TYPE_NONE,
		    3, GTK_TYPE_POINTER, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, mew_signals, 
				LAST_SIGNAL);

  object_class->destroy = destroy; 
  mew_class->menu_entry = 0;

  return;
}

static void menu_entry_wiz_init (MenuEntryWiz * mew)
{
  mew->icon = 0;
  mew->name = 0;
  mew->command = 0;
  mew->icon_pixmap = 0;

  gtk_window_set_title ( GTK_WINDOW(mew), "Menu Entry Builder");

  gtk_signal_connect_object(GTK_OBJECT(mew), "apply",
			    GTK_SIGNAL_FUNC(emit_menu_entry),
			    GTK_OBJECT(mew));

  create_advanced(mew);
}

GtkWidget * menu_entry_wiz_new(gboolean ask_for_icon, 
			       gboolean ask_for_command)
{
  MenuEntryWiz * mew = gtk_type_new (menu_entry_wiz_get_type());

  set_icon_pixmap (mew, 0);

  if (ask_for_icon == FALSE) {
    gtk_widget_set_sensitive (mew->icon_container, FALSE);
  }

  if (ask_for_command == FALSE) {
    gtk_widget_set_sensitive (mew->command_container, FALSE);
  }

  return GTK_WIDGET(mew);
}

GtkWidget * menu_entry_wiz_new_with_defaults(gboolean ask_for_icon,
					     gboolean ask_for_command,
					     gchar * name,
					     gchar * icon,
					     gchar * command)
{
  MenuEntryWiz * mew = 
    MENU_ENTRY_WIZ(menu_entry_wiz_new(ask_for_icon, ask_for_command));

  /* setting the entry sets in motion changing the pixmap, etc.*/
  /* It's increasingly obvious that there's no point 
     maintaining the strings outside the entries */
  
  set_name_entry(mew, name);

  set_icon_entry(mew, icon);

  set_command_entry(mew, command);

  return GTK_WIDGET(mew);
}

static void popup_file_selection (MenuEntryWiz * mew)
{
  GtkWidget * fs;

  g_return_if_fail ( IS_MENU_ENTRY_WIZ(mew) );

  fs = gtk_file_selection_new("Choose an icon file");

  gtk_object_set_user_data(GTK_OBJECT(fs), mew);

  gtk_signal_connect ( GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
		       "clicked",
		       GTK_SIGNAL_FUNC(file_selected_callback),
		       fs );
  
  gtk_signal_connect_object 
    (  GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
       "clicked",
       GTK_SIGNAL_FUNC(gtk_widget_destroy),
       GTK_OBJECT(fs) );

  gtk_widget_show(fs);
}

static void popup_icon_browse (MenuEntryWiz * mew)
{
  GtkWidget * ib;

  g_return_if_fail ( IS_MENU_ENTRY_WIZ(mew) );

  ib =  
    icon_browse_new("an icon");

  gtk_signal_connect ( GTK_OBJECT(ib), "icon_chosen",
		       GTK_SIGNAL_FUNC(icon_chosen_callback), 
		       mew );

  gtk_widget_show(ib);
}

static void create_advanced(MenuEntryWiz * mew)
{
  GtkWidget * vbox;
  GtkWidget * icon_box;
  GtkWidget * command_box;
  GtkWidget * name_box;
  GtkWidget * icon_entry, * command_entry, * name_entry;
  GtkWidget * icon_label, * command_label, * name_label;
  GtkWidget * button, * button_box, * ip_cont;
  GtkWidget * tab_label;

  vbox = gtk_vbox_new( FALSE, 10 );

  mew->icon_container = icon_box = gtk_vbox_new( FALSE, 3 );
  mew->command_container = command_box = gtk_vbox_new( FALSE, 3 );
  name_box = gtk_vbox_new( FALSE, 3 );

  gtk_box_pack_start( GTK_BOX(vbox), name_box, FALSE, FALSE, 5 );
  gtk_box_pack_start( GTK_BOX(vbox), icon_box, FALSE, FALSE, 5 );
  gtk_box_pack_start( GTK_BOX(vbox), command_box, FALSE, FALSE, 5 );

  gtk_widget_show(icon_box);
  gtk_widget_show(command_box);
  gtk_widget_show(name_box);

  mew->icon_entry = icon_entry = 
    gtk_entry_new_with_max_length(PATH_MAX);
  mew->command_entry = command_entry = 
    gtk_entry_new_with_max_length(PATH_MAX);
  mew->name_entry = name_entry = 
    gtk_entry_new_with_max_length(PATH_MAX);

  icon_label =
    gtk_label_new("Icon to use for this menu entry: ");

  command_label = 
    gtk_label_new("Command invoked by this menu entry: ");

  name_label =
    gtk_label_new("What to call this menu entry: ");

  mew->ip_container = ip_cont = gtk_hbox_new(FALSE,5);

  gtk_widget_show(ip_cont);

  gtk_box_pack_start( GTK_BOX(icon_box), 
		      icon_label, FALSE, FALSE, 3 );
  gtk_box_pack_start( GTK_BOX(icon_box), 
		      ip_cont, FALSE, FALSE, 3 );
  gtk_box_pack_start( GTK_BOX(icon_box), 
		      icon_entry, TRUE, TRUE, 3 );

  gtk_box_pack_start( GTK_BOX(command_box), 
		      command_label, FALSE, FALSE, 3 );
  gtk_box_pack_start( GTK_BOX(command_box), 
		      command_entry, TRUE, TRUE, 3 );

  gtk_box_pack_start( GTK_BOX(name_box), 
		      name_label, FALSE, FALSE, 3 );
  gtk_box_pack_start( GTK_BOX(name_box), 
		      name_entry, TRUE, TRUE, 3 );

  gtk_signal_connect ( GTK_OBJECT(icon_entry), "changed",
		       GTK_SIGNAL_FUNC(icon_changed_callback),
		       mew );
  gtk_signal_connect ( GTK_OBJECT(name_entry), "changed",
		       GTK_SIGNAL_FUNC(name_changed_callback),
		       mew );
  gtk_signal_connect ( GTK_OBJECT(command_entry), "changed",
		       GTK_SIGNAL_FUNC(command_changed_callback),
		       mew );

  gtk_widget_show ( icon_label );
  gtk_widget_show ( icon_entry );
  gtk_widget_show ( command_label );
  gtk_widget_show ( command_entry );
  gtk_widget_show ( name_label );
  gtk_widget_show ( name_entry );

  /* Buttons below command entry */

  button_box = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_end( GTK_BOX(command_box), 
		    button_box, FALSE, FALSE, 3 );
  
  gtk_widget_show(button_box);
  
  mew->test_command_button = 
    button = gtk_button_new_with_label ("Test command");

  gtk_widget_set_sensitive(button, FALSE);

  gtk_signal_connect_object ( GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(test_command),
			      GTK_OBJECT(mew) );
  gtk_box_pack_end( GTK_BOX(button_box), 
		    button, FALSE, FALSE, 5 );
  
  gtk_widget_show(button);

  /* Buttons below icon entry */

  button_box = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_end( GTK_BOX(icon_box), 
		    button_box, FALSE, FALSE, 3 );

  gtk_widget_show(button_box);
  

  button = gtk_button_new_with_label ("Browse icons...");
  gtk_signal_connect_object ( GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(popup_icon_browse),
			      GTK_OBJECT(mew) );
  gtk_box_pack_end( GTK_BOX(button_box), 
		    button, FALSE, FALSE, 5 );
  
  gtk_widget_show(button);

  button = gtk_button_new_with_label ("Browse files...");
  gtk_signal_connect_object ( GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(popup_file_selection),
			      GTK_OBJECT(mew) );
  gtk_box_pack_end( GTK_BOX(button_box), 
		    button, FALSE, FALSE, 5 );
  
  gtk_widget_show(button);

  tab_label = gtk_label_new("Advanced");
  gtk_widget_show(tab_label);

  gnome_property_box_append_page ( GNOME_PROPERTY_BOX(mew),
				   vbox, tab_label );


  gtk_widget_show(vbox);
}

static void menu_entry_marshaller (GtkObject      *object,
				   GtkSignalFunc   func,
				   gpointer        func_data,
				   GtkArg         *params)
{
  typedef void (* myfunc)(GtkObject *, gpointer, gpointer, 
			  gpointer, gpointer);

  myfunc rfunc = (myfunc)func;
  
  (* rfunc) (object, 
	     GTK_VALUE_POINTER(params[0]),
	     GTK_VALUE_POINTER(params[1]),
	     GTK_VALUE_POINTER(params[2]),
	     func_data );
}


static void destroy (GtkObject * o)
{
  MenuEntryWiz * mew = MENU_ENTRY_WIZ(o);

  g_return_if_fail ( IS_MENU_ENTRY_WIZ(mew) );

  g_free (mew->icon);
  g_free (mew->name);
  g_free (mew->command);

  GTK_OBJECT_CLASS (parent_class)->destroy (o);
}

static void set_entry_if(GtkWidget * e, gchar * t)
{
  if (t) {
    gtk_entry_set_text ( GTK_ENTRY(e), t );
  }
  else {
    gtk_entry_set_text ( GTK_ENTRY(e), "" );
  }
}

static void set_icon_entry (MenuEntryWiz * mew, gchar * t)
{
  set_entry_if(mew->icon_entry, t);
}

static void set_name_entry (MenuEntryWiz * mew, gchar * t)
{
  set_entry_if(mew->name_entry, t);
}

static void set_command_entry (MenuEntryWiz * mew, gchar * t)
{
  set_entry_if(mew->command_entry, t);
}

static void set_icon(MenuEntryWiz * mew, gchar * filename)
{  
  g_free (mew->icon);
  mew->icon = 0;

  if ( filename && ( strlen(filename) != 0 ) ) {
    mew->icon = g_strdup(filename);
  }
  gnome_property_box_changed(GNOME_PROPERTY_BOX(mew));
}

static void set_icon_pixmap(MenuEntryWiz * mew, gchar * filename)
{
  if (mew->icon_pixmap) {
    gtk_widget_destroy (mew->icon_pixmap);
  }
  
  if (filename) {
    mew->icon_pixmap = gnome_pixmap_new_from_file (filename);
    /* Would like above to return NULL if pixmap can't
       be created. Then the following could be eliminated.
       First test is so NULL return, if implemented, won't
       break things */
    if (mew->icon_pixmap) {
      if ( GNOME_PIXMAP(mew->icon_pixmap)->pixmap == NULL) {
	/* destroy the half-built widget */
	gtk_widget_destroy(mew->icon_pixmap);
	mew->icon_pixmap = NULL;
      }
    }     
  }
  else {
    mew->icon_pixmap = NULL;
  }
  
  if (mew->icon_pixmap == NULL) {
    mew->icon_pixmap = gtk_label_new("No icon");    
  }
  
  gtk_container_add( GTK_CONTAINER(mew->ip_container),
		     mew->icon_pixmap );
  gtk_widget_show(mew->icon_pixmap);
}

static void set_name(MenuEntryWiz * mew, gchar * name)
{
  g_free (mew->name);
  if (strlen(name) == 0) {
    mew->name = 0;
  }
  else {
    mew->name = g_strdup(name);
  }
  gnome_property_box_changed(GNOME_PROPERTY_BOX(mew));
}

static void set_command(MenuEntryWiz * mew, gchar * command)
{
  g_free (mew->command);
  if (strlen(command) == 0) {
    mew->command = 0;

    gtk_widget_set_sensitive(mew->test_command_button, FALSE);

  }
  else {
    mew->command = g_strdup(command);
    gtk_widget_set_sensitive(mew->test_command_button, TRUE);
  }
  gnome_property_box_changed(GNOME_PROPERTY_BOX(mew));
}

static void emit_menu_entry (MenuEntryWiz * mew, gint page)
{
  /* if the page is -1, this signals the end of a global apply */
  if ( page != -1 ) {
    gtk_signal_emit ( GTK_OBJECT(mew), mew_signals[MENU_ENTRY_SIGNAL],
		      mew->name, 
		      mew->icon, 
		      mew->command );
  }
}

static void icon_chosen_callback(IconBrowse * ib, gchar * filename, 
				 MenuEntryWiz * mew)
{
  g_return_if_fail ( IS_MENU_ENTRY_WIZ(mew) );
  g_return_if_fail ( IS_ICON_BROWSE(ib) );

  set_icon_entry ( mew, filename );
}

static void file_selected_callback(GtkButton * whocares, 
				   GtkFileSelection * fs)
{
  MenuEntryWiz * mew;
  gchar * icon;

  g_return_if_fail ( fs != 0 );
  g_return_if_fail ( GTK_IS_FILE_SELECTION(fs) );

  mew = MENU_ENTRY_WIZ( gtk_object_get_user_data(GTK_OBJECT(fs)) );
  g_return_if_fail ( IS_MENU_ENTRY_WIZ(mew) );

  icon = gtk_file_selection_get_filename(fs);
  /* oops, no need for icon variable anymore */
  set_icon_entry ( mew, icon );
}

/* fixme? this is a touch ineffecient, doing all this
   every time the entry changes. */

static void icon_changed_callback(GtkEntry * e, MenuEntryWiz * mew)
{
  gchar * text = gtk_entry_get_text(e);

  set_icon( mew, text );

  if ( g_file_exists (text) &&
       is_image_file (text) ) {
    set_icon_pixmap ( mew, text );
  }
  else {
    set_icon_pixmap( mew, 0 );
  }
}

static void name_changed_callback(GtkEntry * e, MenuEntryWiz * mew)
{
  set_name( mew, gtk_entry_get_text(e) );
}

static void command_changed_callback(GtkEntry * e, MenuEntryWiz * mew)
{
  set_command( mew, gtk_entry_get_text(e) );
}

static void test_command(MenuEntryWiz * mew)
{
  if (mew->command == NULL) {
    return;
  }
  
  run_command(mew->command);
}




