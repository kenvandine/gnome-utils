
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "logview.h"
#include "logrtns.h"
#include "log_repaint.h"
#include "misc.h"

/*
 * -------------------
 * Module variables 
 * -------------------
 */

char *month[12] =
{N_("January"), N_("February"), N_("March"), N_("April"), N_("May"),
 N_("June"), N_("July"), N_("August"), N_("September"), N_("October"),
 N_("November"), N_("December")};

enum {
   DATE = 0,
   HOSTNAME,
   PROCESS,
   MESSAGE
};

static void DrawLogLines (LogviewWindow *window, Log *current_log);
static void UpdateStatusArea (LogviewWindow *window);

/**
 * Recursively called until the row specified by orig is found.
 *
 * *count will be set to the  row number of the child
 * relative to the row that was initially passed in as tree_path.
 *
 * *count will be -1 if orig is not found as a child.
 * If NULL is passed in as orig, *count will
 * be a count of the all the children (visible + collapsed (invisible)).
 *
 * NOTE: the value for depth must be 0 when this recursive function
 * is initially called, or it may not function as expected.
 **/
static void 
iterate_thru_children(GtkTreeView  *tree_view,
                      GtkTreeModel *tree_model,
                      GtkTreePath  *tree_path,
                      GtkTreePath  *orig,
                      gint         *count,
                      gint         depth)
{
  GtkTreeIter iter;

  gtk_tree_model_get_iter (tree_model, &iter, tree_path);

  if (orig != NULL && !gtk_tree_path_compare (tree_path, orig)) 
    /* Found it! */
    return;

  if (orig != NULL && gtk_tree_path_compare (tree_path, orig) > 0)
    {
      /* Past it, so return -1 */
      *count = -1;
      return;
    }
  else if (gtk_tree_model_iter_has_child (tree_model, &iter))
    {
      gtk_tree_path_append_index (tree_path, 0);
      iterate_thru_children (tree_view, tree_model, tree_path,
                             orig, count, (depth + 1));
      return;
    }
  else if (gtk_tree_model_iter_next (tree_model, &iter)) 
    {
      (*count)++;
      tree_path = gtk_tree_model_get_path (tree_model, &iter);
      iterate_thru_children (tree_view, tree_model, tree_path,
                             orig, count, depth); 
      gtk_tree_path_free (tree_path);
      return;
  }
  else if (gtk_tree_path_up (tree_path))
    {
      GtkTreeIter temp_iter;
      gboolean exit_loop = FALSE;
      gint new_depth = depth - 1;

      (*count)++;

     /*
      * Make sure that we back up until we find a row
      * where gtk_tree_path_next does not return NULL.
      */
      while (!exit_loop)
        {
          if (gtk_tree_path_get_depth (tree_path) == 0)
              /* depth is now zero so */
            return;
          gtk_tree_path_next (tree_path);	

          /* Verify that the next row is a valid row! */
          exit_loop = gtk_tree_model_get_iter (tree_model, &temp_iter, tree_path);

          if (!exit_loop)
            {
              /* Keep going up until we find a row that has a valid next */
              if (gtk_tree_path_get_depth(tree_path) > 1)
                {
                  new_depth--;
                  gtk_tree_path_up (tree_path);
                }
              else
                {
                 /*
                  * If depth is 1 and gtk_tree_model_get_iter returns FALSE,
                  * then we are at the last row, so just return.
                  */ 
                  if (orig != NULL)
                    *count = -1;

                  return;
                }
            }
        }

     /*
      * This guarantees that we will stop when we hit the end of the
      * children.
      */
      if (new_depth < 0)
        return;

      iterate_thru_children (tree_view, tree_model, tree_path,
                            orig, count, new_depth);
      return;
    }

 /*
  * If it gets here, then the path wasn't found.  Situations
  * that would cause this would be if the path passed in is
  * invalid or contained within the last row, but not visible
  * because the last row is not expanded.  If NULL was passed
  * in then a row count is desired, so only set count to -1
  * if orig is not NULL.
  */
  if (orig != NULL)
    *count = -1;

  return;
}
/* ----------------------------------------------------------------------
   NAME:        handle_row_activation_cb
   DESCRIPTION: User pressed space bar on a row in the main window
   ---------------------------------------------------------------------- */

