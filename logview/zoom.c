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
#include "gnome.h"
#include "zoom.h"
#include "misc.h"

/*
 *  --------------------------
 *  Local and global variables
 *  --------------------------
 */

extern char *month[12];
extern GList *regexp_db, *descript_db;

int match_line_in_db (LogLine *line, GList *db);


void
zoom_close_button (GtkWidget *zoom_dialog, int arg, gpointer data)
{
	LogviewWindow *window = data;
	close_zoom_view (window);
}

/* ----------------------------------------------------------------------
   NAME:          create_zoom_view
   DESCRIPTION:   Display the statistics of the log.
   ---------------------------------------------------------------------- */

void
create_zoom_view (LogviewWindow *window)
{

   if (window->curlog == NULL || window->zoom_visible)
      return;

   if (window->zoom_dialog == NULL) {
	GtkWidget *zoom_dialog;
   	zoom_dialog  = gtk_dialog_new_with_buttons (_("Entry Detail"), 
					       	    GTK_WINDOW_TOPLEVEL, 
					            GTK_DIALOG_DESTROY_WITH_PARENT,
					            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, 
					            NULL);
        g_signal_connect (G_OBJECT (zoom_dialog), "response",
		          G_CALLBACK (zoom_close_button),
			  window);
   	g_signal_connect (G_OBJECT (zoom_dialog), "destroy",
		          G_CALLBACK (quit_zoom_view),
			  window);
        g_signal_connect (G_OBJECT (zoom_dialog), "delete_event",
		          G_CALLBACK (gtk_true),
			  NULL);
	gtk_dialog_set_has_separator (GTK_DIALOG (zoom_dialog), FALSE);
	gtk_window_set_default_size (GTK_WINDOW (zoom_dialog), 500, 225);
	gtk_dialog_set_default_response (GTK_DIALOG (zoom_dialog), GTK_RESPONSE_CLOSE);
   	gtk_container_set_border_width (GTK_CONTAINER (zoom_dialog), 5);
	window->zoom_dialog = zoom_dialog;
   }

   repaint_zoom(window);
   gtk_widget_show (window->zoom_dialog);
   window->zoom_visible = TRUE; 
}

/* ----------------------------------------------------------------------
   NAME:          repaint_zoom
   DESCRIPTION:   Repaint the zoom window.
   ---------------------------------------------------------------------- */

