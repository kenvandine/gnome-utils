/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   guname: System information dialog.
 *
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com> except marquee code.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <config.h>
#include <gnome.h>

#include <sys/utsname.h>
#include <unistd.h>

#include "list.h"
#include "moreinfo.h"
#include "info.h"

#define APPNAME "guname"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#ifndef VERSION
#define VERSION "0.0.0"
#endif

/****************************
  Function prototypes
  ******************************/

static void popup_main_dialog();
static void save_callback(GtkWidget * menuitem, gpointer data);
static void mail_callback(GtkWidget * menuitem, gpointer data);
static void detailed_callback(GtkWidget * menuitem, gpointer data);
static gint list_clicked_cb(GtkCList * list, GdkEventButton * e);

static void write_to_filestream(FILE * f);

static void do_logo_box(GtkWidget * box);
static void do_list_box(GtkWidget * box);
static void do_popup(GtkWidget * clist);
static void do_marquee(GtkWidget * box);
static int marquee_timer (gpointer data);

/****************************
  Globals
  ************************/

GtkWidget * popup = NULL;

/*******************************
  Main
  *******************************/

int main ( int argc, char ** argv )
{
  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init (APPNAME, 0, argc, argv, 0, 0);

  load_system_info();
  load_moreinfo();

  popup_main_dialog();

  gtk_main();

  exit(EXIT_SUCCESS);
}

/**************************************
  Set up the GUI
  ******************************/

static void popup_main_dialog()
{
  GtkWidget * d;
  GtkWidget * logo_box;
  GtkWidget * list_box;

  d = gnome_dialog_new( _("System Information"), 
                        GNOME_STOCK_BUTTON_OK, NULL );

  gnome_dialog_set_close(GNOME_DIALOG(d), TRUE);
 
  logo_box = gtk_vbox_new(FALSE, GNOME_PAD);
  do_logo_box(logo_box);

  list_box = gtk_vbox_new(TRUE, GNOME_PAD);
  do_list_box(list_box);

  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(d)->vbox), logo_box,
                     FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(d)->vbox), list_box,
                     TRUE, TRUE, GNOME_PAD);

  gtk_signal_connect(GTK_OBJECT(d), "close",
                     GTK_SIGNAL_FUNC(gtk_main_quit),
                     NULL);

  gtk_window_position(GTK_WINDOW(d), GTK_WIN_POS_CENTER);
  gtk_widget_show_all(d);
}

static void do_logo_box(GtkWidget * box)
{
  GtkWidget * label, *contrib_label;
  GtkWidget * pixmap;
  GtkStyle * style;
  GdkFont * font;
  gchar * s;
  GtkWidget * marquee_frame, * align;

  s = gnome_pixmap_file ("gnome-default.png");

  if (s) {
    pixmap = gnome_pixmap_new_from_file(s);
    g_free(s);
  }
  if ( (s == NULL) || (pixmap == NULL) ) {
    pixmap = gtk_label_new(_("Logo file not found"));
  }

  /* Up here so it uses the default font */
  contrib_label = gtk_label_new(_("GNOME contributors:"));

  style = gtk_style_new ();
  font = gdk_font_load ("-Adobe-Helvetica-Medium-R-Normal--*-160-*-*-*-*-*-*");

  if (font) {
    gdk_font_unref (style->font);
    style->font = font;
  }

  if (style->font) {
    gtk_widget_push_style (style);
  }

  label = gtk_label_new(_("GNOME: The GNU Network Object Model Environment"));

  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(box), pixmap, TRUE, TRUE, GNOME_PAD);

  marquee_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(marquee_frame), GTK_SHADOW_IN);
  do_marquee(marquee_frame);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), marquee_frame);

  gtk_box_pack_start(GTK_BOX(box), contrib_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);

  if (style->font) {
    gtk_widget_pop_style();
  }
  else gtk_style_unref(style);
}

