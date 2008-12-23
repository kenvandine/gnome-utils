/*
 * gtk-message-area.c
 * This file is part of GTK+
 *
 * Copyright (C) 2005 - Paolo Maggi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a
 * list of people on the gtk Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the GTK+ team, 2008.
 */


#include "config.h"

#include "gtkmessagearea.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#define GTK_MESSAGE_AREA_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), \
                                GTK_TYPE_MESSAGE_AREA, \
                                GtkMessageAreaPrivate))

#ifdef ENABLE_NLS
#define P_(String) g_dgettext(GETTEXT_PACKAGE "-properties",String)
#else 
#define P_(String) (String)
#endif

#define I_(string) g_intern_static_string (string)

struct _GtkMessageAreaPrivate
{
  GtkWidget *content_area;
  GtkWidget *action_area;
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  gint response_id;
};

enum
{
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void gtk_message_area_style_set  (GtkWidget      *widget,
                                         GtkStyle       *prev_style);
static gboolean gtk_message_area_expose (GtkWidget      *widget,
                                         GdkEventExpose *event);
static void      gtk_message_area_buildable_interface_init     (GtkBuildableIface *iface);
static GObject * gtk_message_area_buildable_get_internal_child (GtkBuildable  *buildable,
                                                                GtkBuilder    *builder,
                                                                 const gchar   *childname);
static gboolean  gtk_message_area_buildable_custom_tag_start   (GtkBuildable  *buildable,
                                                                GtkBuilder    *builder,
                                                                GObject       *child,
                                                                const gchar   *tagname,
                                                                GMarkupParser *parser,
                                                                gpointer      *data);
static void      gtk_message_area_buildable_custom_finished    (GtkBuildable  *buildable,
                                                                GtkBuilder    *builder,
                                                                GObject       *child,
                                                                const gchar   *tagname,
                                                                gpointer       user_data);


G_DEFINE_TYPE_WITH_CODE (GtkMessageArea, gtk_message_area, GTK_TYPE_HBOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                gtk_message_area_buildable_interface_init))



static void
gtk_message_area_finalize (GObject *object)
{
  G_OBJECT_CLASS (gtk_message_area_parent_class)->finalize (object);
}

static void
response_data_free (gpointer data)
{
  g_slice_free (ResponseData, data);
}

static ResponseData *
get_response_data (GtkWidget *widget,
                   gboolean   create)
{
  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "gtk-message-area-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);

      g_object_set_data_full (G_OBJECT (widget),
                              I_("gtk-message-area-response-data"),
                              ad,
                              response_data_free);
    }

  return ad;
}

static GtkWidget *
find_button (GtkMessageArea *message_area,
             gint            response_id)
{
  GList *children, *list;
  GtkWidget *child = NULL;

  children = gtk_container_get_children (GTK_CONTAINER (message_area->priv->action_area));

  for (list = children; list; list = list->next)
    {
      ResponseData *rd = get_response_data (list->data, FALSE);

      if (rd && rd->response_id == response_id)
        {
          child = list->data;
          break;
        }
    }

  g_list_free (children);

  return child;
}

static void
gtk_message_area_close (GtkMessageArea *message_area)
{
  if (!find_button (message_area, GTK_RESPONSE_CANCEL))
    return;

  gtk_message_area_response (GTK_MESSAGE_AREA (message_area),
                             GTK_RESPONSE_CANCEL);
}

static gboolean
gtk_message_area_expose (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  gboolean use_tooltip_style;

  gtk_widget_style_get (widget,
                        "use-tooltip-style", &use_tooltip_style,
                        NULL);

  gtk_paint_flat_box (widget->style,
                      widget->window,
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL,
                      widget,
                      use_tooltip_style ? "tooltip" : "messagearea",
                      widget->allocation.x,
                      widget->allocation.y,
                      widget->allocation.width + 1,
                      widget->allocation.height + 1);

  if (GTK_WIDGET_CLASS (gtk_message_area_parent_class)->expose_event)
    GTK_WIDGET_CLASS (gtk_message_area_parent_class)->expose_event (widget, event);

  return FALSE;
}

