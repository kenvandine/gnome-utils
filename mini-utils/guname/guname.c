/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   guname: System information dialog. Does lots more than uname on 
 *    Linux, on any Posix system should do at least something though.
 *    It shouldn't be so Linux-centric; if you have another system,
 *    please add the appropriate stuff.
 *
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
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
#include <string.h> /* memset */

#define APPNAME "guname"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#define DEBIAN_STRING "Debian GNU/Linux"
#define REDHAT_STRING "Red Hat Linux"

/****************************
  Function prototypes
  ******************************/

static void popup_main_dialog();
static void save_callback(GtkWidget * menuitem, gpointer data);
static void mail_callback(GtkWidget * menuitem, gpointer data);
static gint list_clicked_cb(GtkCList * list, GdkEventButton * e);

static void write_to_filestream(FILE * f);

static void do_logo_box(GtkWidget * box);
static void do_list_box(GtkWidget * box);
static void do_popup(GtkWidget * clist);

static void get_system_info();
static void get_portable_info();
static void get_uptime();
static void get_linux_info();
/* Add your own system! Whee! */

/***********************************
  Types
  ***********************************/

typedef enum {
  di_device,
  di_size,
  di_used,
  di_free,
  di_percent,
  di_mountpoint,

  end_disk_info
} disk_info;

/* All of these won't apply to all systems. */
typedef enum {
  si_distribution, /* Debian, Solaris, etc. */
  si_OS,           /* from uname. */
  si_distribution_version, 
  si_OS_version,
  si_release,

  si_CPU_type,
  si_CPU_speed,

  si_host,
  si_domain,

  si_user,
  si_display,

  si_uptime,

  si_mem,          
  si_mem_swap,
  si_mem_total,     
  si_mem_total_free,

  end_system_info
} system_info;

/************************************
  Globals
  ***********************************/

const gchar * descriptions[] = {
  N_("Distribution:"), 
  N_("Operating System:"),
  N_("Distribution Version:"),
  N_("Operating System Version:"),
  N_("Operating System Release:"),
  N_("Processor Type:"),
  N_("Processor Speed:"),
  N_("Host Name:"),
  N_("Domain Name:"),
  N_("User Name:"),
  N_("X Display Name:"),
  N_("System Status:"),
  N_("Real Memory:"),
  N_("Swap Space (\"virtual memory\"):"),
  N_("Total Memory:"),
  N_("Free Memory:")
};

const gchar * disk_descriptions[] = {
  N_("Device"),
  N_("Size"),
  N_("Used space"),
  N_("Free space"),
  N_("Percent free"),
  N_("Mount point")
};

const gchar * info[end_system_info];
GList * disks = NULL;
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

  get_system_info();

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

  gtk_signal_connect(GTK_OBJECT(d), "clicked",
                     GTK_SIGNAL_FUNC(gtk_main_quit),
                     NULL);

  gtk_widget_show_all(d);
}

static void do_logo_box(GtkWidget * box)
{
  GtkWidget * label;
  GtkWidget * pixmap;
  gchar * s;

  /*  gtk_widget_set_usize(box, , 150); */

  s = gnome_pixmap_file ("gnome-default.png");

  if (s) {
    pixmap = gnome_pixmap_new_from_file(s);
    g_free(s);
  }
  if ( (s == NULL) || (pixmap == NULL) ) {
    pixmap = gtk_label_new(_("Logo file not found"));
  }

  label = gtk_label_new(_("GNOME: The GNU Network Object Model Environment"));

  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(box), pixmap, TRUE, TRUE, GNOME_PAD);
}

/* Want a function not a macro, so a and b are calculated only once */
static gint max(gint a, gint b)
{
  if ( a > b )
    return a;
  else return b;
}

