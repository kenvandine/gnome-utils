/* gdict-defbox.c - display widget for dictionary definitions
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

#include "gdict-defbox.h"
#include "gdict-utils.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"


typedef struct
{
  GdictDefinition *definition;

  gint begin;
} Definition;

#define GDICT_DEFBOX_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_DEFBOX, GdictDefboxPrivate))

struct _GdictDefboxPrivate
{
  GtkWidget *text_view;
  
  /* the "find" pane */
  GtkWidget *find_pane;
  GtkWidget *find_entry;
  GtkWidget *find_next;
  GtkWidget *find_prev;
  GtkWidget *find_label;

  GtkWidget *progress_dialog;
  
  GtkTextBuffer *buffer;

  GtkTooltips *tooltips;

  GdictContext *context;
  GSList *definitions;
  
  gchar *word;
  gchar *database;
  
  guint show_find : 1;
  guint is_searching : 1;
  
  GdkCursor *busy_cursor;
  
  guint start_id;
  guint end_id;
  guint define_id;
  guint error_id;
};

enum
{
  PROP_0,
  
  PROP_CONTEXT,
  PROP_WORD,
  PROP_DATABASE,
  PROP_COUNT
};

enum
{
  SHOW_FIND,
  HIDE_FIND,
  FIND_NEXT,
  FIND_PREVIOUS,
  
  LAST_SIGNAL
};

static guint gdict_defbox_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GdictDefbox, gdict_defbox, GTK_TYPE_VBOX);


static Definition *
definition_new (void)
{
  Definition *def;
  
  def = g_new0 (Definition, 1);
  def->definition = NULL;
  def->begin = -1;
  
  return def;
}

static void
definition_free (Definition *def)
{
  if (!def)
    return;
  
  gdict_definition_unref (def->definition);
  g_free (def);
}

static void
gdict_defbox_finalize (GObject *object)
{
  GdictDefbox *defbox = GDICT_DEFBOX (object);
  GdictDefboxPrivate *priv = defbox->priv;
  
  if (priv->start_id)
    {
      g_signal_handler_disconnect (priv->context, priv->start_id);
      g_signal_handler_disconnect (priv->context, priv->end_id);
      g_signal_handler_disconnect (priv->context, priv->define_id);
    }
  
  if (priv->error_id)
    g_signal_handler_disconnect (priv->context, priv->error_id);
      
  if (priv->context)
    g_object_unref (priv->context);
  
  if (priv->database)
    g_free (priv->database);
  
  if (priv->word)
    g_free (priv->word);
  
  if (priv->definitions)
    {
      g_slist_foreach (priv->definitions,
      		       (GFunc) definition_free,
      		       NULL);
      g_slist_free (priv->definitions);
      
      priv->definitions = NULL;
    }
  
  g_object_unref (priv->tooltips);
  
  g_object_unref (priv->buffer);
  
  if (priv->busy_cursor)
    gdk_cursor_unref (priv->busy_cursor);
  
  G_OBJECT_CLASS (gdict_defbox_parent_class)->finalize (object);
}