static void
gtk_message_area_class_init (GtkMessageAreaClass *klass)
{
  GtkWidgetClass *widget_class;
  GObjectClass *object_class;
  GtkBindingSet *binding_set;

  widget_class = GTK_WIDGET_CLASS (klass);
  object_class = G_OBJECT_CLASS (klass);

  widget_class->style_set = gtk_message_area_style_set;
  widget_class->expose_event = gtk_message_area_expose;

  object_class->finalize = gtk_message_area_finalize;

  klass->close = gtk_message_area_close;

  signals[RESPONSE] = g_signal_new (I_("response"),
                                    G_OBJECT_CLASS_TYPE (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (GtkMessageAreaClass, response),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__INT,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);

  signals[CLOSE] =  g_signal_new (I_("close"),
                                  G_OBJECT_CLASS_TYPE (klass),
                                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (GtkMessageAreaClass, close),
                                  NULL, NULL,
                                  g_cclosure_marshal_VOID__VOID,
                                  G_TYPE_NONE, 0);

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-border",
                                                             P_("Content area border"),
                                                             P_("Width of border around the main dialog area"),
                                                             0,
                                                             G_MAXINT,
                                                             8,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-spacing",
                                                             P_("Content area spacing"),
                                                             P_("Spacing between elements of the main dialog area"),
                                                             0,
                                                             G_MAXINT,
                                                             16,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button-spacing",
                                                             P_("Button spacing"),
                                                             P_("Spacing between buttons"),
                                                             0,
                                                             G_MAXINT,
                                                             6,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("action-area-border",
                                                             P_("Action area border"),
                                                             P_("Width of border around the button area at the bottom of the dialog"),
                                                             0,
                                                             G_MAXINT,
                                                             5,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("use-tooltip-style",
                                                                 P_("Use tooltip style"),
                                                                 P_("Wether to use the same style as GtkTooltip for drawing"),
                                                                 TRUE,
                                                                 G_PARAM_READABLE));

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_Escape, 0, "close", 0);

  g_type_class_add_private (object_class, sizeof (GtkMessageAreaPrivate));
}

static void
gtk_message_area_style_set (GtkWidget *widget,
                            GtkStyle  *prev_style)
{
  GtkStyle *style;
  GdkColor default_border_color = {0, 0xb800, 0xad00, 0x9d00};
  GdkColor default_fill_color = {0, 0xff00, 0xff00, 0xbf00};
  GdkColor *fg, *bg;
  gint button_spacing;
  gint action_area_border;
  gint content_area_spacing;
  gint content_area_border;
  gboolean use_tooltip_style;

  gtk_widget_style_get (widget,
                        "button-spacing", &button_spacing,
                        "action-area-border", &action_area_border,
                        "content-area-spacing", &content_area_spacing,
                        "content-area-border", &content_area_border,
                        "use-tooltip-style", &use_tooltip_style,
                        NULL);

  gtk_box_set_spacing (GTK_BOX (GTK_MESSAGE_AREA (widget)->priv->action_area),
                       button_spacing);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_MESSAGE_AREA (widget)->priv->action_area),
                                  action_area_border);
  gtk_box_set_spacing (GTK_BOX (GTK_MESSAGE_AREA (widget)->priv->content_area),
                       content_area_spacing);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_MESSAGE_AREA (widget)->priv->content_area),
                                  content_area_border);

  if (use_tooltip_style)
    {
      style = gtk_rc_get_style_by_paths (gtk_settings_get_default (),
                                         "gtk-tooltip", "GtkTooltip", G_TYPE_NONE);
      if (style)
        {
          fg = &style->fg[GTK_STATE_NORMAL];
          bg = &style->bg[GTK_STATE_NORMAL];
        }
      else
        {
          fg = &default_border_color;
          bg = &default_fill_color;
        }

      if (!gdk_color_equal (bg, &widget->style->bg[GTK_STATE_NORMAL]))
        gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, bg);
      if (!gdk_color_equal (fg, &widget->style->fg[GTK_STATE_NORMAL]))
        gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, fg);
    }
}

static void
gtk_message_area_init (GtkMessageArea *message_area)
{
  GtkWidget *content_area;
  GtkWidget *action_area;

  gtk_widget_push_composite_child ();

  message_area->priv = GTK_MESSAGE_AREA_GET_PRIVATE (message_area);

  content_area = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (content_area);
  gtk_box_pack_start (GTK_BOX (message_area), content_area, TRUE, TRUE, 0);

  action_area = gtk_vbutton_box_new ();
  gtk_widget_show (action_area);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_START);
  gtk_box_pack_start (GTK_BOX (message_area), action_area, FALSE, TRUE, 0);

  gtk_widget_set_app_paintable (GTK_WIDGET (message_area), TRUE);

  message_area->priv->content_area = content_area;
  message_area->priv->action_area = action_area;

  gtk_widget_pop_composite_child ();
}

static GtkBuildableIface *parent_buildable_iface;

static void
gtk_message_area_buildable_interface_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = gtk_message_area_buildable_get_internal_child;
  iface->custom_tag_start = gtk_message_area_buildable_custom_tag_start;
  iface->custom_finished = gtk_message_area_buildable_custom_finished;
}

