/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   edit-menus: Gnome app for editing window manager, panel menus.
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

#include "iconbrowse.h"
#include "is_image.h"

#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <limits.h>

static void icon_browse_class_init (IconBrowseClass * ib_class);
static void icon_browse_init (IconBrowse * ib);

static void destroy (GtkObject * ib);
static void ok_clicked (GtkWidget * ib);

static void icon_clicked (GtkWidget * button, IconBrowse * ib);

static GSList * filenames_from_dir (const gchar * dir);

static void icon_chosen_marshaller (GtkObject      *object,
				    GtkSignalFunc   func,
				    gpointer        func_data,
				    GtkArg         *params);

guint icon_browse_get_type (void)
{
  static guint ib_type = 0;

  if (!ib_type) {
    GtkTypeInfo ib_info = 
    {
      "IconBrowse",
      sizeof (IconBrowse),
      sizeof (IconBrowseClass),
      (GtkClassInitFunc) icon_browse_class_init,
      (GtkObjectInitFunc) icon_browse_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    ib_type = gtk_type_unique (gtk_dialog_get_type(), &ib_info);
  }

  return ib_type;
}

enum {
  ICON_CHOSEN_SIGNAL,
  LAST_SIGNAL
};

static gint ib_signals[LAST_SIGNAL] = { 0 };

/* Maybe won't end up needing this, consider deleting */
static GtkDialogClass * parent_class = NULL;

static void icon_browse_class_init (IconBrowseClass * ib_class)
{
  GtkObjectClass * object_class;
  
  object_class = GTK_OBJECT_CLASS (ib_class);
  parent_class = gtk_type_class (gtk_dialog_get_type ());

  ib_signals[ICON_CHOSEN_SIGNAL] = 
    gtk_signal_new ("icon_chosen",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (IconBrowseClass, icon_chosen),
		    icon_chosen_marshaller,
		    GTK_TYPE_NONE,
		    1, GTK_TYPE_POINTER );

  gtk_object_class_add_signals (object_class, ib_signals, LAST_SIGNAL);

  object_class->destroy = destroy; 
  ib_class->icon_chosen = 0;

  return;
}

static void icon_browse_init (IconBrowse * ib)
{
  GtkWidget * button;

  ib->free_me = 0;
  ib->last_clicked = 0;
  ib->using_gnome_dir = FALSE;

  gtk_signal_connect_object (GTK_OBJECT(ib), "delete_event", 
                             GTK_SIGNAL_FUNC(gtk_widget_destroy), 
                             GTK_OBJECT(ib));
  
  button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
  gtk_box_pack_start ( GTK_BOX(GTK_DIALOG(ib)->action_area),
		       button,
		       TRUE, TRUE, 10 );

  gtk_signal_connect_object (GTK_OBJECT(button), "clicked", 
                             GTK_SIGNAL_FUNC(ok_clicked), 
                             GTK_OBJECT(ib));
  gtk_widget_show(button);
		       
  button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
  gtk_box_pack_start ( GTK_BOX(GTK_DIALOG(ib)->action_area),
		       button,
		       TRUE, TRUE, 10 );

  gtk_signal_connect_object (GTK_OBJECT(button), "clicked", 
                             GTK_SIGNAL_FUNC(gtk_widget_destroy), 
                             GTK_OBJECT(ib));
  gtk_widget_show(button);

  ib->label = gtk_label_new("No Selection");

  gtk_box_pack_end ( GTK_BOX(GTK_DIALOG(ib)->vbox),
		     ib->label,
		     FALSE, FALSE, 10 );
  gtk_widget_show(ib->label);

  gtk_box_set_homogeneous ( GTK_BOX(GTK_DIALOG(ib)->vbox), FALSE );
}

typedef struct {
  GnomePixmap * pixmap;
  gchar * name;
} pair;

#define TITLE_FORMAT "Select %s"

static GtkWidget * icon_browse_new_from_pairs (const gchar * what,
					       GSList * pairs)
{
  gchar * buf;
  GtkWidget * pixmap;
  GtkWidget * button;
  GtkWidget * table;
  GtkWidget * scrolled_win;
  gchar * name;
  gint num_icons, row, col, num_rows, num_cols;
  GSList * ib_and_name, * first;
  GtkWidget * ib;
  GSList * free_me;

  ib = GTK_WIDGET( gtk_type_new (icon_browse_get_type()) );

  free_me = ICON_BROWSE(ib)->free_me;

  num_icons = g_slist_length(pairs);
  first = pairs;

  if (what == 0) what = "an icon";
  
  num_cols = 4;
  num_rows = (num_icons / num_cols) + ( num_icons % num_cols ? 1 : 0 );

  gtk_widget_set_usize ( ib, num_cols * 70, 500);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end ( GTK_BOX(GTK_DIALOG(ib)->vbox), 
		     scrolled_win, TRUE, TRUE, 10);

  table = gtk_table_new (num_rows, num_cols, FALSE);
  gtk_container_add ( GTK_CONTAINER(scrolled_win), table);

  /* The %s in the format gives us room for '\0' */
  buf = g_malloc ( strlen(what) + strlen(TITLE_FORMAT) );

  sprintf ( buf, TITLE_FORMAT, what );

  gtk_window_set_title ( GTK_WINDOW(ib), buf );

  g_free(buf);

  row = 0; col = 0;

  while ( pairs ) {

    ib_and_name = 0;

    pixmap = GTK_WIDGET(((pair *)(pairs->data))->pixmap);
    name = ((pair *)(pairs->data))->name;

    /* Store gchar arrays to be freed on destroy */
    free_me = g_slist_append (free_me, name);

    button = gtk_button_new();
    gtk_container_add ( GTK_CONTAINER(button), pixmap );
    
    gtk_table_attach ( GTK_TABLE(table),
		       button,
		       col, col + 1, row, row + 1, 0, 0, 0, 0 );

    if (col == num_cols) {
      ++row;
      col = 0;
    } 
    else ++col;

    /* Store name to recover in callback */
    gtk_object_set_user_data ( GTK_OBJECT(button), name );

    gtk_signal_connect ( GTK_OBJECT(button), 
			 "clicked",
			 GTK_SIGNAL_FUNC(icon_clicked),
			 ib );

    gtk_widget_show(pixmap);
    gtk_widget_show(button);

    g_print("Added icon: %s\n", name);

    pairs = g_slist_next(pairs);		 
  }

  g_slist_free(first);

  gtk_widget_show(table);
  gtk_widget_show(scrolled_win);

  return ib;
}

GtkWidget * icon_browse_new_from_dir (const gchar * what, const gchar * dir)
{
  return icon_browse_new_from_filenames( what,
					 filenames_from_dir(dir) );
}

GtkWidget * icon_browse_new_from_dirs (const gchar * what, GSList * dirs)
{
  GSList * filenames = 0;
  GSList * first = dirs;

  while (dirs) {
    filenames = g_slist_concat( filenames, 
				filenames_from_dir((gchar *)(dirs->data)) );
    dirs = g_slist_next (dirs);
  }

  g_slist_free (first);

  return icon_browse_new_from_filenames( what, filenames );
}

static void cancelled_cb(GtkWidget * button, gpointer cancelled_bool)
{
  g_return_if_fail(cancelled_bool != 0);
  *((gboolean *)cancelled_bool) = TRUE;
}

/* I hate this function. Rewrite it, please, please */
GtkWidget * icon_browse_new_from_filenames (const gchar * what, 
					    GSList * filenames)
{
  pair * new_pair;
  GSList * pairlist = 0;
  GtkWidget * pixmap;
  GSList * first = filenames;
  GtkWidget * dialog, * pbar, *label, *button;
  gboolean show_progress = FALSE;
  gboolean cancelled = FALSE; /* hack solution */
  gint numfiles;
  gdouble prog_inc;

  g_return_val_if_fail (filenames != 0, 0);

  numfiles = g_slist_length(filenames);

  if ( numfiles > 10 ) show_progress = TRUE;

  if (show_progress) {

    prog_inc = 1.0/((gdouble)numfiles);

    dialog = gtk_dialog_new();
    gtk_window_set_title ( GTK_WINDOW(dialog), "Progress" ); 
   
    button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
    gtk_box_pack_start ( GTK_BOX(GTK_DIALOG(dialog)->action_area),
			 button,
			 TRUE, TRUE, 10 );

    /* such a hack to pass a pointer to a local variable */    
    gtk_signal_connect ( GTK_OBJECT(button), "clicked",
			 GTK_SIGNAL_FUNC(cancelled_cb),
			 &cancelled ); 

    gtk_widget_show(button);

    label = gtk_label_new("Loading image files... ");

    gtk_box_pack_start ( GTK_BOX(GTK_DIALOG(dialog)->vbox),
			 label,
			 TRUE, TRUE, 10 );
    gtk_widget_show(label);

    pbar = gtk_progress_bar_new ();
    gtk_widget_set_usize (pbar, 200, 20);
    gtk_box_pack_start ( GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			 pbar, TRUE, TRUE, 0);
    gtk_widget_show (pbar);

    gtk_widget_show (dialog);
  }


  while (filenames) {
    pixmap = 
      gnome_pixmap_new_from_file ( (gchar *)(filenames->data) );

    if ( pixmap && GNOME_PIXMAP(pixmap)->pixmap ) {

      new_pair = g_malloc(sizeof(pair));
      
      new_pair->pixmap = GNOME_PIXMAP(pixmap);
      new_pair->name = (gchar *)(filenames->data);
      
      pairlist = g_slist_append (pairlist, new_pair);
    }
    else {
      /* No need to keep this filename */
      g_free (filenames->data);
    }

    if (show_progress) {
      gtk_progress_bar_update(GTK_PROGRESS_BAR(pbar), 
			      GTK_PROGRESS_BAR(pbar)->percentage + prog_inc ); 
      while ( gtk_events_pending() ) {
	gtk_main_iteration();
      }
      if (cancelled) {
	/* Just use pairs constructed so far */
	break;
      }
    }
    
    filenames = g_slist_next(filenames);
  }

  if (show_progress) {
    gtk_widget_destroy(dialog);
  }
  
  g_slist_free(first);

  return icon_browse_new_from_pairs (what, pairlist);
}

GtkWidget * icon_browse_new (const gchar * what)
{
  GtkWidget * ib;
  gchar * pixmapdir;

  pixmapdir = gnome_unconditional_pixmap_file("");

  ib =  
    icon_browse_new_from_dir(what,pixmapdir);

  ICON_BROWSE(ib)->using_gnome_dir = TRUE;

  g_free(pixmapdir);

  return ib;
}

static void destroy (GtkObject * o)
{
  GSList * free_me;
  GSList * first;

  g_return_if_fail ( o != 0 );
  g_return_if_fail ( IS_ICON_BROWSE(o) );

  first = free_me = ICON_BROWSE(o)->free_me;

  while ( free_me ) {
    g_free ( free_me->data );
    free_me = g_slist_next(free_me);
  }

  if (first) g_slist_free (first);

  if ( GTK_OBJECT_CLASS(parent_class)->destroy ) {
    (* GTK_OBJECT_CLASS(parent_class)->destroy) (o);
  }
}

static void ok_clicked (GtkWidget * ib)
{
  if ( ICON_BROWSE(ib)->last_clicked ) {
    gtk_signal_emit ( GTK_OBJECT(ib), 
		      ib_signals[ICON_CHOSEN_SIGNAL], 
		      g_strdup(ICON_BROWSE(ib)->last_clicked) );
  }
  
  gtk_widget_destroy ( ib );
}

static gchar * strip_path(gchar * filename)
{
  gint len = strlen(filename);

  while ( len >= 0 ) {
    if ( filename[len] == '/' ) { /* '/' should be a define */
      break;
    }
    --len;
  }
  return g_strdup(&filename[len + 1]);
}

static void icon_clicked (GtkWidget * button, IconBrowse * ib)
{
  gchar * name;

  g_return_if_fail ( ib != 0 );
  g_return_if_fail ( IS_ICON_BROWSE(ib) );

  name = (gchar *) gtk_object_get_user_data(GTK_OBJECT(button));

  g_return_if_fail( name != 0 );

  ib->last_clicked = name;

  if ( ib->using_gnome_dir ) {
    name = strip_path(name);
  }

  gtk_label_set( GTK_LABEL(ib->label), name );
  g_print("Clicked: %s\n", name);

  if ( ib->using_gnome_dir ) {
    g_free(name);
  }
}

static void icon_chosen_marshaller (GtkObject      *object,
				    GtkSignalFunc   func,
				    gpointer        func_data,
				    GtkArg         *params)
{
  typedef void (* myfunc)(GtkObject *, gpointer, gpointer);

  myfunc rfunc = (myfunc)func;
  
  (* rfunc) ( object, 
	      (gchar *)GTK_VALUE_POINTER(params[0]), 
	      func_data );
}

static int pick_entries (struct dirent * entry)
{
  return is_image_file(entry->d_name);
}

static GSList * filenames_from_dir (const gchar * dir)
{
  int num_scanned;
  struct dirent ** namelist;
  GSList * filenames = 0;
  int i;
  gchar * full_path;

  g_return_val_if_fail ( dir != 0, 0 );
    
  /* Is scandir() portable? I'm thinking no. Too lazy to fix it now */
  num_scanned = scandir ( dir, &namelist, pick_entries, alphasort );
  
  if (num_scanned > 0) {
    i = 0;
    while ( i < num_scanned ) {
      full_path = g_concat_dir_and_file( dir,
					 namelist[i]->d_name );
      free (namelist[i]);
      
      filenames = g_slist_append (filenames, full_path);
      
      ++i;
    }
  }
  else if (num_scanned == 0) {
    g_warning("No images found in directory %s", dir);
    filenames = 0;
  }
  else {
    g_warning("Error opening directory %s", dir);
    filenames = 0;
  } 

  return filenames;
}