static void
set_gdict_context (GdictDefbox  *defbox,
		   GdictContext *context)
{
  GdictDefboxPrivate *priv;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  if (priv->context)
    {
      if (priv->start_id)
        {
          gdict_debug ("Removing old context handlers\n");
          
          g_signal_handler_disconnect (priv->context, priv->start_id);
          g_signal_handler_disconnect (priv->context, priv->define_id);
          g_signal_handler_disconnect (priv->context, priv->end_id);
          
          priv->start_id = 0;
          priv->end_id = 0;
          priv->define_id = 0;
        }
      
      if (priv->error_id)
        {
          g_signal_handler_disconnect (priv->context, priv->error_id);

          priv->error_id = 0;
        }

      gdict_debug ("Removing old context\n");
      
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

  gdict_debug ("Setting new context\n");
    
  priv->context = context;
  g_object_ref (G_OBJECT (priv->context));
}

static void
gdict_defbox_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GdictDefbox *defbox = GDICT_DEFBOX (object);
  GdictDefboxPrivate *priv = defbox->priv;
  
  switch (prop_id)
    {
    case PROP_CONTEXT:
      set_gdict_context (defbox, g_value_get_object (value));
      break;
    case PROP_DATABASE:
      g_free (priv->database);
      priv->database = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_defbox_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GdictDefbox *defbox = GDICT_DEFBOX (object);
  GdictDefboxPrivate *priv = defbox->priv;
  
  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;
    case PROP_DATABASE:
      g_value_set_string (value, priv->database);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
gdict_defbox_find_backward (GdictDefbox *defbox,
			    const gchar *text)
{
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter start_iter, end_iter;
  GtkTextIter match_start, match_end;
  GtkTextIter iter;
  GtkTextMark *last_search;
  gboolean res;

  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  gtk_text_buffer_get_bounds (priv->buffer, &start_iter, &end_iter);
  
  /* if there already has been another result, begin from there */
  last_search = gtk_text_buffer_get_mark (priv->buffer, "last-search-prev");
  if (last_search)
    gtk_text_buffer_get_iter_at_mark (priv->buffer, &iter, last_search);
  else
    iter = end_iter;
  
  res = gtk_text_iter_backward_search (&iter, text,
  				       GTK_TEXT_SEARCH_VISIBLE_ONLY | GTK_TEXT_SEARCH_TEXT_ONLY,
  				       &match_start,
  				       &match_end,
  				       NULL);
  if (res)
    {
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view),
      				    &match_start,
      				    0.0,
      				    TRUE,
      				    0.0, 0.0);
      gtk_text_buffer_place_cursor (priv->buffer, &match_end);
      gtk_text_buffer_move_mark (priv->buffer,
      				 gtk_text_buffer_get_mark (priv->buffer, "selection_bound"),
      				 &match_start);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-prev", &match_start, FALSE);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-next", &match_end, FALSE);
      
      return TRUE;
    }
  
  return FALSE;
}

static void
find_prev_clicked_cb (GtkWidget *widget,
		      gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  const gchar *text;
  gboolean found;

  gtk_widget_hide (priv->find_label);
  
  text = gtk_entry_get_text (GTK_ENTRY (priv->find_entry));
  if (!text)
    return;
  
  found = gdict_defbox_find_backward (defbox, text);
  if (!found)
    {
      gchar *str;
      
      str = g_strconcat ("  <i>", _("Not found"), "</i>", NULL);
      gtk_label_set_markup (GTK_LABEL (priv->find_label), str);
      gtk_widget_show (priv->find_label);
      
      g_free (str);
    }
}

static gboolean
gdict_defbox_find_forward (GdictDefbox *defbox,
			   const gchar *text,
			   gboolean     is_typing)
{
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter start_iter, end_iter;
  GtkTextIter match_start, match_end;
  GtkTextIter iter;
  GtkTextMark *last_search;
  gboolean res;

  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  gtk_text_buffer_get_bounds (priv->buffer, &start_iter, &end_iter);

  if (!is_typing)
    {
      /* if there already has been another result, begin from there */
      last_search = gtk_text_buffer_get_mark (priv->buffer, "last-search-next");

      if (last_search)
        gtk_text_buffer_get_iter_at_mark (priv->buffer, &iter, last_search);
      else
	iter = start_iter;
    }
  else
    {
      last_search = gtk_text_buffer_get_mark (priv->buffer, "last-search-prev");

      if (last_search)
	gtk_text_buffer_get_iter_at_mark (priv->buffer, &iter, last_search);
      else
	iter = start_iter;
    }
  
  res = gtk_text_iter_forward_search (&iter, text,
  				      GTK_TEXT_SEARCH_VISIBLE_ONLY | GTK_TEXT_SEARCH_TEXT_ONLY,
  				      &match_start,
  				      &match_end,
  				      NULL);
  if (res)
    {
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view),
      				    &match_start,
      				    0.0,
      				    TRUE,
      				    0.0, 0.0);
      gtk_text_buffer_place_cursor (priv->buffer, &match_end);
      gtk_text_buffer_move_mark (priv->buffer,
      				 gtk_text_buffer_get_mark (priv->buffer, "selection_bound"),
      				 &match_start);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-prev", &match_start, FALSE);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-next", &match_end, FALSE);
      
      return TRUE;
    }
  
  return FALSE;
}

static void
find_next_clicked_cb (GtkWidget *widget,
		      gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  const gchar *text;
  gboolean found;
  
  gtk_widget_hide (priv->find_label);
  
  text = gtk_entry_get_text (GTK_ENTRY (priv->find_entry));
  if (!text)
    return;
  
  found = gdict_defbox_find_forward (defbox, text, FALSE);
  if (!found)
    {
      gchar *str;
      
      str = g_strconcat ("  <i>", _("Not found"), "</i>", NULL);
      gtk_label_set_markup (GTK_LABEL (priv->find_label), str);
      gtk_widget_show (priv->find_label);
      
      g_free (str);
    }
}

