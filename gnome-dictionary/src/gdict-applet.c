/* gdict-applet.c - GNOME Dictionary Applet
 *
 * Copyright (c) 2005  Emmanuele Bassi <ebassi@gmail.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libgnomeui/gnome-authentication-manager.h>
#include <libgnomeui/gnome-help.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprint/gnome-print-pango.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>

#include "gdict.h"

#include "gdict-applet.h"
#include "gdict-about.h"
#include "gdict-pref-dialog.h"
#include "gdict-print.h"

#include "gtkalignedwindow.h"

#define GDICT_APPLET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_APPLET, GdictAppletClass))
#define GDICT_APPLET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_APPLET, GdictAppletClass))

struct _GdictAppletClass
{
  PanelAppletClass parent_class;
};

#define GDICT_APPLET_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_APPLET, GdictAppletPrivate))

struct _GdictAppletPrivate
{
  guint size;
  GtkOrientation orient;
  
  GtkTooltips *tooltips;
  
  GConfClient *gconf_client;
  guint notify_id;

  gchar *database;
  gchar *strategy;
  gchar *source_name;  
  gchar *print_font;

  gchar *word;  
  GdictContext *context;
  guint lookup_start_id;
  guint lookup_end_id;
  guint error_id;

  GdictSourceLoader *loader;

  GtkWidget *box;
  GtkWidget *toggle;
  GtkWidget *image;
  GtkWidget *entry;
  GtkWidget *window;
  GtkWidget *defbox;

  guint idle_draw_id;

  GdkPixbuf *icon;

  gint window_width;
  gint window_height;

  guint is_window_showing : 1;
};

G_DEFINE_TYPE (GdictApplet, gdict_applet, PANEL_TYPE_APPLET);


static const GtkTargetEntry drop_types[] =
{
  { "text/plain",    0, 0 },
  { "TEXT",          0, 0 },
  { "STRING",        0, 0 },
  { "UTF8_STRING",   0, 0 },
};
static const guint n_drop_types = G_N_ELEMENTS (drop_types);


/* shows an error dialog making it transient for @parent */
static void
show_error_dialog (GtkWindow   *parent,
		   const gchar *message,
		   const gchar *detail)
{
  GtkWidget *dialog;
  
  dialog = gtk_message_dialog_new (parent,
  				   GTK_DIALOG_DESTROY_WITH_PARENT,
  				   GTK_MESSAGE_ERROR,
  				   GTK_BUTTONS_OK,
  				   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), "");
  
  if (detail)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
  					      "%s", detail);
  
  if (parent->group)
    gtk_window_group_add_window (parent->group, GTK_WINDOW (dialog));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

void
set_atk_name_description (GtkWidget  *widget,
			  const char *name,
			  const char *description)
{	
  AtkObject *aobj;
	
  aobj = gtk_widget_get_accessible (widget);
  if (!GTK_IS_ACCESSIBLE (aobj))
    return;

  atk_object_set_name (aobj, name);
  atk_object_set_description (aobj, description);
}

static void
clear_cb (GtkWidget   *widget,
	  GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  gtk_entry_set_text (GTK_ENTRY (priv->entry), "");

  if (!priv->defbox)
    return;
  
  gdict_defbox_clear (GDICT_DEFBOX (priv->defbox));
}

static void
print_cb (GtkWidget   *widget,
	  GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  if (!priv->defbox)
    return;
  
  gdict_show_print_dialog (GTK_WINDOW (priv->window),
  			   _("Print"),
  			   GDICT_DEFBOX (priv->defbox));
}

