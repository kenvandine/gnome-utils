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

#include "gtt.h"
#include "journal.h"
#include "phtml.h"
#include "proj.h"

extern GtkWidget *window;

static gboolean glade_inited = FALSE;

typedef struct wiggy_s {
	GttPhtml  ph;     /* must be first elt for cast to work */
	GtkHTML *htmlw;
	GtkHTMLStream *handle;
	GtkWidget *top;
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

static void 
on_print_clicked_cb (GtkWidget *w, gpointer data)
{
	printf ("duude print!!\n");
}

static void 
on_save_clicked_cb (GtkWidget *w, gpointer data)
{
	printf ("duude save!!\n");
}

static void 
on_close_clicked_cb (GtkWidget *w, gpointer data)
{
	Wiggy *wig = (Wiggy *) data;
	gtk_widget_destroy( wig->top);
	g_free (wig);
}

/* ============================================================== */

void
edit_journal(GtkWidget *widget, gpointer data)
{
	GtkWidget *jnl_top, *jnl_viewport, *jnl_browser;
	GladeXML  *glxml;
	Wiggy *wig;

	if (!glade_inited)
	{
		glade_gnome_init ();
		glade_inited = TRUE;
	}

	glxml = glade_xml_new ("glade/journal.glade", "Journal Window");

	jnl_top = glade_xml_get_widget (glxml, "Journal Window");
	jnl_viewport = glade_xml_get_widget (glxml, "Journal ScrollWin");

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

	wig = g_new0 (Wiggy, 1);
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
	  

	gtk_widget_show (jnl_browser);
	gtk_widget_show (jnl_top);

	if (!cur_proj)
	{
		gtt_phtml_display (&(wig->ph), "noproject.phtml");
	} 
	else 
	{
		gtt_phtml_display (&(wig->ph), "journal.phtml");
	}
}

/* ===================== END OF FILE ==============================  */