int
repaint_zoom (LogviewWindow *window)
{
   static GtkWidget *zoom_tree;
   static GtkCellRenderer *renderer;
   static GtkTreeViewColumn *column1, *column2;
   const gchar *titles[] = { N_("Date:"), N_("Process:"),
                N_("Message:"), N_("Description:"), NULL }; 
   GtkTreeIter iter;
   LogLine *line;
   struct tm date = {0};
   char buffer[1024];
   char *utf8;
   gint i;
   GtkWidget *zoom_dialog = window->zoom_dialog;

   g_return_if_fail (window->curlog);

   if (window->zoom_scrolled_window == NULL) {
	   GtkWidget *scrolled_window;
	   GtkListStore *store;
   
	   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	   gtk_widget_set_sensitive (scrolled_window, TRUE);
	   gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 5);
	   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					   GTK_POLICY_AUTOMATIC,
					   GTK_POLICY_AUTOMATIC);

	   store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	   zoom_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (zoom_tree), TRUE); 

	   gtk_container_add (GTK_CONTAINER (scrolled_window), zoom_tree);

	   /* Create Columns */
	   renderer = gtk_cell_renderer_text_new ();
	   column1 = gtk_tree_view_column_new_with_attributes (_("Log Line Details"),
							       renderer, "text", 0, NULL);
	   gtk_tree_view_column_set_sizing (column1, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	   gtk_tree_view_column_set_resizable (column1, TRUE); 
	   gtk_tree_view_append_column (GTK_TREE_VIEW (zoom_tree), column1);
	   gtk_tree_view_column_set_spacing (column1, 12);
  
	   renderer = gtk_cell_renderer_text_new ();
	   column2 = gtk_tree_view_column_new_with_attributes ("", renderer,
							       "text", 1, NULL);
	   gtk_tree_view_column_set_sizing (column2, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	   gtk_tree_view_column_set_resizable (column2, TRUE); 
	   gtk_tree_view_append_column (GTK_TREE_VIEW (zoom_tree), column2);
	   gtk_tree_view_column_set_spacing (column2, 12);

	   /* Add entries to the list */
	   for (i = 0; titles[i]; ++i) {
		   gtk_list_store_append (GTK_LIST_STORE (store), &iter);
		   gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, _(titles[i]), -1);
	   }
	   
	   window->zoom_scrolled_window = scrolled_window;
	   window->zoom_store = store;
	   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window->zoom_dialog)->vbox),
			       scrolled_window, TRUE, TRUE, 0);
   }

   if (window->curlog != NULL)
       g_snprintf (buffer, sizeof (buffer), "%s", window->curlog->name);
   else
       g_snprintf (buffer, sizeof (buffer), _("<No log loaded>"));
   gtk_tree_view_column_set_title (column2, buffer);

   gtk_widget_show_all (window->zoom_scrolled_window);

   /* Check that there is at least one log */
   if (window->curlog == NULL) {
       gtk_list_store_clear (GTK_LIST_STORE (window->zoom_store));
       /* Add entries to the list */
       for (i = 0; titles[i]; ++i) {
           gtk_list_store_append (GTK_LIST_STORE (window->zoom_store), &iter);
           gtk_list_store_set (GTK_LIST_STORE (window->zoom_store), &iter, 0, _(titles[i]), -1);
       }
       return FALSE;
   }
  
   if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (window->zoom_store), &iter)) {

       if (window->curlog->current_line_no <= 0 || 
           ((line = (window->curlog->lines)[window->curlog->current_line_no]) == NULL)) {
       	   return FALSE;
       }

       date.tm_mon = line->month;
       date.tm_year = 70 /* bogus */;
       date.tm_mday = line->date;
       date.tm_hour = line->hour;
       date.tm_min = line->min;
       date.tm_sec = line->sec;
       date.tm_isdst = 0;

       /* Translators: do not include year, it would be bogus */
       if (strftime (buffer, sizeof (buffer), _("%B %e %X"), &date) <= 0) {
           /* as a backup print in US style */
           utf8 = g_strdup_printf ("%s %d %02d:%02d:%02d",
                       _(month[(int)line->month]),
                       (int) line->date,
                       (int) line->hour,
                       (int) line->min,
                       (int) line->sec);
       } else {
           utf8 = LocaleToUTF8 (buffer);
       }

       gtk_list_store_set (GTK_LIST_STORE (window->zoom_store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (window->zoom_store), &iter);
       utf8 = LocaleToUTF8 (line->process);
       gtk_list_store_set (GTK_LIST_STORE (window->zoom_store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (window->zoom_store), &iter);
       utf8 = LocaleToUTF8 (line->message);
       gtk_list_store_set (GTK_LIST_STORE (window->zoom_store), &iter, 1, utf8, -1);
       g_free (utf8);

       gtk_tree_model_iter_next (GTK_TREE_MODEL (window->zoom_store), &iter);
       if (match_line_in_db (line, regexp_db))
           if (find_tag_in_db (line, descript_db)) {
               utf8 = LocaleToUTF8 (line->description->description);
               gtk_list_store_set (GTK_LIST_STORE (window->zoom_store), &iter, 1, utf8, -1);
               g_free (utf8);
       }
               
   }

   return TRUE;

}

/* ----------------------------------------------------------------------
   NAME:          close zoom view
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
close_zoom_view (LogviewWindow *window)
{
   if (window->zoom_visible) {
      gtk_widget_hide (GTK_WIDGET (window->zoom_dialog));
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
				      (gtk_ui_manager_get_widget 
				       (window->ui_manager, "/LogviewMenu/ViewMenu/ShowDetails")),
				      FALSE);
      window->zoom_dialog = NULL;
      window->zoom_scrolled_window = NULL;
      window->zoom_visible = FALSE;
   }
}

void
quit_zoom_view (GtkWidget *zoom_dialog, gpointer data) {
	LogviewWindow *window = data;
	window->zoom_dialog = NULL;
	window->zoom_scrolled_window = NULL;
	window->zoom_visible = FALSE;
}