static void
save_cb (GtkWidget   *widget,
         GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *dialog;

  if (!priv->defbox)
    return;
  
  dialog = gtk_file_chooser_dialog_new (_("Save a Copy"),
  					GTK_WINDOW (priv->window),
  					GTK_FILE_CHOOSER_ACTION_SAVE,
  					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
  					NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  /* default to user's home */
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("Untitled document"));
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;
      gchar *text;
      gsize len;
      GError *write_error = NULL;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      
      text = gdict_defbox_get_text (GDICT_DEFBOX (priv->defbox), &len);
      
      g_file_set_contents (filename,
      			   text,
      			   len,
      			   &write_error);
      if (write_error)
        {
          gchar *message;
          
          message = g_strdup_printf (_("Error while writing to '%s'"), filename);
          
          show_error_dialog (GTK_WINDOW (priv->window),
          		     message,
          		     write_error->message);
          
          g_error_free (write_error);
          g_free (message);
        }
      
      g_free (text);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
gdict_applet_set_menu_items_sensitive (GdictApplet *applet,
				       gboolean     is_sensitive)
{
  BonoboUIComponent *popup_component;
  
  popup_component = panel_applet_get_popup_component (PANEL_APPLET (applet));
  if (!BONOBO_IS_UI_COMPONENT (popup_component))
    return;

  bonobo_ui_component_set_prop (popup_component,
		  		"/commands/Clear",
				"sensitive", (is_sensitive ? "1" : "0"),
				NULL);
  bonobo_ui_component_set_prop (popup_component,
		  		"/commands/Print",
				"sensitive", (is_sensitive ? "1" : "0"),
				NULL);
  bonobo_ui_component_set_prop (popup_component,
		  		"/commands/Save",
				"sensitive", (is_sensitive ? "1" : "0"),
				NULL);
}

static void
gdict_applet_build_window (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *clear;
  GtkWidget *print;
  GtkWidget *save;
  PangoContext *pango_context;
  PangoFontDescription *font_desc;
  gint width, height, font_size;

  if (!priv->entry)
    {
      g_warning ("No entry widget defined");

      return;
    }
  
  window = gtk_aligned_window_new (priv->toggle);

  /* compute the minimum size of the window pane depending on the
   * size of the system font used to render the text.  this will
   * be updated if the style changes
   */
  pango_context = gtk_widget_get_pango_context (window);
  font_desc = pango_context_get_font_description (pango_context);
  font_size = pango_font_description_get_size (font_desc) / PANGO_SCALE;

  width = MAX (52 * font_size, 330);
  height = MAX (24 * font_size, 220);
  
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_size_request (frame, width, height);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);
  
  priv->defbox = gdict_defbox_new ();
  if (priv->context)
    gdict_defbox_set_context (GDICT_DEFBOX (priv->defbox), priv->context);
  
  gtk_box_pack_start (GTK_BOX (vbox), priv->defbox, TRUE, TRUE, 0);
  gtk_widget_show (priv->defbox);
  
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);
  gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);
  
  clear = gtk_button_new_from_stock (GTK_STOCK_CLEAR);

  gtk_tooltips_set_tip (priv->tooltips,
		  	clear,
		  	_("Clear the definitions found"),
			NULL);
  set_atk_name_description (clear,
		  	    _("Clear definition"),
			    _("Clear the text of the definition"));
  
  g_signal_connect (clear, "clicked", G_CALLBACK (clear_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), clear, FALSE, FALSE, 0);
  gtk_widget_show (clear);

  print = gtk_button_new_from_stock (GTK_STOCK_PRINT);

  gtk_tooltips_set_tip (priv->tooltips,
		  	print,
			_("Print the definitions found"),
			NULL);
  set_atk_name_description (print,
		  	    _("Print definition"),
			    _("Print the text of the definition"));
  
  g_signal_connect (print, "clicked", G_CALLBACK (print_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), print, FALSE, FALSE, 0);
  gtk_widget_show (print);

  save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  
  gtk_tooltips_set_tip (priv->tooltips,
		  	save,
			_("Save the definitions found"),
			NULL);
  set_atk_name_description (save,
		  	    _("Save definition"),
			    _("Save the text of the definition to a file"));
  
  g_signal_connect (save, "clicked", G_CALLBACK (save_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), save, FALSE, FALSE, 0);
  gtk_widget_show (save);
  
  priv->window = window;
  priv->is_window_showing = FALSE;
}

static void
gdict_applet_toggle_window (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (!priv->window)
    gdict_applet_build_window (applet);
      
  if (!priv->is_window_showing)
    {
      gtk_widget_show (priv->window);
      priv->is_window_showing = TRUE;
    }
  else
    {
      gtk_widget_hide (priv->window);
      priv->is_window_showing = FALSE;
    }
}

static gboolean
gdict_applet_icon_toggled_cb (GtkWidget   *widget,
			      GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  if (!priv->window)
    gdict_applet_build_window (applet);
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gtk_widget_show (priv->window);
      priv->is_window_showing = TRUE;
    }
  else
    {
      gtk_widget_hide (priv->window);
      priv->is_window_showing = FALSE;
    }
}