static void
find_entry_changed_cb (GtkWidget *widget,
		       gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  gchar *text;
  gboolean found;

  gtk_widget_hide (priv->find_label);
  
  text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  if (!text)
    return;

  found = gdict_defbox_find_forward (defbox, text, TRUE);
  if (!found)
    {
      gchar *str;
      
      str = g_strconcat ("  <i>", _("Not found"), "</i>", NULL);
      gtk_label_set_markup (GTK_LABEL (priv->find_label), str);
      gtk_widget_show (priv->find_label);
      
      g_free (str);
    }

  g_free (text);
}

static void
create_find_pane (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkWidget *label;
  GtkWidget *sep;
  GtkWidget *hbox1, *hbox2;
 
  priv = defbox->priv;
  
  priv->find_pane = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (priv->find_pane), 0);
  
  hbox1 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (priv->find_pane), hbox1, TRUE, TRUE, 0);
  gtk_widget_show (hbox1);
 
  label = gtk_label_new_with_mnemonic (_("F_ind:"));
  gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

  priv->find_entry = gtk_entry_new ();
  g_signal_connect (priv->find_entry, "changed",
  		    G_CALLBACK (find_entry_changed_cb), defbox);
  gtk_box_pack_start (GTK_BOX (hbox1), priv->find_entry, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->find_entry);
  
  sep = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (priv->find_pane), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  hbox2 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (priv->find_pane), hbox2, TRUE, TRUE, 0);
  gtk_widget_show (hbox2);
  
  priv->find_prev = gtk_button_new_with_mnemonic (_("_Previous"));
  gtk_button_set_image (GTK_BUTTON (priv->find_prev),
  			gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
  						  GTK_ICON_SIZE_MENU));
  g_signal_connect (priv->find_prev, "clicked",
  		    G_CALLBACK (find_prev_clicked_cb), defbox);
  gtk_box_pack_start (GTK_BOX (hbox2), priv->find_prev, FALSE, FALSE, 0);

  priv->find_next = gtk_button_new_with_mnemonic (_("_Next"));
  gtk_button_set_image (GTK_BUTTON (priv->find_next),
  			gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
  						  GTK_ICON_SIZE_MENU));
  g_signal_connect (priv->find_next, "clicked",
  		    G_CALLBACK (find_next_clicked_cb), defbox);
  gtk_box_pack_start (GTK_BOX (hbox2), priv->find_next, FALSE, FALSE, 0);
  
  priv->find_label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (priv->find_label), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox2), priv->find_label, FALSE, FALSE, 0);
  gtk_widget_hide (priv->find_label);
}

static void
gdict_defbox_init_tags (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  gtk_text_buffer_create_tag (priv->buffer, "italic",
  			      "style", PANGO_STYLE_ITALIC,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "bold",
  			      "weight", PANGO_WEIGHT_BOLD,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "underline",
  			      "underline", PANGO_UNDERLINE_SINGLE,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "big",
  			      "scale", 1.6,
  			      "pixels-below-lines", 5,
  			      "pixels-above-lines", 5,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "gray",
  			      "foreground", "dark gray",
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "emphasis",
  			      "foreground", "dark green",
  			      NULL);
}

static GObject *
gdict_defbox_default_constructor (GType                  type,
				  guint                  n_construct_properties,
				  GObjectConstructParam *construct_params)
{
  GdictDefbox *defbox;
  GdictDefboxPrivate *priv;
  GObject *object;
  GtkWidget *sw;
  
  object = G_OBJECT_CLASS (gdict_defbox_parent_class)->constructor (type,
  						   n_construct_properties,
  						   construct_params);
  defbox = GDICT_DEFBOX (object);
  priv = defbox->priv;
  
  gtk_widget_push_composite_child ();
  
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
  				  GTK_POLICY_AUTOMATIC,
  				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
  				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (defbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);
  
  priv->buffer = gtk_text_buffer_new (NULL);
  gdict_defbox_init_tags (defbox);
  
  priv->text_view = gtk_text_view_new_with_buffer (priv->buffer);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->text_view), FALSE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (priv->text_view), 4);
  gtk_container_add (GTK_CONTAINER (sw), priv->text_view);
  gtk_widget_show (priv->text_view);
  
  create_find_pane (defbox);
  gtk_box_pack_end (GTK_BOX (defbox), priv->find_pane, FALSE, FALSE, 0);
  
  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_defbox_style_set (GtkWidget *widget,
			GtkStyle  *old_style)
{
  GdictDefbox *defbox;
  
  defbox = GDICT_DEFBOX (widget);
  
  if (GTK_WIDGET_CLASS (gdict_defbox_parent_class)->style_set)
    GTK_WIDGET_CLASS (gdict_defbox_parent_class)->style_set (widget, old_style);
  
  /* TODO handle our style here */
}

