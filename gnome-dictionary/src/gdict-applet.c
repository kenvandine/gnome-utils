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
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libgnome/gnome-help.h>
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

  gchar *source_name;  
  gchar *print_font;

  gchar *word;  
  GdictContext *context;
  guint lookup_start_id;
  guint lookup_end_id;
  guint error_id;

  GdictSourceLoader *loader;

  GtkWidget *box;
  GtkWidget *image;
  GtkWidget *entry;
  GtkWidget *window;
  GtkWidget *defbox;
  
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
  
  gdict_defbox_clear (GDICT_DEFBOX (priv->defbox));
}

static void
print_cb (GtkWidget   *widget,
	  GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  
  gdict_show_print_dialog (NULL,
  			   _("Print"),
  			   GDICT_DEFBOX (priv->defbox));
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
  
  g_message ("(in %s) creating aligned window", G_STRFUNC);
  
  window = gtk_aligned_window_new (priv->entry);
  
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_size_request (frame, 330, 200);
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
  g_signal_connect (clear, "clicked", G_CALLBACK (clear_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), clear, FALSE, FALSE, 0);
  gtk_widget_show (clear);

  print = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  g_signal_connect (print, "clicked", G_CALLBACK (print_cb), applet);
  gtk_box_pack_start (GTK_BOX (bbox), print, FALSE, FALSE, 0);
  gtk_widget_show (print);

  
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
gdict_applet_button_press_event_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    GdictApplet    *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  
  g_message ("(in %s) event->button = %d", G_STRFUNC, event->button);
  
  if (event->button == 1)
    {
      gdict_applet_toggle_window (applet); 
      return TRUE;
    }
  else
    return FALSE;
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
  
  /* force window display */  
  gtk_widget_show (priv->window);
  priv->is_window_showing = TRUE;
  
  gdict_defbox_lookup (GDICT_DEFBOX (priv->defbox), priv->word);
}

static gboolean
gdict_applet_entry_button_press_event_cb (GtkWidget      *widget,
					  GdkEventButton *event,
					  GdictApplet    *applet)
{
  panel_applet_request_focus (PANEL_APPLET (applet), event->time);

  return FALSE;
}

static void
gdict_applet_draw (GdictApplet *applet)
{
  GdictAppletPrivate *priv = applet->priv;
  GtkWidget *box;
  GtkWidget *event;
  gchar *text;

  g_message ("(in %s) applet { size = %d, orient = %s }",
  	     G_STRFUNC,
  	     priv->size,
  	     (priv->orient == GTK_ORIENTATION_HORIZONTAL ? "H" : "V"));

#if 0  
  if (priv->entry)
    text = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->entry)));
#endif
  
  if (priv->box)
    gtk_widget_destroy (priv->box);

  g_message ("(in %s) build box (orient : %s)",
  	     G_STRFUNC,
  	     (priv->orient == GTK_ORIENTATION_HORIZONTAL ? "H" : "Z"));
  
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
  
  g_message ("(in %s) box is type %s",
             G_STRFUNC,
             g_type_name (G_OBJECT_TYPE (box)));
  
  g_message ("(in %s) building entry...", G_STRFUNC);
    
  priv->entry = gtk_entry_new ();
  gtk_entry_set_editable (GTK_ENTRY (priv->entry), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (priv->entry), 12);
  g_signal_connect (priv->entry, "activate",
  		    G_CALLBACK (gdict_applet_entry_activate_cb),
  		    applet);
  g_signal_connect (priv->entry, "button-press-event",
		    G_CALLBACK (gdict_applet_entry_button_press_event_cb),
		    applet);
  gtk_box_pack_start (GTK_BOX (box), priv->entry, FALSE, FALSE, 0);
  gtk_widget_show (priv->entry);

#if 0  
  if (text)
    {
      gtk_entry_set_text (GTK_ENTRY (priv->entry), text);
      
      g_free (text);
    }
#endif
  
  g_message ("(in %s) building image (size : %d)...",
             G_STRFUNC,
             priv->size);
             
  event = gtk_event_box_new ();
  g_signal_connect (event, "button-press-event",
  		    G_CALLBACK (gdict_applet_button_press_event_cb), applet);
  gtk_box_pack_start (GTK_BOX (box), event, FALSE, FALSE, 0);
  gtk_widget_show (event);
  
  priv->image = gtk_image_new ();
  gtk_image_set_pixel_size (GTK_IMAGE (priv->image), priv->size);
  gtk_image_set_from_icon_name (GTK_IMAGE (priv->image), "gdict", -1);
  gtk_container_add (GTK_CONTAINER (event), priv->image);
  gtk_widget_show (priv->image);
  
  priv->box = box;
  
  gtk_widget_grab_focus (priv->entry);
  
  gtk_widget_show_all (GTK_WIDGET (applet));
}

static void
gdict_applet_lookup_start_cb (GdictContext *context,
			      GdictApplet  *applet)
{

}

static void
gdict_applet_lookup_end_cb (GdictContext *context,
			    GdictApplet  *applet)
{

}

static void
gdict_applet_error_cb (GdictContext *context,
		       const GError *error,
		       GdictApplet  *applet)
{

}

static void
gdict_applet_cmd_lookup (BonoboUIComponent *component,
			 GdictApplet       *applet,
			 const gchar       *cname)
{

}