static void
gdict_applet_entry_activate_cb (GtkWidget   *widget,
				GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  gchar *text;
  
  text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  if (!text)
    return;
  
  g_free (priv->word);
  priv->word = text;
  
  if (!priv->window)
    gdict_applet_build_window (applet);

  gdict_defbox_lookup (GDICT_DEFBOX (priv->defbox), priv->word);
}

static gboolean
gdict_applet_icon_button_press_event_cb (GtkWidget      *widget,
					 GdkEventButton *event,
					 GdictApplet    *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  
  /* we don't want to block the applet's popup menu unless the
   * user is toggling the button
   */
  if (event->button != 1)
    g_signal_stop_emission_by_name(priv->toggle, "button-press-event");

  return FALSE;
}

static gboolean
gdict_applet_entry_button_press_event_cb (GtkWidget      *widget,
					  GdkEventButton *event,
					  GdictApplet    *applet)
{
  panel_applet_request_focus (PANEL_APPLET (applet), event->time);

  return FALSE;
}

static gboolean
gdict_applet_draw (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *box;
  GtkWidget *hbox;
  GtkWidget *label;
  gchar *text = NULL;

  if (priv->entry)
    text = gtk_editable_get_chars (GTK_EDITABLE (priv->entry), 0, -1);
  
  if (priv->box)
    gtk_widget_destroy (priv->box);

  switch (priv->orient)
    {
    case GTK_ORIENTATION_VERTICAL:
      box = gtk_vbox_new (FALSE, 0);
      break;
    case GTK_ORIENTATION_HORIZONTAL:
      box = gtk_hbox_new (FALSE, 0);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  gtk_container_add (GTK_CONTAINER (applet), box);
  gtk_widget_show (box);

  /* toggle button */
  priv->toggle = gtk_toggle_button_new ();
  
  set_atk_name_description (priv->toggle,
			    _("Toggle dictionary window"),
		  	    _("Show or hide the definition window"));
  gtk_tooltips_set_tip (priv->tooltips,
		  	priv->toggle,
		  	_("Click to view the dictionary window"),
			NULL);
  
  gtk_button_set_relief (GTK_BUTTON (priv->toggle),
		  	 GTK_RELIEF_NONE);
  g_signal_connect (priv->toggle, "toggled",
		    G_CALLBACK (gdict_applet_icon_toggled_cb),
		    applet);
  g_signal_connect (priv->toggle, "button-press-event",
		    G_CALLBACK (gdict_applet_icon_button_press_event_cb),
		    applet);
  gtk_box_pack_start (GTK_BOX (box), priv->toggle, FALSE, FALSE, 0);
  gtk_widget_show (priv->toggle);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (priv->toggle), hbox);
  gtk_widget_show (hbox);

  if (priv->icon)
    {
      GdkPixbuf *scaled;
      
      priv->image = gtk_image_new ();
      gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size - 10);

      scaled = gdk_pixbuf_scale_simple (priv->icon,
		      			priv->size - 5,
					priv->size - 5,
					GDK_INTERP_BILINEAR);
      
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
      g_object_unref (scaled);
      
      gtk_box_pack_start (GTK_BOX (hbox), priv->image, FALSE, FALSE, 0);
      
      gtk_widget_show (priv->image);
    }
  else
    {
      priv->image = gtk_image_new ();

      gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size - 10);
      gtk_image_set_from_stock (GTK_IMAGE (priv->image),
				GTK_STOCK_MISSING_IMAGE,
				-1);
      
      gtk_box_pack_start (GTK_BOX (hbox), priv->image, FALSE, FALSE, 0);
      gtk_widget_show (priv->image);
    }

  /* entry */
  priv->entry = gtk_entry_new ();
  
  set_atk_name_description (priv->entry,
		  	    _("Dictionary entry"),
			    _("Look up words in dictionaries"));
  gtk_tooltips_set_tip (priv->tooltips,
		  	priv->entry,
			_("Type the word you want to look up"),
			NULL);
  
  gtk_entry_set_editable (GTK_ENTRY (priv->entry), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (priv->entry), 12);
  g_signal_connect (priv->entry, "activate",
  		    G_CALLBACK (gdict_applet_entry_activate_cb),
  		    applet);
  g_signal_connect (priv->entry, "button-press-event",
		    G_CALLBACK (gdict_applet_entry_button_press_event_cb),
		    applet);
  gtk_box_pack_end (GTK_BOX (box), priv->entry, FALSE, FALSE, 0);
  gtk_widget_show (priv->entry);

  if (text)
    {
      gtk_entry_set_text (GTK_ENTRY (priv->entry), text);

      g_free (text);
    }
  
  priv->box = box;
  
  gtk_widget_grab_focus (priv->entry);
  
  gtk_widget_show_all (GTK_WIDGET (applet));

  return FALSE;
}

