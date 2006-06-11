/* gdict-print.c - print-related helper functions
 *
 * This file is part of GNOME Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <gconf/gconf-client.h>

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprint/gnome-print-pango.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>

#include <libgdict/gdict.h>

#include "gdict-pref-dialog.h"
#include "gdict-print.h"

static void
get_page_margins (GnomePrintConfig *config,
		  gdouble          *top_margin,
		  gdouble          *right_margin,
		  gdouble          *bottom_margin,
		  gdouble          *left_margin)
{
  g_assert (config != NULL);

  gnome_print_config_get_length (config,
  				 (const guchar *) GNOME_PRINT_KEY_PAGE_MARGIN_TOP,
  				 top_margin,
  				 NULL);
  gnome_print_config_get_length (config,
  				 (const guchar *) GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT,
  				 right_margin,
  				 NULL);
  gnome_print_config_get_length (config,
  				 (const guchar *) GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM,
  				 bottom_margin,
  				 NULL);
  gnome_print_config_get_length (config,
  				 (const guchar *) GNOME_PRINT_KEY_PAGE_MARGIN_LEFT,
  				 left_margin,
  				 NULL);
}

static gchar *
get_print_font (void)
{
  GConfClient *client;
  gchar *print_font;
  
  client = gconf_client_get_default ();
  print_font = gconf_client_get_string (client,
  					GDICT_GCONF_PRINT_FONT_KEY,
  					NULL);
  if (!print_font)
    print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  g_object_unref (client);
  
  return print_font;
}

/* FIXME - add copies and collate support */
static gboolean
prepare_print (GdictDefbox   *defbox,
	       GnomePrintJob *job,
	       gint           n_copies,
	       gint           collate)
{
  GnomePrintContext *pc;
  GnomePrintConfig *config;
  gchar *print_font;
  PangoLayout *layout;
  PangoFontDescription *font_desc;
  gdouble page_width, page_height;
  gdouble top_margin, right_margin, bottom_margin, left_margin;
  gint lines_per_page, total_lines, written_lines, remaining_lines;
  gint line_width;
  gint num_pages;
  gint x, y, i;
  gchar *text, *header_line;
  gchar **text_lines;
  gsize length;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  g_assert (job != NULL);
  
  pc = gnome_print_job_get_context (job);
  if (!pc)
    {
      g_warning ("Unable to create a GnomePrintContext for the job\n");
      
      return FALSE;
    }
  
  /* take the defbox text, and break it up into lines */
  total_lines = 0;
  text = gdict_defbox_get_text (defbox, &length);
  if (!text)
    {
      g_object_unref (pc);
      
      return TRUE;
    }
  
  text_lines = g_strsplit (text, "\n", -1);
  header_line = text_lines[0];
  remaining_lines = total_lines = g_strv_length (text_lines);
  
  layout = gnome_print_pango_create_layout (pc);

  print_font = get_print_font ();   
  font_desc = pango_font_description_from_string (print_font);
  line_width = pango_font_description_get_size (font_desc) / PANGO_SCALE;
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);
  
  config = gnome_print_job_get_config (job);
  gnome_print_job_get_page_size_from_config (config,
                                             &page_width,
                                             &page_height);
  get_page_margins (config, &top_margin, &right_margin, &bottom_margin, &left_margin);
  
  pango_layout_set_width (layout, (page_width - left_margin - right_margin) * PANGO_SCALE);
  
  lines_per_page = (page_height - top_margin - bottom_margin) / line_width;

  /* take into account that we are printing the header (word + page, line)
   * and the footer (line), so update the real number of lines per page before
   * using it to compute the number of pages.
   */
  lines_per_page -= 6;
  
  num_pages = ceil ((float) total_lines / (float) lines_per_page);

  /* the first line of the buffer is the header, and will be printed on each
   * page, so we skip it
   */
  written_lines = 1;
  x = left_margin;
  for (i = 0; i < num_pages; i++)
    {
      gchar *page;
      gchar *header;
      gint j;
      
      if (written_lines == total_lines)
        break;
      
      page = g_strdup_printf ("%d", i + 1);
      gnome_print_beginpage (pc, (const guchar *) page);
      
      y = page_height - top_margin;
      
      /* header (tile + line) */
      gnome_print_moveto (pc, x, y);
      
      header = g_strdup_printf (_("%s (page %d)"), header_line, i + 1);
      pango_layout_set_text (layout, header, -1);
      gnome_print_pango_layout (pc, layout);
      
      y -= 2 * line_width;
      
      gnome_print_moveto (pc, x, y);
      gnome_print_lineto (pc, x + (page_width - right_margin - left_margin), y);
      gnome_print_stroke (pc);
      
      y -= 2 * line_width;
      
      for (j = 0; j < lines_per_page; j++)
        {
          gchar *line = text_lines[written_lines++];
          
          if (!line)
            break;
          
          gnome_print_moveto (pc, x, y);
          
          pango_layout_set_text (layout, line, -1);
          gnome_print_pango_layout (pc, layout);
          
          remaining_lines -= 1;
          y -= line_width;
        }
      
      /* footer (line) */
      y = bottom_margin + line_width;
      
      gnome_print_moveto (pc, x, y);
      gnome_print_lineto (pc, x + (page_width - right_margin - left_margin), y);
      gnome_print_stroke (pc);
      
      g_free (page);
      g_free (header);
      
      gnome_print_showpage (pc);
    }
  
  g_free (print_font);
  
  g_object_unref (pc);

  g_strfreev (text_lines);
  g_free (text);
  
  gnome_print_job_close (job);
  
  return TRUE;
}

