/* gdict-pref-dialog.c - preferences dialog
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
#include <time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <libgnomeui/gnome-help.h>

#include "gdict.h"

#include "gdict-pref-dialog.h"

enum
{
  SOURCES_ACTIVE_COLUMN = 0,
  SOURCES_NAME_COLUMN,
  SOURCES_DESCRIPTION_COLUMN,
  
  SOURCES_N_COLUMNS
};


#define GDICT_PREFERENCES_GLADE 	PKGDATADIR "/gnome-dictionary-preferences.glade"

struct _GdictPrefDialog
{
  GtkDialog parent_instance;

  GladeXML *xml;
  
  GtkTooltips *tips;
  
  GConfClient *gconf_client;
  guint notify_id;
  
  gchar *active_source;
  GdictSourceLoader *loader;
  GtkListStore *sources_list;
  
  /* direct pointers to widgets */
  GtkWidget *notebook;
  
  GtkWidget *sources_view;
  GtkWidget *sources_add;
  GtkWidget *sources_remove;
  
  gchar *print_font;
  GtkWidget *font_button;
  
  GtkWidget *help_button;
  GtkWidget *close_button;
};

struct _GdictPrefDialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,

  PROP_ACTIVE_SOURCE,
  PROP_SOURCE_LOADER
};


G_DEFINE_TYPE (GdictPrefDialog, gdict_pref_dialog, GTK_TYPE_DIALOG);


static gboolean
select_active_source_name (GtkTreeModel *model,
			   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   gpointer      data)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (data);
  gboolean is_active;
  
  gtk_tree_model_get (model, iter,
      		      SOURCES_ACTIVE_COLUMN, &is_active,
      		      -1);
  if (is_active)
    {
      GtkTreeSelection *selection;
      
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_view));
      
      gtk_tree_selection_select_iter (selection, iter);
      
      return TRUE;
    }
  
  return FALSE;
}

static void
update_sources_view (GdictPrefDialog *dialog)
{
  const GSList *sources, *l;
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view), NULL);
  
  gtk_list_store_clear (dialog->sources_list);
  
  /* force update of the sources list */
  gdict_source_loader_update (dialog->loader);
  
  sources = gdict_source_loader_get_sources (dialog->loader);
  for (l = sources; l != NULL; l = l->next)
    {
      GdictSource *source = GDICT_SOURCE (l->data);
      GtkTreeIter iter;
      const gchar *name, *description;
      gboolean is_active = FALSE;
      
      name = gdict_source_get_name (source);
      description = gdict_source_get_description (source);
      if (!description)
	description = name;

      if (strcmp (name, dialog->active_source) == 0)
        is_active = TRUE;
      
      gtk_list_store_append (dialog->sources_list, &iter);
      gtk_list_store_set (dialog->sources_list, &iter,
      			  SOURCES_ACTIVE_COLUMN, is_active,
      			  SOURCES_NAME_COLUMN, g_strdup (name),
      			  SOURCES_DESCRIPTION_COLUMN, g_strdup (description),
      			  -1);
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view),
  			   GTK_TREE_MODEL (dialog->sources_list));
  
  /* select the currently active source name */
  gtk_tree_model_foreach (GTK_TREE_MODEL (dialog->sources_list),
  			  select_active_source_name,
  			  dialog);
}

static void
source_renderer_toggled_cb (GtkCellRendererToggle *renderer,
			    const gchar           *path,
			    GdictPrefDialog       *dialog)
{
  GtkTreePath *treepath;
  GtkTreeIter iter;
  gboolean res;
  gboolean is_active;
  gchar *name;
  
  treepath = gtk_tree_path_new_from_string (path);
  res = gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->sources_list),
                                 &iter,
                                 treepath);
  if (!res)
    {
      gtk_tree_path_free (treepath);
      
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->sources_list), &iter,
      		      SOURCES_NAME_COLUMN, &name,
      		      SOURCES_ACTIVE_COLUMN, &is_active,
      		      -1);
  if (!is_active && name != NULL)
    {
      g_free (dialog->active_source);
      dialog->active_source = g_strdup (name);
      
      gconf_client_set_string (dialog->gconf_client,
      			       GDICT_GCONF_SOURCE_KEY,
      			       dialog->active_source,
      			       NULL);
      
      update_sources_view (dialog);
      
      g_free (name);
    }
  
  gtk_tree_path_free (treepath);
}