void
handle_row_activation_cb (GtkTreeView *treeview, GtkTreePath *path,
     GtkTreeViewColumn *arg2, gpointer user_data)
{
    GtkTreeModel *model;
    GtkTreePath *root_tree;
    gint row = 0;
    LogviewWindow *window = user_data;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (window->view));
    root_tree = gtk_tree_path_new_root ();

    iterate_thru_children (GTK_TREE_VIEW (window->view), model, root_tree, path, &row, 0);
    
    gtk_tree_path_free (root_tree);

    window->curlog->current_line_no = row;
}

/* ----------------------------------------------------------------------
   NAME:        handle_selection_changed_cb
   DESCRIPTION: User clicked on main window
   ---------------------------------------------------------------------- */

void
handle_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
    GtkTreeModel *model;
    GList *selected_paths, *i;
    int selected_first = -1, selected_last = -1;
    LogviewWindow *window = data;
    GtkTreePath *selected_path;

    selected_paths = gtk_tree_selection_get_selected_rows (selection, &model);

    if (selected_paths) {
	    for (i = selected_paths; i != NULL; i = g_list_next (i)) {		    
		    GtkTreePath *root_tree = gtk_tree_path_new_root ();
		    int row = 0;
		    selected_path = i->data;
		    iterate_thru_children (GTK_TREE_VIEW (window->view), model, root_tree, selected_path, &row, 0);		    
		    if (selected_last == -1 || row > selected_last)
			    selected_last = row;
		    if (selected_first == -1 || row < selected_first)
			    selected_first = row;
	    }
	    window->curlog->current_line_no = selected_last;
	    window->curlog->current_path = gtk_tree_path_copy (selected_path);
	    window->curlog->selected_line_first = selected_first;
	    window->curlog->selected_line_last = selected_last;
	    g_list_foreach (selected_paths, (GFunc) gtk_tree_path_free, NULL);
	    g_list_free (selected_paths);
    }
}

/* ----------------------------------------------------------------------
   NAME:        log_repaint
   DESCRIPTION: Redraw screen.
   ---------------------------------------------------------------------- */

gboolean
log_repaint (LogviewWindow *window)
{
   GtkTreeModel *tree_model;

   g_return_val_if_fail (LOGVIEW_IS_WINDOW (window), FALSE);

   /* Clear the tree view first */
   tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (window->view));
   gtk_tree_store_clear (GTK_TREE_STORE (tree_model));

   UpdateStatusArea (window);	   

   /* Check that there is at least one log */
   if (window->curlog == NULL) {
	   return FALSE;
   }
	 
	 if (window->curlog->monitored) {
		 gtk_widget_hide (window->log_scrolled_window);
		 gtk_widget_show (window->mon_scrolled_window);
	 } else {
		 gtk_widget_hide (window->mon_scrolled_window);
		 gtk_widget_show (window->log_scrolled_window);
	 }

   /* Draw the tree view */ 
   DrawLogLines (window, window->curlog); 

   return TRUE;

}

void
UpdateStatusArea (LogviewWindow *window)
{
   struct tm *tdm;
   char status_text[255];
   char *utf8;
   char *buffer;
   char *statusbar_text;
   /* Translators: Date only format, %x should well do really */
   const char *time_fmt = _("%x"); /* an evil way to avoid warning */
 
   if (window->curlog == NULL) { 
       gtk_statusbar_pop (GTK_STATUSBAR (window->statusbar), 0);
       return;
   }

   logview_set_window_title (window);

   if (window->curlog->curmark != NULL) {
	   tdm = &(window->curlog)->curmark->fulldate;

       if (strftime (status_text, sizeof (status_text), time_fmt, tdm) <= 0) {
   	       /* as a backup print in US style */
  	       utf8 = g_strdup_printf ("%02d/%02d/%02d", tdm->tm_mday,
                    tdm->tm_mon, tdm->tm_year % 100);
       } else {
           utf8 = LocaleToUTF8 (status_text);
       }
       statusbar_text = g_strconcat (_("Last Modified:"), utf8, NULL);
       gtk_statusbar_pop (GTK_STATUSBAR(window->statusbar), 0);
       gtk_statusbar_push (GTK_STATUSBAR(window->statusbar), 0, statusbar_text);
       
       g_free (utf8);
       g_free (statusbar_text);
   }

}

/* ----------------------------------------------------------------------
   NAME:        GetDateHeader
   DESCRIPTION: Gets the Date to display 
   ---------------------------------------------------------------------- */