static void
gdict_applet_queue_draw (GdictApplet *applet)
{
  if (!applet->priv->idle_draw_id)
    applet->priv->idle_draw_id = g_idle_add ((GSourceFunc) gdict_applet_draw,
					     applet);
}

static void
gdict_applet_lookup_start_cb (GdictContext *context,
			      GdictApplet  *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  if (!priv->window)
    gdict_applet_build_window (applet);

  if (!priv->is_window_showing)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle), TRUE);
      
      gtk_widget_show (priv->window);
      priv->is_window_showing = TRUE;
    }

  gdict_applet_set_menu_items_sensitive (applet, FALSE);
}

static void
gdict_applet_lookup_end_cb (GdictContext *context,
			    GdictApplet  *applet)
{
  gdict_applet_set_menu_items_sensitive (applet, TRUE);
}

static void
gdict_applet_error_cb (GdictContext *context,
		       const GError *error,
		       GdictApplet  *applet)
{
  GdictAppletPrivate *priv = applet->priv;

  /* force window hide */
  gtk_widget_hide (priv->window);
  priv->is_window_showing = FALSE;
  
  /* disable menu items */
  gdict_applet_set_menu_items_sensitive (applet, FALSE);
}

static void
gdict_applet_cmd_lookup (BonoboUIComponent *component,
			 GdictApplet       *applet,
			 const gchar       *cname)
{
  GdictAppletPrivate *priv = applet->priv;
  gchar *text = NULL;;
  
  text = gtk_editable_get_chars (GTK_EDITABLE (priv->entry), 0, -1);
  if (!text)
    return;
  
  g_free (priv->word);
  priv->word = text;
  
  if (!priv->window)
    gdict_applet_build_window (applet);
  
  gdict_defbox_lookup (GDICT_DEFBOX (priv->defbox), priv->word);
}

