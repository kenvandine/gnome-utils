/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gsearchtool-alert-dialog.h: An HIG compliant alert dialog.

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

#ifndef GSEARCHTOOL_ALERT_DIALOG_H
#define GSEARCHTOOL_ALERT_DIALOG_H

#include <gtk/gtkmessagedialog.h>

#define GSEARCH_TYPE_ALERT_DIALOG        (gsearch_alert_dialog_get_type ())
#define GSEARCH_ALERT_DIALOG(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSEARCH_TYPE_ALERT_DIALOG, GsearchAlertDialog))

typedef struct _GsearchAlertDialog        GsearchAlertDialog;
typedef struct _GsearchAlertDialogClass   GsearchAlertDialogClass;

struct _GsearchAlertDialog
{
	GtkDialog parent_instance;
  
	GtkWidget *image;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	GtkWidget *details_expander;
	GtkWidget *details_label;
};

struct _GsearchAlertDialogClass
{
	GtkDialogClass parent_class;
};

GType      gsearch_alert_dialog_get_type (void);

GtkWidget* gsearch_alert_dialog_new                 (GtkWindow      *parent,
                                                     GtkDialogFlags  flags,
                                                     GtkMessageType  type,
                                                     GtkButtonsType  buttons,
                                                     const gchar    *primary_message,
                                                     const gchar    *secondary_message,
                                                     const gchar    *title);

void       gsearch_alert_dialog_set_primary_label   (GsearchAlertDialog *dialog,
		                                     const gchar    *message);
						 
void       gsearch_alert_dialog_set_secondary_label (GsearchAlertDialog *dialog,
		                                     const gchar    *message);

void       gsearch_alert_dialog_set_details_label   (GsearchAlertDialog *dialog,
						     const gchar    *message);
#endif /* GSEARCHTOOL_ALERT_DIALOG_H */
