/*   Task Properties for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <glade/glade.h>
#include <gnome.h>
#include <libgnome/gnome-help.h>
#include <string.h>

#include "proj.h"
#include "props-task.h"
#include "util.h"


typedef struct _PropTaskDlg 
{
	GladeXML *gtxml;
	GnomePropertyBox *dlg;
	GtkEntry *memo;
	GtkText *notes;
	GtkOptionMenu *billstatus;
	GtkOptionMenu *billable;
	GtkOptionMenu *billrate;
	GtkEntry *unit;
	GttTask *task;
} PropTaskDlg;


/* ============================================================== */

static void 
task_prop_set(GnomePropertyBox * pb, gint page, PropTaskDlg *dlg)
{
	GttBillStatus status;
	GttBillable able;
	GttBillRate rate;
	GtkWidget *menu, *menu_item;
	int ivl;
	gchar *str;

	if (!dlg->task) return;

	if (0 == page)
	{
		gtt_task_freeze (dlg->task);
		str = gtk_entry_get_text(dlg->memo);
		if (str && str[0]) 
		{
			gtt_task_set_memo(dlg->task, str);
		} 
		else 
		{
			gtt_task_set_memo(dlg->task, _("empty"));
			gtk_entry_set_text(dlg->memo, _("empty"));
		}
	
		gtt_task_set_notes(dlg->task, xxxgtk_text_get_text(dlg->notes));

		ivl = (int) (60.0 * atof (gtk_entry_get_text(dlg->unit)));
		gtt_task_set_bill_unit (dlg->task, ivl);

	        menu = gtk_option_menu_get_menu (dlg->billstatus);
        	menu_item = gtk_menu_get_active(GTK_MENU(menu));
        	status = (GttBillStatus) (gtk_object_get_data(
			GTK_OBJECT(menu_item), "billstatus"));
		gtt_task_set_billstatus (dlg->task, status);

	        menu = gtk_option_menu_get_menu (dlg->billable);
        	menu_item = gtk_menu_get_active(GTK_MENU(menu));
        	able = (GttBillable) (gtk_object_get_data(
			GTK_OBJECT(menu_item), "billable"));
		gtt_task_set_billable (dlg->task, able);

	        menu = gtk_option_menu_get_menu (dlg->billrate);
        	menu_item = gtk_menu_get_active(GTK_MENU(menu));
        	rate = (GttBillRate) (gtk_object_get_data(
			GTK_OBJECT(menu_item), "billrate"));
		gtt_task_set_billrate (dlg->task, rate);
		gtt_task_thaw (dlg->task);
	}
}


/* ============================================================== */


static void 
do_set_task(GttTask *tsk, PropTaskDlg *dlg)
{
	GttBillStatus status;
	GttBillable able;
	GttBillRate rate;
	char buff[132];

	if (!tsk) 
	{

		dlg->task = NULL;
		gtk_entry_set_text(dlg->memo, "");
		gtk_text_insert(dlg->notes, NULL, NULL, NULL, "", 0);
		gtk_entry_set_text(dlg->unit, "0.0");
		return;
	}

	/* set the task, evenm if its same as the old task.  Do this because
	 * the widget may contain rejected edit values  */
	dlg->task = tsk;

	gtk_entry_set_text(dlg->memo, gtt_task_get_memo(tsk));
	xxxgtk_text_set_text(dlg->notes, gtt_task_get_notes (tsk));

	g_snprintf (buff, 132, "%g", ((double) gtt_task_get_bill_unit(tsk))/60.0);
	gtk_entry_set_text(dlg->unit, buff);

	status = gtt_task_get_billstatus (tsk);
	if (GTT_HOLD == status) gtk_option_menu_set_history (dlg->billstatus, 0);
	else if (GTT_BILL == status) gtk_option_menu_set_history (dlg->billstatus, 1);
	else if (GTT_PAID == status) gtk_option_menu_set_history (dlg->billstatus, 2);

	able = gtt_task_get_billable (tsk);
	if (GTT_BILLABLE == able) gtk_option_menu_set_history (dlg->billable, 0);
	else if (GTT_NOT_BILLABLE == able) gtk_option_menu_set_history (dlg->billable, 1);
	else if (GTT_NO_CHARGE == able) gtk_option_menu_set_history (dlg->billable, 2);

	rate = gtt_task_get_billrate (tsk);
	if (GTT_REGULAR == rate) gtk_option_menu_set_history (dlg->billrate, 0);
	else if (GTT_OVERTIME == rate) gtk_option_menu_set_history (dlg->billrate, 1);
	else if (GTT_OVEROVER == rate) gtk_option_menu_set_history (dlg->billrate, 2);
	else if (GTT_FLAT_FEE == rate) gtk_option_menu_set_history (dlg->billrate, 3);

	/* set to unmodified as it reflects the current state of the project */
	gnome_property_box_set_modified(GNOME_PROPERTY_BOX(dlg->dlg),
					FALSE);
}

