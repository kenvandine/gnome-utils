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

void
cb_about_click (GtkWidget *widget, gpointer user_data)
{
    const gchar *authors[] =
    {
        "Hongli Lai (hongli@telekabel.nl)",
        NULL
    };
    gchar *documenters[] = {
	    NULL
    };
    /* Translator credits */
    gchar *translator_credits = _("translator_credits");
    GtkWidget *dialog;
    GdkPixbuf *logo = NULL;
    gchar *logo_fn;

    logo_fn = gnome_pixmap_file("gcharmap-logo.png");
    if (logo_fn != NULL)
    {
	    logo = gdk_pixbuf_new_from_file(logo_fn, NULL /* error */);
	    g_free(logo_fn);
    }

    dialog = gnome_about_new (
      _("Gnome Character Map"),
      VERSION,
      "Copyright (c) 2000 Hongli Lai",
      _("The Gnome equalivant of Microsoft Windows' Character Map. "
      "Warning: might contain bad English."),
      authors,
      (const char **)documenters,
      strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
      logo
    );
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
    gchar *text;

    gtk_label_get (label, &text);

    if (strcmp (text, _("del")) == 0) {
	    if ( ! mainapp->insert_at_end) {
		    GtkEditable *editable = GTK_EDITABLE (mainapp->entry);
		    /* snarfed from GTK+
		     * -George */
		    gint sel_start, sel_end;

		    gtk_editable_get_selection_bounds(editable, &sel_start,
				    &sel_end);
		    if (sel_start != sel_end) {
			    gtk_editable_delete_selection (editable);
		    } else {
			    gint old_pos = gtk_editable_get_position(editable);

			    if (gtk_editable_get_position(editable) < -1)
				    gtk_editable_set_position(editable, 0);
			    else
				    gtk_editable_set_position(editable, gtk_editable_get_position(editable) + 1);

			    gtk_editable_delete_text (editable, old_pos, gtk_editable_get_position(editable));
		    }
	    }
    } else if ( ! mainapp->insert_at_end) {
	    gint current_pos = gtk_editable_get_position(GTK_EDITABLE(mainapp->entry));
	    gtk_editable_insert_text (GTK_EDITABLE (mainapp->entry), text,
				      strlen (text), &current_pos);
    } else {
	    gtk_entry_append_text (GTK_ENTRY (mainapp->entry), text);
    }
}


void
cb_charbtn_enter (GtkButton *button, gpointer user_data)
{
    GtkLabel *label = GTK_LABEL (GTK_BIN (button)->child);
    gchar *text, *s;
   int code;

    gtk_label_get (label, &text);
    if (strcmp (text, _("del")) == 0) {
	    code = 127;
    } else {
            code = g_utf8_get_char(text);
    }

    s = g_strdup_printf (_(" %s: Character code %d"), text, code);
    gnome_appbar_set_status (GNOME_APPBAR (GNOME_APP (mainapp->window)->statusbar), s);
#if 0
    gtk_label_set_text (GTK_LABEL (mainapp->preview_label), text);
#endif
    g_free (s);
}


void
cb_charbtn_leave (GtkButton *button, gpointer user_data)
{
    gnome_appbar_pop (GNOME_APPBAR (GNOME_APP (mainapp->window)->statusbar));
#if 0    
    gtk_label_set_text (GTK_LABEL (mainapp->preview_label), NULL);
#endif
}


void
cb_clear_click (GtkWidget *widget, gpointer user_data)
{
    gtk_editable_delete_text (GTK_EDITABLE (mainapp->entry), 0, -1);
}


void
cb_copy_click (GtkWidget *widget, gpointer user_data)
{
    cb_select_all_click (widget, user_data);
    gtk_editable_copy_clipboard (GTK_EDITABLE (mainapp->entry));
    gnome_app_flash (GNOME_APP (mainapp->window), _("Text copied to clipboard..."));
}


void
cb_cut_click (GtkWidget *widget, gpointer user_data)
{
    cb_select_all_click (widget, user_data);
    while (gtk_events_pending ()) gtk_main_iteration ();
    usleep (500000);
    while (gtk_events_pending ()) gtk_main_iteration ();
    gtk_editable_cut_clipboard (GTK_EDITABLE (mainapp->entry));
    gnome_app_flash (GNOME_APP (mainapp->window), _("Text cut to clipboard..."));
}


void
cb_exit_click (GtkWidget *widget, gpointer user_data)
{
    gtk_widget_destroy (mainapp->window);
}


void
cb_fontpicker_font_set (GnomeFontPicker *gfp, gchar *font_name)
{
    GdkFont *font;
    
    font = gtk_style_get_font(mainapp->btnstyle);
    gdk_font_unref (font);
    
    font = gdk_font_load (font_name);
    gtk_style_set_font(mainapp->btnstyle, font);

    gtk_widget_hide (mainapp->chartable);
    gtk_widget_show (mainapp->chartable);
}


void
cb_help_click (GtkWidget *widget, gpointer user_data)
{
    gnome_help_display (
        gnome_program_get_human_readable_name(gnome_program_get ()),
		    "index.html",
		    NULL /* error */);
}


void
cb_insert_char_click (GtkWidget *widget, gpointer user_data)
{
    AsciiSelect *ascii_selector;

    ascii_selector = ascii_select_new ();
    gtk_widget_show (ascii_selector->window);
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
    gtk_entry_select_region (GTK_ENTRY (mainapp->entry), 0, -1);
}


void
cb_set_button_focusable (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    guint i;

    if (checkmenuitem->active == TRUE)
    {
        for (i = 0; i < g_list_length (mainapp->buttons); i++)
            GTK_WIDGET_SET_FLAGS (GTK_WIDGET (g_list_nth_data (
              mainapp->buttons, i)), GTK_CAN_FOCUS);
    } else
    {
        for (i = 0; i < g_list_length (mainapp->buttons); i++)
            GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (g_list_nth_data (
              mainapp->buttons, i)), GTK_CAN_FOCUS);
    }
}


void
cb_set_chartable_font (GtkWidget *widget, gpointer user_data)
{
    gtk_button_clicked (GTK_BUTTON (mainapp->fontpicker));
}


void
cb_set_insert_at_end (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    mainapp->insert_at_end = checkmenuitem->active;
}


void
cb_toggle_actionbar (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    if (checkmenuitem->active == TRUE)
        gtk_widget_show (mainapp->actionbar);
    else
        gtk_widget_hide (mainapp->actionbar);
}


void
cb_toggle_textbar (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    if (checkmenuitem->active == TRUE)
        gtk_widget_show (mainapp->textbar);
    else
        gtk_widget_hide (mainapp->textbar);
}


void
cb_toggle_statusbar (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    if (checkmenuitem->active == TRUE)
        gtk_widget_show (GNOME_APP (mainapp->window)->statusbar);
    else
        gtk_widget_hide (GNOME_APP (mainapp->window)->statusbar);
}


#endif /* _CALLBACKS_C_ */