static GObject *
gtk_message_area_buildable_get_internal_child (GtkBuildable *buildable,
                                               GtkBuilder   *builder,
                                               const gchar  *childname)
{
  if (strcmp (childname, "content_area") == 0)
    return G_OBJECT (GTK_MESSAGE_AREA (buildable)->priv->content_area);
  else if (strcmp (childname, "action_area") == 0)
    return G_OBJECT (GTK_MESSAGE_AREA (buildable)->priv->action_area);

  return parent_buildable_iface->get_internal_child (buildable,
                                                     builder,
                                                     childname);
}

static gint
get_response_for_widget (GtkMessageArea *message_area,
                         GtkWidget      *widget)
{
  ResponseData *rd;

  rd = get_response_data (widget, FALSE);
  if (!rd)
    return GTK_RESPONSE_NONE;
  else
    return rd->response_id;
}

static void
action_widget_activated (GtkWidget      *widget,
                         GtkMessageArea *message_area)
{
  gint response_id;

  response_id = get_response_for_widget (message_area, widget);
  gtk_message_area_response (message_area, response_id);
}

void
gtk_message_area_add_action_widget (GtkMessageArea *message_area,
                                    GtkWidget      *child,
                                    gint            response_id)
{
  ResponseData *ad;
  guint signal_id;

  g_return_if_fail (GTK_IS_MESSAGE_AREA (message_area));
  g_return_if_fail (GTK_IS_WIDGET (child));

  ad = get_response_data (child, TRUE);

  ad->response_id = response_id;

  if (GTK_IS_BUTTON (child))
    signal_id = g_signal_lookup ("clicked", GTK_TYPE_BUTTON);
  else
    signal_id = GTK_WIDGET_GET_CLASS (child)->activate_signal;

  if (signal_id)
    {
      GClosure *closure;

      closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                       G_OBJECT (message_area));
      g_signal_connect_closure_by_id (child, signal_id, 0, closure, FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a GtkMessageArea");

  gtk_box_pack_end (GTK_BOX (message_area->priv->action_area),
                    child, FALSE, FALSE, 0);
  if (response_id == GTK_RESPONSE_HELP)
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (message_area->priv->action_area),
                                        child, TRUE);
}

/**
 * gtk_message_area_set_contents:
 * @message_area: a #GtkMessageArea
 * @contents: widget you want to add to the contents area
 *
 * Adds the @contents widget to the contents area of #GtkMessageArea.
 */
void
gtk_message_area_set_contents (GtkMessageArea *message_area,
                               GtkWidget      *contents)
{
  g_return_if_fail (GTK_IS_MESSAGE_AREA (message_area));
  g_return_if_fail (GTK_IS_WIDGET (contents));

  gtk_box_pack_start (GTK_BOX (message_area->priv->content_area),
                      contents, TRUE, TRUE, 0);
}

/**
 * gtk_message_area_add_button:
 * @message_area: a #GtkMessageArea
 * @button_text: text of button, or stock ID
 * @response_id: response ID for the button
 *
 * Adds a button with the given text (or a stock button, if button_text
 * is a stock ID) and sets things up so that clicking the button will emit
 * the "response" signal with the given response_id. The button is appended
 * to the end of the message area's action area. The button widget is
 * returned, but usually you don't need it.
 *
 * Returns: the button widget that was added
 */
