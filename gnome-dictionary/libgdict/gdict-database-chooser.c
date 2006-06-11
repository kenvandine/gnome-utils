/* gdict-database-chooser.c - display widget for database names
 *
 * Copyright (C) 2006  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkbindings.h>
#include <glib/gi18n.h>

#include "gdict-database-chooser.h"
#include "gdict-utils.h"
#include "gdict-private.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"

#define GDICT_DATABASE_CHOOSER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_DATABASE_CHOOSER, GdictDatabaseChooserPrivate))

struct _GdictDatabaseChooserPrivate
{
  GtkListStore *store;
  
  GdictContext *context;
  GSList *matches;
  
  guint start_id;
  guint match_id;
  guint end_id;
  guint error_id;
};

enum
{
  PROP_0,
  
  PROP_CONTEXT,
  PROP_COUNT
};


G_DEFINE_TYPE (GdictDatabaseChooser,
               gdict_database_chooser,
               GTK_TYPE_COMBO_BOX_ENTRY);


static void
set_gdict_context (GdictDatabaseChooser *chooser,
		   GdictContext         *context)
{
  GdictDatabaseChooserPrivate *priv;
  
  g_assert (GDICT_IS_DATABASE_CHOOSER (chooser));
  priv = chooser->priv;
  
  if (priv->context)
    {
      if (priv->start_id)
        {
          _gdict_debug ("Removing old context handlers\n");
          
          g_signal_handler_disconnect (priv->context, priv->start_id);
          g_signal_handler_disconnect (priv->context, priv->match_id);
          g_signal_handler_disconnect (priv->context, priv->end_id);
          
          priv->start_id = 0;
          priv->end_id = 0;
          priv->match_id = 0;
        }
      
      if (priv->error_id)
        {
          g_signal_handler_disconnect (priv->context, priv->error_id);

          priv->error_id = 0;
        }

      _gdict_debug ("Removing old context\n");
      
      g_object_unref (G_OBJECT (priv->context));
    }

  if (!context)
    return;

  if (!GDICT_IS_CONTEXT (context))
    {
      g_warning ("Object of type '%s' instead of a GdictContext\n",
      		 g_type_name (G_OBJECT_TYPE (context)));
      return;
    }

  _gdict_debug ("Setting new context\n");
    
  priv->context = context;
  g_object_ref (G_OBJECT (priv->context));
}

static void
gdict_database_chooser_finalize (GObject *gobject)
{
  GdictDatabaseChooserPrivate *priv;
  
  priv = GDICT_DATABASE_CHOOSER_GET_PRIVATE (gobject);
  
  if (priv->context)
    {
      if (priv->start_id)
        {
          _gdict_debug ("Removing old context handlers\n");
          
          g_signal_handler_disconnect (priv->context, priv->start_id);
          g_signal_handler_disconnect (priv->context, priv->match_id);
          g_signal_handler_disconnect (priv->context, priv->end_id);
          
          priv->start_id = 0;
          priv->end_id = 0;
          priv->match_id = 0;
        }
      
      if (priv->error_id)
        {
          g_signal_handler_disconnect (priv->context, priv->error_id);

          priv->error_id = 0;
        }

      _gdict_debug ("Removing old context\n");
      
      g_object_unref (G_OBJECT (priv->context));
    }
  
  if (priv->store)
    g_object_unref (priv->store);
  
  G_OBJECT_CLASS (gdict_database_chooser_parent_class)->finalize (gobject);
}

static void
gdict_database_chooser_set_property (GObject      *gobject,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (gobject);
  
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
gdict_database_chooser_get_property (GObject    *gobject,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (gobject);
  
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
gdict_database_chooser_class_init (GdictDatabaseChooserClass *klass)
{

}

static void
gdict_database_chooser_init (GdictDatabaseChooser *chooser)
{

}

/**
 * gdict_database_chooser_new:
 *
 * FIXME
 *
 * Return value: FIXME
 *
 * Since: 0.9
 */
GtkWidget *
gdict_database_chooser_new (void)
{
  return g_object_new (GDICT_TYPE_DATABASE_CHOOSER, NULL);
}

GtkWigdet *
gdict_database_chooser_new_with_context (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  return g_object_new (GDICT_TYPE_DATABASE_CHOOSER,
                       "context", context,
                       NULL);
}

GdictContext *
gdict_database_chooser_get_context (GdictDatabaseChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), NULL);
  
  
}

void
gdict_database_chooser_set_context (GdictDatabaseChooser *chooser,
				    GdictContext         *context)
{

}

gchar **
gdict_database_chooser_get_databases (GdictDatabaseChooser  *chooser,
				      gsize                  length,
				      GError               **error)
{

}

gboolean
gdict_database_chooser_has_database (GdictDatabaseChooser *chooser,
				     const gchar          *database)
{

}

gboolean
gdict_database_chooser_set_database (GdictDatabaseChooser  *chooser,
				     const gchar           *database,
				     GError               **error)
{

}

gchar *
gdict_database_chooser_get_database (GdictDatabaseChooser *chooser)
{

}