/* we override the GtkWidget::show_all method since we have widgets
 * we don't want to show, such as the find pane
 */
static void
gdict_defbox_show_all (GtkWidget *widget)
{
  GdictDefbox *defbox = GDICT_DEFBOX (widget);
  GdictDefboxPrivate *priv = defbox->priv;
  
  gtk_widget_show (widget);
  
  if (priv->show_find)
    gtk_widget_show_all (priv->find_pane);
}

static void
gdict_defbox_real_show_find (GdictDefbox *defbox)
{
  gtk_widget_show_all (defbox->priv->find_pane);
  defbox->priv->show_find = TRUE;
  
  gtk_widget_grab_focus (defbox->priv->find_entry);
}

static void
gdict_defbox_real_find_next (GdictDefbox *defbox)
{
  /* synthetize a "clicked" signal to the "next" button */
  gtk_button_clicked (GTK_BUTTON (defbox->priv->find_next));
}

static void
gdict_defbox_real_find_previous (GdictDefbox *defbox)
{
  /* synthetize a "clicked" signal to the "prev" button */
  gtk_button_clicked (GTK_BUTTON (defbox->priv->find_prev));
}

static void
gdict_defbox_real_hide_find (GdictDefbox *defbox)
{
  gtk_widget_hide (defbox->priv->find_pane);
  defbox->priv->show_find = FALSE;
  
  gtk_widget_grab_focus (defbox->priv->text_view);
}