GtkWidget*
gtk_message_area_add_button (GtkMessageArea *message_area,
                             const gchar    *button_text,
                             gint            response_id)
{
  GtkWidget *button;

  g_return_val_if_fail (GTK_IS_MESSAGE_AREA (message_area), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = gtk_button_new_from_stock (button_text);

  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  gtk_widget_show (button);

  gtk_message_area_add_action_widget (message_area, button, response_id);

  return button;
}

static void
add_buttons_valist (GtkMessageArea *message_area,
                    const gchar    *first_button_text,
                    va_list         args)
{
  const gchar* text;
  gint response_id;

  g_return_if_fail (GTK_IS_MESSAGE_AREA (message_area));

  if (first_button_text == NULL)
    return;

  text = first_button_text;
  response_id = va_arg (args, gint);

  while (text != NULL)
    {
      gtk_message_area_add_button (message_area, text, response_id);

      text = va_arg (args, gchar*);
      if (text == NULL)
        break;

      response_id = va_arg (args, int);
    }
}

/**
 * gtk_message_area_add_buttons:
 * @message_area: a #GtkMessageArea
 * @first_button_text: button text or stock ID
 * @...: response ID for first button, then more text-response_id pairs
 *
 * Adds more buttons, same as calling gtk_message_area_add_button()
 * repeatedly. The variable argument list should be NULL-terminated as with
 * gtk_message_area_new_with_buttons(). Each button must have both text
 * and response ID.
 */
void
gtk_message_area_add_buttons (GtkMessageArea *message_area,
                              const gchar    *first_button_text,
                              ...)
{
  va_list args;

  va_start (args, first_button_text);
  add_buttons_valist (message_area, first_button_text, args);
  va_end (args);
}

/**
 * gtk_message_area_new:
 *
 * Creates a new #GtkMessageArea object.
 *
 * Returns: a new #GtkMessageArea object
 */
GtkWidget *
gtk_message_area_new (void)
{
   return g_object_new (GTK_TYPE_MESSAGE_AREA, NULL);
}

/**
 * gtk_message_area_new_with_buttons:
 * @first_button_text: stock ID or text to go in first button, or NULL
 * @...: response ID for first button, then additional buttons, ending
 *    with NULL
 *
 * Creates a new #GtkMessageArea with buttons. Button text/response ID
 * pairs should be listed, with a NULL pointer ending the list. Button
 * text can be either a stock ID such as %GTK_STOCK_OK, or some arbitrary
 * text. A response ID can be any positive number, or one of the values
 * in the GtkResponseType enumeration. If the user clicks one of these
 * dialog buttons, GtkMessageArea will emit the "response"
 * signal with the corresponding response ID.
 *
 * Returns: a new #GtkMessageArea
 */
GtkWidget *
gtk_message_area_new_with_buttons (const gchar *first_button_text,
                                   ...)
{
  GtkMessageArea *message_area;
  va_list args;

  message_area = GTK_MESSAGE_AREA (gtk_message_area_new ());

  va_start (args, first_button_text);
  add_buttons_valist (message_area, first_button_text, args);
  va_end (args);

  return GTK_WIDGET (message_area);
}

/**
 * gtk_message_area_set_response_sensitive:
 * @message_area: a #GtkMessageArea
 * @response_id: a response ID
 * @setting: TRUE for sensitive
 *
 * Calls gtk_widget_set_sensitive (widget, setting) for each widget
 * in the dialog's action area with the given response_id. A convenient
 * way to sensitize/desensitize dialog buttons.
 */
void
gtk_message_area_set_response_sensitive (GtkMessageArea *message_area,
                                         gint            response_id,
                                         gboolean        setting)
{
  GList *children, *list;

  g_return_if_fail (GTK_IS_MESSAGE_AREA (message_area));

  children = gtk_container_get_children (GTK_CONTAINER (message_area->priv->action_area));

  for (list = children; list; list = list->next)
    {
      GtkWidget *widget = list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        gtk_widget_set_sensitive (widget, setting);
    }

  g_list_free (children);
}

/**
 * gtk_message_area_set_default_response:
 * @message_area: a #GtkMessageArea
 * @response_id: a response ID
 *
 * Sets the last widget in the message area's action area with the
 * given response_id as the default widget for the dialog. Pressing
 * "Enter" normally activates the default widget.
 */
void
gtk_message_area_set_default_response (GtkMessageArea *message_area,
                                       gint            response_id)
{
  GList *children, *list;

  g_return_if_fail (GTK_IS_MESSAGE_AREA (message_area));

  children = gtk_container_get_children (GTK_CONTAINER (message_area->priv->action_area));

  for (list = children; list; list = list->next)
    {
      GtkWidget *widget = list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        gtk_widget_grab_default (widget);
    }

  g_list_free (children);
}

/**
 * gtk_message_area_set_default_response:
 * @message_area: a #GtkMessageArea
 * @response_id: a response ID
 *
 * Emits the 'response' signal with the given @response_id.
 */
void
gtk_message_area_response (GtkMessageArea *message_area,
                           gint            response_id)
{
  g_return_if_fail (GTK_IS_MESSAGE_AREA (message_area));

  g_signal_emit (message_area, signals[RESPONSE], 0, response_id);
}

/**
 * gtk_message_area_add_stock_button_with_text:
 * @message_area: a #GtkMessageArea
 * @text: the text to visualize in the button
 * @stock_id: the stock ID of the button
 * @response_id: a response ID
 *
 * Same as gtk_message_area_add_button() but with a specific text.
 */
GtkWidget *
gtk_message_area_add_stock_button_with_text (GtkMessageArea *message_area,
                                             const gchar    *text,
                                             const gchar    *stock_id,
                                             gint            response_id)
{
  GtkWidget *button, *image;

  g_return_val_if_fail (GTK_IS_MESSAGE_AREA (message_area), NULL);
  g_return_val_if_fail (text != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  button = gtk_button_new_with_mnemonic (text);
  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  gtk_widget_show (button);

  gtk_message_area_add_action_widget (message_area, button, response_id);

  return button;
}

typedef struct
{
  gchar *widget_name;
  gchar *response_id;
} ActionWidgetInfo;

typedef struct
{
  GtkMessageArea *message_area;
  GtkBuilder *builder;
  GSList *items;
  gchar *response;
} ActionWidgetsSubParserData;

static void
attributes_start_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **names,
                          const gchar        **values,
                          gpointer             user_data,
                          GError             **error)
{
  ActionWidgetsSubParserData *parser_data = (ActionWidgetsSubParserData*)user_data;
  guint i;

  if (strcmp (element_name, "action-widget") == 0)
    {
      for (i = 0; names[i]; i++)
        if (strcmp (names[i], "response") == 0)
          parser_data->response = g_strdup (values[i]);
    }
  else if (strcmp (element_name, "action-widgets") == 0)
    return;
  else
    g_warning ("Unsupported tag for GtkMessageArea: %s\n", element_name);
}

static void
attributes_text_element (GMarkupParseContext *context,
                         const gchar         *text,
                         gsize                text_len,
                         gpointer             user_data,
                         GError             **error)
{
  ActionWidgetsSubParserData *parser_data = (ActionWidgetsSubParserData*)user_data;
  ActionWidgetInfo *item;

  if (!parser_data->response)
    return;

  item = g_new (ActionWidgetInfo, 1);
  item->widget_name = g_strndup (text, text_len);
  item->response_id = parser_data->response;
  parser_data->items = g_slist_prepend (parser_data->items, item);
  parser_data->response = NULL;
}

static const GMarkupParser attributes_parser =
{
  attributes_start_element,
  NULL,
  attributes_text_element,
};

gboolean
gtk_message_area_buildable_custom_tag_start (GtkBuildable  *buildable,
                                             GtkBuilder    *builder,
                                             GObject       *child,
                                             const gchar   *tagname,
                                             GMarkupParser *parser,
                                             gpointer      *data)
{
  ActionWidgetsSubParserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "action-widgets") == 0)
    {
      parser_data = g_slice_new0 (ActionWidgetsSubParserData);
      parser_data->message_area = GTK_MESSAGE_AREA (buildable);
      parser_data->items = NULL;

      *parser = attributes_parser;
      *data = parser_data;
      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, data);
}

