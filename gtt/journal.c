/*   Display & Edit Journal of Timestamps for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas
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
#include <gtkhtml/gtkhtml.h>
#include <stdio.h>
#include <string.h>

#include "cur-proj.h"
#include "journal.h"
#include "phtml.h"
#include "proj.h"
#include "props-invl.h"
#include "props-task.h"


/* this struct is a random mish-mash of stuff, not well organized */

typedef struct wiggy_s {
	GttPhtml  ph;     /* must be first elt for cast to work */
	GtkHTML *htmlw;
	GtkHTMLStream *handle;
	GtkWidget *top;
	GtkWidget *interval_popup;
	EditIntervalDialog *edit_ivl;
	GttInterval * interval;
	GttTask *task;
	GttProject *prj;
} Wiggy;

/* ============================================================== */
/* html i/o routines */

static void
wiggy_open (GttPhtml *pl)
{
	Wiggy *wig = (Wiggy *) pl;

	/* open the browser for writing */
	wig->handle = gtk_html_begin_content(wig->htmlw, "text/html");
}

static void
wiggy_close (GttPhtml *pl)
{
	Wiggy *wig = (Wiggy *) pl;

	/* close the browser stream */
	gtk_html_end (wig->htmlw, wig->handle, GTK_HTML_STREAM_OK);
	wig->handle = NULL;
}

static void
wiggy_write (GttPhtml *pl, const char *str, size_t len)
{
	Wiggy *wig = (Wiggy *) pl;

	/* write to the browser stream */
	gtk_html_write (wig->htmlw, wig->handle, str, len);
}

static void
wiggy_error (GttPhtml *pl, int err, const char * msg)
{
	Wiggy *wig = (Wiggy *) pl;
	GtkHTML *htmlw = wig->htmlw;
	GtkHTMLStream *han;
	char * buff;

	han = gtk_html_begin_content(htmlw, "text/html");

	if (404 == err)
	{
		buff = _("<html><body><h1>Error 404 Not Found</h1>"
		       "The file ");
		gtk_html_write (htmlw, han, buff, strlen (buff));
	
		buff = msg? (char*) msg : _("(null)");
		gtk_html_write (htmlw, han, buff, strlen (buff));
		
		buff = _(" was not found. </body></html>");
		gtk_html_write (htmlw, han, buff, strlen (buff));
	}
	else
	{
		buff = _("<html><body><h1>Unkown Error</h1>");
		gtk_html_write (htmlw, han, buff, strlen (buff));
	}
	
	gtk_html_end (htmlw, han, GTK_HTML_STREAM_OK);

}

/* ============================================================== */
/* engine callbacks */

static void 
redraw (GttProject * prj, gpointer data)
{
	Wiggy *wig = (Wiggy *) data;
	gtt_phtml_display (&(wig->ph), "journal.phtml", wig->prj);
}

/* ============================================================== */

static void 
on_print_clicked_cb (GtkWidget *w, gpointer data)
{
	GladeXML  *glxml;
	printf ("duude print!!\n");
	glxml = glade_xml_new ("glade/not-implemented.glade", "Not Implemented");

}

static void 
on_save_clicked_cb (GtkWidget *w, gpointer data)
{
	GladeXML  *glxml;
	printf ("duude save!!\n");
	glxml = glade_xml_new ("glade/not-implemented.glade", "Not Implemented");
}

static void 
on_close_clicked_cb (GtkWidget *w, gpointer data)
{
	Wiggy *wig = (Wiggy *) data;

	/* close the main journal window ... everything */
	gtt_project_remove_notifier (wig->prj, redraw, wig);
	edit_interval_dialog_destroy (wig->edit_ivl);
	gtk_widget_destroy (wig->top);
	g_free (wig);
}

/* ============================================================== */
/* interval popup actions */

static void
interval_edit_clicked_cb(GtkWidget * dw, gpointer data) 
{
	Wiggy *wig = (Wiggy *) data;

	if (NULL == wig->edit_ivl) wig->edit_ivl = edit_interval_dialog_new();
	edit_interval_set_interval (wig->edit_ivl, wig->interval);
	edit_interval_dialog_show (wig->edit_ivl);
}

static void
interval_delete_clicked_cb(GtkWidget * w, gpointer data) 
{
	Wiggy *wig = (Wiggy *) data;
	gtt_interval_destroy (wig->interval);
	wig->interval = NULL;
	gtt_phtml_display (&(wig->ph), "journal.phtml", wig->prj);
}

static void
interval_merge_up_clicked_cb(GtkWidget * w, gpointer data) 
{
	Wiggy *wig = (Wiggy *) data;
	gtt_interval_merge_up (wig->interval);
	wig->interval = NULL;
	gtt_phtml_display (&(wig->ph), "journal.phtml", wig->prj);
}

static void
interval_merge_down_clicked_cb(GtkWidget * w, gpointer data) 
{
	Wiggy *wig = (Wiggy *) data;
	gtt_interval_merge_down (wig->interval);
	wig->interval = NULL;
	gtt_phtml_display (&(wig->ph), "journal.phtml", wig->prj);
}