static void
gdict_defbox_class_init (GdictDefboxClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet *binding_set;
  
  gobject_class->constructor = gdict_defbox_default_constructor;
  gobject_class->set_property = gdict_defbox_set_property;
  gobject_class->get_property = gdict_defbox_get_property;
  gobject_class->finalize = gdict_defbox_finalize;
  
  widget_class->show_all = gdict_defbox_show_all;
  widget_class->style_set = gdict_defbox_style_set;
  
  /**
   * GdictDefbox:context
   *
   * The #GdictContext object used to get the word definition.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			_("Context"),
  				   			_("The GdictContext object used to get the word definition"),
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
  /**
   * GdictDefbox:database
   *
   * The database used by the #GdictDefbox bound to this object to get the word
   * definition.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
		  		   PROP_DATABASE,
				   g_param_spec_string ("database",
					   		_("Database"),
							_("The database used to query the GdictContext"),
							GDICT_DEFAULT_DATABASE,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  
  gtk_widget_class_install_style_property (widget_class,
  					   g_param_spec_boxed ("emphasis-color",
  					   		       _("Emphasys Color"),
  					   		       _("Color of emphasised text"),
  					   		       GDK_TYPE_COLOR,
  					   		       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  gtk_widget_class_install_style_property (widget_class,
  					   g_param_spec_boxed ("link-color",
  					   		       _("Link Color"),
  					   		       _("Color of hyperlinks"),
  					   		       GDK_TYPE_COLOR,
  					   		       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  gtk_widget_class_install_style_property (widget_class,
  					   g_param_spec_boxed ("word-color",
  					   		       _("Word Color"),
  					   		       _("Color of the dictionary word"),
  					   		       GDK_TYPE_COLOR,
  					   		       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  gtk_widget_class_install_style_property (widget_class,
  					   g_param_spec_boxed ("source-color",
  					   		       _("Source Color"),
  					   		       _("Color of the dictionary source"),
  					   		       GDK_TYPE_COLOR,
  					   		       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  gdict_defbox_signals[SHOW_FIND] =
    g_signal_new ("show-find",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GdictDefboxClass, show_find),
		  NULL, NULL,
		  gdict_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gdict_defbox_signals[FIND_PREVIOUS] =
    g_signal_new ("find-previous",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GdictDefboxClass, find_previous),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  gdict_defbox_signals[FIND_NEXT] =
    g_signal_new ("find-next",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GdictDefboxClass, find_next),
		  NULL, NULL,
		  gdict_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gdict_defbox_signals[HIDE_FIND] =
    g_signal_new ("hide-find",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GdictDefboxClass, hide_find),
		  NULL, NULL,
		  gdict_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  klass->show_find = gdict_defbox_real_show_find;
  klass->hide_find = gdict_defbox_real_hide_find;
  klass->find_next = gdict_defbox_real_find_next;
  klass->find_previous = gdict_defbox_real_find_previous;
  
  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set,
  			        GDK_f, GDK_CONTROL_MASK,
  			        "show-find",
  			        0);
  gtk_binding_entry_add_signal (binding_set,
  			        GDK_g, GDK_CONTROL_MASK,
  			        "find-next",
  			        0);
  gtk_binding_entry_add_signal (binding_set,
  				GDK_g, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
  				"find-previous",
  				0);
  gtk_binding_entry_add_signal (binding_set,
  			        GDK_Escape, 0,
  			        "hide-find",
  			        0);

  g_type_class_add_private (klass, sizeof (GdictDefboxPrivate));
}

static void
gdict_defbox_init (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  
  gtk_box_set_spacing (GTK_BOX (defbox), 6);
  
  priv = GDICT_DEFBOX_GET_PRIVATE (defbox);
  defbox->priv = priv;
  
  priv->context = NULL;
  priv->database = g_strdup (GDICT_DEFAULT_DATABASE);
  priv->word = NULL;
  
  priv->definitions = NULL;
  
  priv->busy_cursor = NULL;
  
  priv->show_find = FALSE;
  priv->is_searching = FALSE;
  
  priv->tooltips = gtk_tooltips_new ();
  g_object_ref (priv->tooltips);
  gtk_object_sink (GTK_OBJECT (priv->tooltips));
}

/**
 * gdict_defbox_new:
 *
 * Creates a new #GdictDefbox widget.  Use this widget to search for
 * a word using a #GdictContext, and to show the resulting definition(s).
 * You must set a #GdictContext for this widget using gdict_defbox_set_context().
 *
 * Return value: a new #GdictDefbox widget.
 *
 * Since: 1.0
 */
GtkWidget *
gdict_defbox_new (void)
{
  return g_object_new (GDICT_TYPE_DEFBOX, NULL);
}

/**
 * gdict_defbox_new_with_context:
 * @context: a #GdictContext
 *
 * Creates a new #GdictDefbox widget. Use this widget to search for
 * a word using @context, and to show the resulting definition.
 *
 * Return value: a new #GdictDefbox widget.
 *
 * Since: 1.0
 */
GtkWidget *
gdict_defbox_new_with_context (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  return g_object_new (GDICT_TYPE_DEFBOX,
                       "context", context,
                       NULL);
}

/**
 * gdict_defbox_set_context:
 * @defbox: a #GdictDefbox
 * @context: a #GdictContext
 *
 * Sets @context as the #GdictContext to be used by @defbox in order
 * to retrieve the definitions of a word.
 *
 * Since: 1.0
 */
void
gdict_defbox_set_context (GdictDefbox  *defbox,
			  GdictContext *context)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  g_return_if_fail (context == NULL || GDICT_IS_CONTEXT (context));
  
  g_object_set (defbox, "context", context, NULL);
}

/**
 * gdict_defbox_get_context:
 * @defbox: a #GdictDefbox
 *
 * Gets the #GdictContext used by @defbox.
 *
 * Return value: a #GdictContext.
 *
 * Since: 1.0
 */
GdictContext *
gdict_defbox_get_context (GdictDefbox *defbox)
{
  GdictContext *context;
  
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);
  
  g_object_get (defbox, "context", &context, NULL);
  if (context)
    g_object_unref (context);
  
  return context;
}

/**
 * gdict_defbox_set_database:
 * @defbox: a #GdictDefbox
 * @database: a database
 *
 * Sets @database as the database used by the #GdictContext bound to @defbox to
 * query for word definitions.
 *
 * Since: 1.0
 */
void
gdict_defbox_set_database (GdictDefbox *defbox,
			   const gchar *database)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_object_set (G_OBJECT (defbox), "database", database, NULL);
}

/**
 * gdict_defbox_get_database:
 * @defbox: a #GdictDefbox
 *
 * Gets the database used by @defbox.  See gdict_defbox_set_database().
 *
 * Return value: the name of a database.  The string is owned by the
 * #GdictDefbox and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_defbox_get_database (GdictDefbox *defbox)
{
  gchar *database;
  
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);

  g_object_get (G_OBJECT (defbox), "database", &database, NULL);

  return database;
}

/**
 * gdict_defbox_set_show_find:
 * @defbox: a #GdictDefbox
 * @show_find: %TRUE to show the find pane
 *
 * Whether @defbox should show the find pane.
 *
 * Since: 1.0
 */