static void
build_sources_view (GdictPrefDialog *dialog)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  
  if (dialog->sources_list)
    return;
    
  dialog->sources_list = gtk_list_store_new (SOURCES_N_COLUMNS,
  					     G_TYPE_BOOLEAN,
  					     G_TYPE_STRING,
  					     G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dialog->sources_list),
  					SOURCES_DESCRIPTION_COLUMN,
  					GTK_SORT_ASCENDING);
  
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
  g_signal_connect (renderer, "toggled",
  		    G_CALLBACK (source_renderer_toggled_cb),
  		    dialog);
  
  column = gtk_tree_view_column_new_with_attributes ("active",
  						     renderer,
  						     "active", SOURCES_ACTIVE_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_view), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("description",
  						     renderer,
  						     "text", SOURCES_DESCRIPTION_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_view), column);
  
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->sources_view), FALSE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view),
  			   GTK_TREE_MODEL (dialog->sources_list));
}

static void
source_transport_combo_changed_cb (GtkComboBox *combo,
				   GtkWidget   *dialog)
{
  GladeXML *xml;
  GtkWidget *add_button;
  gchar *transport;
  
  g_assert (GTK_IS_DIALOG (dialog));
  
  xml = g_object_get_data (G_OBJECT (dialog), "glade-xml");
  g_assert (xml != NULL);
  
  transport = gtk_combo_box_get_active_text (combo);
  if (!transport)
    return;

  add_button = g_object_get_data (G_OBJECT (dialog), "add-button");
  g_assert (add_button != NULL);
  
  /* Translators: this is the same string used in the file
   * gnome-dictionary-preferences.glade for the transport_combo
   * widget items.
   */
  if (strcmp (transport, _("Dictionary Server")) == 0)
    {
      gtk_widget_show (glade_xml_get_widget (xml, "hostname_label"));
      gtk_widget_show (glade_xml_get_widget (xml, "hostname_entry"));
      gtk_widget_show (glade_xml_get_widget (xml, "port_label"));
      gtk_widget_show (glade_xml_get_widget (xml, "port_entry"));
      
      gtk_widget_set_sensitive (add_button, TRUE);
      
      g_object_set_data (G_OBJECT (dialog), "transport",
      			 GINT_TO_POINTER (GDICT_SOURCE_TRANSPORT_DICTD));
    }
  else
    {
      gtk_widget_hide (glade_xml_get_widget (xml, "hostname_label"));
      gtk_widget_hide (glade_xml_get_widget (xml, "hostname_entry"));
      gtk_widget_hide (glade_xml_get_widget (xml, "port_label"));
      gtk_widget_hide (glade_xml_get_widget (xml, "port_entry"));

      gtk_widget_set_sensitive (add_button, TRUE);

      g_object_set_data (G_OBJECT (dialog), "transport",
      			 GINT_TO_POINTER (GDICT_SOURCE_TRANSPORT_INVALID));
    }
}

