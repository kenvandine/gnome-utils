/*
 *  Gnome Character Map
 *  interface.h - The main window
 *
 *  Copyright (C) Hongli Lai
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

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gnome.h>
#include <bonobo/bonobo-dock-layout.h>


#define MAIN_APP_TYPE             (main_app_get_type ())
#define MAIN_APP(obj)             G_TYPE_CHECK_INSTANCE_CAST (obj, MAIN_APP_TYPE, MainApp)
#define MAIN_APP_CLASS(klass)     G_TYPE_CHECK_CLASS_CAST ((klass), MAIN_APP_TYPE, MainAppClass)
#define MAIN_IS_APP(obj)          G_TYPE_CHECK_INSTANCE_TYPE (obj, MAIN_APP_TYPE)
#define MAIN_IS_APP_CLASS(klass)  G_TYPE_CHECK_CLASS_TYPE ((klass), MAIN_APP_TYPE)

typedef struct _MainApp
{
    GObject parent_struct;
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *textbar;
    GtkWidget *preview_label;
    GtkWidget *fontpicker;
    GtkWidget *chartable;
    GList *buttons;
    GtkStyle *btnstyle;
    GConfClient *conf;
} MainApp;

typedef struct _MainAppClass
{
    GObjectClass parent_klass;
} MainAppClass;


extern MainApp *mainapp;

GType main_app_get_type (void);
MainApp *main_app_new (void);
void main_app_destroy (MainApp *obj);

gboolean check_gail(GtkWidget *widget);
void add_atk_namedesc(GtkWidget *widget, const gchar *name, const gchar *desc);
void add_atk_relation(GtkWidget *obj1, GtkWidget *obj2, AtkRelationType type);
void edit_menu_set_sensitivity (gboolean flag);

#endif /* _INTERFACE_H_ */
