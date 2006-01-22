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
#include <sys/stat.h>

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

  if (app->database)
    g_free (app->database);

  if (app->word)
    g_free (app->word);
  
  if (app->source_name)
    g_free (app->source_name);
  
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
  gchar *data_dir;
  
  app->windows = NULL;
  app->current_window = NULL;
  
  /* create the data directory inside $HOME, if it doesn't exist yet */
  data_dir = g_build_filename (g_get_home_dir (),
  			       ".gnome2",
  			       "gnome-dictionary",
  			       NULL);
  
  if (g_mkdir (data_dir, 0700) == -1)
    {
      if (errno != EEXIST)
        g_warning ("Unable to create the data directory '%s': %s",
		   data_dir,
		   strerror (errno));
    }
  
  g_free (data_dir);
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
  
  g_assert (GDICT_IS_APP (app));

  app->windows = g_slist_remove (app->windows, window);
  
  if (window == app->current_window)
    app->current_window = app->windows ? app->windows->data : NULL;
  
  if (app->windows == NULL)
    gtk_main_quit ();  
}

static void
gdict_window_created_cb (GdictWindow *parent,
			 GdictWindow *new_window,
			 gpointer     user_data)
{
  GdictApp *app = GDICT_APP (user_data);
  
  /* this might seem convoluted - but it's necessary, since I don't want
   * GdictWindow to know about the GdictApp singleton.  every time a new
   * window is created by a GdictWindow, it will register its "child window"
   * here; the lifetime handlers will check every child window created and
   * destroyed, and will add/remove it to the windows list accordingly
   */
  g_signal_connect (new_window, "created",
  		    G_CALLBACK (gdict_window_created_cb), app);
  g_signal_connect (new_window, "destroy",
  		    G_CALLBACK (gdict_window_destroy_cb), app);

  if (GTK_WINDOW (parent)->group)
    gtk_window_group_add_window (GTK_WINDOW (parent)->group,
		    		 GTK_WINDOW (new_window));
  
  app->windows = g_slist_prepend (app->windows, new_window);
  app->current_window = new_window;
}

static void
gdict_create_window (GdictApp *app)
{
  GtkWidget *window;
  
  window = gdict_window_new (singleton->loader,
		  	     singleton->source_name,
			     singleton->word);
  
  g_signal_connect (window, "created",
  		    G_CALLBACK (gdict_window_created_cb), app);
  g_signal_connect (window, "destroy",
  		    G_CALLBACK (gdict_window_destroy_cb), app);
  
  app->windows = g_slist_prepend (app->windows, window);
  app->current_window = GDICT_WINDOW (window);
  
  gtk_widget_show (window);
}

static void
lookup_start_cb (GdictContext *context,
		 gpointer      user_data)
{
  GdictApp *app = GDICT_APP (user_data);

  g_print (_("Definitions for \"%s\":\n\n"), app->word);
}

static void
definition_found_cb (GdictContext    *context,
		     GdictDefinition *definition,
		     gpointer         user_data)
{
  /* Translators: source name first, then definition */
  g_print (_("From %s:\n%s\n"),
	   gdict_definition_get_database (definition),
	   gdict_definition_get_text (definition));
}

static void
error_cb (GdictContext *context,
          const GError *error,
	  gpointer      user_data)
{
  g_print (_("Error: %s\n"), error->message);
}

