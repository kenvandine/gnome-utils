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
#include "config.h"
#include "info.h"
#include "mountlist.h"
#include "fsusage.h"
#include "moreinfo.h"
#include "list.h"
#include <glibtop.h>
#define GLIBTOP_NAMES 1 /* hmm, no, this is wrong. hmm. */
#include <glibtop/mem.h>
#include <glibtop/swap.h>

/* From the comp.lang.c FAQ */
#define MAX_ITOA_LEN ((sizeof(long) * CHAR_BIT + 2) / 3 + 1)  /* +1 for '-' */

/* blocks are 512 bytes */
#define BLOCKS_TO_MB(blocks) ( blocks / 2048 )

GList * filesystems = NULL;
GList * filesystems_percent_full = NULL;
gchar ** memory = NULL;
gchar ** memory_descriptions = NULL;

gdouble memory_percent_full;
gdouble swap_percent_full;

static GtkWidget * create_disk_box(const gchar ** fs_info, gdouble * percent_full)
{
  GtkWidget * label;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * bar;
  GtkWidget * frame;

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
  if (fs_info[fs_percent_full] && percent_full) {
    hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
            
    label = gtk_label_new(fs_info[fs_percent_full]);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);

    bar = gtk_progress_bar_new();
    gtk_box_pack_end(GTK_BOX(hbox), bar, TRUE, TRUE, GNOME_PAD);
    
    gtk_progress_bar_update(GTK_PROGRESS_BAR(bar), *percent_full);
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
  GList * tmp1, *tmp2;
  
  frame = gtk_frame_new(_("Mounted filesystems"));

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
  
  
  tmp1 = filesystems;
  tmp2 = filesystems_percent_full;
  while ( tmp1 ) {
    disk_box = create_disk_box((const gchar**)tmp1->data, 
                               (gdouble *)tmp2->data);

    gtk_container_border_width(GTK_CONTAINER(disk_box), GNOME_PAD_SMALL);
    gtk_box_pack_start(GTK_BOX(scrolled_box), disk_box, FALSE, FALSE, 0);

    tmp1 = g_list_next(tmp1);
    tmp2 = g_list_next(tmp2);
  }
}

static void fill_mem_page(GtkWidget * box)
{
  GtkWidget * clist;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * bar;
  GtkWidget * label;
  const gchar * titles[] = { _("Memory"), _("Megabytes") };
  gchar * s;
  gint len;
  static const gchar * format = _("%ld%% %s used.");

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_add(GTK_CONTAINER(box), vbox);

  clist = create_clist(titles);
  gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, GNOME_PAD);
  gtk_clist_freeze(GTK_CLIST(clist));
  fill_clist(GTK_CLIST(clist), memory_descriptions, 
             memory, end_memory_info);
  gtk_clist_thaw(GTK_CLIST(clist));

  len = strlen(format) + MAX_ITOA_LEN + strlen("memory");
  s = g_malloc(len);
  g_snprintf(s, len, format, 
             (guint)(memory_percent_full * 100),
             _("memory"));
  hbox = gtk_hbox_new(FALSE, GNOME_PAD);
  label = gtk_label_new(s);
  g_free(s);
  bar = gtk_progress_bar_new();
  gtk_progress_bar_update(GTK_PROGRESS_BAR(bar), memory_percent_full);
  gtk_box_pack_start (GTK_BOX(hbox), bar,   FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX(vbox), hbox,  FALSE, FALSE, GNOME_PAD);

  len = strlen(format) + MAX_ITOA_LEN + strlen("swap");
  s = g_malloc(len);
  g_snprintf(s, len, format, 
             (guint)(swap_percent_full * 100),
             _("swap"));
  hbox = gtk_hbox_new(FALSE, GNOME_PAD);
  label = gtk_label_new(s);
  g_free(s);
  bar = gtk_progress_bar_new();
  gtk_progress_bar_update(GTK_PROGRESS_BAR(bar), swap_percent_full);
  gtk_box_pack_start (GTK_BOX(hbox), bar,   FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX(vbox), hbox,  FALSE, FALSE, GNOME_PAD);
}

static void create_page(GtkWidget * notebook, 
                        void (* fill_func)(GtkWidget *),
                        const gchar * label)
{
  GtkWidget * vbox;

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD);
  (*fill_func)(vbox);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
                           gtk_label_new(label));
}

