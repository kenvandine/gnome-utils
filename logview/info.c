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


#include <config.h>
#include <gnome.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "logview.h"
#if 0
#include "close.xpm"
#endif

void LogInfo (GtkWidget * widget, gpointer user_data);
void CloseLogInfo (GtkWidget * widget, GtkWindow ** window);
void QuitLogInfo (GtkWidget *widget, gpointer data);
int RepaintLogInfo (void);



/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

int loginfovisible;
static GtkWidget *scrolled_window = NULL;
GtkWidget *InfoDialog;

extern Log *curlog;


/* ----------------------------------------------------------------------
   NAME:          LogInfo
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
LogInfo (GtkWidget * widget, gpointer user_data)
{

   if (curlog == NULL || loginfovisible)
      return;

   if (InfoDialog == NULL) {
      InfoDialog = gtk_dialog_new_with_buttons (_("Log stats"),
                                                  GTK_WINDOW_TOPLEVEL,
                                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                                  NULL);

      /* FIXME: no need to pass InfoDialog - its global !! */ 
      g_signal_connect (G_OBJECT (InfoDialog), "response",
                        G_CALLBACK (CloseLogInfo),
                        &InfoDialog);
      g_signal_connect (GTK_OBJECT (InfoDialog), "destroy",
                        G_CALLBACK (QuitLogInfo),
                        &InfoDialog);
      g_signal_connect (GTK_OBJECT (InfoDialog), "delete_event",
                        G_CALLBACK (gtk_true),
                        NULL);

      gtk_window_set_default_size (GTK_WINDOW (InfoDialog), 425, 200);
      gtk_dialog_set_default_response (GTK_DIALOG (InfoDialog),
                                       GTK_RESPONSE_CLOSE);
      gtk_container_set_border_width (GTK_CONTAINER (InfoDialog), 0);
      gtk_widget_realize (InfoDialog);

      RepaintLogInfo ();

   }

   gtk_widget_show_all (InfoDialog);

   loginfovisible = TRUE;

}

/* ----------------------------------------------------------------------
   NAME:          RepaintLogInfo
   DESCRIPTION:   Repaint the log info window.
   ---------------------------------------------------------------------- */

int
RepaintLogInfo (void)
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

   if (scrolled_window == NULL) {

       scrolled_window = gtk_scrolled_window_new (NULL, NULL);
       gtk_widget_set_sensitive (scrolled_window, TRUE);
       gtk_container_set_border_width (GTK_CONTAINER (scrolled_window),
                                       GNOME_PAD_SMALL);
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
       gtk_tree_view_column_set_spacing (column1, GNOME_PAD_BIG);

       renderer = gtk_cell_renderer_text_new ();
       column2 = gtk_tree_view_column_new_with_attributes ("", renderer,
                    "text", 1, NULL);
       gtk_tree_view_column_set_sizing (column2, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
       gtk_tree_view_column_set_resizable (column2, TRUE);
       gtk_tree_view_append_column (GTK_TREE_VIEW (info_tree), column2);
       gtk_tree_view_column_set_spacing (column2, GNOME_PAD_BIG);

       /* Add entries to the list */
       for (i = 0; titles[i]; i++) {
           gtk_list_store_append (GTK_LIST_STORE (store), &iter);
           gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, titles[i], -1);
       }

   }

   if (curlog != NULL)
       g_snprintf (buffer, sizeof (buffer), "%s", curlog->name);
   else
       g_snprintf (buffer, sizeof (buffer), _("<No log loaded>"));
   gtk_tree_view_column_set_title (column2, buffer);

   gtk_widget_show_all (scrolled_window);

   /* Check that there is at least one log */
   if (curlog == NULL) {
       if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (store), &iter)) {
           g_snprintf (buffer, sizeof (buffer), "");
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
                   _("%ld bytes"), (long) curlog->lstats.size);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, buffer, -1);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, strlen (ctime (&curlog->lstats.mtime)), "%s",
                   ctime (&curlog->lstats.mtime));
       utf8 = LocaleToUTF8 (buffer);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, strlen (ctime (&curlog->lstats.startdate)), "%s",
                   ctime (&curlog->lstats.startdate));
       utf8 = LocaleToUTF8 (buffer);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, strlen (ctime (&curlog->lstats.enddate)), "%s",
                   ctime (&curlog->lstats.enddate));
       utf8 = LocaleToUTF8 (buffer);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, utf8, -1);
       g_free (utf8);
 
       gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
       g_snprintf (buffer, sizeof (buffer), "%ld ", curlog->lstats.numlines);
       gtk_list_store_set (GTK_LIST_STORE (store), &iter, 1, buffer, -1);
 
   }

   return TRUE;
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogInfo
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
CloseLogInfo (GtkWidget * widget, GtkWindow ** window)
{
   if (loginfovisible)
      gtk_widget_hide (InfoDialog);
   InfoDialog = NULL;
   scrolled_window = NULL;
   loginfovisible = FALSE;

}

void QuitLogInfo (GtkWidget *widget, gpointer data)
{
   scrolled_window = NULL;
   gtk_widget_destroy (GTK_WIDGET (InfoDialog));
}

