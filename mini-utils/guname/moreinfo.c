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


#include <gnome.h>
#include "info.h"
#include "mountlist.h"
#include "fsusage.h"
#include "moreinfo.h"

/* From the comp.lang.c FAQ */
#define MAX_ITOA_LEN ((sizeof(long) * CHAR_BIT + 2) / 3 + 1)  /* +1 for '-' */

/* blocks are 512 bytes */
#define BLOCKS_TO_MB(blocks) ( blocks / 2048 )

GList * filesystems = NULL;

/* External code can pretend this struct 
   is an array of strings, I hope. */
struct fs_information {
  gchar * array_part[end_filesystem_info];
  gdouble percent_full;
};

static GtkWidget * create_disk_box(const struct fs_information * fsi)
{
  GtkWidget * label;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * bar;
  GtkWidget * frame;
  gchar ** fs_info = fsi->array_part;

  frame = gtk_frame_new(NULL);
  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  if (fs_info[fs_description]) {
    hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    
    label = gtk_label_new(fs_info[fs_description]);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD_SMALL);
  }
  else {
    hbox = gtk_label_new(_("Unknown filesystem"));
  }
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, GNOME_PAD_SMALL);

    
  if (fs_info[fs_numbers]) {
    label = gtk_label_new(fs_info[fs_numbers]);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, GNOME_PAD_SMALL);
  }
  if (fs_info[fs_percent_full]) {
    hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
            
    label = gtk_label_new(fs_info[fs_percent_full]);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);

    bar = gtk_progress_bar_new();
    gtk_box_pack_end(GTK_BOX(hbox), bar, TRUE, TRUE, GNOME_PAD);
    
    gtk_progress_bar_update(GTK_PROGRESS_BAR(bar), fsi->percent_full);
  }
  else {
    hbox = gtk_label_new(_("No information for this filesystem."));
  }
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, GNOME_PAD_SMALL);

  return frame;
}

static void fill_disk_page(GtkWidget * box)
{
  GtkWidget * disk_box;
  GtkWidget * scrolled_win;
  GtkWidget * scrolled_box;
  GtkWidget * frame;
  GList * tmp;
  
  frame = gtk_frame_new(_("Mounted filesystems"));
  gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, 
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_border_width(GTK_CONTAINER(scrolled_win), GNOME_PAD);
  gtk_container_add(GTK_CONTAINER(frame), scrolled_win);

  scrolled_box = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
  gtk_container_border_width(GTK_CONTAINER(scrolled_box), GNOME_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(scrolled_win), scrolled_box);
  gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);
  
  
  tmp = filesystems;
  while ( tmp ) {
    disk_box = create_disk_box((struct fs_information *)(tmp->data));

    gtk_container_border_width(GTK_CONTAINER(disk_box), GNOME_PAD_SMALL);
    gtk_box_pack_start(GTK_BOX(scrolled_box), disk_box, FALSE, FALSE, 0);

    tmp = g_list_next(tmp);
  }
}

void display_moreinfo()
{
  static GtkWidget * dialog = NULL;
  GtkWidget * notebook;
  GtkWidget * clist;
  GtkWidget * vbox;

  if ( dialog == NULL ) {
    dialog = gnome_dialog_new(_("Detailed System Information"),
                              GNOME_STOCK_BUTTON_CLOSE,
                              NULL);
    gnome_dialog_close_hides(GNOME_DIALOG(dialog), TRUE);
    gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);
    /* Allow resizing */
    gtk_window_set_policy(GTK_WINDOW(dialog), TRUE, TRUE, FALSE);
    
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox),
                       notebook, TRUE, TRUE, GNOME_PAD);

    vbox = gtk_vbox_new(FALSE, GNOME_PAD);
    
    fill_disk_page(vbox);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
                             gtk_label_new(_("Disk Information")));
    
    gtk_widget_show_all(dialog);
  }
  else {
    if ( ! GTK_WIDGET_VISIBLE(dialog) ) gtk_widget_show(dialog);
  }
}

/********************************
  Load the information
  ****************************/

void load_fsinfo()
{
  struct fs_information * fsi;
  gchar ** fs_info;
  struct mount_entry * me, * tmp;
  const gchar * percent_full_format = _("%2d%% full ");
  const gchar * device_info_format = 
    _("%ld total megabytes, %ld (%ld superuser) free, %ld total inodes, %ld free inodes.");
  gchar * s;
  gint    percent;
  gint    len;
  struct fs_usage fu;

  me = read_filesystem_list(TRUE, TRUE);
  
  while ( me ) {
    fsi = g_new(struct fs_information, 1);
    filesystems = g_list_append(filesystems, fsi);

    fs_info = fsi->array_part;

    fs_info[fs_description] = 
      g_copy_strings(_("Mount Point: "), me->me_mountdir, 
                     _("  Device: "), me->me_devname,
                     _("  Filesystem Type: "), me->me_type,
                       NULL);
  
    if ( get_fs_usage(me->me_mountdir, me->me_devname, &fu) == 0 ) {
      if (fu.fsu_blocks == 0) {
        /* /proc or the like */
        fs_info[fs_numbers] = NULL;
        fs_info[fs_percent_full] = NULL;
      }
      else {
        len = strlen (device_info_format) + (MAX_ITOA_LEN * 5);
        s = g_malloc(len);
        g_snprintf(s, len, device_info_format, 
                   BLOCKS_TO_MB(fu.fsu_blocks),
                   BLOCKS_TO_MB(fu.fsu_bavail), 
                   BLOCKS_TO_MB(fu.fsu_bfree), 
                   fu.fsu_files,
                   fu.fsu_ffree);
        fs_info[fs_numbers] = s;

        fsi->percent_full =
          1.0 - ((gdouble)fu.fsu_bavail)/((gdouble)fu.fsu_blocks);

        percent = (gint)(fsi->percent_full * 100);
        
        len = strlen(percent_full_format); /* The format makes it big enough */
        s = g_malloc(len);
        g_snprintf(s, len, percent_full_format, percent);
        fs_info[fs_percent_full] = s;
      }
    }
    else {
      fs_info[fs_numbers] = NULL;
      fs_info[fs_percent_full] = NULL;
    }

    tmp = me;
    me = me->me_next;
    /* Hmm, I hope this is right */
    g_free(tmp->me_mountdir);
    g_free(tmp->me_devname);
    g_free(tmp->me_type);
    g_free(tmp);  
  }
}


void load_moreinfo()
{
  load_fsinfo();
}