static void
gdict_applet_cmd_clear (BonoboUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{
  clear_cb (NULL, applet);
}

static void
gdict_applet_cmd_print (BonoboUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{
  print_cb (NULL, applet);
}

static void
gdict_applet_cmd_save (BonoboUIComponent *component,
		       GdictApplet       *applet,
		       const gchar       *cname)
{
  save_cb (NULL, applet);
}

static void
gdict_applet_cmd_preferences (BonoboUIComponent *component,
			      GdictApplet       *applet,
			      const gchar       *cname)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *pref_dialog;
  
  pref_dialog = gdict_pref_dialog_new (NULL,
  				       _("Preferences"),
  				       priv->loader);
  
  gtk_dialog_run (GTK_DIALOG (pref_dialog));
  
  gtk_widget_destroy (pref_dialog);
}

static void
gdict_applet_cmd_about (BonoboUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{
  gdict_show_about_dialog (NULL);
}

static void
gdict_applet_cmd_help (BonoboUIComponent *component,
		       GdictApplet       *applet,
		       const gchar       *cname)
{
  GError *err = NULL;
  
  gnome_help_display_desktop_on_screen (NULL,
  					"gnome-dictionary",
  					"gnome-dictionary",
  					"gnome-dictionary-applet",
  					gtk_widget_get_screen (GTK_WIDGET (applet)),
  					&err);
  if (err)
    {
      show_error_dialog (NULL,
      			 _("There was an error while displaying help"),
      			 err->message);
      g_error_free (err);
    }
}

static void
gdict_applet_change_background (PanelApplet               *applet,
				PanelAppletBackgroundType  type,
				GdkColor                  *color,
				GdkPixmap                 *pixmap)
{
  if (PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_background)
    PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_background (applet,
    								       type,
    								       color,
    								       pixmap);
}

static void
gdict_applet_change_orient (PanelApplet       *applet,
			    PanelAppletOrient  orient)
{
  GdictAppletPrivate *priv = GDICT_APPLET (applet)->priv;
  guint new_size;
  
  switch (orient)
    {
    case PANEL_APPLET_ORIENT_LEFT:
    case PANEL_APPLET_ORIENT_RIGHT:
      priv->orient = GTK_ORIENTATION_VERTICAL;
      new_size = GTK_WIDGET (applet)->allocation.width;
      break;
    case PANEL_APPLET_ORIENT_UP:
    case PANEL_APPLET_ORIENT_DOWN:
      priv->orient = GTK_ORIENTATION_HORIZONTAL;
      new_size = GTK_WIDGET (applet)->allocation.height;
      break;
    }
  
  if (new_size != priv->size)
    priv->size = new_size;
  
  gdict_applet_queue_draw (GDICT_APPLET (applet));
  
  if (PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_orient)
    PANEL_APPLET_CLASS (gdict_applet_parent_class)->change_orient (applet,
    								   orient);
}

static void
gdict_applet_size_allocate (GtkWidget    *widget,
			    GdkRectangle *allocation)
{
  GdictApplet *applet = GDICT_APPLET (widget);
  GdictAppletPrivate *priv = applet->priv;
  guint new_size;
 
  if (priv->orient == GTK_ORIENTATION_HORIZONTAL)
    new_size = allocation->height;
  else
    new_size = allocation->width;
  
  if (priv->size != new_size)
    {
      priv->size = new_size;

      gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size - 10);

      /* re-scale the icon, if it was found */
      if (priv->icon)
        {
          GdkPixbuf *scaled;

	  scaled = gdk_pixbuf_scale_simple (priv->icon,
			  		    priv->size - 5,
					    priv->size - 5,
					    GDK_INTERP_BILINEAR);
	  
	  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
	  g_object_unref (scaled);
	}
     }

  if (GTK_WIDGET_CLASS (gdict_applet_parent_class)->size_allocate)
    GTK_WIDGET_CLASS (gdict_applet_parent_class)->size_allocate (widget,
		    						 allocation);
}

static void
gdict_applet_style_set (GtkWidget *widget,
			GtkStyle  *old_style)
{
  GdictApplet *applet;
  GdictAppletPrivate *priv;
  PangoContext *pango_context;
  PangoFontDescription *font_desc;
  gint font_size, width, height;
  GtkWidget *frame;
  
  if (GTK_WIDGET_CLASS (gdict_applet_parent_class)->style_set)
    GTK_WIDGET_CLASS (gdict_applet_parent_class)->style_set (widget,
		    					     old_style);

  applet = GDICT_APPLET (widget);
  priv = applet->priv;

  if (!priv->window)
    return;

  frame = GTK_BIN (priv->window)->child;
  if (!frame)
    return;

  pango_context = gtk_widget_get_pango_context (priv->window);
  font_desc = pango_context_get_font_description (pango_context);
  font_size = pango_font_description_get_size (font_desc) / PANGO_SCALE;

  width = MAX (52 * font_size, 330);
  height = MAX (24 * font_size, 220);

  gtk_widget_set_size_request (frame, width, height);
}

static void
gdict_applet_set_database (GdictApplet *applet,
			   const gchar *database)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (priv->database)
    g_free (priv->database);

  if (!database)
    database = gconf_client_get_string (priv->gconf_client,
		    			GDICT_GCONF_DATABASE_KEY,
					NULL);
  
  if (!database)
    database = GDICT_DEFAULT_DATABASE;

  priv->database = g_strdup (database);

  if (priv->defbox)
    gdict_defbox_set_database (GDICT_DEFBOX (priv->defbox), database);
}

static void
gdict_applet_set_strategy (GdictApplet *applet,
			   const gchar *strategy)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (priv->strategy)
    g_free (priv->strategy);

  if (!strategy)
    strategy = gconf_client_get_string (priv->gconf_client,
		    			GDICT_GCONF_STRATEGY_KEY,
					NULL);

  if (!strategy)
    strategy = GDICT_DEFAULT_STRATEGY;

  priv->strategy = g_strdup (strategy);
}

