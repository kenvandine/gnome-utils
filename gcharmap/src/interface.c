/*
 *  Gnome Character Map
 *  interface.c - The main window
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

#ifndef _INTERFACE_C_
#define _INTERFACE_C_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <interface.h>
#include <menus.h>
#include <callbacks.h>
MainApp *mainapp;

void
edit_menu_set_sensitivity (gboolean flag)
{
    static gboolean sensitivity = TRUE;
    gint i, items[4] = {0, 1, 4, 6};
    
    if (! (sensitivity ^ flag))
	return;
    for (i=0; i < 4; i++) {
       sensitivity = flag;
       gtk_widget_set_sensitive (GTK_WIDGET (edit_menu[items[i]].widget), flag);
    }
}

static GtkWidget *
create_button (const gchar *label, GtkSignalFunc func)
{
    GtkWidget *button;

    button = gtk_button_new_with_label (label);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    if (func != NULL)
        g_signal_connect (G_OBJECT (button), "clicked",
          func, NULL);
    gtk_widget_show (button);
    return button;
}


static GtkWidget *
create_chartable (void)
{
    GtkWidget *chartable, *button;
    gint v, h;

    chartable = gtk_table_new (8, 24, TRUE);

    for (v = 0; v <= 3; v++)
    {
        for (h = 0; h <= 23; h++)
        {
	    char buf[7];
            int n;
            int ch = v * 24 + h + 32;
	   
	    n = g_unichar_to_utf8 (ch, buf);
            buf[n] = 0;
            
            button = gtk_button_new_with_label (buf);
            mainapp->buttons = g_list_append (mainapp->buttons, button);
            gtk_table_attach (GTK_TABLE (chartable), button, h, h + 1, v, v + 1,
              (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
              (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
              0, 0);

            g_signal_connect (G_OBJECT (button), "clicked",
              G_CALLBACK (cb_charbtn_click), NULL);
            g_signal_connect (G_OBJECT (button), "enter",
              G_CALLBACK (cb_charbtn_enter), NULL);
            g_signal_connect (G_OBJECT (button), "leave",
              G_CALLBACK (cb_charbtn_leave), NULL);


            g_signal_connect (G_OBJECT (button), "focus_in_event",
              GTK_SIGNAL_FUNC (cb_charbtn_enter), NULL);
            g_signal_connect (G_OBJECT (button), "focus_out_event",
              GTK_SIGNAL_FUNC (cb_charbtn_leave), NULL);


        }
    }

    for (v = 0; v <= 3; v++)
    {
        for (h = 0; h <= 23; h++)
        {
            char buf[7];
            int n;
	    int ch = v * 24 + h + 161;
	    
	    if (ch > 0xff)
		    continue;

            n = g_unichar_to_utf8 (ch, buf);
            buf[n] = 0;
	    
            button = gtk_button_new_with_label (buf);
            mainapp->buttons = g_list_append (mainapp->buttons, button);
            gtk_table_attach (GTK_TABLE (chartable), button,
              h, h + 1, v + 4, v + 5,
              (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
              (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
              0, 0);

            g_signal_connect (G_OBJECT (button), "clicked",
              G_CALLBACK (cb_charbtn_click), NULL);
            g_signal_connect (G_OBJECT (button), "enter",
              G_CALLBACK (cb_charbtn_enter), NULL);
            g_signal_connect (G_OBJECT (button), "leave",
              G_CALLBACK (cb_charbtn_leave), NULL);


            g_signal_connect (G_OBJECT (button), "focus_in_event",
              GTK_SIGNAL_FUNC (cb_charbtn_enter), NULL);
            g_signal_connect (G_OBJECT (button), "focus_out_event",
              GTK_SIGNAL_FUNC (cb_charbtn_leave), NULL);


        }
    }

    gtk_widget_show_all (chartable);
    return chartable;
}

/* Check if gail is loaded */
gboolean
check_gail(GtkWidget *widget)
{
   return GTK_IS_ACCESSIBLE(gtk_widget_get_accessible(widget));
}


/* Add AtkName and AtkDescription */
void
add_atk_namedesc(GtkWidget *widget, const gchar *name, const gchar *desc)
{
   AtkObject *atk_widget;
   atk_widget = gtk_widget_get_accessible(widget);
   atk_object_set_name(atk_widget, name);
   atk_object_set_description(atk_widget, desc);
}


/* Add AtkRelation */
void
add_atk_relation(GtkWidget *obj1, GtkWidget *obj2, AtkRelationType type)
{

    AtkObject *atk_obj1, *atk_obj2;
    AtkRelationSet *relation_set;
    AtkRelation *relation;

    atk_obj1 = gtk_widget_get_accessible(obj1);

    atk_obj2 = gtk_widget_get_accessible(obj2);

    relation_set = atk_object_ref_relation_set (atk_obj1);
    relation = atk_relation_new(&atk_obj2, 1, type);
    atk_relation_set_add(relation_set, relation);
    g_object_unref(G_OBJECT (relation));

}