static void do_list_box(GtkWidget * box)
{
  GtkCList * list;
  const gchar * row[2];
  int i;
  gchar * titles[] = { _("Category"), _("Your System") };
  gint col_zero_width, col_one_width;
  GdkFont * font;

  list = GTK_CLIST(gtk_clist_new_with_titles(2, titles));

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

  font = gtk_widget_get_style(GTK_WIDGET(list))->font;

  i = 0; col_zero_width = 0; col_one_width = 0;
  while ( i < end_system_info ) {

    if ( info[i] == NULL ) {
      /* Don't have this information. */
      ++i;
      continue; 
    }

    row[0] = descriptions[i];
    row[1] = info[i];
    gtk_clist_append(list, (gchar **)row);

    /* If the string is longer than any previous ones,
       increase the column width */
    
    col_zero_width = max ( gdk_string_width(font, row[0]), col_zero_width );
    col_one_width =  max ( gdk_string_width(font, row[1]), col_one_width );

    ++i;
  }

  /* The first column is a little wider than the largest string, so 
     it's not too close to the second column. */
  gtk_clist_set_column_width(list, 0, col_zero_width + 10);
  gtk_clist_set_column_width(list, 1, col_one_width);
  
  gtk_widget_set_usize(GTK_WIDGET(list), col_zero_width+col_one_width + 30, 
                       200);

  gtk_clist_thaw(list);

  gtk_container_add(GTK_CONTAINER(box), GTK_WIDGET(list));
}

static void do_popup(GtkWidget * clist)
{
  GtkWidget * mi;

  popup = gtk_menu_new();
    
  mi = gtk_menu_item_new_with_label(_("Save As..."));
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
                     GTK_SIGNAL_FUNC(save_callback), NULL);
  gtk_menu_append(GTK_MENU(popup), mi);
  gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label(_("Mail To..."));
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
                     GTK_SIGNAL_FUNC(mail_callback), NULL);
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

/*********************************************
  Non-GUI
  ***********************/

static void get_system_info()
{
  /* Set all the pointers in the array to NULL */
  memset(&info, '\0', sizeof(info));
  
  get_portable_info();

  get_linux_info();
}

static void get_portable_info()
{
  struct utsname buf;

  if ( uname(&buf) == -1 ) {
    g_error("uname() failed, %s\n", g_unix_error_string(errno));
  }

  info[si_OS] = g_strdup(buf.sysname);
  info[si_release] = g_strdup(buf.release);
  info[si_OS_version] = g_strdup(buf.version);
  info[si_CPU_type] = g_strdup(buf.machine);
  info[si_host] = g_strdup(buf.nodename);
  /*  info[si_domain] = g_strdup(buf.domainname); */ /* man page wrong? */

  info[si_user] = g_strdup(getlogin());

  info[si_display] = g_strdup(getenv("DISPLAY")); /* hmm */

  get_uptime();
  
  /* Add more later */

}

static void get_linux_info()
{
  /* Identify distribution (really this could be compiled in) */
  if (g_file_exists("/etc/debian_version")) {
    FILE * f;
    gchar buf[10];

    info[si_distribution] = g_strdup(DEBIAN_STRING);
    f = fopen("/etc/debian_version", "r");
    if (f) { 
      fscanf(f, "%8s", buf);
      info[si_distribution_version] = g_strdup(buf);
      fclose(f);
    }
  }
  /* OK, people using other dists will need to add theirs, I have
     no idea how to identify a Red Hat system. */
  else {
    /* This is about the only case where we'll put Unknown instead
       of simply not showing the info. */
    info[si_distribution_version] = g_strdup(_("Unknown"));
  }

  /* More to come. */

}

static void get_uptime()
{
  FILE * f;
  static const gint bufsize = 80;
  gchar buffer[bufsize + 1];
  gint chars_read;
  int i = 0;

  memset(buffer, '\0', bufsize + 1);

  f = popen("uptime", "r");

  if (f) {
    chars_read = fread(buffer, sizeof(char), bufsize, f);

    if ( chars_read > 0 ) {

      /* Strip leading whitespace from uptime results, looks nicer */
      while ( (buffer[i] == ' ') && (buffer[i] != '\0') ) {
        ++i;
      }

      info[si_uptime] = g_strdup(&buffer[i]);
    }

    pclose(f);
  }
}

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