void
gdict_defbox_set_show_find (GdictDefbox *defbox,
			    gboolean     show_find)
{
  GdictDefboxPrivate *priv;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  
  if (priv->show_find == show_find)
    return;
  
  priv->show_find = show_find;
  if (priv->show_find)
    {
      gtk_widget_show_all (priv->find_pane);
      gtk_widget_grab_focus (priv->find_entry);
    }
  else
    gtk_widget_hide (priv->find_pane);
}

/**
 * gdict_defbox_get_show_find:
 * @defbox: a #GdictDefbox
 *
 * FIXME
 *
 * Return value: FIXME
 *
 * Since: 1.0
 */
gboolean
gdict_defbox_get_show_find (GdictDefbox *defbox)
{
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), FALSE);
  
  return (defbox->priv->show_find == TRUE);
}

/* find the toplevel widget for @widget */
static GtkWindow *
get_toplevel_window (GtkWidget *widget)
{
  GtkWidget *toplevel;
  
  toplevel = gtk_widget_get_toplevel (widget);
  if (!GTK_WIDGET_TOPLEVEL (toplevel))
    return NULL;
  else
    return GTK_WINDOW (toplevel);
}

static GtkWidget *
create_progress_dialog (GdictDefbox *defbox)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *progress;

  dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
		                get_toplevel_window (GTK_WIDGET (defbox)));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
  
  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (dialog), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (NULL);
  g_object_set_data (G_OBJECT (dialog), "progress-label", label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  progress = gtk_progress_bar_new ();
  g_object_set_data (G_OBJECT (dialog), "progress-bar", progress);
  gtk_box_pack_end (GTK_BOX (vbox), progress, FALSE, FALSE, 0);
  gtk_widget_show (progress);

  return dialog;
}

static void
update_progress_dialog (GdictDefbox     *defbox,
			GdictDefinition *definition)
{
  GdictDefboxPrivate *priv;
  GtkWidget *progress, *label;
  gchar *text;
  gint total, current;
  gdouble fraction;

  priv = defbox->priv;

  total = gdict_definition_get_total (definition);
  current = g_slist_length (priv->definitions);
  
  text = g_strdup_printf (_("Definition for '%s' (%d/%d)"),
			  gdict_definition_get_word (definition),
			  current,
			  total);
  
  label = g_object_get_data (G_OBJECT (priv->progress_dialog), "progress-label");
  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);

  fraction = ((gdouble) current / (gdouble) total);
  progress = g_object_get_data (G_OBJECT (priv->progress_dialog), "progress-bar");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
		  		 MIN (fraction, 1.0));
}

static void
lookup_start_cb (GdictContext *context,
		 gpointer      user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GdkWindow *window;

  priv->is_searching = TRUE;
  
  gdict_defbox_clear (defbox);

  if (!priv->busy_cursor)
    priv->busy_cursor = gdk_cursor_new (GDK_WATCH);
  
  window = gtk_text_view_get_window (GTK_TEXT_VIEW (priv->text_view),
  				     GTK_TEXT_WINDOW_WIDGET);
  
  gdk_window_set_cursor (window, priv->busy_cursor);

  g_assert (priv->progress_dialog == NULL);
  
  priv->progress_dialog = create_progress_dialog (defbox);
  gtk_widget_show_all (priv->progress_dialog);
}

static void
lookup_end_cb (GdictContext *context,
	       gpointer      user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GdkWindow *window;
  
  /* explicitely move the cursor to the beginning */
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_place_cursor (buffer, &start);
  
  if (priv->progress_dialog)
    {
      gtk_widget_destroy (priv->progress_dialog);
      priv->progress_dialog = NULL;
    }
  
  window = gtk_text_view_get_window (GTK_TEXT_VIEW (priv->text_view),
  				     GTK_TEXT_WINDOW_WIDGET);
  
  gdk_window_set_cursor (window, NULL);
  
  g_free (priv->word);
  priv->word = NULL;
  
  priv->is_searching = FALSE;
}

static void
gdict_defbox_insert_word (GdictDefbox *defbox,
			  GtkTextIter *iter,
			  const gchar *word)
{
  GdictDefboxPrivate *priv;
  gchar *text;
  
  if (!word)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  text = g_strdup_printf ("%s\n\n", word);
  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
  					    iter,
  					    text, strlen (text),
  					    "big",
  					    NULL);
  g_free (text);
}

