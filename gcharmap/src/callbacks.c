/*
 *  Gnome Character Map
 *  callbacks.c - Callbacks for the main window
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

#ifndef _CALLBACKS_C_
#define _CALLBACKS_C_

#include <config.h>
#include "callbacks.h"
#include <unistd.h>

#include "interface.h"
#include "asciiselect.h"

#include <gnome.h>

void
cb_about_click (GtkWidget *widget, gpointer user_data)
{
    const gchar *authors[] =
    {
        "Hongli Lai (h.lai@chello.nl)",
        NULL
    };
    gchar *documenters[] = {
	    NULL
    };
    /* Translator credits */
    gchar *translator_credits = _("translator_credits");
    GtkWidget *dialog;
    GdkPixbuf *logo  = NULL;
    GError    *error = NULL;
    gchar     *logo_fn;

    logo_fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "gnome-character-map.png", FALSE, NULL);
    logo = gdk_pixbuf_new_from_file (logo_fn, &error);
    
    if (error) {
    	    g_warning (G_STRLOC ": cannot open %s: %s", logo_fn, error->message);
	    g_error_free (error);
    }
    
    g_free (logo_fn);

    dialog = gnome_about_new (
      _("GNOME Character Map"),
      VERSION,
      "Copyright (c) 2000 Hongli Lai",
      _("Select, copy and paste characters from your font "
	"into other applications"),
      authors,
      (const char **)documenters,
      strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
      logo
    );
    
    if (logo) {
    	    gdk_pixbuf_unref (logo);
    }
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (user_data));
    gtk_widget_show (dialog);
}

void 
cb_browsebtn_click (GtkButton *button, gpointer data)
{
	cb_insert_char_click (NULL, data);
}


void
cb_charbtn_click (GtkButton *button, gpointer user_data)
{
    GtkLabel *label = GTK_LABEL (GTK_BIN (button)->child);
    const gchar *text;
    gint current_pos;
   
    text = gtk_label_get_text (label);

    gtk_editable_get_position(GTK_EDITABLE(mainapp->entry));
    gtk_editable_insert_text (GTK_EDITABLE (mainapp->entry), text,
			      strlen (text), &current_pos);
    gtk_editable_set_position (GTK_EDITABLE (mainapp->entry), current_pos + 1);
}


gboolean
cb_charbtn_enter (GtkButton *button, GdkEventFocus *event, gpointer user_data)
{
    GtkLabel *label = GTK_LABEL (GTK_BIN (button)->child);
    gchar *s;
   int code;
    const gchar *text;

    text = gtk_label_get_text (label);
    if (strcmp (text, _("del")) == 0) {
	    code = 127;
    } else {
            code = g_utf8_get_char(text);
    }

    s = g_strdup_printf (_(" %s: Character code %d"), text, code);
    gnome_appbar_set_status (GNOME_APPBAR (GNOME_APP (mainapp->window)->statusbar), s);
    g_free (s);
    return FALSE;
}


gboolean
cb_charbtn_leave (GtkButton *button, GdkEventFocus *event, gpointer user_data)
{
    gnome_appbar_pop (GNOME_APPBAR (GNOME_APP (mainapp->window)->statusbar));
    return FALSE;
}


void
cb_clear_click (GtkWidget *widget, gpointer user_data)
{
    gtk_editable_delete_text (GTK_EDITABLE (mainapp->entry), 0, -1);
}


void
cb_copy_click (GtkWidget *widget, gpointer user_data)
{
    gint start, end;
    gboolean selection_flag;

    selection_flag = gtk_editable_get_selection_bounds (GTK_EDITABLE (mainapp->entry), &start, &end);

    if(selection_flag)
        gtk_editable_select_region (GTK_EDITABLE (mainapp->entry), start, end);
    else
        cb_select_all_click (widget, user_data);

    gtk_editable_copy_clipboard (GTK_EDITABLE (mainapp->entry));
    gnome_app_flash (GNOME_APP (mainapp->window), _("Text copied to clipboard..."));
}


void
cb_cut_click (GtkWidget *widget, gpointer user_data)
{
    gint start, end;
    gboolean selection_flag;

    selection_flag = gtk_editable_get_selection_bounds (GTK_EDITABLE (mainapp->entry), &start, &end);

    if(selection_flag)
        gtk_editable_select_region (GTK_EDITABLE (mainapp->entry), start, end);
    else
        cb_select_all_click (widget, user_data);
    gtk_editable_cut_clipboard (GTK_EDITABLE (mainapp->entry));
    gnome_app_flash (GNOME_APP (mainapp->window), _("Text cut to clipboard..."));
}


void
cb_exit_click (GtkWidget *widget, gpointer user_data)
{
    gtk_widget_destroy (mainapp->window);
}

void
cb_help_click (GtkWidget *widget, gpointer user_data)
{
    GError *error = NULL;
    gnome_help_display("gnome-character-map",NULL,&error);
}


void
cb_insert_char_click (GtkWidget *widget, gpointer user_data)
{
    AsciiSelect *ascii_selector = NULL;

    ascii_selector = ascii_select_new ();
    if (ascii_selector) {
        gtk_window_set_transient_for (GTK_WINDOW (ascii_selector->window), 
				      GTK_WINDOW (mainapp->window));
        gtk_widget_show (ascii_selector->window);
    }
}


void
cb_paste_click (GtkWidget *widget, gpointer user_data)
{
    gtk_editable_paste_clipboard (GTK_EDITABLE (mainapp->entry));
    gnome_app_flash (GNOME_APP (mainapp->window), _("Text pasted from clipboard..."));
}


void
cb_select_all_click (GtkWidget *widget, gpointer user_data)
{
    gtk_editable_select_region (GTK_EDITABLE (mainapp->entry), 0, -1);
}


void
cb_set_chartable_font (GtkWidget *widget, gpointer user_data)
{
    gtk_button_clicked (GTK_BUTTON (mainapp->fontpicker));
}

void
cb_entry_changed (GtkWidget *widget, gpointer data)
{
    gchar *text;

    text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
    if (strcmp (text, "") == 0) 
      edit_menu_set_sensitivity (FALSE);
    else
      edit_menu_set_sensitivity (TRUE);

    g_free (text);
}

#endif /* _CALLBACKS_C_ */