static void
interval_popup_cb (Wiggy *wig)
{
	gtk_menu_popup(GTK_MENU(wig->interval_popup), 
		NULL, NULL, NULL, wig, 1, 0);
}

/* ============================================================== */
/* html events */

static void
html_link_clicked_cb(GtkHTML * html, const gchar * url, gpointer data) 
{
	Wiggy *wig = (Wiggy *) data;
	gpointer addr = NULL;
	char *str;

	/* decode the address buried in the URL (if its there) */
	str = strstr (url, "0x");
	if (str)
	{
		addr = (gpointer) strtoul (str, NULL, 16);
	}

	if (0 == strncmp (url, "gtt:interval", 12))
	{
		wig->interval = addr;
		if (addr) interval_popup_cb (wig);
	}
	else
	if (0 == strncmp (url, "gtt:task", 8))
	{
		wig->task = addr;
		prop_task_dialog_show (wig->task);
	}
	else
	{
		g_warning ("clicked on unknown link duude=%s\n", url);
	}
}

static void
html_on_url_cb(GtkHTML * html, const gchar * url, gpointer data) 
{
	// printf ("hover over the url show hover help duude=%s\n", url);
}


/* ============================================================== */

void
edit_journal(GtkWidget *widget, gpointer data)
{
	GtkWidget *jnl_top, *jnl_viewport, *jnl_browser;
	GladeXML  *glxml;
	Wiggy *wig;

	glxml = glade_xml_new ("glade/journal.glade", "Journal Window");

	jnl_top = glade_xml_get_widget (glxml, "Journal Window");
	jnl_viewport = glade_xml_get_widget (glxml, "Journal ScrollWin");

	/* ---------------------------------------------------- */
	/* create browser, plug it into the viewport */
	jnl_browser = gtk_html_new();


	/* FIXME hack alert -- scroled window add causes a hard crash
	 * #0  0x40260ea5 in gtk_layout_get_type () from /usr/lib/libgtk-1.2.so.0
	 * #1  0x40574a62 in html_alignment_to_paragraph () from /usr/lib/libgtkhtml.so.14
	* but a simple container-add works fine ... */

	/*
	gtk_scrolled_window_add_with_viewport
		(GTK_SCROLLED_WINDOW(jnl_viewport), jnl_browser);
	*/
	gtk_container_add(GTK_CONTAINER(jnl_viewport), jnl_browser);

	/* ---------------------------------------------------- */
	/* signals for the browser, and the journal window */

	wig = g_new0 (Wiggy, 1);
	wig->edit_ivl = NULL;

	wig->top = jnl_top;
	wig->htmlw = GTK_HTML(jnl_browser);
	wig->ph.open_stream = wiggy_open;
	wig->ph.write_stream = wiggy_write;
	wig->ph.close_stream = wiggy_close;
	wig->ph.error = wiggy_error;
	
	glade_xml_signal_connect_data (glxml, "on_close_clicked",
	        GTK_SIGNAL_FUNC (on_close_clicked_cb), wig);
	  
	glade_xml_signal_connect_data (glxml, "on_save_clicked",
	        GTK_SIGNAL_FUNC (on_save_clicked_cb), wig);
	  
	glade_xml_signal_connect_data (glxml, "on_print_clicked",
	        GTK_SIGNAL_FUNC (on_print_clicked_cb), wig);
	  
	gtk_signal_connect(GTK_OBJECT(jnl_browser), "link_clicked",
		GTK_SIGNAL_FUNC(html_link_clicked_cb), wig);

	gtk_signal_connect(GTK_OBJECT(jnl_browser), "on_url",
		GTK_SIGNAL_FUNC(html_on_url_cb), wig);


	gtk_widget_show (jnl_browser);
	gtk_widget_show (jnl_top);

	/* ---------------------------------------------------- */
	/* this is the popup menu that says 'edit/delete/merge' */
	/* for intervals */

	glxml = glade_xml_new ("glade/interval_popup.glade", "Interval Popup");
	wig->interval_popup = glade_xml_get_widget (glxml, "Interval Popup");
	wig->interval=NULL;

	glade_xml_signal_connect_data (glxml, "on_edit_activate",
	        GTK_SIGNAL_FUNC (interval_edit_clicked_cb), wig);
	  
	glade_xml_signal_connect_data (glxml, "on_delete_activate",
	        GTK_SIGNAL_FUNC (interval_delete_clicked_cb), wig);
	  
	glade_xml_signal_connect_data (glxml, "on_merge_up_activate",
	        GTK_SIGNAL_FUNC (interval_merge_up_clicked_cb), wig);
	  
	glade_xml_signal_connect_data (glxml, "on_merge_down_activate",
	        GTK_SIGNAL_FUNC (interval_merge_down_clicked_cb), wig);
	  
	/* ---------------------------------------------------- */
	/* finally ... display the actual journal */

	if (!cur_proj)
	{
		wig->prj = cur_proj;
		gtt_phtml_display (&(wig->ph), "noproject.phtml", NULL);
	} 
	else 
	{

		gtt_project_add_notifier (cur_proj, redraw, wig);
		wig->prj = cur_proj;
		gtt_phtml_display (&(wig->ph), "journal.phtml", cur_proj);
	}
}

/* ===================== END OF FILE ==============================  */