/* ============================================================== */

static  PropTaskDlg *
prop_task_dialog_new (void)
{
	PropTaskDlg *dlg = NULL;
	GladeXML *gtxml;
	GtkWidget *e;
	GtkWidget *menu, *menu_item;
        static GnomeHelpMenuEntry help_entry = { NULL, "index.html#TASK" };

	dlg = g_malloc(sizeof(PropTaskDlg));

	gtxml = glade_xml_new ("glade/task_properties.glade", "Task Properties");
	dlg->gtxml = gtxml;

	dlg->dlg = GNOME_PROPERTY_BOX (glade_xml_get_widget (gtxml,  "Task Properties"));

	help_entry.name = gnome_app_id;
	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "help",
			   GTK_SIGNAL_FUNC(gnome_help_pbox_display),
			   &help_entry);

	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "apply",
			   GTK_SIGNAL_FUNC(task_prop_set), dlg);

	/* ------------------------------------------------------ */
	/* grab the various entry boxes and hook them up */
	e = glade_xml_get_widget (gtxml, "memo box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->memo = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "notes box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->notes = GTK_TEXT(e);

	e = glade_xml_get_widget (gtxml, "billstatus menu");
	dlg->billstatus = GTK_OPTION_MENU(e);
	e = gtk_option_menu_get_menu (GTK_OPTION_MENU(e));
	gtk_signal_connect_object(GTK_OBJECT(e), "selection_done",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));

	e = glade_xml_get_widget (gtxml, "billable menu");
	dlg->billable = GTK_OPTION_MENU(e);
	e = gtk_option_menu_get_menu (GTK_OPTION_MENU(e));
	gtk_signal_connect_object(GTK_OBJECT(e), "selection_done",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));

	e = glade_xml_get_widget (gtxml, "billrate menu");
	dlg->billrate = GTK_OPTION_MENU(e);
	e = gtk_option_menu_get_menu (GTK_OPTION_MENU(e));
	gtk_signal_connect_object(GTK_OBJECT(e), "selection_done",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));

	e = glade_xml_get_widget (gtxml, "unit box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->unit = GTK_ENTRY(e);

	/* ------------------------------------------------------ */
	/* associate values with the two option menus */
	menu = gtk_option_menu_get_menu (dlg->billstatus);

	gtk_option_menu_set_history (dlg->billstatus, 0);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billstatus",
		(gpointer) GTT_HOLD);

	gtk_option_menu_set_history (dlg->billstatus, 1);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billstatus",
		(gpointer) GTT_BILL);

	gtk_option_menu_set_history (dlg->billstatus, 2);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billstatus",
		(gpointer) GTT_PAID);


	menu = gtk_option_menu_get_menu (dlg->billable);

	gtk_option_menu_set_history (dlg->billable, 0);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billable",
		(gpointer) GTT_BILLABLE);

	gtk_option_menu_set_history (dlg->billable, 1);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billable",
		(gpointer) GTT_NOT_BILLABLE);

	gtk_option_menu_set_history (dlg->billable, 2);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billable",
		(gpointer) GTT_NO_CHARGE);


	menu = gtk_option_menu_get_menu (dlg->billrate);

	gtk_option_menu_set_history (dlg->billrate, 0);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billrate",
		(gpointer) GTT_REGULAR);

	gtk_option_menu_set_history (dlg->billrate, 1);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billrate",
		(gpointer) GTT_OVERTIME);

	gtk_option_menu_set_history (dlg->billrate, 2);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billrate",
		(gpointer) GTT_OVEROVER);

	gtk_option_menu_set_history (dlg->billrate, 3);
	menu_item =  gtk_menu_get_active(GTK_MENU(menu));
	gtk_object_set_data(GTK_OBJECT(menu_item), "billrate",
		(gpointer) GTT_FLAT_FEE);


	gnome_dialog_close_hides(GNOME_DIALOG(dlg->dlg), TRUE);
/*
	gnome_dialog_set_parent(GNOME_DIALOG(dlg->dlg), GTK_WINDOW(window));

*/
	return dlg;
}

/* ============================================================== */

static PropTaskDlg *dlog = NULL;

void 
prop_task_dialog_show (GttTask *task)
{
	if (!dlog) dlog = prop_task_dialog_new();

	do_set_task(task, dlog);
	gtk_widget_show(GTK_WIDGET(dlog->dlg));
	
}

/* ================= END OF FILE ================================ */