static void do_list_box(GtkWidget * box)
{
  GtkCList * list;
  const gchar * titles[] = { _("Category"), _("Your System") };

  list = GTK_CLIST(create_clist(titles));

  gtk_clist_freeze(list); /* does this matter if we haven't shown yet? */

  gtk_clist_set_border(list, GTK_SHADOW_OUT);
  /* Fixme, eventually you might could select an item 
     for cut and paste, or some other effect. */
  gtk_clist_set_selection_mode(list, GTK_SELECTION_BROWSE);
  gtk_clist_set_policy(list, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_clist_column_titles_passive(list);

  do_popup(GTK_WIDGET(list));
  gtk_widget_set_events(GTK_WIDGET(list), GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(list), "button_press_event",
                     GTK_SIGNAL_FUNC(list_clicked_cb), NULL);

  fill_clist(list, descriptions, info, end_system_info);

  gtk_clist_thaw(list);

  gtk_container_add(GTK_CONTAINER(box), GTK_WIDGET(list));
}

static void do_popup(GtkWidget * clist)
{
  GtkWidget * mi;

  popup = gtk_menu_new();
    
  mi = gnome_stock_menu_item(GNOME_STOCK_MENU_SAVE_AS,_("Save As..."));
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
                     GTK_SIGNAL_FUNC(save_callback), NULL);
  gtk_menu_append(GTK_MENU(popup), mi);
  gtk_widget_show(mi);

  mi = gnome_stock_menu_item(GNOME_STOCK_MENU_MAIL_SND,_("Mail To..."));
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
                     GTK_SIGNAL_FUNC(mail_callback), NULL);
  gtk_menu_append(GTK_MENU(popup), mi);
  gtk_widget_show(mi);  

  mi = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK,
                             _("Detailed Information..."));
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
                     GTK_SIGNAL_FUNC(detailed_callback), NULL);
  gtk_menu_append(GTK_MENU(popup), mi);
  gtk_widget_show(mi);  
}

/**********************************
  Callbacks
  *******************************/
static gint list_clicked_cb(GtkCList * list, GdkEventButton * e)
{
  if (e->button == 1) {
    /* Ignore button 1 */
    return FALSE; 
  }

  /* don't change the CList selection. */
  gtk_signal_emit_stop_by_name (GTK_OBJECT (list), "button_press_event");

  gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL,
                 NULL, e->button, time(NULL));
  return TRUE; 
}

static void file_selection_cb(GtkWidget * button, gpointer fs)
{
  gchar * fn;
  FILE * f;

  fn = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
  gtk_widget_hide(fs);

  if (fn) {
    f = fopen(fn, "w");
    if (f) {
      write_to_filestream(f);
      fclose(f);
    }
    else {
      gchar * s = g_copy_strings(_("Couldn't open file `"), fn, "': ", 
                                 g_unix_error_string(errno));
      gnome_error_dialog(s);
      g_free(s);
    }
  }
  /* I think this frees fn */
  gtk_widget_destroy(fs);
}

static void save_callback(GtkWidget * menuitem, gpointer data)
{
  GtkWidget * fs;

  fs = gtk_file_selection_new(_("Save System Information As..."));
  
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), "clicked",
                     GTK_SIGNAL_FUNC(file_selection_cb), fs);

  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(fs));
  
  gtk_widget_show(fs);
}

static void mailx_callback(gchar * string, gpointer data)
{
  FILE * p;
  gchar * command;
  gint failure;
  if (string == NULL) return;
  
  /* This isn't translated; maybe good, since most tech support email should be
     in English? don't know. */
  command = g_copy_strings("mailx -s \"System Information for host ", 
                           info[si_host] ? info[si_host] : "Unknown",
                           "\" ", string, NULL);
  
  p = popen(command, "w");

  if (p) {
    write_to_filestream(p);
    failure = pclose(p);
    if (failure) {
      /* I don't think the error_string() will reliably mean anything. */
      gchar * s = g_copy_strings(_("Command failed `"), command, "': ", 
                                 g_unix_error_string(errno));
      gnome_error_dialog(s);
      g_free(s);
    }
  }
  else {
    gchar * t = g_copy_strings(_("Couldn't run command `"), command, "': ", 
                               g_unix_error_string(errno));
    gnome_error_dialog(t);
    g_free(t);
  }

  g_free(command);
  g_free(string);
}

static void mail_callback(GtkWidget * menuitem, gpointer data)
{
  gnome_request_string_dialog(_("Address to mail to:"), mailx_callback, NULL);
}

static void detailed_callback(GtkWidget * menuitem, gpointer data)
{
  display_moreinfo();
}

/*********************************************
  Non-GUI
  ***********************/

static void write_to_filestream(FILE * f)
{
  gint i = 0;
  while ( i < end_system_info ) {
    if (info[i] == NULL) {
      /* No information on this. */
      ;
    }
    else {
      fprintf (f, "%-30s %s\n", descriptions[i], info[i]);
    }
    ++i;
  }  
}