static void
source_add_dialog_response_cb (GtkDialog       *add_dialog,
			       gint             response,
			       GdictPrefDialog *dialog)
{
  switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
      {
      GladeXML *xml;
      GtkWidget *widget;
      GdictSource *source;
      gchar *name;
      gchar *text;
      GdictSourceTransport transport;
      gchar *host, *port;
      gchar *data;
      gsize length;
      GError *error;
      gchar *filename;
      
      xml = g_object_get_data (G_OBJECT (add_dialog), "glade-xml");
      g_assert (xml != NULL);
      
      source = gdict_source_new ();
      
      /* use the timestamp and the pid to get a unique name */
      name = g_strdup_printf ("source-%lu-%u",
      			      (gulong) time (NULL),
      			      (guint) getpid ());
      gdict_source_set_name (source, name);
      g_free (name);
      
      widget = glade_xml_get_widget (xml, "description_entry");
      text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
      gdict_source_set_description (source, text);
      g_free (text);
      
      widget = glade_xml_get_widget (xml, "database_combo");
      text = gtk_combo_box_get_active_text (GTK_COMBO_BOX (widget));
      gdict_source_set_database (source, text);
      g_free (text);

      widget = glade_xml_get_widget (xml, "strategy_combo");
      text = gtk_combo_box_get_active_text (GTK_COMBO_BOX (widget));
      gdict_source_set_strategy (source, text);
      g_free (text);
      
      /* get the selected transport id */
      transport = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (add_dialog), "transport"));
      switch (transport)
        {
        case GDICT_SOURCE_TRANSPORT_DICTD:
          widget = glade_xml_get_widget (xml, "hostname_entry");
          host = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
          
          widget = glade_xml_get_widget (xml, "port_entry");
          port = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
          
          gdict_source_set_transport (source, GDICT_SOURCE_TRANSPORT_DICTD,
          			      "hostname", host,
          			      "port", atoi (port),
          			      NULL);
          
          g_free (host);
          g_free (port);
          break;
        case GDICT_SOURCE_TRANSPORT_INVALID:
        default:
          g_warning ("Invalid transport");
          return;
        }
      
      error = NULL;
      data = gdict_source_to_data (source, &length, &error);
      if (error)
        {
          g_warning ("Unable to dump GdictSource: %s", error->message);
          
          g_error_free (error);
          g_object_unref (source);
          
          return;
        }
      
      name = g_strdup_printf ("%s.desktop", gdict_source_get_name (source));
      filename = g_build_filename (g_get_home_dir (),
      				   ".gnome2",
      				   "gnome-dictionary",
      				   name,
      				   NULL);
      g_free (name);
      
      g_file_set_contents (filename, data, length, &error);
      if (error)
        {
          g_warning ("Unable to dump GdictSource '%s' into '%s': %s",
                     gdict_source_get_description (source),
                     filename,
                     error->message);
          
          g_error_free (error);
          g_free (filename);
          g_free (data);
          g_object_unref (source);
          
          return;
        }
        
      g_free (filename);
      g_free (data);
      
      g_object_unref (source);
      }
      break;
    case GTK_RESPONSE_HELP:
      {
      GError *err = NULL;
      
      gnome_help_display_desktop_on_screen (NULL,
      					    "gnome-dictionary",
      					    "gnome-dictionary",
      					    "gnome-dictionary-add-source",
      					    gtk_widget_get_screen (GTK_WIDGET (dialog)),
      					    &err);
      if (err)
        {
          GtkWidget *error_dialog;
          gchar *message;
          
          error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      						 GTK_DIALOG_DESTROY_WITH_PARENT,
      						 GTK_MESSAGE_ERROR,
      						 GTK_BUTTONS_OK,
      						 "%s", _("There was an error while displaying help"));
          gtk_window_set_title (GTK_WINDOW (error_dialog), "");
          
          gtk_dialog_run (GTK_DIALOG (error_dialog));
          
          gtk_widget_destroy (error_dialog);
          
          g_error_free (err);
        }
      }
      break;
    case GTK_RESPONSE_CANCEL:
    default:
      break;
    }
}

