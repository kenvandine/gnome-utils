/* -*- Mode: C -*-
 * $Id$
 *
 * GDiskFree -- A disk free space toy (df on steriods).
 * Copyright 1998,1999 Gregory McLean
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include "gdiskfree_options.h"

extern GDiskFreeOptions    *current_options;
/* working holds a copy of the un-applied config. */
static GDiskFreeOptions    *working;
/****************************************************************************
 * Forward references 
 **/
static gint     gdiskfree_option_dialog_close       (GtkWidget        *widget,
						     GdkEventAny      *e,
						     GDiskFreeOptions *opt);
static void     gdiskfree_option_dialog_apply       (GnomePropertyBox *box,
						     gint             page_num,
						     GDiskFreeOptions *opt);
static void     gdiskfree_sync_option_changed       (GtkWidget        *widget,
						     GnomePropertyBox *box);
static void     gdiskfree_orientation_callback      (GtkWidget        *widget,
						     GnomePropertyBox *box);
static void     gdiskfree_update_interval_changed   (GtkWidget        *widget,
						     GnomePropertyBox *box);

/****************************************************************************
 * Callback functions
 **/
static gint
gdiskfree_option_dialog_close (GtkWidget *widget, GdkEventAny *e, 
			       GDiskFreeOptions *opt)
{
  g_free (opt);
  return FALSE;
}
static void
gdiskfree_option_dialog_apply (GnomePropertyBox *box, gint page_num,
			       GDiskFreeOptions *opt)
{
  /* Copy the working options to the current options */
  current_options->sync_required = opt->sync_required;
  if (current_options->orientation != opt->orientation)
    {
      GDiskFreeApp *app;
      /* Orientation changed re-do the app window */
      current_options->orientation   = opt->orientation;
      app = gtk_object_get_data (GTK_OBJECT (box), "app");
      if (app)
	gdiskfree_app_change_orient (app, current_options->orientation);
    }
  current_options->update_interval = opt->update_interval;
  /* Save the options -- Structure free'd on window destruction.*/
  gdiskfree_option_save ();
}
static void
gdiskfree_sync_option_changed (GtkWidget *widget, GnomePropertyBox *box)
{
  working->sync_required = GTK_TOGGLE_BUTTON (widget)->active;
  gnome_property_box_changed (GNOME_PROPERTY_BOX (box));
}
static void
gdiskfree_orientation_callback (GtkWidget *widget, GnomePropertyBox *box)
{
  working->orientation = GPOINTER_TO_INT(gtk_object_get_data (GTK_OBJECT (widget), "value"));
  if (GTK_WIDGET_MAPPED (box))
    {
      /* Only call this when the prop box is visible. */
      gnome_property_box_changed (GNOME_PROPERTY_BOX (box));
    }
}
static void
gdiskfree_update_interval_changed (GtkWidget *widget, GnomePropertyBox *box)
{
  GtkAdjustment  *adjustment;

  adjustment = GTK_ADJUSTMENT (widget);
  working->update_interval = adjustment->value;
  gnome_property_box_changed (GNOME_PROPERTY_BOX (box));
}
/****************************************************************************
 * Implementation 
 **/

/**
 * gdiskfree_option_init:
 **/
void
gdiskfree_option_init ()
{
  if (!current_options)
    current_options = g_malloc (sizeof (GDiskFreeOptions));
  current_options->sync_required = gnome_config_get_bool_with_default
    ("/GDiskFree/properties/sync_required=FALSE", NULL);
  current_options->update_interval = gnome_config_get_int_with_default
    ("/GDiskFree/properties/update_interval=1000", NULL);
  current_options->orientation = gnome_config_get_int_with_default 
    ("/GDiskFree/properties/orientation=GTK_ORIENTATION_VERTICAL", NULL);
}
/**
 * gdiskfree_option_save:
 **/
void
gdiskfree_option_save ()
{
  gnome_config_set_bool("/GDiskFree/properties/sync_required",
			current_options->sync_required);
  gnome_config_set_int ("/GDiskFree/properties/update_interval",
			current_options->update_interval);
  gnome_config_set_int ("/GDiskFree/properties/orientation",
			current_options->orientation);
  gnome_config_sync ();
}
/**
 * gdiskfree_option_dialog:
 **/