/***************************** 
  Marquee thingy - probably there should be a widget to do this.  Code
  taken from the GIMP, about_dialog.c, Copyright Spencer Kimball and
  Peter Mattis.
  ***********************************/

static const gchar * scroll_text[] = {
  "John Doe",
  "Doe Jane",
  "Your Name Here",
  "Yet Another Name",
  "Wanda the GNOME Fish"
};

static int nscroll_texts = sizeof (scroll_text) / sizeof (scroll_text[0]);
static int scroll_text_widths[100] = { 0 };
static int cur_scroll_text = 0;
static int cur_scroll_index = 0;

static int shuffle_array[ sizeof(scroll_text) / sizeof(scroll_text[0]) ];

static GtkWidget *scroll_area = NULL;
static GdkPixmap *scroll_pixmap = NULL;
static int do_scrolling = TRUE;
static int scroll_state = 0;
static int offset = 0;
static int timer = 0;

static gint marquee_expose (GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
  if (!timer) {
    marquee_timer(NULL);
    timer = gtk_timeout_add (75, marquee_timer, NULL);
  }
  return FALSE;
}

static void do_marquee(GtkWidget * box)
{
  int i; 
  int max_width;

  max_width = 0;
  for (i = 0; i < nscroll_texts; i++) {
    scroll_text_widths[i] = gdk_string_width (box->style->font, scroll_text[i]);
    max_width = MAX (max_width, scroll_text_widths[i]);
  }
  
  scroll_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (scroll_area),
                         max_width + 10,
                         box->style->font->ascent +
                         box->style->font->descent + 5 ); /* 5 is for border. */
  gtk_container_add (GTK_CONTAINER (box), scroll_area);

  for (i = 0; i < nscroll_texts; i++) {
    shuffle_array[i] = i;
  }
  
  for (i = 0; i < nscroll_texts; i++) {
    int j, k;
    j = rand() % nscroll_texts;
    k = rand() % nscroll_texts;
    if (j != k) 
      {
        int t;
        t = shuffle_array[j];
        shuffle_array[j] = shuffle_array[k];
        shuffle_array[k] = t;
      }
  }

  gtk_widget_set_events(scroll_area, GDK_EXPOSURE_MASK);
  gtk_signal_connect(GTK_OBJECT(scroll_area), "expose_event",
                     GTK_SIGNAL_FUNC(marquee_expose), NULL);
}

static int marquee_timer (gpointer data)
{
  gint return_val;

  return_val = TRUE;

  if (do_scrolling)
    {
      if (!scroll_pixmap)
        scroll_pixmap = gdk_pixmap_new (scroll_area->window,
                                        scroll_area->allocation.width,
                                        scroll_area->allocation.height,
                                        -1);
  
      switch (scroll_state)
        {
        case 1:
          scroll_state = 2;
          timer = gtk_timeout_add (700, marquee_timer, NULL);
          return_val = FALSE;
          break;
        case 2:
          scroll_state = 3;
          timer = gtk_timeout_add (75, marquee_timer, NULL);
          return_val = FALSE;
          break;
        }

      if (offset > (scroll_text_widths[cur_scroll_text] + scroll_area->allocation.width))
        {
          scroll_state = 0;
          cur_scroll_index += 1;
          if (cur_scroll_index == nscroll_texts)
            cur_scroll_index = 0;
	  
          cur_scroll_text = shuffle_array[cur_scroll_index];

          offset = 0;
        }

      gdk_draw_rectangle (scroll_pixmap,
                          scroll_area->style->white_gc,
                          TRUE, 0, 0,
                          scroll_area->allocation.width,
                          scroll_area->allocation.height);
      gdk_draw_string (scroll_pixmap,
                       scroll_area->style->font,
                       scroll_area->style->black_gc,
                       scroll_area->allocation.width - offset,
                       scroll_area->style->font->ascent + 2, /* 2 from top */
                       scroll_text[cur_scroll_text]);
      gdk_draw_pixmap (scroll_area->window,
                       scroll_area->style->black_gc,
                       scroll_pixmap, 0, 0, 0, 0,
                       scroll_area->allocation.width,
                       scroll_area->allocation.height);

      offset += 15;
      if (scroll_state == 0)
        {
          if (offset > ((scroll_area->allocation.width + scroll_text_widths[cur_scroll_text]) / 2))
            {
              scroll_state = 1;
              offset = (scroll_area->allocation.width + scroll_text_widths[cur_scroll_text]) / 2;
            }
        }
    }

  return return_val;
}