static void
source_add_clicked_cb (GtkWidget       *widget,
		       GdictPrefDialog *dialog)
{
  GtkWidget *add_dialog, *button;
  GladeXML *xml;
  
  add_dialog = gtk_dialog_new ();

  /* set dialog properties */
  gtk_window_set_title (GTK_WINDOW (add_dialog), _("Add Dictionary Source"));
  gtk_window_set_transient_for (GTK_WINDOW (add_dialog), GTK_WINDOW (dialog));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (add_dialog), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (add_dialog), FALSE);
  
  gtk_container_set_border_width (GTK_CONTAINER (add_dialog), 5);
  
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (add_dialog)->vbox), 2);
  
  gtk_dialog_set_has_separator (GTK_DIALOG (add_dialog), FALSE);
  
  /* add buttons, and keep the "add" button around */
  gtk_dialog_add_button (GTK_DIALOG (add_dialog),
  			 "gtk-help",
  			 GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (add_dialog),
  			 "gtk-cancel",
  			 GTK_RESPONSE_CANCEL);
  button = gtk_dialog_add_button (GTK_DIALOG (add_dialog),
  				  "gtk-add",
  				  GTK_RESPONSE_ACCEPT);
  gtk_widget_set_sensitive (button, FALSE);
  g_object_set_data (G_OBJECT (add_dialog), "add-button", button);
  
  xml = glade_xml_new (GDICT_PREFERENCES_GLADE,
  		       "source_add_root",
  		       NULL);
  g_assert (xml != NULL);
  g_object_set_data (G_OBJECT (add_dialog), "glade-xml", xml);
  
  /* the main widget */
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (add_dialog)->vbox),
                     glade_xml_get_widget (xml, "source_add_root"));
  
  /* show the transport-specific widgets when the transport changes */
  g_signal_connect (glade_xml_get_widget (xml, "transport_combo"),
  		    "changed",
  		    G_CALLBACK (source_transport_combo_changed_cb),
  		    add_dialog);
  
  g_signal_connect (add_dialog, "response",
  		    G_CALLBACK (source_add_dialog_response_cb),
  		    dialog);
  
  gtk_dialog_run (GTK_DIALOG (add_dialog));

  gtk_widget_destroy (add_dialog);
  g_object_unref (xml);
  
  update_sources_view (dialog);
}

static void
source_remove_clicked_cb (GtkWidget       *widget,
			  GdictPrefDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean is_selected;
  GdictSource *source;
  gchar *name, *description;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_view));
  if (!selection)
    return;
  
  is_selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!is_selected)
    return;
    
  gtk_tree_model_get (model, &iter,
  		      SOURCES_NAME_COLUMN, &name,
  		      SOURCES_DESCRIPTION_COLUMN, &description,
  		      -1);
  if (!name) 
    return;
  else
    {
      GtkWidget *confirm_dialog;
      gint response;
      
      confirm_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					       GTK_DIALOG_DESTROY_WITH_PARENT,
      					       GTK_MESSAGE_WARNING,
      					       GTK_BUTTONS_NONE,
      					       _("Remove '%s'"), description);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (confirm_dialog),
      						_("This will permanently remove the "
      						  "dictionary source from the list"));
      
      gtk_dialog_add_button (GTK_DIALOG (confirm_dialog),
      			     GTK_STOCK_REMOVE,
      			     GTK_RESPONSE_OK);
      gtk_dialog_add_button (GTK_DIALOG (confirm_dialog),
      			     GTK_STOCK_CANCEL,
      			     GTK_RESPONSE_CANCEL);
      
      gtk_window_set_title (GTK_WINDOW (confirm_dialog), "");
      
      response = gtk_dialog_run (GTK_DIALOG (confirm_dialog));
      if (response == GTK_RESPONSE_CANCEL)
        {
          gtk_widget_destroy (confirm_dialog);
          
          goto out;
        }
      
      gtk_widget_destroy (confirm_dialog);
    }
  
  if (gdict_source_loader_remove_source (dialog->loader, name))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  else
    {
      GtkWidget *error_dialog;
      gchar *message;
      
      message = g_strdup_printf (_("Unable to remove source '%s'"),
      				 description);
      
      error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					     GTK_DIALOG_DESTROY_WITH_PARENT,
      					     GTK_MESSAGE_ERROR,
      					     GTK_BUTTONS_OK,
      					     "%s", message);
      gtk_window_set_title (GTK_WINDOW (error_dialog), "");
      
      gtk_dialog_run (GTK_DIALOG (error_dialog));
      
      gtk_widget_destroy (error_dialog);
    }