static GdictContext *
get_context_from_loader (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GdictSource *source;
  GdictContext *retval;

  if (!priv->source_name)
    priv->source_name = g_strdup (GDICT_DEFAULT_SOURCE_NAME);

  source = gdict_source_loader_get_source (priv->loader,
		  			   priv->source_name);
  if (!source)
    {
      gchar *detail;
      
      detail = g_strdup_printf (_("No dictionary source available with name '%s'"),
      				priv->source_name);

      show_error_dialog (NULL,
                         _("Unable to find dictionary source"),
                         NULL);
      
      g_free (detail);

      return NULL;
    }
  
  gdict_applet_set_database (applet, gdict_source_get_database (source));
  gdict_applet_set_strategy (applet, gdict_source_get_strategy (source));
  
  retval = gdict_source_get_context (source);
  if (!retval)
    {
      gchar *detail;
      
      detail = g_strdup_printf (_("No context available for source '%s'"),
      				gdict_source_get_description (source));
      				
      show_error_dialog (NULL,
                         _("Unable to create a context"),
                         detail);
      
      g_free (detail);
      g_object_unref (source);
      
      return NULL;
    }
  
  g_object_unref (source);
  
  return retval;
}

static void
gdict_applet_set_print_font (GdictApplet *applet,
			     const gchar *print_font)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (!print_font)
    print_font = gconf_client_get_string (priv->gconf_client,
    					  GDICT_GCONF_PRINT_FONT_KEY,
    					  NULL);
  
  if (!print_font)
    print_font = GDICT_DEFAULT_PRINT_FONT;
  
  if (priv->print_font)
    g_free (priv->print_font);
  
  priv->print_font = g_strdup (print_font);
}

static void
gdict_applet_set_word (GdictApplet *applet,
		       const gchar *word)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (priv->word)
    g_free (priv->word);
  
  priv->word = g_strdup (word);
}

static void
gdict_applet_set_context (GdictApplet  *applet,
			  GdictContext *context)
{
  GdictAppletPrivate *priv = applet->priv;
  
  if (priv->context)
    {
      g_signal_handler_disconnect (priv->context, priv->lookup_start_id);
      g_signal_handler_disconnect (priv->context, priv->lookup_end_id);
      g_signal_handler_disconnect (priv->context, priv->error_id);
      
      priv->lookup_start_id = 0;
      priv->lookup_end_id = 0;
      priv->error_id = 0;
      
      g_object_unref (priv->context);
      priv->context = NULL;
    }

  if (priv->defbox)
    gdict_defbox_set_context (GDICT_DEFBOX (priv->defbox), context);

  if (!context)
    return;
  
  /* attach our callbacks */
  priv->lookup_start_id = g_signal_connect (context, "lookup-start",
					    G_CALLBACK (gdict_applet_lookup_start_cb),
					    applet);
  priv->lookup_end_id   = g_signal_connect (context, "lookup-end",
					    G_CALLBACK (gdict_applet_lookup_end_cb),
					    applet);
  priv->error_id        = g_signal_connect (context, "error",
		  			    G_CALLBACK (gdict_applet_error_cb),
					    applet);
  
  priv->context = context;
}

static void
gdict_applet_set_source_name (GdictApplet *applet,
			      const gchar *source_name)
{
  GdictAppletPrivate *priv = applet->priv;
  GdictContext *context;
  
  /* some sanity checks first */
  if (!source_name)
    source_name = gconf_client_get_string (priv->gconf_client,
      					   GDICT_GCONF_SOURCE_KEY,
      					   NULL);
  
  if (!source_name)
    source_name = GDICT_DEFAULT_SOURCE_NAME;
  
  if (priv->source_name)
    g_free (priv->source_name);
  
  priv->source_name = g_strdup (source_name);
  
  context = get_context_from_loader (applet);
  
  gdict_applet_set_context (applet, context);
}

