/*
 *  Gnome Character Map
 *  notifiers.c Notifiers when the GConf preferences change
 *
 *  Copyright (C) Christophe Fergeau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include <gnome.h>

#include "interface.h"
#include "menus.h"

static gint id_set_insert_at_end;
static gint id_toggle_actionbar;
static gint id_toggle_statusbar;
static gint id_toggle_textbar;
static gint id_set_buttons_focusable;

static void notifier_set_insert_at_end (GConfClient *client,
					guint cnxn_id,
					GConfEntry *entry,
					gpointer data);
static void notifier_toggle_actionbar (GConfClient *client,
				       guint cnxn_id,
				       GConfEntry *entry,
				       gpointer data);
static void notifier_toggle_statusbar (GConfClient *client,
				       guint cnxn_id,
				       GConfEntry *entry,
				       gpointer data);
static void notifier_toggle_textbar (GConfClient *client,
				     guint cnxn_id,
				     GConfEntry *entry,
				     gpointer data);
static void notifier_set_button_focusable (GConfClient *client,
					   guint cnxn_id,
					   GConfEntry *entry,
					   gpointer data);
static void set_button_focusable(gboolean value);
static void toggle_actionbar(gboolean value);
static void toggle_textbar(gboolean value);
static void toggle_statusbar(gboolean value);
static void set_insert_at_end(gboolean value);

static void
deinstall_notifiers (void)
{
    if (mainapp->conf == NULL) {
	return;
    }
    
    gconf_client_notify_remove(mainapp->conf, id_set_insert_at_end);
    gconf_client_notify_remove(mainapp->conf, id_set_buttons_focusable);
    gconf_client_notify_remove(mainapp->conf, id_toggle_statusbar);
    gconf_client_notify_remove(mainapp->conf, id_toggle_textbar);
    gconf_client_notify_remove(mainapp->conf, id_toggle_actionbar);
    g_object_unref (G_OBJECT (mainapp->conf));
    mainapp->conf = NULL;
}



void 
install_notifiers ()
{
    GConfClient *client;
    GError *err;

    client = gconf_client_get_default ();

    err = NULL;
    gconf_client_add_dir (client, "/apps/gcharmap",
			  GCONF_CLIENT_PRELOAD_ONELEVEL,
			  &err);
    if (err)
    {
	g_printerr (_("There was an error loading config from %s. (%s)\n"),
		    "/apps/gcharmap", err->message);
	g_error_free (err);
    }
    
    
    err = NULL;
    id_toggle_statusbar =
	gconf_client_notify_add (client,
				 "/apps/gcharmap/show_statusbar",
				 (GConfClientNotifyFunc)notifier_toggle_statusbar,
				 NULL, NULL, &err);    
    if (err)
    {
	g_printerr (_("There was an error subscribing to notification of statusbar visibility changes. (%s)\n"),
		    err->message);
	g_error_free (err);
    }  

    err = NULL;
    id_toggle_textbar = 
	gconf_client_notify_add (client,
				 "/apps/gcharmap/show_textbar",
				 (GConfClientNotifyFunc)notifier_toggle_textbar,
				 NULL, NULL, &err);    
    if (err)
    {
	g_printerr (_("There was an error subscribing to notification of textbar visibility changes. (%s)\n"),
		    err->message);
	g_error_free (err);
    }  

    err = NULL;
    id_toggle_actionbar = 
	gconf_client_notify_add (client,
				 "/apps/gcharmap/show_actionbar",
				 (GConfClientNotifyFunc)notifier_toggle_actionbar,
				 NULL, NULL, &err);    
    if (err)
    {
	g_printerr (_("There was an error subscribing to notification of actionbar visibility changes. (%s)\n"),
		    err->message);
	g_error_free (err);
    }  

    err = NULL;
    id_set_insert_at_end = 
	gconf_client_notify_add (client,
				 "/apps/gcharmap/insert_at_end",
				 (GConfClientNotifyFunc)notifier_set_insert_at_end,
				 NULL, NULL, &err);
    if (err)
    {
	g_printerr (_("There was an error subscribing to notification of insertion behaviour changes. (%s)\n"),
		    err->message);
	g_error_free (err);
    }  

    err = NULL;
    id_set_buttons_focusable = 
	gconf_client_notify_add (client,
				 "/apps/gcharmap/can_focus_buttons",
				 (GConfClientNotifyFunc)notifier_set_button_focusable,
				 NULL, NULL, &err);    
    if (err)
    {
	g_printerr (_("There was an error subscribing to notification of button focusability changes. (%s)\n"),
		    err->message);
	g_error_free (err);
    }  

    mainapp->conf = client;
    g_atexit (deinstall_notifiers);

}


void init_prefs()
{
    gboolean value;
    
       
    value = gconf_client_get_bool(mainapp->conf, 
				  "/apps/gcharmap/insert_at_end", 
				  NULL);
    set_insert_at_end(value);


    value = gconf_client_get_bool(mainapp->conf, 
				  "/apps/gcharmap/show_statusbar",
				  NULL);
    toggle_statusbar(value);


    value = gconf_client_get_bool(mainapp->conf, 
				  "/apps/gcharmap/show_actionbar",
				  NULL);
    toggle_actionbar(value);

    
    value = gconf_client_get_bool(mainapp->conf, 
				  "/apps/gcharmap/show_textbar",
				  NULL);
    toggle_textbar(value);

    value = gconf_client_get_bool(mainapp->conf, 
				  "/apps/gcharmap/can_focus_buttons",
				  NULL);
    set_button_focusable(value);


}


static void
notifier_set_insert_at_end (GConfClient *client,
			    guint cnxn_id,
			    GConfEntry *entry,
			    gpointer data)
{
    set_insert_at_end(gconf_value_get_bool(gconf_entry_get_value(entry)));
}


static void
notifier_toggle_actionbar (GConfClient *client,
			   guint cnxn_id,
			   GConfEntry *entry,
			   gpointer data)
{
    toggle_actionbar(gconf_value_get_bool(gconf_entry_get_value(entry)));
}


static void
notifier_toggle_textbar (GConfClient *client,
			 guint cnxn_id,
			 GConfEntry *entry,
			 gpointer data)
{
    toggle_textbar(gconf_value_get_bool(gconf_entry_get_value(entry)));
}


static void
notifier_toggle_statusbar (GConfClient *client,
			   guint cnxn_id,
			   GConfEntry *entry,
			   gpointer data)
{
    toggle_statusbar(gconf_value_get_bool(gconf_entry_get_value(entry)));
}


static void
notifier_set_button_focusable (GConfClient *client,
			       guint cnxn_id,
			       GConfEntry *entry,
			       gpointer data)
{
    set_button_focusable(gconf_value_get_bool(gconf_entry_get_value(entry)));
}


static void 
set_button_focusable(gboolean value)
{
    guint i;


    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (
					settings_menu[1].widget),
				    value);
    if (value)
    {
        for (i = 0; i < g_list_length (mainapp->buttons); i++)
            GTK_WIDGET_SET_FLAGS (GTK_WIDGET (g_list_nth_data (
              mainapp->buttons, i)), GTK_CAN_FOCUS);
    } else
    {
        for (i = 0; i < g_list_length (mainapp->buttons); i++)
            GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (g_list_nth_data (
              mainapp->buttons, i)), GTK_CAN_FOCUS);
    }
}


static void 
set_insert_at_end(gboolean value) 
{
    mainapp->insert_at_end = value;
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (
					settings_menu[0].widget),
				    value);
}


static void 
toggle_statusbar(gboolean value)
{
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view_menu[1].widget),
				    value);
    if (value == TRUE)
        gtk_widget_show (GNOME_APP (mainapp->window)->statusbar);
    else
        gtk_widget_hide (GNOME_APP (mainapp->window)->statusbar);

}


static void 
toggle_actionbar(gboolean value)
{
    if (value == TRUE)
        gtk_widget_show (mainapp->actionbar);
    else
        gtk_widget_hide (mainapp->actionbar);
}


static void 
toggle_textbar(gboolean value)
{
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view_menu[0].widget),
				    value);
    if (value == TRUE) {
        gtk_widget_show (mainapp->textbar);
    } else {
        gtk_widget_hide (mainapp->textbar);
    }
}