out:
  g_free (name);
  g_free (description);
  
  update_sources_view (dialog);
}

static void
set_source_loader (GdictPrefDialog   *dialog,
		   GdictSourceLoader *loader)
{
  if (!dialog->sources_list)
    return;
  
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  dialog->loader = g_object_ref (loader);
  
  update_sources_view (dialog);
}

static void
font_button_font_set_cb (GtkWidget       *font_button,
			 GdictPrefDialog *dialog)
{
  const gchar *font;
  
  font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (font_button));
  
  if (strcmp (dialog->print_font, font) != 0)
    {
      g_free (dialog->print_font);
      dialog->print_font = g_strdup (font);
      
      gconf_client_set_string (dialog->gconf_client,
  			       GDICT_GCONF_PRINT_FONT_KEY,
  			       dialog->print_font,
  			       NULL);
    }
}

static void
gdict_pref_dialog_gconf_notify_cb (GConfClient *client,
				   guint        cnxn_id,
				   GConfEntry  *entry,
				   gpointer     user_data)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (user_data);
  
  if (strcmp (entry->key, GDICT_GCONF_SOURCE_KEY) == 0)
    {
      if (entry->value && entry->value->type == GCONF_VALUE_STRING)
        {
          g_free (dialog->active_source);
          dialog->active_source = g_strdup (gconf_value_get_string (entry->value));
        }
      else
        {
          g_free (dialog->active_source);
          dialog->active_source = g_strdup (GDICT_DEFAULT_SOURCE_NAME);
        }
      
      update_sources_view (dialog);
    }
  else if (strcmp (entry->key, GDICT_GCONF_PRINT_FONT_KEY) == 0)
    {
      if (entry->value && entry->value->type == GCONF_VALUE_STRING)
        {
          g_free (dialog->print_font);
          dialog->print_font = g_strdup (gconf_value_get_string (entry->value));
        }
      else
        {
          g_free (dialog->print_font);
          dialog->print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
        }
    }
}

static void
gdict_pref_dialog_finalize (GObject *object)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  if (dialog->tips)
    g_object_unref (dialog->tips);
  
  if (dialog->notify_id);
    gconf_client_notify_remove (dialog->gconf_client, dialog->notify_id);
  
  if (dialog->gconf_client)
    g_object_unref (dialog->gconf_client);
  
  if (dialog->xml)
    g_object_unref (dialog->xml);

  if (dialog->active_source)
    g_free (dialog->active_source);
  
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  G_OBJECT_CLASS (gdict_pref_dialog_parent_class)->finalize (object);
}

static void
gdict_pref_dialog_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_ACTIVE_SOURCE:
      g_free (dialog->active_source);
      dialog->active_source = g_strdup (g_value_get_string (value));
      break;
    case PROP_SOURCE_LOADER:
      set_source_loader (dialog, g_value_get_object (value));
      break;
    default:
      break;
    }
}

static void
gdict_pref_dialog_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_ACTIVE_SOURCE:
      g_value_set_object (value, dialog->active_source);
      break;
    case PROP_SOURCE_LOADER:
      g_value_set_object (value, dialog->loader);
      break;
    default:
      break;
    }
}

