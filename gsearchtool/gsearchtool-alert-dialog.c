/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gsearchtool-alert-dialog.c: An HIG compliant alert dialog.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

*/

#include "gsearchtool-alert-dialog.h"
#include <bonobo/bonobo-i18n.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkiconfactory.h>
#include <string.h>

enum {
  PROP_0,
  PROP_ALERT_TYPE,
  PROP_BUTTONS
};

static gpointer parent_class;

static void gsearch_alert_dialog_class_init   (GsearchAlertDialogClass *klass);
static void gsearch_alert_dialog_init         (GsearchAlertDialog      *dialog);
static void gsearch_alert_dialog_style_set    (GtkWidget           *widget,
                                               GtkStyle            *prev_style);
static void gsearch_alert_dialog_set_property (GObject          *object,
					       guint             prop_id,
					       const GValue     *value,
					       GParamSpec       *pspec);
static void gsearch_alert_dialog_get_property (GObject          *object,
					       guint             prop_id,
					       GValue           *value,
					       GParamSpec       *pspec);
static void gsearch_alert_dialog_add_buttons  (GsearchAlertDialog   *alert_dialog,
					       GtkButtonsType    buttons);

GType
gsearch_alert_dialog_get_type (void)
{
	static GType dialog_type = 0;

	if (!dialog_type) {
	
		static const GTypeInfo dialog_info =
		{
			sizeof (GsearchAlertDialogClass),
			NULL,
			NULL,
			(GClassInitFunc) gsearch_alert_dialog_class_init,
			NULL,
			NULL,
			sizeof (GsearchAlertDialog),
			0,
			(GInstanceInitFunc) gsearch_alert_dialog_init,
		};
	
		dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "GsearchAlertDialog",
	                                              &dialog_info, 0);
	}
	return dialog_type;
}

