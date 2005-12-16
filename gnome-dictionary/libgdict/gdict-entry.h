/* gdict-entry.h - GtkEntry widget with dictionary completion
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
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

#ifndef __GDICT_ENTRY_H__
#define __GDICT_ENTRY_H__

#include <gtk/gtk.h>

#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_ENTRY		(gdict_entry_get_type ())
#define GDICT_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_ENTRY, GdictEntry))
#define GDICT_IS_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_ENTRY))
#define GDICT_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_ENTRY, GdictEntryClass))
#define GDICT_IS_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_ENTRY))
#define GDICT_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_ENTRY, GdictEntryClass))

typedef struct _GdictEntry        GdictEntry;
typedef struct _GdictEntryClass   GdictEntryClass;
typedef struct _GdictEntryPrivate GdictEntryPrivate;

struct _GdictEntry
{
  /*< private >*/
  GtkEntry parent_instance;
  
  GdictEntryPrivate *priv;
};

struct _GdictEntryClass
{
  GtkEntryClass parent_class;
  
  /* padding for future expansion */
  void (*_gdict_entry_1) (void);
  void (*_gdict_entry_2) (void);
  void (*_gdict_entry_3) (void);
  void (*_gdict_entry_4) (void);
};

GType         gdict_entry_get_type         (void) G_GNUC_CONST;

GtkWidget *   gdict_entry_new              (void);
GtkWidget *   gdict_entry_new_with_context (GdictContext *context);

void          gdict_entry_set_context      (GdictEntry   *entry,
					    GdictContext *context);
GdictContext *gdict_entry_get_context      (GdictEntry   *entry);

G_END_DECLS

#endif /* __GDICT_ENTRY_H__ */