char *
GetDateHeader (LogLine *line)
{
   char buf[1024];
   char *utf8;

   if (line->month >= 0 && line->month < 12) {
       struct tm tm = {0};

       tm.tm_mday = line->date;
       tm.tm_year = 2000 /* bogus */;
       tm.tm_mon = line->month;
       /* Translators: Make sure this is only Month and Day format, year
        * will be bogus here */
       if (strftime (buf, sizeof (buf), _("%B %e"), &tm) <= 0) {
           /* If we fail just use the US format */
           utf8 = g_strdup_printf ("%s %d", _(month[(int) line->month]), 
                                   line->date);
       } else {
           utf8 = LocaleToUTF8 (buf);
       }
   } else {
       utf8 = g_strdup_printf ("?%d? %d", (int) line->month, line->date);
   }

   return utf8;

}

/* ----------------------------------------------------------------------
   NAME:        DrawLogLines
   DESCRIPTION: Displays a single log line
   ---------------------------------------------------------------------- */

void
DrawLogLines (LogviewWindow *window, Log *current_log)
{
   GtkTreeModel *tree_model;
   GtkTreeIter iter;
   GtkTreeIter child_iter;
   GtkTreePath *path;
   LogLine *line;
   char tmp[4096];
   char *utf8 = NULL;
   gint i = 0;
   gint cm, cd;

   
   if (!current_log->total_lines) 
       return;

   tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (window->view));
   cm = cd = -1;

   for (i = 0; i < current_log->total_lines; ++i) {
              
       line = (current_log->lines)[i];
       
       /* If data has changed then draw the date header */
       if ((line->month != cm && line->month > 0) ||
           (line->date != cd && line->date > 0)) {
           utf8 = GetDateHeader (line);
           gtk_tree_store_append (GTK_TREE_STORE (tree_model), &iter, NULL);
           gtk_tree_store_set (GTK_TREE_STORE (tree_model), &iter,
               DATE, utf8, HOSTNAME, FALSE,
               PROCESS, FALSE, MESSAGE, FALSE, -1);
           cm = line->month;
           cd = line->date;

           /* store pointer to this date header, using the current
            * month and day as the key */
           g_hash_table_insert (current_log->date_headers,
                        DATEHASH (cm, cd),
                        gtk_tree_model_get_path (tree_model, &iter));
       }
       if (line->hour >= 0 && line->min >= 0 && line->sec >= 0) {
	       struct tm date = {0};
           date.tm_mon = line->month;
           date.tm_year = 70 /* bogus */;
           date.tm_mday = line->date;
           date.tm_hour = line->hour;
           date.tm_min = line->min;
           date.tm_sec = line->sec;
           date.tm_isdst = 0;

	       /* Translators: should be only the time, date could be bogus */
	       if (strftime (tmp, sizeof (tmp), _("%X"), &date) <= 0) {
               /* as a backup print in 24 hours style */
		       utf8 = g_strdup_printf ("%02d:%02d:%02d", line->hour, 
                                       line->min, line->sec);
	       } else {
		       utf8 = LocaleToUTF8 (tmp);
	       }
       } else {
	       utf8 = g_strdup (" ");
       }

       gtk_tree_store_append (GTK_TREE_STORE (tree_model), &child_iter, &iter);
       gtk_tree_store_set (GTK_TREE_STORE (tree_model), &child_iter,
           DATE, utf8, HOSTNAME, LocaleToUTF8 (line->hostname),
           PROCESS, LocaleToUTF8 (line->process),
           MESSAGE, LocaleToUTF8 (line->message), -1);
   }
 
   if (current_log->first_time) {

       /* Expand the rows */
       path = gtk_tree_model_get_path (tree_model, &iter);
       gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view), path, FALSE);
       current_log->first_time = FALSE;

       /* Scroll and set focus on last row */
       path = gtk_tree_model_get_path (tree_model, &child_iter);
       gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->view), path, NULL, FALSE); 
       gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view), path, NULL, TRUE, 
          1, 1);
       gtk_tree_path_free (path);

   } else {
       /* Expand the rows */
       for (i = 0; current_log->expand_paths[i]; ++i) 
           gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view),
               current_log->expand_paths[i], FALSE);
			   
       /* Scroll and set focus on the previously focused row */
       gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->view), 
           current_log->current_path, NULL, FALSE); 
       gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view), 
           current_log->current_path, NULL, TRUE, 1, 1);
   }

   g_free (utf8);
   
}
