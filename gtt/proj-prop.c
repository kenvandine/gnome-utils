/*   Project Properties for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
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

#include "gtt.h"
#include "proj.h"


typedef struct _PropDlg 
{
	GladeXML *gtxml;
	GnomePropertyBox *dlg;
	GtkEntry *title;
	GtkEntry *desc;
	GtkText  *notes;
	GtkEntry *regular;
	GtkEntry *overtime;
	GtkEntry *overover;
	GtkEntry *flatfee;
	GtkEntry *minvl;
	GtkEntry *ivl;
	GtkEntry *gap;
	GttProject *proj;
} PropDlg;


/* ============================================================== */

static void 
prop_set(GnomePropertyBox * pb, gint page, PropDlg *dlg)
{
	int ivl;
	double rate;
	gchar *str;

	if (!dlg->proj) return;

	if (0 == page)
	{
		int len;

		str = gtk_entry_get_text(dlg->title);
		if (str && str[0]) 
		{
			gtt_project_set_title(dlg->proj, str);
		} 
		else 
		{
			gtt_project_set_title(dlg->proj, _("empty"));
			gtk_entry_set_text(dlg->title, _("empty"));
		}
	
		gtt_project_set_desc(dlg->proj, gtk_entry_get_text(dlg->desc));

		/* crazy text handling; note this is broken for
		 * double-byte character sets */
		len = gtk_text_get_length(dlg->notes);
		if (len >= dlg->notes->text_len) len = dlg->notes->text_len -1;
		dlg->notes->text.ch[len] = 0x0;  /* null-erminate */
		gtt_project_set_notes (dlg->proj, dlg->notes->text.ch);
	}

	if (1 == page)
	{
		rate = atof (gtk_entry_get_text(dlg->regular));
		gtt_project_set_billrate (dlg->proj, rate);
		rate = atof (gtk_entry_get_text(dlg->overtime));
		gtt_project_set_overtime_rate (dlg->proj, rate);
		rate = atof (gtk_entry_get_text(dlg->overover));
		gtt_project_set_overover_rate (dlg->proj, rate);
		rate = atof (gtk_entry_get_text(dlg->flatfee));
		gtt_project_set_flat_fee (dlg->proj, rate);
	}
	
	if (2 == page)
	{
		ivl = atoi (gtk_entry_get_text(dlg->minvl));
		gtt_project_set_min_interval (dlg->proj, ivl);
		ivl = atoi (gtk_entry_get_text(dlg->ivl));
		gtt_project_set_auto_merge_interval (dlg->proj, ivl);
		ivl = atoi (gtk_entry_get_text(dlg->gap));
		gtt_project_set_auto_merge_gap (dlg->proj, ivl);
	}
}


/* ============================================================== */

static PropDlg *dlg = NULL;

void 
prop_dialog_set_project(GttProject *proj)
{
	char buff[132];
	const char * str;

	if (!dlg) return;

	if (!proj) {
		dlg->proj = NULL;
		gtk_entry_set_text(dlg->title, "");
		gtk_entry_set_text(dlg->desc, "");
		gtk_text_insert(dlg->notes, NULL, NULL, NULL, "", 0);
		gtk_entry_set_text(dlg->regular, "0.0");
		gtk_entry_set_text(dlg->overtime, "0.0");
		gtk_entry_set_text(dlg->overover, "0.0");
		gtk_entry_set_text(dlg->flatfee, "0.0");
		gtk_entry_set_text(dlg->minvl, "0");
		gtk_entry_set_text(dlg->ivl, "0");
		gtk_entry_set_text(dlg->gap, "0");
		return;
	}

	if (dlg->proj == proj) return;

	dlg->proj = proj;

	gtk_entry_set_text(dlg->title, gtt_project_get_title(proj));
	gtk_entry_set_text(dlg->desc, gtt_project_get_desc(proj));
	str = gtt_project_get_notes (proj);
	gtk_text_insert(dlg->notes, NULL, NULL, NULL, str, strlen (str));

	/* hack alert should use local currencies for this */
	g_snprintf (buff, 132, "%.2f", gtt_project_get_billrate(proj));
	gtk_entry_set_text(dlg->regular, buff);
	g_snprintf (buff, 132, "%.2f", gtt_project_get_overtime_rate(proj));
	gtk_entry_set_text(dlg->overtime, buff);
	g_snprintf (buff, 132, "%.2f", gtt_project_get_overover_rate(proj));
	gtk_entry_set_text(dlg->overover, buff);
	g_snprintf (buff, 132, "%.2f", gtt_project_get_flat_fee(proj));
	gtk_entry_set_text(dlg->flatfee, buff);

	g_snprintf (buff, 132, "%d", gtt_project_get_min_interval(proj));
	gtk_entry_set_text(dlg->minvl, buff);
	g_snprintf (buff, 132, "%d", gtt_project_get_auto_merge_interval(proj));
	gtk_entry_set_text(dlg->ivl, buff);
	g_snprintf (buff, 132, "%d", gtt_project_get_auto_merge_gap(proj));
	gtk_entry_set_text(dlg->gap, buff);

	/* set to unmodified as it reflects the current state of the project */
	gnome_property_box_set_modified(GNOME_PROPERTY_BOX(dlg->dlg),
					FALSE);
}

/* ============================================================== */

void prop_dialog(GttProject *proj)
{
	GladeXML *gtxml;
	GtkWidget *e;
        static GnomeHelpMenuEntry help_entry = { NULL, "index.html#PROP" };

	if (!proj) return;

	if (dlg) 
	{
		prop_dialog_set_project(proj);
		gtk_widget_show(GTK_WIDGET(dlg->dlg));
		return;
	}

	dlg = g_malloc(sizeof(PropDlg));

	gtxml = glade_xml_new ("glade/project_properties.glade", "Project Properties");
	dlg->gtxml = gtxml;

	dlg->dlg = GNOME_PROPERTY_BOX (glade_xml_get_widget (gtxml,  "Project Properties"));

	help_entry.name = gnome_app_id;
	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "help",
			   GTK_SIGNAL_FUNC(gnome_help_pbox_display),
			   &help_entry);

	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "apply",
			   GTK_SIGNAL_FUNC(prop_set), dlg);

	/* ------------------------------------------------------ */
	/* grab the various entry boxes and hook them up */
	e = glade_xml_get_widget (gtxml, "title box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->title = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "desc box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->desc = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "notes box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->notes = GTK_TEXT(e);

	e = glade_xml_get_widget (gtxml, "regular box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->regular = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "overtime box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->overtime = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "overover box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->overover = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "flatfee box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->flatfee = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "minimum box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->minvl = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "interval box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->ivl = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "gap box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->gap = GTK_ENTRY(e);

	/* ------------------------------------------------------ */
	prop_dialog_set_project(proj);
	gtk_widget_show(GTK_WIDGET(dlg->dlg));


	gnome_dialog_close_hides(GNOME_DIALOG(dlg->dlg), TRUE);
/*
	gnome_dialog_set_parent(GNOME_DIALOG(dlg->dlg), GTK_WINDOW(window));

*/
}