GtkWidget *
gdiskfree_option_dialog (GDiskFreeApp *app)
{
  GtkWidget    *propbox;
  GtkWidget    *label;
  GtkWidget    *box;
  GtkWidget    *checkbox;
  GtkObject    *udp_adjust;
  GSList       *orientation_group = NULL;
  
  working = g_malloc (sizeof (GDiskFreeOptions));
  propbox = gnome_property_box_new ();
  gtk_object_set_data (GTK_OBJECT (propbox), "app", app);
  /*
   * The gnome_property_box should have an accessor function for this!!
   */
  gtk_window_set_title (GTK_WINDOW 
			(&GNOME_PROPERTY_BOX(propbox)->dialog.window), 
			_("GDiskFree Properties"));
  box = gtk_table_new (3, 3, FALSE);
  /* General settings for GDiskFree */

  checkbox = gtk_check_button_new_with_label 
    (_("Invoke sync before getting usage info"));
  working->sync_required = current_options->sync_required;
  if (current_options->sync_required)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
  gtk_signal_connect (GTK_OBJECT (checkbox), "toggled",
		      (GtkSignalFunc) gdiskfree_sync_option_changed,
		      propbox);
  gtk_table_attach (GTK_TABLE (box), checkbox, 0, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2, 2);
  working->orientation = current_options->orientation;
  label = gtk_label_new (_("Dial Orientation"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (box), label, 0, 1, 1, 2, 
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  checkbox = gtk_radio_button_new_with_label (orientation_group, 
					      _("Vertical"));
  if (working->orientation == GTK_ORIENTATION_VERTICAL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
  gtk_signal_connect (GTK_OBJECT (checkbox), "clicked",
		      (GtkSignalFunc) gdiskfree_orientation_callback,
		      propbox);
  gtk_object_set_data (GTK_OBJECT (checkbox), "value",
		       GINT_TO_POINTER (GTK_ORIENTATION_VERTICAL));
  
  orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (checkbox));
  gtk_table_attach (GTK_TABLE (box), checkbox, 1, 2, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 3, 0);
  checkbox = gtk_radio_button_new_with_label (orientation_group,
					      _("Horizontal"));
  if (working->orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
  gtk_signal_connect (GTK_OBJECT (checkbox), "clicked",
		      (GtkSignalFunc) gdiskfree_orientation_callback,
		      propbox);
  gtk_object_set_data (GTK_OBJECT (checkbox), "value",
		       GINT_TO_POINTER (GTK_ORIENTATION_HORIZONTAL));
  orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (checkbox));
  gtk_table_attach (GTK_TABLE (box), checkbox, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 3, 0);
  working->update_interval = current_options->update_interval;
  udp_adjust = gtk_adjustment_new ((gfloat)working->update_interval, 
				   1, 20000, 1, 10, 0);
  gtk_signal_connect (GTK_OBJECT (udp_adjust), "value_changed",
		      (GtkSignalFunc) gdiskfree_update_interval_changed,
		      propbox);
  label = gtk_label_new (_("Update interval (seconds)"));
  gtk_table_attach (GTK_TABLE (box), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 3, 0);
  checkbox = gtk_hscale_new (GTK_ADJUSTMENT (udp_adjust));  
  gtk_range_set_update_policy (GTK_RANGE (checkbox), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_digits (GTK_SCALE (checkbox), 0);
  gtk_table_attach (GTK_TABLE (box), checkbox, 1, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL | GTK_EXPAND, GTK_SHRINK, 3, 3);

  gtk_widget_show_all (box);

  label = gtk_label_new (_("General Settings"));
  gnome_property_box_append_page (GNOME_PROPERTY_BOX (propbox), box, label);

  gtk_signal_connect (GTK_OBJECT (propbox), "delete_event",
		      (GtkSignalFunc) gdiskfree_option_dialog_close,
		      working);
  gtk_signal_connect (GTK_OBJECT (propbox), "apply",
		      (GtkSignalFunc) gdiskfree_option_dialog_apply,
		      working);
  return propbox;
}