static void
gdict_look_up_word_and_quit (GdictApp *app)
{
  GdictSource *source;
  GdictContext *context;
  GError *err;
  
  if (!app->word)
    {
      g_print (_("See gnome-dictionary --help for usage\n"));

      gdict_cleanup ();
      exit (1);
    }

  if (app->source_name)
    source = gdict_source_loader_get_source (app->loader, app->source_name);
  else
    source = gdict_source_loader_get_source (app->loader, GDICT_DEFAULT_SOURCE_NAME);

  if (!source)
    {
      g_warning (_("Unable to find a suitable dictionary source"));

      gdict_cleanup ();
      exit (1);
    }

  /* we'll just use this one context, so we can destroy it along with
   * the source that contains it
   */
  context = gdict_source_peek_context (source);
  g_assert (GDICT_IS_CONTEXT (context));

  g_signal_connect (context, "lookup-start",
		    G_CALLBACK (lookup_start_cb), app);
  g_signal_connect (context, "definition-found",
		    G_CALLBACK (definition_found_cb), app);
  g_signal_connect (context, "lookup-end",
		    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (context, "error",
		    G_CALLBACK (error_cb), app);

  err = NULL;
  gdict_context_define_word (context,
		  	     app->database,
		  	     app->word,
			     &err);
  if (err)
    {
      g_warning (_("Error while looking up the definition of \"%s\":\n%s"),
		 app->word,
		 err->message);

      g_error_free (err);
    }

  gtk_main ();

  g_object_unref (source);

  gdict_cleanup ();
  exit (0);
}

void
gdict_init (int *argc, char ***argv)
{
  GError *gconf_error;
  GOptionContext *context;
  GOptionGroup *group;
  gchar *loader_path;
  gchar *word = NULL;
  gchar *database = NULL;
  gchar *source_name = NULL;
  gboolean no_window = FALSE;
  gboolean list_sources = FALSE;

  const GOptionEntry gdict_app_goptions[] =
  {
    { "look-up", 0, 0, G_OPTION_ARG_STRING, &word,
       N_("Word to look up"), N_("word") },
    { "source", 's', 0, G_OPTION_ARG_STRING, &source_name,
       N_("Dictionary source to use"), N_("source") },
    { "list-sources", 'l', 0, G_OPTION_ARG_NONE, &list_sources,
       N_("Show available dictionary sources"), NULL },
    { "no-window", 'n', 0, G_OPTION_ARG_NONE, &no_window,
       N_("Print result to the console"), NULL },
    { "database", 'd', 0, G_OPTION_ARG_STRING, &database,
       N_("Database to use"), NULL },
    { NULL },
  };
  
  /* we must have GLib's type system up and running in order to create the
   * singleton object for gnome-dictionary; thus, we can't rely on
   * gnome_program_init() calling g_type_init() for us.
   */
  g_type_init ();

  g_assert (singleton == NULL);  
  
  singleton = GDICT_APP (g_object_new (GDICT_TYPE_APP, NULL));
  g_assert (GDICT_IS_APP (singleton));

  /* create the new option context */
  context = g_option_context_new (_(" - Look up words in dictionaries"));
  
  /* gnome dictionary option group */
  group = g_option_group_new ("gnome-dictionary",
		  	      _("Dictionary and spelling tool"),
			      _("Show Dictionary options"),
			      NULL, NULL);
  g_option_group_add_entries (group, gdict_app_goptions);
  g_option_context_add_group (context, group);
  
  singleton->program = gnome_program_init ("gnome-dictionary",
					   VERSION,
					   LIBGNOMEUI_MODULE,
					   *argc, *argv,
					   GNOME_PARAM_APP_DATADIR, DATADIR,
					   GNOME_PARAM_GOPTION_CONTEXT, context,
					   NULL);
  g_set_application_name (_("Dictionary"));
  
  /* session management */
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
      g_object_unref (singleton->gconf_client);
    }

  /* add user's path for fetching dictionary sources */  
  singleton->loader = gdict_source_loader_new ();
  loader_path = g_build_filename (g_get_home_dir (),
                                  ".gnome2",
                                  "gnome-dictionary",
                                  NULL);
  gdict_source_loader_add_search_path (singleton->loader, loader_path);
  g_free (loader_path);

  if (word)
    singleton->word = g_strdup (word);

  if (database)
    singleton->database = g_strdup (database);
  
  if (source_name)
    singleton->source_name = g_strdup (source_name);

  if (no_window)
    singleton->no_window = TRUE;

  if (list_sources)
    singleton->list_sources = TRUE;

  g_option_context_free (context);
}

void
gdict_main (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_init()\n");
      return;
    }
  
  if (!singleton->no_window)
    gdict_create_window (singleton);
  else
    gdict_look_up_word_and_quit (singleton);
  
  gtk_main ();
}

void
gdict_cleanup (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_init()\n");
      return;
    }

  g_object_unref (singleton);
}
