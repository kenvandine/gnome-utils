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

#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

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
  
  gchar *active_source;
  GdictSourceLoader *loader;
  GtkListStore *sources_list;
  
  /* direct pointers to widgets */
  GtkWidget *notebook;
  
  GtkWidget *sources_view;
  GtkWidget *sources_add;
  GtkWidget *sources_remove;
  
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



static void
update_sources_view (GdictPrefDialog *dialog)
{
  const GSList *sources, *l;
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view), NULL);
  
  gtk_list_store_clear (dialog->sources_list);
  
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
      			  SOURCES_NAME_COLUMN, name,
      			  SOURCES_DESCRIPTION_COLUMN, description,
      			  -1);
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view),
  			   GTK_TREE_MODEL (dialog->sources_list));
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
  
  column = gtk_tree_view_column_new_with_attributes (_("Active"),
  						     renderer,
  						     "active", SOURCES_ACTIVE_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_view), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Description"),
  						     renderer,
  						     "text", SOURCES_DESCRIPTION_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_view), column);
  
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->sources_view), FALSE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view),
  			   GTK_TREE_MODEL (dialog->sources_list));
}

static GdictSource *
build_source_from_dialog (GladeXML *xml)
{
  GtkWidget *label;
  gchar *data;
  GdictSource *source;
  
}

static void
populate_transport_combo (GtkWidget *combo)
{
  GtkListStore *store;
  
  store = gtk_list_store_new (2, G_TYPE_STRING, GDICT_TYPE_SOURCE_TRANSPORT);
  
  
}

static void
source_add_clicked_cb (GtkWidget       *widget,
		       GdictPrefDialog *dialog)
{
#if 0
  GtkWidget *add_dialog;
  GtkWidget *vbox;
  GtkWidget *combo;
  GladeXML *xml;
  GdictSource *source;
  gchar *filename;
  gint response;
  
  xml = glade_xml_new (GDICT_PREFERENCES_GLADE,
  		       "source_add_root",
  		       NULL);
  if (!xml)
    {
      g_warning ("Unable to create the dialog UI");
      
      return;
    }
  
  add_dialog = gtk_dialog_new ();
  gtk_dialog_add_button (GTK_DIALOG (add_dialog),
  			 GTK_STOCK_ADD,
  			 GTK_RESPONSE_APPLY);
  gtk_dialog_add_button (GTK_DIALOG (add_dialog),
  			 GTK_STOCK_CANCEL,
  			 GTK_RESPONSE_CANCEL);	 
  
  vbox = glade_xml_get_widget (xml, "source_add_root");
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (add_dialog)->vbox), vbox);
  gtk_widget_show_all (vbox);
  
  combo = glade_xml_get_widget (xml, "transport_combo");
  populate_transport_combo (combo);
  
  response = gtk_dialog_run (GTK_DIALOG (add_dialog));
  
  switch (response)
    {
    case GTK_RESPONSE_APPLY:
      source = build_source_from_dialog (xml);
      break;
    default:
      break;
    }
  
  if (source)
    g_object_unref (source);
  
  g_free (filename);
  
  gtk_widget_destroy (add_dialog);
  g_object_unref (xml);
#endif
}

static void
source_remove_clicked_cb (GtkWidget       *widget,
			  GdictPrefDialog *dialog)
{
#if 0
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
  		      SOURCES_DESCRIPTION_COLUMN, &description
  		      -1);
  if (!name)
    return;
  
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
  
  g_free (name);
  g_free (description);
#endif
}

static void
set_source_loader (GdictPrefDialog   *dialog,
		   GdictSourceLoader *loader)
{
  const GSList *sources, *l;
  
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
  
  gconf_client_set_string (dialog->gconf_client,
  			   GDICT_GCONF_PRINT_FONT_KEY,
  			   font,
  			   NULL);
}

static void
gdict_pref_dialog_finalize (GObject *object)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  if (dialog->tips)
    g_object_unref (dialog->tips);
  
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
  			GCONF_CLIENT_PRELOAD_NONE,
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
    dialog->active_source = g_strdup (_("Default"));
  
  dialog->sources_add = glade_xml_get_widget (dialog->xml, "add_button");
  g_signal_connect (dialog->sources_add, "clicked",
  		    G_CALLBACK (source_add_clicked_cb), dialog);
  		    
  dialog->sources_remove = glade_xml_get_widget (dialog->xml, "remove_button");
  g_signal_connect (dialog->sources_remove, "clicked",
  		    G_CALLBACK (source_remove_clicked_cb), dialog);
  
  font = gconf_client_get_string (dialog->gconf_client,
  				  GDICT_GCONF_PRINT_FONT_KEY,
  				  NULL);
  if (!font)
    font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  dialog->font_button = glade_xml_get_widget (dialog->xml, "print_font_button");
  gtk_font_button_set_font_name (GTK_FONT_BUTTON (dialog->font_button), font);
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