static void
show_print_preview (GtkWindow     *parent,
		    GnomePrintJob *job)
{
  GtkWidget *preview;
  
  preview = gnome_print_job_preview_new (job, (const guchar *) _("Print Preview"));
  gtk_window_set_transient_for (GTK_WINDOW (preview), parent);
  
  gtk_widget_show (preview);
}

static void
gdict_print_dialog_response_cb (GtkWidget *widget,
				gint       response_id,
				gpointer   user_data)
{
  GdictDefbox *defbox;
  GnomePrintJob *job;
  
  defbox = g_object_get_data (G_OBJECT (widget), "defbox");
  job = g_object_get_data (G_OBJECT (widget), "print-job");
  
  switch (response_id)
    {
    case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
      {
      gint copies, collate;
      
      gnome_print_dialog_get_copies (GNOME_PRINT_DIALOG (widget),
      				     &copies,
      				     &collate);
      
      if (prepare_print (defbox, job, copies, collate))
        gnome_print_job_print (job);
      }
      break;
    case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
      if (prepare_print (defbox, job, 1, 0));
        show_print_preview (GTK_WINDOW (widget), job);
      break;
    case GNOME_PRINT_DIALOG_RESPONSE_CANCEL:
    default:
      break;
    }
}

void
gdict_show_print_dialog (GtkWindow   *parent,
			 const gchar *title,
			 GdictDefbox *defbox)
{
  GtkWidget *print_dialog;
  GnomePrintJob *job;
  
  g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  job = gnome_print_job_new (NULL);
  
  print_dialog = gnome_print_dialog_new (job, (const guchar *) title, 0);
  gtk_window_set_transient_for (GTK_WINDOW (print_dialog),
  				parent);
 
  g_object_set_data (G_OBJECT (print_dialog), "defbox", defbox);
  g_object_set_data (G_OBJECT (print_dialog), "print-job", job);
  
  g_signal_connect (print_dialog, "response",
      		    G_CALLBACK (gdict_print_dialog_response_cb),
      		    parent);
  
  gtk_dialog_run (GTK_DIALOG (print_dialog));
  
  gtk_widget_destroy (print_dialog);
  
  g_object_unref (job);
}