static void
gdict_pref_dialog_class_init (GdictPrefDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = gdict_pref_dialog_set_property;
  gobject_class->get_property = gdict_pref_dialog_get_property;
  gobject_class->finalize = gdict_pref_dialog_finalize;
  
  g_object_class_install_property (gobject_class,
		  		   PROP_ACTIVE_SOURCE,
				   g_param_spec_string ("active-source",
					   		_("Active Source"),
							_("The name of the active source"),
							NULL,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  				   			_("Source Loader"),
  				   			_("The GdictSourceLoader used by the application"),
  				   			GDICT_TYPE_SOURCE_LOADER,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
}

static void
gdict_pref_dialog_init (GdictPrefDialog *dialog)
{
  gchar *font;
  
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 2);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  
  /* add buttons */
  gtk_dialog_add_button (GTK_DIALOG (dialog),
  			 "gtk-help",
  			 GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (dialog),
  			 "gtk-close",
  			 GTK_RESPONSE_ACCEPT);

  dialog->tips = gtk_tooltips_new ();
  g_object_ref (dialog->tips);
  gtk_object_sink (GTK_OBJECT (dialog->tips));
  
  dialog->gconf_client = gconf_client_get_default ();
  gconf_client_add_dir (dialog->gconf_client,
  			GDICT_GCONF_DIR,
  			GCONF_CLIENT_PRELOAD_ONELEVEL,
  			NULL);
  dialog->notify_id = gconf_client_notify_add (dialog->gconf_client,
  					       GDICT_GCONF_DIR,
		  			       gdict_pref_dialog_gconf_notify_cb,
  					       dialog,
  					       NULL,
  					       NULL);

  /* get the UI from the glade file */
  dialog->xml = glade_xml_new (GDICT_PREFERENCES_GLADE,
  			       "preferences_root",
  			       NULL);
  g_assert (dialog->xml);
  
  /* the main widget */
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                     glade_xml_get_widget (dialog->xml, "preferences_root"));

  /* keep all the interesting widgets around */  
  dialog->notebook = glade_xml_get_widget (dialog->xml, "preferences_notebook");
  
  dialog->sources_view = glade_xml_get_widget (dialog->xml, "sources_treeview");
  build_sources_view (dialog);

  dialog->active_source = gconf_client_get_string (dialog->gconf_client,
		  				   GDICT_GCONF_SOURCE_KEY,
						   NULL);
  if (!dialog->active_source)
    dialog->active_source = g_strdup (GDICT_DEFAULT_SOURCE_NAME);
  
  dialog->sources_add = glade_xml_get_widget (dialog->xml, "add_button");
  gtk_tooltips_set_tip (dialog->tips,
  			dialog->sources_add,
  			_("Add a new dictionary source"),
  			NULL);
  g_signal_connect (dialog->sources_add, "clicked",
  		    G_CALLBACK (source_add_clicked_cb), dialog);
  		    
  dialog->sources_remove = glade_xml_get_widget (dialog->xml, "remove_button");
  gtk_tooltips_set_tip (dialog->tips,
  			dialog->sources_remove,
  			_("Remove the curretly selected dictionary source"),
  			NULL);
  g_signal_connect (dialog->sources_remove, "clicked",
  		    G_CALLBACK (source_remove_clicked_cb), dialog);
  
  font = gconf_client_get_string (dialog->gconf_client,
  				  GDICT_GCONF_PRINT_FONT_KEY,
  				  NULL);
  if (!font)
    font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  dialog->font_button = glade_xml_get_widget (dialog->xml, "print_font_button");
  gtk_font_button_set_font_name (GTK_FONT_BUTTON (dialog->font_button), font);
  gtk_tooltips_set_tip (dialog->tips,
  			dialog->font_button,
  			_("Set the font used for printing the definitions"),
  			NULL);
  g_signal_connect (dialog->font_button, "font-set",
  		    G_CALLBACK (font_button_font_set_cb), dialog);
  g_free (font);
  
  gtk_widget_show_all (dialog->notebook);
}

GtkWidget *
gdict_pref_dialog_new (GtkWindow         *parent,
		       const gchar       *title,
		       GdictSourceLoader *loader)
{
  GtkWidget *retval;
  
  g_return_val_if_fail ((parent == NULL || GTK_IS_WINDOW (parent)), NULL);
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  retval = g_object_new (GDICT_TYPE_PREF_DIALOG,
  			 "source-loader", loader,
  			 "title", title,
  			 NULL);

  gtk_window_set_transient_for (GTK_WINDOW (retval), parent);
  
  return retval;
}