static void
main_app_create_ui (MainApp *app)
{
    GtkWidget *appbar;
    GtkWidget *vbox, *hbox, *hbox2, *hbox3, *vbox2;
    GtkWidget *vsep, *alabel, *label;
    GtkWidget *chartable;
    GtkWidget *buttonbox, *button;
    GtkWidget *align;
    GtkWidget *viewport;
    GtkWidget *image;


    /* Main window */
    {
        BonoboDockLayoutItem *item;

        app->window = gnome_app_new (_(PACKAGE), _("Gnome Character Map"));
        gtk_widget_set_name (app->window, "mainapp");
        
        g_signal_connect_swapped (G_OBJECT (app->window), "destroy",
          G_CALLBACK (main_app_destroy), G_OBJECT (app));
        gtk_widget_realize (app->window);
	
        appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
        gnome_app_set_statusbar (GNOME_APP (app->window), appbar);
        gtk_widget_show (appbar);

        gnome_app_create_menus_with_data (GNOME_APP (app->window), menubar, app->window);
	edit_menu_set_sensitivity (FALSE);

        gnome_app_install_menu_hints (GNOME_APP (app->window), menubar);

        item = g_list_nth_data (GNOME_APP (app->window)->layout->items, 0);
    }

    /* The toplevel vbox */
    vbox = gtk_vbox_new (FALSE, 8);
    gnome_app_set_contents (GNOME_APP (app->window), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (GNOME_APP (
      app->window)->contents), 8);

    {
        BonoboDockLayoutItem *item;

        hbox = gtk_hbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
        gnome_app_add_docked (GNOME_APP (app->window), hbox, _("Action Toolbar"),
          BONOBO_DOCK_ITEM_BEH_EXCLUSIVE | BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL,
          BONOBO_DOCK_TOP, 2, 0, 1);

        alabel = gtk_label_new_with_mnemonic (_("_Text to copy:"));
        gtk_misc_set_padding (GTK_MISC (alabel), GNOME_PAD_SMALL, -1);
        gtk_box_pack_start (GTK_BOX (hbox), alabel, FALSE, TRUE, 0);

        app->entry = gtk_entry_new ();
        gtk_label_set_mnemonic_widget (GTK_LABEL (alabel), app->entry);
        gtk_box_pack_start (GTK_BOX (hbox), app->entry, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (GTK_EDITABLE (app->entry)), "changed",
			  G_CALLBACK (cb_entry_changed), NULL);
        
	button = gtk_button_new ();
	if (GTK_BIN (button)->child)
                gtk_container_remove (GTK_CONTAINER (button),
                                      GTK_BIN (button)->child);

	label = gtk_label_new_with_mnemonic (_("_Copy"));
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

        image = gtk_image_new_from_stock (GTK_STOCK_COPY, GTK_ICON_SIZE_BUTTON);
        hbox3 = gtk_hbox_new (FALSE, 2);

        align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

        gtk_box_pack_start (GTK_BOX (hbox3), image, FALSE, FALSE, 0);
        gtk_box_pack_end (GTK_BOX (hbox3), label, FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (button), align);
        gtk_container_add (GTK_CONTAINER (align), hbox3);
	
        gtk_widget_show_all (align);

        gtk_container_set_border_width (GTK_CONTAINER (button), 2);
        g_signal_connect (G_OBJECT (button), "clicked",
        		  G_CALLBACK (cb_copy_click), NULL);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);		 
        if (check_gail(app->entry))
        {
          add_atk_namedesc(GTK_WIDGET(alabel), _("Text to copy"), _("Text to copy"));
          add_atk_namedesc(app->entry, _("Entry"), _("Text to copy"));
          add_atk_namedesc(button, _("Copy"), _("Copy the text"));
          add_atk_relation(app->entry, GTK_WIDGET(alabel), ATK_RELATION_LABELLED_BY);
        }
 
        gtk_widget_show_all (hbox);
        item = g_list_nth_data (GNOME_APP (app->window)->layout->items, 0);
        app->textbar = GTK_WIDGET (item->item);
        gtk_container_set_border_width (GTK_CONTAINER (app->textbar), 1);
    }

    hbox2 = gtk_hbox_new (FALSE, 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);

    /* The character table */
    {
        chartable = create_chartable ();
        gtk_box_pack_start (GTK_BOX (hbox2), chartable, TRUE, TRUE, 0);
        app->chartable = chartable;
    }

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), vbox2, FALSE, TRUE, 0);

    gtk_widget_show_all (vbox);
    gtk_widget_grab_focus (app->entry);
    
}


static void
main_app_init (MainApp *obj)
{
    mainapp = obj;
    main_app_create_ui (obj);
}

static void
main_app_class_init (MainAppClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    
}

GType
main_app_get_type (void)
{
    static GType ga_type = 0;

    g_type_init ();
    
    if (!ga_type) {
        static const GTypeInfo ga_info = {
          sizeof (MainAppClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) main_app_class_init,
          NULL,
          NULL,
          sizeof (MainApp),
          0,
          (GInstanceInitFunc) main_app_init,
        };
        ga_type = g_type_register_static (G_TYPE_OBJECT, "MainApp",
        				  &ga_info, 0);
    }
    return ga_type;
}


MainApp *
main_app_new (void)
{
    return MAIN_APP (g_object_new (MAIN_APP_TYPE, NULL));
}


void
main_app_destroy (MainApp *obj)
{
    g_return_if_fail (obj != NULL);
    g_return_if_fail (MAIN_IS_APP (obj) == TRUE);

    gtk_main_quit ();
}


#endif /* _INTERFACE_C_ */
