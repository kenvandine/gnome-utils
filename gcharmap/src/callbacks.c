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
    gchar utf [7];
    gunichar wc;
   
    text = gtk_label_get_text (label);

    if (!text || text [0] == 0) {
        wc = GPOINTER_TO_INT (user_data) + mainapp->current_page * 256;
        text = utf;
        utf [g_unichar_to_utf8 (wc, utf)] = 0;
    }

    current_pos = gtk_editable_get_position(GTK_EDITABLE(mainapp->entry));
    gtk_editable_insert_text (GTK_EDITABLE (mainapp->entry), text,
			      strlen (text), &current_pos);
    gtk_editable_set_position (GTK_EDITABLE (mainapp->entry), current_pos);
}

void
set_status_bar (GtkButton *button, gpointer user_data)
{
    GtkLabel *label = GTK_LABEL (GTK_BIN (button)->child);
    const char *charset;
    gchar *s;
    gchar *locale;
    gchar mbstr[MB_LEN_MAX*2+1];
    gchar utf[7];
    gunichar wc;
    gsize len;
    int i;

    g_get_charset (&charset);
    wc = GPOINTER_TO_INT (user_data) + mainapp->current_page * 256;
    utf [g_unichar_to_utf8 (wc, utf)] = 0;

    if (utf [0]!=0 && (locale = g_locale_from_utf8 (utf, -1, NULL, &len, NULL)) != NULL) {
	if (len == 1 && (utf[0]&0xff) != 0x3f && locale[0] == 0x3f)
		s = g_strdup_printf (_("Character code: (Unicode) U%04lX"), wc);
	else {
		for (i=0; i<len; i++)
			g_snprintf (mbstr + i*2, MB_LEN_MAX*2 + 1 - i*2, "%02X", (int) ((unsigned char) locale[i]));
		s = g_strdup_printf (_("Character code: (Unicode) U%04lX, (%s) 0x%s"), wc, charset, mbstr);
	}
	g_free (locale);
    } else {
        s = g_strdup_printf (_("Character code: (Unicode) U%04lX"), wc);
    }

    gnome_appbar_set_status (GNOME_APPBAR (GNOME_APP (mainapp->window)->statusbar), s);
    g_free (s);
}

void
cb_charbtn_enter (GtkButton *button, gpointer user_data)
{
    set_status_bar (button, user_data);
}

gboolean
cb_charbtn_focus_in (GtkButton *button, GdkEventFocus *event, gpointer user_data)
{
    set_status_bar (button, user_data);
    return FALSE;
}

void
cb_charbtn_leave (GtkButton *button, gpointer user_data)
{
    gnome_appbar_pop (GNOME_APPBAR (GNOME_APP (mainapp->window)->statusbar));
}

gboolean
cb_charbtn_focus_out (GtkButton *button, GdkEventFocus *event, gpointer user_data)
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
cb_fontpicker_font_set (GnomeFontPicker *picker, gchar *font, gpointer app)
{
    /* if font setting isn't changed, do nothing */
    if ( strcmp(font, ((MainApp *)app)->font) == 0 )
      return;
  
    if (((MainApp *)app)->font) 
      g_free( ((MainApp *)app)->font);
    ((MainApp *)app)->font = g_strdup(font);
   
    main_app_set_font ((MainApp *) app, font);
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
    if (strcmp (text, "") == 0) {
        gtk_widget_set_sensitive (mainapp->copy_button, FALSE);
        edit_menu_set_sensitivity (FALSE);
    }
    else {
        gtk_widget_set_sensitive (mainapp->copy_button, TRUE);
        edit_menu_set_sensitivity (TRUE);
    }
    g_free (text);
}

void 
cb_page_select_spin_changed (GtkSpinButton *spin, gpointer user_data)
{
    static gboolean updating = FALSE;

    if (updating == TRUE) return;

    updating = TRUE;
    mainapp->current_page = gtk_spin_button_get_value_as_int (spin);

    if (mainapp->current_page < 0) mainapp->current_page = 0;
    else if (mainapp->current_page > 255) mainapp->current_page = 255;

    set_chartable_labels();
    updating = FALSE;
}

void
set_chartable_labels (void)
{
    GtkButton *button;
    gchar utf[7];
    gunichar wc;
    gint len, i;

    for (i=0; i<g_list_length (mainapp->buttons); i++)
    {
	wc = i + mainapp->current_page * 256;
        len = g_unichar_to_utf8 (wc, utf);

	switch (g_unichar_type (wc)) {
/*
	    case  G_UNICODE_CONTROL:
            case  G_UNICODE_FORMAT:
            case  G_UNICODE_UNASSIGNED:
*/
	    case  G_UNICODE_PRIVATE_USE:
/*
	    case  G_UNICODE_SURROGATE:
*/
	    case  G_UNICODE_LOWERCASE_LETTER:
            case  G_UNICODE_MODIFIER_LETTER:
            case  G_UNICODE_OTHER_LETTER:
            case  G_UNICODE_TITLECASE_LETTER:
            case  G_UNICODE_UPPERCASE_LETTER:
/*
	    case  G_UNICODE_COMBINING_MARK:
            case  G_UNICODE_ENCLOSING_MARK:
            case  G_UNICODE_NON_SPACING_MARK:
*/
	    case  G_UNICODE_DECIMAL_NUMBER:
            case  G_UNICODE_LETTER_NUMBER:
            case  G_UNICODE_OTHER_NUMBER:
            case  G_UNICODE_CONNECT_PUNCTUATION:
            case  G_UNICODE_DASH_PUNCTUATION:
            case  G_UNICODE_CLOSE_PUNCTUATION:
            case  G_UNICODE_FINAL_PUNCTUATION:
            case  G_UNICODE_INITIAL_PUNCTUATION:
            case  G_UNICODE_OTHER_PUNCTUATION:
            case  G_UNICODE_OPEN_PUNCTUATION:
            case  G_UNICODE_CURRENCY_SYMBOL:
            case  G_UNICODE_MODIFIER_SYMBOL:
            case  G_UNICODE_MATH_SYMBOL:
            case  G_UNICODE_OTHER_SYMBOL:
/*
	    case  G_UNICODE_LINE_SEPARATOR:
            case  G_UNICODE_PARAGRAPH_SEPARATOR:
            case  G_UNICODE_SPACE_SEPARATOR:
*/
		break;
            default:
		len = 0;
        }

        utf [len] = 0;

	button = GTK_BUTTON (g_list_nth_data (mainapp->buttons, i));
        gtk_button_set_label (button, utf);
    }
   
    main_app_set_font(mainapp, mainapp->font);
}
 
#endif /* _CALLBACKS_C_ */