static void
gtk_message_area_buildable_custom_finished (GtkBuildable *buildable,
                                            GtkBuilder   *builder,
                                            GObject      *child,
                                            const gchar  *tagname,
                                            gpointer      user_data)
{
  GSList *l;
  ActionWidgetsSubParserData *parser_data;
  GObject *object;
  GtkMessageArea *message_area;
  ResponseData *ad;
  guint signal_id;

  if (strcmp (tagname, "action-widgets"))
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
      return;
    }

  message_area = GTK_MESSAGE_AREA (buildable);
  parser_data = (ActionWidgetsSubParserData*)user_data;
  parser_data->items = g_slist_reverse (parser_data->items);

  for (l = parser_data->items; l; l = l->next)
    {
      ActionWidgetInfo *item = l->data;

      object = gtk_builder_get_object (builder, item->widget_name);
      if (!object)
        {
          g_warning ("Unknown object %s specified in action-widgets of %s",
                     item->widget_name,
                     gtk_buildable_get_name (GTK_BUILDABLE (buildable)));
          continue;
        }

      ad = get_response_data (GTK_WIDGET (object), TRUE);
      ad->response_id = atoi (item->response_id);

      if (GTK_IS_BUTTON (object))
        signal_id = g_signal_lookup ("clicked", GTK_TYPE_BUTTON);
      else
        signal_id = GTK_WIDGET_GET_CLASS (object)->activate_signal;

      if (signal_id)
        {
          GClosure *closure;

          closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                           G_OBJECT (message_area));
          g_signal_connect_closure_by_id (object,
                                          signal_id,
                                          0,
                                          closure,
                                          FALSE);
        }

      if (ad->response_id == GTK_RESPONSE_HELP)
        gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (message_area->priv->action_area),
                                            GTK_WIDGET (object), TRUE);

      g_free (item->widget_name);
      g_free (item->response_id);
      g_free (item);
    }
  g_slist_free (parser_data->items);
  g_slice_free (ActionWidgetsSubParserData, parser_data);
}