static void
gdict_applet_gconf_notify_cb (GConfClient *client,
			      guint        cnxn_id,
			      GConfEntry  *entry,
			      gpointer     user_data)
{
  GdictApplet *applet = GDICT_APPLET (user_data);
  GdictAppletPrivate *priv = applet->priv;

  if (strcmp (entry->key, GDICT_GCONF_PRINT_FONT_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_applet_set_print_font (applet, gconf_value_get_string (entry->value));
      else
        gdict_applet_set_print_font (applet, GDICT_DEFAULT_PRINT_FONT);
    }
  else if (strcmp (entry->key, GDICT_GCONF_SOURCE_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_applet_set_source_name (applet, gconf_value_get_string (entry->value));
      else
        gdict_applet_set_source_name (applet, GDICT_DEFAULT_SOURCE_NAME);
    }
  else if (strcmp (entry->key, GDICT_GCONF_DATABASE_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_applet_set_database (applet, gconf_value_get_string (entry->value));
      else
        gdict_applet_set_database (applet, GDICT_DEFAULT_DATABASE);
    }
  else if (strcmp (entry->key, GDICT_GCONF_STRATEGY_KEY) == 0)
    {
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        gdict_applet_set_strategy (applet, gconf_value_get_string (entry->value));
      else
        gdict_applet_set_strategy (applet, GDICT_DEFAULT_STRATEGY);
    }
}

static void
gdict_applet_finalize (GObject *object)
{
  GdictApplet *applet = GDICT_APPLET (object);
  GdictAppletPrivate *priv = applet->priv;
  
  if (priv->gconf_client)
    g_object_unref (priv->gconf_client);
  
  if (priv->context)
    {
      if (priv->lookup_start_id)
        {
          g_signal_handler_disconnect (priv->context, priv->lookup_start_id);
	  g_signal_handler_disconnect (priv->context, priv->lookup_end_id);
	  g_signal_handler_disconnect (priv->context, priv->error_id);
	}

      g_object_unref (priv->context);
    }
  
  if (priv->loader)
    g_object_unref (priv->loader);
  
  if (priv->tooltips)
    g_object_unref (priv->tooltips);

  if (priv->icon)
    g_object_unref (priv->icon);
  
  g_free (priv->source_name);
  g_free (priv->print_font);
  g_free (priv->word);
  
  G_OBJECT_CLASS (gdict_applet_parent_class)->finalize (object);
}

static void
gdict_applet_class_init (GdictAppletClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  PanelAppletClass *applet_class = PANEL_APPLET_CLASS (klass);
  
  gobject_class->finalize = gdict_applet_finalize;
  
  widget_class->size_allocate = gdict_applet_size_allocate;
  widget_class->style_set = gdict_applet_style_set;
  
  applet_class->change_background = gdict_applet_change_background;
  applet_class->change_orient = gdict_applet_change_orient;
  
  g_type_class_add_private (gobject_class, sizeof (GdictAppletPrivate));
}

static void
gdict_applet_init (GdictApplet *applet)
{
  GdictAppletPrivate *priv;
  gchar *data_dir, *icon_file;
  GdkPixbuf *icon;
  GError *icon_error;

  priv = GDICT_APPLET_GET_PRIVATE (applet);
  applet->priv = priv;
      
  /* create the data directory inside $HOME, if it doesn't exist yet */
  data_dir = g_build_filename (g_get_home_dir (),
  			       ".gnome2",
  			       "gnome-dictionary",
  			       NULL);

  if (g_mkdir (data_dir, 0700) == -1)
    {
      if (errno != EEXIST)
	show_error_dialog (NULL,
			   _("Unable to create the data directory '%s'"),
			   NULL);
    }
  
  icon_file = g_build_filename (DATADIR,
		  		"pixmaps",
				"gnome-dictionary.png",
				NULL);
  icon_error = NULL;
  icon = gdk_pixbuf_new_from_file (icon_file, &icon_error);
  if (icon_error)
    {
      show_error_dialog (NULL,
			 _("Unable to load the applet icon"),
			 icon_error->message);

      g_error_free (icon_error);
      priv->icon = NULL;
    }
  else
    {
      gtk_window_set_default_icon (icon);
      priv->icon = icon;
    }

  g_free (icon_file);
  
  panel_applet_set_flags (PANEL_APPLET (applet),
			  PANEL_APPLET_EXPAND_MINOR);
  
  /* get the default gconf client */
  if (!priv->gconf_client)
    priv->gconf_client = gconf_client_get_default ();
  
  gconf_client_add_dir (priv->gconf_client,
  			GDICT_GCONF_DIR,
  			GCONF_CLIENT_PRELOAD_ONELEVEL,
  			NULL);
  gconf_client_notify_add (priv->gconf_client,
		  	   GDICT_GCONF_DIR,
			   (GConfClientNotifyFunc) gdict_applet_gconf_notify_cb,
			   applet, NULL,
			   NULL);
  
  if (!priv->loader)
    priv->loader = gdict_source_loader_new ();

  /* add our data dir inside $HOME to the loader's search paths */
  gdict_source_loader_add_search_path (priv->loader, data_dir);

  g_free (data_dir);

#ifndef GDICT_APPLET_STAND_ALONE
  priv->size = panel_applet_get_size (PANEL_APPLET (applet));

  switch (panel_applet_get_orient (PANEL_APPLET (applet)))
    {
    case PANEL_APPLET_ORIENT_LEFT:
    case PANEL_APPLET_ORIENT_RIGHT:
      priv->orient = GTK_ORIENTATION_VERTICAL;
      break;
    case PANEL_APPLET_ORIENT_UP:
    case PANEL_APPLET_ORIENT_DOWN:
      priv->orient = GTK_ORIENTATION_HORIZONTAL;
      break;
    }
#else
  priv->size = 24;
  priv->orient = GTK_ORIENTATION_HORIZONTAL;
  g_message ("(in %s) applet { size = %d, orient = %s }",
  	     G_STRFUNC,
  	     priv->size,
  	     (priv->orient == GTK_ORIENTATION_HORIZONTAL ? "H" : "V"));
#endif /* !GDICT_APPLET_STAND_ALONE */
  
  priv->tooltips = gtk_tooltips_new ();
  g_object_ref (priv->tooltips);
  gtk_object_sink (GTK_OBJECT (priv->tooltips));

  /* force first draw */
  gdict_applet_draw (applet);

  gdict_applet_set_source_name (applet, NULL);
}

#ifndef GDICT_APPLET_STAND_ALONE
static const BonoboUIVerb gdict_applet_menu_verbs[] =
{
  BONOBO_UI_UNSAFE_VERB ("LookUp", gdict_applet_cmd_lookup),
  
  BONOBO_UI_UNSAFE_VERB ("Clear", gdict_applet_cmd_clear),
  BONOBO_UI_UNSAFE_VERB ("Print", gdict_applet_cmd_print),
  
  BONOBO_UI_UNSAFE_VERB ("Preferences", gdict_applet_cmd_preferences),
  BONOBO_UI_UNSAFE_VERB ("Help", gdict_applet_cmd_help),
  BONOBO_UI_UNSAFE_VERB ("About", gdict_applet_cmd_about),

  BONOBO_UI_VERB_END
};

static gboolean
gdict_applet_factory (PanelApplet *applet,
                      const gchar *iid,
		      gpointer     data)
{
  gboolean retval = FALSE;

  if ((!strcmp (iid, "OAFIID:GNOME_DictionaryApplet")) ||
      (!strcmp (iid, "OAFIID:GNOME_GDictApplet")))
    {
      /* Set up the menu */
      panel_applet_setup_menu_from_file (applet, UIDATADIR,
					 "GNOME_DictionaryApplet.xml",
					 NULL,
					 gdict_applet_menu_verbs,
					 applet);

      gtk_widget_show (GTK_WIDGET (applet));

      /* set the menu items insensitive */
      gdict_applet_set_menu_items_sensitive (GDICT_APPLET (applet), FALSE);
      
      retval = TRUE;
    }

  return retval;
}
#endif /* !GDICT_APPLET_STAND_ALONE */

int
main (int argc, char *argv[])
{
#ifdef GDICT_APPLET_STAND_ALONE
  GtkWidget *window;
  GtkWidget *applet;
#endif

  /* gettext stuff */
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

#ifndef GDICT_APPLET_STAND_ALONE
  gnome_authentication_manager_init();

  gnome_program_init ("gnome-dictionary-applet", VERSION,
		      LIBGNOMEUI_MODULE,
		      argc, argv,
		      GNOME_CLIENT_PARAM_SM_CONNECT, FALSE,
		      GNOME_PROGRAM_STANDARD_PROPERTIES,
		      NULL);

  return panel_applet_factory_main ("OAFIID:GNOME_DictionaryApplet_Factory",
		                    GDICT_TYPE_APPLET,
				    gdict_applet_factory,
				    NULL);
#else
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  applet = GTK_WIDGET (g_object_new (GDICT_TYPE_APPLET, NULL));
  g_message ("(in %s) typeof(applet) = '%s'",
  	     G_STRFUNC,
  	     g_type_name (G_OBJECT_TYPE (applet)));

  gdict_applet_queue_draw (GDICT_APPLET (applet));
  
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  gtk_container_add (GTK_CONTAINER (window), applet);
  
  gtk_widget_show_all (window);
  
  gtk_main ();
  
  return 0;
#endif /* !GDICT_APPLET_STAND_ALONE */
}