void display_moreinfo()
{
  static GtkWidget * dialog = NULL;
  GtkWidget * notebook;

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

    if (filesystems) 
      create_page(notebook, fill_disk_page, _("Disk Information"));
    if (memory)
      create_page(notebook, fill_mem_page, _("Memory Information"));

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
  gchar ** fs_info;
  gdouble * percent_full;
  struct mount_entry * me, * tmp;
  const gchar * percent_full_format = _("%2d%% full ");
  const gchar * device_info_format = 
    _("%ld megabytes, %ld free (%ld superuser); %ld inodes, %ld free.");
  gchar * s;
  gint    percent;
  gint    len;
  struct fs_usage fu;

  me = read_filesystem_list(TRUE, TRUE);
  
  while ( me ) {
    fs_info = g_malloc(sizeof(gchar *) * end_filesystem_info);
    filesystems = g_list_append(filesystems, fs_info);
    percent_full = NULL;

    fs_info[fs_description] = 
      g_copy_strings(_("Mount Point: "), me->me_mountdir, 
                     _("    Device: "), me->me_devname,
                     _("    Filesystem Type: "), me->me_type,
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

        percent_full = g_malloc(sizeof(gdouble));
        *percent_full =
          1.0 - ((gdouble)fu.fsu_bavail)/((gdouble)fu.fsu_blocks);
        
        percent = (gint)((*percent_full) * 100);
        
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

    filesystems_percent_full = 
      g_list_append(filesystems_percent_full, percent_full);

    tmp = me;
    me = me->me_next;
    /* Hmm, I hope this is right */
    g_free(tmp->me_mountdir);
    g_free(tmp->me_devname);
    g_free(tmp->me_type);
    g_free(tmp);  
  }
}

static gchar *
memsize_string(unsigned long kb)
{
  unsigned long mb = kb/1024;
  gchar * s;

  s = g_malloc(MAX_ITOA_LEN + 1);
  g_snprintf(s, MAX_ITOA_LEN, "%ld", mb);
  return s;
}

static void 
add_memory_info(unsigned long glibtop_value, 
                gint glibtop_define,
                memory_info mi)
{
  memory[mi] = memsize_string(glibtop_value);
  memory_descriptions[mi] = 
    g_strdup(glibtop_labels_mem[glibtop_define]);
  /*        glibtop_descriptions_mem[glibtop_define] */
}

static void
add_swap_info(unsigned long glibtop_value, 
              gint glibtop_define,
              memory_info mi)
{
  memory[mi] = memsize_string(glibtop_value);
  memory_descriptions[mi] = 
    g_strdup(glibtop_labels_swap[glibtop_define]);
  /*                     glibtop_descriptions_swap[glibtop_define]) */
}

void load_meminfo()
{
#ifdef HAVE_LIBGTOP
  glibtop_mem membuf;
  glibtop_swap swapbuf;

  memory              = g_malloc(sizeof(gchar *) * end_memory_info);
  memory_descriptions = g_malloc(sizeof(gchar *) * end_memory_info);

  glibtop_get_mem(&membuf);
  glibtop_get_swap(&swapbuf);

  memory_percent_full = ((gdouble)membuf.used)/((gdouble)membuf.total);
  swap_percent_full = ((gdouble)swapbuf.used)/((gdouble)swapbuf.total);

  /* It just doesn't quite work as a loop. Sigh. 
     Maybe with pointer math over the membuf... nah. */
  add_memory_info(membuf.total, GLIBTOP_MEM_TOTAL, mem_total);
  add_memory_info(membuf.used,  GLIBTOP_MEM_USED,  mem_used);
  add_memory_info(membuf.free,  GLIBTOP_MEM_FREE,  mem_free);
  add_memory_info(membuf.shared, GLIBTOP_MEM_SHARED, mem_shared);
  add_memory_info(membuf.buffer, GLIBTOP_MEM_BUFFER, mem_buffer);
  add_memory_info(membuf.cached, GLIBTOP_MEM_CACHED, mem_cached);
  add_memory_info(membuf.user,   GLIBTOP_MEM_USER,   mem_user);

  add_swap_info(swapbuf.total, GLIBTOP_SWAP_TOTAL,   mem_swap_total);
  add_swap_info(swapbuf.used,  GLIBTOP_SWAP_USED,    mem_swap_used);
  add_swap_info(swapbuf.free,  GLIBTOP_SWAP_FREE,    mem_swap_free);

#endif
}

void load_moreinfo()
{
  load_fsinfo();
  load_meminfo();
}
