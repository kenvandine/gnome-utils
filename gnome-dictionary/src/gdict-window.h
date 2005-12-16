/* gdict-window.h - main application window
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

#ifndef __GDICT_WINDOW_H__
#define __GDICT_WINDOW_H__

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

#include "gdict.h"

G_BEGIN_DECLS

#define GDICT_TYPE_WINDOW	(gdict_window_get_type ())
#define GDICT_WINDOW(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_WINDOW, GdictWindow))
#define GDICT_IS_WINDOW(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_WINDOW))

typedef struct _GdictWindow      GdictWindow;
typedef struct _GdictWindowClass GdictWindowClass;

struct _GdictWindow
{
  GtkWindow parent_instance;
  
  GtkWidget *main_box;
  GtkWidget *menubar;
  GtkWidget *entry;
  GtkWidget *defbox;
  GtkWidget *status;
  
  GtkUIManager *ui_manager;
  GtkActionGroup *action_group;
  
  GtkTooltips *tooltips;
  
  gchar *word;
  gint max_definition;
  gint last_definition;
 
  gchar *source_name;
  GdictSourceLoader *loader;
  GdictContext *context;
  guint lookup_start_id;
  guint lookup_end_id;
  guint error_id;
  
  gchar *print_font;
  
  GConfClient *client;
  guint notify_id;
  
  gulong window_id;
};

struct _GdictWindowClass
{
  GtkWindowClass parent_class;
  
  void (*created) (GdictWindow *parent_window,
  		   GdictWindow *new_window);
};

GType      gdict_window_get_type (void) G_GNUC_CONST;
GtkWidget *gdict_window_new      (GdictSourceLoader *loader);

#endif /* __GDICT_WINDOW_H__ */
