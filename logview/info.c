/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */


#include <glib/gi18n.h>
#include "logview.h"
#include "info.h"
#include "misc.h"

/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

static GtkWidget *scrolled_window = NULL;

/* ----------------------------------------------------------------------
   NAME:          LogInfo
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
LogInfo (GtkAction *action, GtkWidget *callback_data)
{
   LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

   if (window->curlog == NULL || window->loginfovisible)
      return;

   if (window->info_dialog == NULL) {
      GtkWidget *InfoDialog;
      InfoDialog = gtk_dialog_new_with_buttons (_("Properties"),
                                                  GTK_WINDOW_TOPLEVEL,
                                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                                  NULL);

      g_signal_connect (G_OBJECT (InfoDialog), "response",
                        G_CALLBACK (CloseLogInfo),
                        window);
      g_signal_connect (GTK_OBJECT (InfoDialog), "destroy",
                        G_CALLBACK (QuitLogInfo),
                        window);
      g_signal_connect (GTK_OBJECT (InfoDialog), "delete_event",
                        G_CALLBACK (gtk_true),
                        NULL);

      gtk_window_set_default_size (GTK_WINDOW (InfoDialog), 425, 200);
      gtk_dialog_set_default_response (GTK_DIALOG (InfoDialog),
                                       GTK_RESPONSE_CLOSE);
      gtk_dialog_set_has_separator (GTK_DIALOG(InfoDialog), FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (InfoDialog), 5);
      gtk_widget_realize (InfoDialog);

      window->info_dialog = InfoDialog;

      RepaintLogInfo (window);
   }

   gtk_widget_show_all (window->info_dialog);
   window->loginfovisible = TRUE;
}

/* ----------------------------------------------------------------------
   NAME:          RepaintLogInfo
   DESCRIPTION:   Repaint the log info window.
   ---------------------------------------------------------------------- */

int
RepaintLogInfo (LogviewWindow *window)
{
   static GtkWidget *info_tree;
   static GtkCellRenderer *renderer;
   static GtkTreeViewColumn *column1, *column2;
   static GtkListStore *store;
   const gchar *titles[] = { N_("Size:"), N_("Modified:"), 
       N_("Start Date:"), N_("Last Date:"), N_("Number of Lines:"), NULL };
   GtkTreeIter iter;
   gchar buffer[1024];
   char *utf8;
   gint i;
   GtkWidget *InfoDialog = window->info_dialog;

   if (scrolled_window == NULL) {

       scrolled_window = gtk_scrolled_window_new (NULL, NULL);
       gtk_widget_set_sensitive (scrolled_window, TRUE);
       gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 5);
       gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_POLICY_AUTOMATIC,
                                       GTK_POLICY_AUTOMATIC);
       gtk_box_pack_start (GTK_BOX (GTK_DIALOG (InfoDialog)->vbox),
                           scrolled_window, TRUE, TRUE, 0);

       store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
       info_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
       gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (info_tree), TRUE);

       gtk_container_add (GTK_CONTAINER (scrolled_window), info_tree);

       /* Create Columns */
       renderer = gtk_cell_renderer_text_new ();
       column1 = gtk_tree_view_column_new_with_attributes (_("Log Information"),
                    renderer, "text", 0, NULL);
       gtk_tree_view_column_set_sizing (column1, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
       gtk_tree_view_column_set_resizable (column1, TRUE);
       gtk_tree_view_append_column (GTK_TREE_VIEW (info_tree), column1);
       gtk_tree_view_column_set_spacing (column1, 12);

       renderer = gtk_cell_renderer_text_new ();
       column2 = gtk_tree_view_column_new_with_attributes ("", renderer,
                    "text", 1, NULL);
       gtk_tree_view_column_set_sizing (column2, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
       gtk_tree_view_column_set_resizable (column2, TRUE);
       gtk_tree_view_append_column (GTK_TREE_VIEW (info_tree), column2);
       gtk_tree_view_column_set_spacing (column2, 12);

       /* Add entries to the list */
       for (i = 0; titles[i]; i++) {
           gtk_list_store_append (GTK_LIST_STORE (store), &iter);
           gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, _(titles[i]), -1);
       }

   }

   if (window->curlog != NULL)
       g_snprintf (buffer, sizeof (buffer), "%s", window->curlog->name);
   else
       g_snprintf (buffer, sizeof (buffer), _("<No log loaded>"));
   gtk_tree_view_column_set_title (column2, buffer);

   gtk_widget_show_all (scrolled_window);

   /* Check that there is at least one log */
   if (window->curlog == NULL) {
       if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (store), &iter)) {
           g_snprintf (buffer, sizeof (buffer), "%c", '\0');
           i = 0;
           while (1) {
               gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, buffer, -1);
               if (!titles[++i])
                   break;
               gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
           }
       }        
       return -1;
   }

   if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (store), &iter)) {
    
       g_snprintf (buffer, sizeof (buffer),
                   ngettext ("%ld byte", "%ld bytes",
                            (long) window->curlog->lstats.size),
                   (long) window->curlog->lstats.size);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, buffer, -1);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, strlen (ctime (&(window->curlog)->lstats.mtime)), "%s",
                   ctime (&(window->curlog)->lstats.mtime));
       utf8 = LocaleToUTF8 (buffer);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, strlen (ctime (&(window->curlog)->lstats.startdate)), "%s",
                   ctime (&(window->curlog)->lstats.startdate));
       utf8 = LocaleToUTF8 (buffer);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, strlen (ctime (&(window->curlog)->lstats.enddate)), "%s",
                   ctime (&(window->curlog)->lstats.enddate));
       utf8 = LocaleToUTF8 (buffer);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, utf8, -1);
       g_free (utf8);
 
       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, sizeof (buffer), "%ld ", window->curlog->lstats.numlines);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, buffer, -1);
 
   }

   return TRUE;
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogInfo
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
CloseLogInfo (GtkWidget *widget, int arg, gpointer data)
{
   LogviewWindow *window = data;
   if (window->loginfovisible)
      gtk_widget_hide (widget);
   window->info_dialog = NULL;
   scrolled_window = NULL;
   window->loginfovisible = FALSE;

}

void QuitLogInfo (GtkWidget *widget, gpointer data)
{
   LogviewWindow *window = data;
   scrolled_window = NULL;
   gtk_widget_destroy (GTK_WIDGET (window->info_dialog));
}

