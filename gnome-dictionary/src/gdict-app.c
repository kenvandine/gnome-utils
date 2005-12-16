/* gdict-app.c - main application class
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib/goption.h>
#include <glib/gi18n.h>
#include <libgnome/libgnome.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprint/gnome-print-pango.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>

#include "gdict-pref-dialog.h"
#include "gdict-app.h"

static GdictApp *singleton = NULL;


struct _GdictAppClass
{
  GObjectClass parent_class;
};



G_DEFINE_TYPE (GdictApp, gdict_app, G_TYPE_OBJECT);


static void
gdict_app_finalize (GObject *object)
{
  GdictApp *app = GDICT_APP (object);
  
  if (app->loader)
    g_object_unref (app->loader);
  
  app->current_window = NULL;
  
  g_slist_foreach (app->windows,
  		   (GFunc) gtk_widget_destroy,
  		   NULL);
  g_slist_free (app->windows);
  
  G_OBJECT_CLASS (gdict_app_parent_class)->finalize (object);
}

static void
gdict_app_class_init (GdictAppClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = gdict_app_finalize;
}

static void
gdict_app_init (GdictApp *app)
{
  app->windows = NULL;
  app->current_window = NULL;
}

static gboolean
save_yourself_cb (GnomeClient        *client,
		  gint                phase,
		  GnomeSaveStyle      save_style,
		  gint                shutdown,
		  GnomeInteractStyle  interact_style,
		  gint                fast,
		  gpointer            user_data)
{
  return TRUE;
}

static void
gdict_window_destroy_cb (GtkWidget *widget,
		         gpointer   user_data)
{
  GdictWindow *window = GDICT_WINDOW (widget);
  GdictApp *app = GDICT_APP (user_data);
  GSList *l;
  
  g_assert (GDICT_IS_APP (app));
  
  for (l = app->windows; l != NULL; l = l->next)
    {
      GdictWindow *w = GDICT_WINDOW (l->data);
      
      if (w->window_id == window->window_id)
        {
          app->windows = g_slist_remove_link (app->windows, l);
          g_slist_free (l);

          break;
        }
    }

  if (g_slist_length (app->windows) == 0)
    gtk_main_quit ();
}

static void
gdict_window_created_cb (GdictWindow *parent,
			 GdictWindow *new_window,
			 gpointer     user_data)
{
  GdictApp *app = GDICT_APP (user_data);
  
  g_signal_connect (new_window, "destroy",
  		    G_CALLBACK (gdict_window_destroy_cb), app);
  
  app->windows = g_slist_prepend (app->windows, new_window);
  app->current_window = new_window;
}

static void
gdict_create_window (GdictApp *app)
{
  GtkWidget *window;
  
  window = gdict_window_new (singleton->loader);
  GDICT_WINDOW (window)->client = app->gconf_client;
  
  g_signal_connect (window, "created",
  		    G_CALLBACK (gdict_window_created_cb), app);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gdict_window_destroy_cb), app);
  
  app->current_window = GDICT_WINDOW (window);
  app->windows = g_slist_prepend (app->windows, window);

  gtk_widget_show (window);
}

void
gdict_init (int *argc, char ***argv)
{
  GdictApp *app;
  GError *error, *gconf_error;
  GOptionContext *context;
  gchar *loader_path;

  g_type_init ();

  g_assert (singleton == NULL);  
  
  singleton = GDICT_APP (g_object_new (GDICT_TYPE_APP, NULL));
  g_assert (GDICT_IS_APP (singleton));
  
  singleton->program = gnome_program_init ("gnome-dictionary",
					   VERSION,
					   LIBGNOMEUI_MODULE,
					   *argc, *argv,
					   GNOME_PARAM_APP_DATADIR, DATADIR,
					   NULL);
  g_set_application_name (_("Dictionary"));
  gtk_window_set_default_icon_name ("gdict");
  
  singleton->client = gnome_master_client ();
  if (singleton->client)
    {
      g_signal_connect (singleton->client, "save-yourself",
                        G_CALLBACK (save_yourself_cb),
                        NULL);
    }
  
  gconf_error = NULL;
  singleton->gconf_client = gconf_client_get_default ();
  gconf_client_add_dir (singleton->gconf_client,
  			GDICT_GCONF_DIR,
  			GCONF_CLIENT_PRELOAD_ONELEVEL,
  			&gconf_error);
  if (gconf_error)
    {
      g_warning ("Unable to access GConf: %s\n", gconf_error->message);
      
      g_error_free (gconf_error);
      singleton->gconf_client;
    }
  
  singleton->loader = gdict_source_loader_new ();
  loader_path = g_build_filename (g_get_home_dir (),
                                  ".gnome2",
                                  "gnome-dictionary",
                                  NULL);
  gdict_source_loader_add_search_path (singleton->loader, loader_path);
  g_free (loader_path);
  
  context = g_option_context_new (_(" - Dictionary and spelling tool"));
  g_option_context_set_ignore_unknown_options (context, TRUE);
  
  error = NULL;
  g_option_context_parse (context, argc, argv, &error);

  g_option_context_free (context);
}

void
gdict_main (void)
{
  GtkWidget *window;
  
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_app_init()\n");
      return;
    }
  
  gdict_create_window (singleton);
  
  gtk_main ();
}

void
gdict_cleanup (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_app_init()\n");
      return;
    }

  g_object_unref (singleton);
}