static void
gdict_applet_cmd_clear (BonoboUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{

}

static void
gdict_applet_cmd_print (BonoboUIComponent *component,
			GdictApplet       *applet,
			const gchar       *cname)
{

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
      			 _("There was an error displaying help"),
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
  
  gdict_applet_draw (GDICT_APPLET (applet));
  
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
    priv->size = new_size;
  
  gtk_image_set_pixel_size (GTK_IMAGE (priv->image), new_size);
  
  GTK_WIDGET_CLASS (gdict_applet_parent_class)->size_allocate (widget, allocation);
}

static GdictContext *
get_context_from_loader (GdictApplet *applet)
{
  GdictSource *source;
  GdictContext *retval;
  GdictAppletPrivate *priv;
  
  priv = applet->priv;

  if (!priv->source_name)
    priv->source_name = g_strdup (GDICT_DEFAULT_SOURCE_NAME);

  source = gdict_source_loader_get_source (priv->loader,
		  			   priv->source_name);
  if (!source)
    {
      show_error_dialog (NULL,
                         _("Unable to find a dictionary source"),
                         NULL);

      return NULL;
    }
  
  retval = gdict_source_get_context (source);

  /* attach our callbacks */
  priv->lookup_start_id = g_signal_connect (retval, "lookup-start",
		  			    G_CALLBACK (gdict_applet_lookup_start_cb),
					    applet);
  priv->lookup_end_id = g_signal_connect (retval, "lookup-end",
		  			  G_CALLBACK (gdict_applet_lookup_end_cb),
					  applet);
  priv->error_id = g_signal_connect (retval, "error",
		  		     G_CALLBACK (gdict_applet_error_cb),
				     applet);
  
  g_object_unref (source);
  
  return retval;
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
      g_free (priv->print_font);

      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        priv->print_font = g_strdup (gconf_value_get_string (entry->value));
      else
        priv->print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
    }
  else if (strcmp (entry->key, GDICT_GCONF_SOURCE_KEY) == 0)
    {
      if (priv->context)
        {
          g_signal_handler_disconnect (priv->context, priv->lookup_start_id);
	  g_signal_handler_disconnect (priv->context, priv->lookup_end_id);
	  g_signal_handler_disconnect (priv->context, priv->error_id);

	  g_object_unref (priv->context);
	  priv->context = NULL;
	}

      g_free (priv->source_name);
      
      if (entry->value && (entry->value->type == GCONF_VALUE_STRING))
        priv->source_name = g_strdup (gconf_value_get_string (entry->value));
      else
        priv->source_name = g_strdup (GDICT_DEFAULT_SOURCE_NAME);

      priv->context = get_context_from_loader (applet);
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
      g_signal_handler_disconnect (priv->context, priv->lookup_start_id);
      g_signal_handler_disconnect (priv->context, priv->lookup_end_id);
      g_signal_handler_disconnect (priv->context, priv->error_id);

      g_object_unref (priv->context);
    }
  
  if (priv->loader)
    g_object_unref (priv->loader);
  
  if (priv->tooltips)
    g_object_unref (priv->tooltips);
  
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
  
  applet_class->change_background = gdict_applet_change_background;
  applet_class->change_orient = gdict_applet_change_orient;
  
  g_type_class_add_private (gobject_class, sizeof (GdictAppletPrivate));
}

static void
gdict_applet_init (GdictApplet *applet)
{
  GdictAppletPrivate *priv;
  gchar *loader_path;
  
  gtk_window_set_default_icon_name ("gdict");

  panel_applet_set_flags (PANEL_APPLET (applet),
			  PANEL_APPLET_EXPAND_MINOR);
  
  priv = GDICT_APPLET_GET_PRIVATE (applet);
  applet->priv = priv;
  
  /* get the default gconf client */
  if (!priv->gconf_client)
    priv->gconf_client = gconf_client_get_default ();
  
  gconf_client_add_dir (priv->gconf_client,
  			GDICT_GCONF_DIR,
  			GCONF_CLIENT_PRELOAD_ONELEVEL,
  			NULL);
  
  if (!priv->loader)
    priv->loader = gdict_source_loader_new ();

  loader_path = g_build_filename (g_get_home_dir (),
                                  ".gnome2",
                                  "gnome-dictionary",
                                  NULL);
  gdict_source_loader_add_search_path (priv->loader, loader_path);
  g_free (loader_path);
  
  if (!priv->context)
    priv->context = get_context_from_loader (applet);

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

  if (!strcmp (iid, "OAFIID:GNOME_DictionaryApplet"))
    {
      /* Set up the menu */
      panel_applet_setup_menu_from_file (applet, UIDATADIR,
					 "GNOME_DictionaryApplet.xml",
					 NULL,
					 gdict_applet_menu_verbs,
					 applet);

      gdict_applet_draw (GDICT_APPLET (applet));
      gtk_widget_show (GTK_WIDGET (applet));
      
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

  gdict_applet_draw (GDICT_APPLET (applet));
  
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  gtk_container_add (GTK_CONTAINER (window), applet);
  
  gtk_widget_show_all (window);
  
  gtk_main ();
  
  return 0;
#endif /* !GDICT_APPLET_STAND_ALONE */
}