static void
gdict_defbox_insert_body (GdictDefbox *defbox,
			  GtkTextIter *iter,
			  const gchar *body)
{
  GdictDefboxPrivate *priv;
  gchar *text;
  
  if (!body)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  text = g_strdup_printf ("%s\n", body);
  
  gtk_text_buffer_insert (priv->buffer,
  			  iter,
  			  text, strlen (text));
  g_free (text);
}

static void
gdict_defbox_insert_from (GdictDefbox *defbox,
			  GtkTextIter *iter,
			  const gchar *database)
{
  GdictDefboxPrivate *priv;
  gchar *text;
  
  if (!database)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  text = g_strdup_printf ("\t-- From %s\n\n", database);
  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
  					    iter,
  					    text, strlen (text),
  					    "gray",
  					    NULL);
  g_free (text);
}

static void
definition_found_cb (GdictContext    *context,
		     GdictDefinition *definition,
		     gpointer         user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter iter;
  Definition *def;
 
  /* insert the word if this is the first definition */
  if (!priv->definitions)
    {
      gtk_text_buffer_get_start_iter (priv->buffer, &iter);
      gdict_defbox_insert_word (defbox, &iter,
				gdict_definition_get_word (definition));
    }
  
  def = definition_new ();
  
  gtk_text_buffer_get_end_iter (priv->buffer, &iter);
  def->begin = gtk_text_iter_get_offset (&iter);
  gdict_defbox_insert_body (defbox, &iter, gdict_definition_get_text (definition));
  
  gtk_text_buffer_get_end_iter (priv->buffer, &iter);
  gdict_defbox_insert_from (defbox, &iter, gdict_definition_get_database (definition));
  
  def->definition = gdict_definition_ref (definition);
  
  priv->definitions = g_slist_append (priv->definitions, def);

  g_assert (priv->progress_dialog != NULL);
  
  update_progress_dialog (defbox, definition);
}

static void
error_cb (GdictContext *context,
	  const GError *error,
	  gpointer      user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;

  if (priv->progress_dialog)
    {
      gtk_widget_destroy (priv->progress_dialog);
      priv->progress_dialog = NULL;
    }

  if (!error)
    return;
  
  /* the error must not be freed */
  gdict_show_error_dialog (GTK_WIDGET (defbox),
  			   _("Error while looking up definition"),
  			   error->message);

  gdict_defbox_clear (defbox);
  
  g_free (priv->word);
  priv->word = NULL;

  /* disconnect everything except the error callback */
  if (priv->start_id)
    {
      g_signal_handler_disconnect (priv->context, priv->start_id);
      g_signal_handler_disconnect (priv->context, priv->define_id);
      g_signal_handler_disconnect (priv->context, priv->end_id);

      priv->start_id = priv->define_id = priv->end_id = 0;
    }
  
  defbox->priv->is_searching = FALSE;
}

/**
 * gdict_defbox_lookup:
 * @defbox: a #GdictDefbox
 * @word: the word to look up
 *
 * Searches @word inside the dictionary sources using the #GdictContext
 * provided when creating @defbox or set using gdict_defbox_set_context().
 *
 * Since: 1.0
 */
void
gdict_defbox_lookup (GdictDefbox *defbox,
		     const gchar *word)
{
  GdictDefboxPrivate *priv;
  GError *define_error;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  
  if (!priv->context)
    {
      g_warning (_("Attempting to look up '%s' but no GdictContext defined.  Aborting."),
                 word);
      return;
    }
  
  if (priv->is_searching)
    {
      gdict_show_error_dialog (GTK_WIDGET (defbox),
      			       _("Another search is in progress"),
      			       _("Please wait until the current search ends."));
      
      return;
    }
  
  if (!priv->start_id)
    priv->start_id = g_signal_connect (priv->context, "lookup-start",
  				       G_CALLBACK (lookup_start_cb),
  				       defbox);
  if (!priv->define_id)
    priv->define_id = g_signal_connect (priv->context, "definition-found",
  				        G_CALLBACK (definition_found_cb),
  				        defbox);
  if (!priv->end_id)
    priv->end_id = g_signal_connect (priv->context, "lookup-end",
  				     G_CALLBACK (lookup_end_cb),
  				     defbox);
  if (!priv->error_id)
    priv->error_id = g_signal_connect (priv->context, "error",
  				       G_CALLBACK (error_cb),
  				       defbox);
  
  priv->word = g_strdup (word);
  
  define_error = NULL;
  gdict_context_define_word (priv->context,
  			     priv->database,
  			     word,
  			     &define_error);
  if (define_error)
    gdict_show_gerror_dialog (GTK_WIDGET (defbox),
    			      _("Error while retrieving the definition"),
    			      define_error);
}