static void
gsearch_alert_dialog_class_init (GsearchAlertDialogClass *class)
{
	GtkWidgetClass *widget_class;
	GObjectClass   *gobject_class;

	widget_class = GTK_WIDGET_CLASS (class);
	gobject_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
  
	widget_class->style_set = gsearch_alert_dialog_style_set;

	gobject_class->set_property = gsearch_alert_dialog_set_property;
	gobject_class->get_property = gsearch_alert_dialog_get_property;
  
	gtk_widget_class_install_style_property (widget_class,
	                                         g_param_spec_int ("alert_border",
	                                         _("Image/label border"),
	                                         _("Width of border around the label and image in the alert dialog"),
	                                         0,
	                                         G_MAXINT,
	                                         5,
	                                         G_PARAM_READABLE));
  
	g_object_class_install_property (gobject_class,
	                                 PROP_ALERT_TYPE,
	                                 g_param_spec_enum ("alert_type",
	                                 _("Alert Type"),
	                                 _("The type of alert"),
	                                 GTK_TYPE_MESSAGE_TYPE,
	                                 GTK_MESSAGE_INFO,
	                                 G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
  
	g_object_class_install_property (gobject_class,
	                                 PROP_BUTTONS,
	                                 g_param_spec_enum ("buttons",
	                                 _("Alert Buttons"),
	                                 _("The buttons shown in the alert dialog"),
	                                 GTK_TYPE_BUTTONS_TYPE,
	                                 GTK_BUTTONS_NONE,
	                                 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gsearch_alert_dialog_init (GsearchAlertDialog *dialog)
{
	GtkWidget *hbox;
	GtkWidget *vbox;

	dialog->primary_label = gtk_label_new (NULL);
	dialog->secondary_label = gtk_label_new (NULL);
	dialog->image = gtk_image_new_from_stock (NULL, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (dialog->image), 0.5, 0.0);

	gtk_label_set_line_wrap (GTK_LABEL (dialog->primary_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (dialog->primary_label), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (dialog->primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (dialog->primary_label), 0.0, 0.5);

	gtk_label_set_line_wrap (GTK_LABEL (dialog->secondary_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (dialog->secondary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (dialog->secondary_label), 0.0, 0.5);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

	gtk_box_pack_start (GTK_BOX (hbox), dialog->image,
	                    FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 12);

	gtk_box_pack_start (GTK_BOX (hbox), vbox,
	                    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), dialog->primary_label,
	                    FALSE, FALSE, 0);
		      
	gtk_box_pack_start (GTK_BOX (vbox), dialog->secondary_label,
	                    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, 
	                    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);
}

static GtkMessageType
gsearch_alert_dialog_get_alert_type (GsearchAlertDialog *dialog)
{
	const gchar* stock_id = NULL;

	g_return_val_if_fail (GTK_IS_MESSAGE_DIALOG (dialog), GTK_MESSAGE_INFO);
	g_return_val_if_fail (GTK_IS_IMAGE (dialog->image), GTK_MESSAGE_INFO);

	stock_id = GTK_IMAGE(dialog->image)->data.stock.stock_id;

	if (strcmp (stock_id, GTK_STOCK_DIALOG_INFO) == 0) {
		return GTK_MESSAGE_INFO;
	}
	else if (strcmp (stock_id, GTK_STOCK_DIALOG_QUESTION) == 0) {
		return GTK_MESSAGE_QUESTION;
	}
	else if (strcmp (stock_id, GTK_STOCK_DIALOG_WARNING) == 0) {
		return GTK_MESSAGE_WARNING;
	}
	else if (strcmp (stock_id, GTK_STOCK_DIALOG_ERROR) == 0) {
		return GTK_MESSAGE_ERROR;
	}
	else
	{
		g_assert_not_reached (); 
		return GTK_MESSAGE_INFO;
	}
}

static void
setup_type (GsearchAlertDialog *dialog,
	    GtkMessageType  type)
{
	const gchar *stock_id = NULL;
	GtkStockItem item;
  
	switch (type) {
		case GTK_MESSAGE_INFO:
			stock_id = GTK_STOCK_DIALOG_INFO;
			break;
		case GTK_MESSAGE_QUESTION:
			stock_id = GTK_STOCK_DIALOG_QUESTION;
			break;
		case GTK_MESSAGE_WARNING:
			stock_id = GTK_STOCK_DIALOG_WARNING;
			break;
		case GTK_MESSAGE_ERROR:
			stock_id = GTK_STOCK_DIALOG_ERROR;
			break;
		default:
			g_warning ("Unknown GtkMessageType %d", type);
			break;
	}

	if (stock_id == NULL) {
		stock_id = GTK_STOCK_DIALOG_INFO;
  	}
	
	if (gtk_stock_lookup (stock_id, &item)) {
		gtk_image_set_from_stock (GTK_IMAGE (dialog->image), stock_id,
		                          GTK_ICON_SIZE_DIALOG);
	}
	else {
		g_warning ("Stock dialog ID doesn't exist?");
	}
}

static void 
gsearch_alert_dialog_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
	GsearchAlertDialog *dialog;
  
	dialog = GSEARCH_ALERT_DIALOG (object);
  
	switch (prop_id) {
		case PROP_ALERT_TYPE:
			setup_type (dialog, g_value_get_enum (value));
			break;
		case PROP_BUTTONS:
			gsearch_alert_dialog_add_buttons (dialog, g_value_get_enum (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void 
gsearch_alert_dialog_get_property (GObject     *object,
                                   guint        prop_id,
                                   GValue      *value,
                                   GParamSpec  *pspec)
{
	GsearchAlertDialog *dialog;
  
	dialog = GSEARCH_ALERT_DIALOG (object);
  
	switch (prop_id) {
		case PROP_ALERT_TYPE:
			g_value_set_enum (value, gsearch_alert_dialog_get_alert_type (dialog));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

void
gsearch_alert_dialog_set_primary_label (GsearchAlertDialog *dialog,
                                        const gchar *message)
{
	gchar *markup_str;
	
	if (message != NULL) {
		markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", message, "</span>", NULL);
		gtk_label_set_markup (GTK_LABEL (GSEARCH_ALERT_DIALOG (dialog)->primary_label),
		                      markup_str);
		g_free (markup_str);
	}
}

void
gsearch_alert_dialog_set_secondary_label (GsearchAlertDialog *dialog,
                                      const gchar *message)
{
	if (message != NULL) {
		gtk_label_set_text (GTK_LABEL (GSEARCH_ALERT_DIALOG (dialog)->secondary_label),
		                    message);
	}
}

GtkWidget*
gsearch_alert_dialog_new (GtkWindow     *parent,
                          GtkDialogFlags flags,
                          GtkMessageType type,
                          GtkButtonsType buttons,
                          const gchar   *primary_message,
                          const gchar   *secondary_message,
		          const gchar   *title)
{
	GtkWidget *widget;
	GtkDialog *dialog;

	g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

	widget = g_object_new (GSEARCH_TYPE_ALERT_DIALOG,
	                       "alert_type", type,
	                       "buttons", buttons,
	                       NULL);

	dialog = GTK_DIALOG (widget);
	
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);		
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_dialog_set_has_separator (dialog, FALSE);
  
	gtk_window_set_title (GTK_WINDOW (dialog),
	                      (title != NULL) ? title : "");
	
	gsearch_alert_dialog_set_primary_label (GSEARCH_ALERT_DIALOG (dialog),
	                                        primary_message);
		       
	gsearch_alert_dialog_set_secondary_label (GSEARCH_ALERT_DIALOG (dialog),
	                                          secondary_message);

	if (parent != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (widget),
		                              GTK_WINDOW (parent));
	}

	if (flags & GTK_DIALOG_MODAL) {
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	}

	if (flags & GTK_DIALOG_DESTROY_WITH_PARENT) {
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	}
	return widget;
}

static void
gsearch_alert_dialog_add_buttons (GsearchAlertDialog* alert_dialog,
                                  GtkButtonsType  buttons)
{
	GtkDialog* dialog;
	
	dialog = GTK_DIALOG (alert_dialog);

	switch (buttons) {
		case GTK_BUTTONS_NONE:
			break;
		case GTK_BUTTONS_OK:
			gtk_dialog_add_button (dialog,
		                               GTK_STOCK_OK,
			                       GTK_RESPONSE_OK);
			gtk_dialog_set_default_response (dialog, 
			                                GTK_RESPONSE_OK);
			break;
		case GTK_BUTTONS_CLOSE:
			gtk_dialog_add_button (dialog,
		                               GTK_STOCK_CLOSE,
			                      GTK_RESPONSE_CLOSE);
			gtk_dialog_set_default_response (dialog, 
			                                 GTK_RESPONSE_CLOSE);
			break;
		case GTK_BUTTONS_CANCEL:
			gtk_dialog_add_button (dialog,
			                       GTK_STOCK_CANCEL,
			                       GTK_RESPONSE_CANCEL);
			gtk_dialog_set_default_response (dialog, 
			                                 GTK_RESPONSE_CANCEL);
			break;
		case GTK_BUTTONS_YES_NO:
			gtk_dialog_add_button (dialog,
			                       GTK_STOCK_NO,
			                       GTK_RESPONSE_NO);
			gtk_dialog_add_button (dialog,
			                       GTK_STOCK_YES,
			                       GTK_RESPONSE_YES);
			gtk_dialog_set_default_response (dialog, 
			                                 GTK_RESPONSE_YES);
			break;
		case GTK_BUTTONS_OK_CANCEL:
			gtk_dialog_add_button (dialog,
			                       GTK_STOCK_CANCEL,
			                       GTK_RESPONSE_CANCEL);
			gtk_dialog_add_button (dialog,
			                       GTK_STOCK_OK,
			                       GTK_RESPONSE_OK);
			gtk_dialog_set_default_response (dialog, 
			                                 GTK_RESPONSE_OK);
			break;
		default:
			g_warning ("Unknown GtkButtonsType");
 			break;
	} 
	g_object_notify (G_OBJECT (alert_dialog), "buttons");
}

static void
gsearch_alert_dialog_style_set (GtkWidget *widget,
                                GtkStyle  *prev_style)
{
	GtkWidget *parent;
	gint border_width;
	
	border_width = 0;

	parent = GTK_WIDGET (GSEARCH_ALERT_DIALOG (widget)->image->parent);

	if (parent != NULL) {
		gtk_widget_style_get (widget, "alert_border",
		                      &border_width, NULL);

		gtk_container_set_border_width (GTK_CONTAINER (parent),
		                                border_width);
	}

	if (GTK_WIDGET_CLASS (parent_class)->style_set) {
		(GTK_WIDGET_CLASS (parent_class)->style_set) (widget, prev_style);
	}
}
