/* GNOME Search Tool
 * Copyright (C) 1997 George Lebl.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>

#include "gsearchtool.h"
#include "outdlg.h"

static GtkWidget *outdlg=NULL;
static GtkWidget *outlist;

gboolean
outdlg_makedlg(gchar name[])
{
	GtkWidget *widget;
	GtkWidget *scrolledw;

	/*we already have a dialog!!!*/
	if(outdlg!=NULL)
		return FALSE;

	outdlg=gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(outdlg), "destroy",
			   GTK_SIGNAL_FUNC(outdlg_closedlg), NULL);
	gtk_window_set_title(GTK_WINDOW(outdlg),name);

	widget=gtk_button_new_with_label("Clear");
	gtk_signal_connect(GTK_OBJECT(widget), "clicked",
			   GTK_SIGNAL_FUNC(outdlg_clearlist), NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(outdlg)->action_area),widget,
		FALSE,FALSE,5);
	gtk_widget_show(widget);

	widget=gtk_button_new_with_label("Close");
	gtk_signal_connect(GTK_OBJECT(widget), "clicked",
			   GTK_SIGNAL_FUNC(outdlg_closedlg),NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(outdlg)->action_area),widget,
		FALSE,FALSE,5);
	gtk_widget_show(widget);

	/*widget=gtk_label_new("Files Found:");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(outdlg)->vbox),widget,
		FALSE,FALSE,5);
	gtk_widget_show(widget);*/

	scrolledw=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_usize(scrolledw,250,200);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(outdlg)->vbox),scrolledw,
		TRUE,TRUE,5);
	gtk_widget_show(scrolledw);

	outlist=gtk_list_new();
	gtk_container_add(GTK_CONTAINER(scrolledw),outlist);
	gtk_widget_show(outlist);

	return TRUE;
}

void
outdlg_additem(gchar item[])
{
	GtkWidget *listitem;

	listitem=gtk_list_item_new_with_label(item);
	gtk_container_add(GTK_CONTAINER(outlist),listitem);
	gtk_widget_show(listitem);
}

void
outdlg_showdlg()
{
	gtk_widget_show(outdlg);
}

void
outdlg_closedlg(GtkWidget * widget, gpointer * data)
{
	gtk_widget_hide(outdlg);
	gtk_widget_destroy(outdlg);
	outdlg=NULL;
}

void
outdlg_clearlist(GtkWidget * widget, gpointer * data)
{
	gtk_list_clear_items(GTK_LIST(outlist),0,-1);
}