/**
 * gdict_defbox_clear:
 * @defbox: a @GdictDefbox
 *
 * Clears the buffer of @defbox
 *
 * Since: 1.0
 */
void
gdict_defbox_clear (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkTextIter start, end;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;

  /* destroy previously found definitions */
  if (priv->definitions)
    {
      g_slist_foreach (priv->definitions,
      		       (GFunc) definition_free,
      		       NULL);
      g_slist_free (priv->definitions);
      
      priv->definitions = NULL;
    }
  
  gtk_text_buffer_get_bounds (priv->buffer, &start, &end);
  gtk_text_buffer_delete (priv->buffer, &start, &end);
}

/**
 * gdict_defbox_find_next:
 * @defbox: a #GdictDefbox
 * 
 * Emits the "find-next" signal.
 * 
 * Since: 1.0
 */
void
gdict_defbox_find_next (GdictDefbox *defbox)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_signal_emit (defbox, gdict_defbox_signals[FIND_NEXT], 0);
}

/**
 * gdict_defbox_find_previous:
 * @defbox: a #GdictDefbox
 * 
 * Emits the "find-previous" signal.
 * 
 * Since: 1.0
 */
void
gdict_defbox_find_previous (GdictDefbox *defbox)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_signal_emit (defbox, gdict_defbox_signals[FIND_PREVIOUS], 0);
}

/**
 * gdict_defbox_select_all:
 * @defbox: a #GdictDefbox
 *
 * Selects all the text displayed by @defbox
 * 
 * Since: 1.0
 */
void
gdict_defbox_select_all (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_select_range (buffer, &start, &end);
}

/**
 * gdict_defbox_copy_to_clipboard:
 * @defbox: a #GdictDefbox
 * @clipboard: a #GtkClipboard
 *
 * Copies the selected text inside @defbox into @clipboard.
 *
 * Since: 1.0
 */
void
gdict_defbox_copy_to_clipboard (GdictDefbox  *defbox,
				GtkClipboard *clipboard)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  g_return_if_fail (GTK_IS_CLIPBOARD (clipboard));

  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

  gtk_text_buffer_copy_clipboard (buffer, clipboard);
}

/**
 * gdict_defbox_count_definitions:
 * @defbox: a #GdictDefbox
 *
 * Gets the number of definitions displayed by @defbox
 *
 * Return value: the number of definitions.
 *
 * Since: 1.0
 */
gint
gdict_defbox_count_definitions (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), -1);
  
  priv = defbox->priv;
  if (!priv->definitions)
    return -1;
  
  return g_slist_length (priv->definitions);
}

/**
 * gdict_defbox_jump_to_definition:
 * @defbox: a #GdictDefbox
 * @number: the definition to jump to
 *
 * Scrolls to the definition identified by @number.  If @number is -1,
 * jumps to the last definition.
 *
 * Since: 1.0
 */
void
gdict_defbox_jump_to_definition (GdictDefbox *defbox,
				 gint         number)
{
  GdictDefboxPrivate *priv;
  gint count;
  Definition *def;
  GtkTextBuffer *buffer;
  GtkTextIter def_start;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  count = gdict_defbox_count_definitions (defbox) - 1;
  if (count == -1)
    return;
  
  if ((number == -1) || (number > count))
    number = count;
  
  priv = defbox->priv;
  def = (Definition *) g_slist_nth_data (priv->definitions, number);
  if (!def)
    return;
  
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  gtk_text_buffer_get_iter_at_offset (buffer, &def_start, def->begin);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view),
      				&def_start,
      				0.0,
      				TRUE,
      				0.5, 0.5);
}

/**
 * gdict_defbox_get_text:
 * @defbox: a #GdictDefbox
 * @length: return location for the text length or %NULL
 *
 * Gets the full contents of @defbox.
 *
 * Return value: a newly allocated string containing the text displayed by
 *   @defbox.
 *
 * Since: 1.0
 */
gchar *
gdict_defbox_get_text (GdictDefbox *defbox,
		       gsize       *length)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  gchar *retval;
  
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);
  
  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  
  retval = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  
  if (length)
    *length = strlen (retval);
  
  return retval;
}
