
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
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <gnome.h>
#include "logview.h"
#include "logrtns.h"

/* 
 * -------------------
 * Global variables 
 * -------------------
 */

extern int loginfovisible, calendarvisible;
extern int zoom_visible;
extern ConfigData *cfg;
extern GtkLabel *date_label;
extern UserPrefsStruct *user_prefs;
extern GtkWidget *view;
extern GtkWidget *app;


/*
 * -------------------
 * Module variables 
 * -------------------
 */

Log *loglist[MAX_NUM_LOGS];
Log *curlog;
int numlogs, curlognum;

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

/*
 * -------------------
 * Function prototypes
 * -------------------
 */

int InitPages (void);
int GetLineAtCursor (int y);
int RepaintCalendar (GtkWidget * widget, GdkEventExpose * event);
int RepaintLogInfo (void);
int repaint_zoom (void);
gboolean log_repaint (void);
void log_redrawdetail (void);
void DrawLogLines (Log *current_log);
void CloseApp (void);
void UpdateStatusArea (void);
void change_log (int direction);
void create_zoom_view (void);
void close_zoom_view (GtkWidget *widget, gpointer data);
gboolean handle_log_mouse_button (GtkWidget *view, GdkEventButton * event_key);
void handle_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
void handle_row_activation_cb (GtkTreeView *treeview, GtkTreePath *path,
     GtkTreeViewColumn *arg2, gpointer user_data);
void save_rows_to_expand (Log *log);

static void iterate_thru_children(GtkTreeView  *tree_view,
                      GtkTreeModel *tree_model, GtkTreePath  *tree_path,
                      GtkTreePath  *orig, gint *count, gint depth);

/**
 * Recursively called until the row specified by orig is found.
 *
 * *count will be set to the visible row number of the child
 * relative to the row that was initially passed in as tree_path.
 *
 * *count will be -1 if orig is not found as a child (a row that is
 * not visible will not be found, e.g. if the row is inside a
 * collapsed row).  If NULL is passed in as orig, *count will
 * be a count of the visible children.
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
  else if (gtk_tree_view_row_expanded (tree_view, tree_path) && 
    gtk_tree_model_iter_has_child (tree_model, &iter)) 
    {
      (*count)++;
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
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *root_tree;
    gint row = 0;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
    root_tree = gtk_tree_path_new_root ();

    iterate_thru_children (GTK_TREE_VIEW (view), model, root_tree, path, &row, 0);
    
    gtk_tree_path_free (root_tree);

    curlog->current_line_no = row;
    create_zoom_view ();

}


/* ----------------------------------------------------------------------
   NAME:        handle_selection_changed_cb
   DESCRIPTION: User clicked on main window
   ---------------------------------------------------------------------- */

void
handle_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path, *root_tree;
    gint row = 0;

    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

        path = gtk_tree_model_get_path (model, &iter);
        root_tree = gtk_tree_path_new_root ();

        curlog->current_path = gtk_tree_path_copy (path);
        iterate_thru_children (GTK_TREE_VIEW (view), model, root_tree, path, &row, 0);

        gtk_tree_path_free (root_tree);
        gtk_tree_path_free (path);

        curlog->current_line_no = row;
        log_redrawdetail ();
    }

}

/* ----------------------------------------------------------------------
   NAME:        handle_log_mouse_button
   DESCRIPTION: User clicked on main window: open zoom view. 
   ---------------------------------------------------------------------- */

gboolean
handle_log_mouse_button (GtkWidget * win, GdkEventButton *event)
{
    if (event->type == GDK_2BUTTON_PRESS) {
        if (!zoom_visible)
        create_zoom_view ();
    }

    return FALSE;

}

/* ----------------------------------------------------------------------
   NAME:        change_log
   DESCRIPTION: User switchs to another Log file
   ---------------------------------------------------------------------- */

void
change_log (int direction)
{
   if (numlogs == 1)
        return;
   if (direction > 0) {
       if (curlognum == numlogs - 1)
	       curlognum = 0;
       else
	       curlognum++;
   }
   else {
       if (curlognum == 0)
	       curlognum = numlogs - 1;
       else
	       curlognum--;
   }

   save_rows_to_expand (curlog);

   curlog = loglist[curlognum];

   log_repaint ();
   if (loginfovisible)
       RepaintLogInfo ();
   if (calendarvisible)
       init_calendar_data ();
   UpdateStatusArea();

}

void 
save_rows_to_expand (Log *log)
{
   GtkTreeModel *tree_model;
   GtkTreePath *tree_path;
   GtkTreeIter iter;
   gint i = 0;

   if (!log->total_lines)
       return;

   tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (view)); 
   tree_path = gtk_tree_path_new_root ();
   gtk_tree_model_get_iter (tree_model, &iter, tree_path);

   do {
       tree_path = gtk_tree_model_get_path (tree_model, &iter);
       if (gtk_tree_model_iter_has_child (tree_model, &iter) &&
           gtk_tree_view_row_expanded (GTK_TREE_VIEW (view), tree_path)) {
           log->expand_paths[i] = gtk_tree_path_copy (tree_path);
           ++i; 
       }
   }
   while (gtk_tree_model_iter_next (tree_model, &iter));

   log->expand_paths[i] = NULL;

}

/* ----------------------------------------------------------------------
   NAME:        log_repaint
   DESCRIPTION: Redraw screen.
   ---------------------------------------------------------------------- */

gboolean
log_repaint ()
{
   GtkTreeModel *tree_model;

   /* Clear the tree view first */
   tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
   gtk_tree_store_clear (GTK_TREE_STORE (tree_model));

   log_redrawdetail (); 
   UpdateStatusArea ();	   

   /* Check that there is at least one log */
   if (curlog == NULL)
      return FALSE;
   
   /* Draw the tree view */ 
   DrawLogLines (curlog); 

   return TRUE;

}

void
UpdateStatusArea ()
{
   struct tm *tdm;
   char status_text[255];
   char *utf8;
   char *buffer;
   char *window_title;
   /* Translators: Date only format, %x should well do really */
   const char *time_fmt = _("%x"); /* an evil way to avoid warning */
 
   if (curlog == NULL) { 
       gtk_label_set_text (date_label, "");    
       gtk_window_set_title (GTK_WINDOW (app), APP_NAME);
       return;
   }

   if (curlog->name != NULL) {
       window_title = g_strdup_printf ("%s - %s", curlog->name, APP_NAME);
       gtk_window_set_title (GTK_WINDOW (app), window_title);
       g_free (window_title);
   }

   if (curlog->curmark != NULL) {
       tdm = &curlog->curmark->fulldate;

       if (strftime (status_text, sizeof (status_text), time_fmt, tdm) <= 0) {
   	       /* as a backup print in US style */
  	       utf8 = g_strdup_printf ("%02d/%02d/%02d", tdm->tm_mday,
                    tdm->tm_mon, tdm->tm_year % 100);
       } else {
           utf8 = LocaleToUTF8 (status_text);
       }
       gtk_label_get (date_label, (char **)&buffer);
       /* FIXME: is this if statement needed?  would it make sense 
       to set the text every time?  doesn't gtk test for it in 
       a more efficient manner? */
       if (strcmp (utf8, buffer) != 0)
	       gtk_label_set_text (date_label, utf8);

       g_free (utf8);
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
DrawLogLines (Log *current_log)
{
   GtkTreeModel *tree_model;
   GtkTreeIter iter;
   GtkTreeIter child_iter;
   GtkTreePath *path;
   LogLine *line;
   char tmp[4096];
   char *utf8;
   gint i = 0, count = 0;
   gint cm, cd;

   
   if (!current_log->total_lines) 
       return;

   tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
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
       gtk_tree_view_expand_row (GTK_TREE_VIEW (view), path, FALSE);
       current_log->first_time = FALSE;

       /* Scroll and set focus on last row */
       path = gtk_tree_model_get_path (tree_model, &child_iter);
       gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE); 
       gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL, TRUE, 
          1, 1);
       gtk_tree_path_free (path);

   } else {
       /* Expand the rows */
       for (i = 0; current_log->expand_paths[i]; ++i) 
           gtk_tree_view_expand_row (GTK_TREE_VIEW (view),
               current_log->expand_paths[i], FALSE);
			   
       /* Scroll and set focus on the previously focused row */
       gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), 
           current_log->current_path, NULL, FALSE); 
       gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), 
           current_log->current_path, NULL, TRUE, 1, 1);
   }

   g_free (utf8);
   
}

/* ----------------------------------------------------------------------
   NAME:        log_redrawdetail
   DESCRIPTION: Redraw area were the detailed information is drawn.
   ---------------------------------------------------------------------- */

void
log_redrawdetail ()
{
  if (zoom_visible)
    repaint_zoom ();

}

/* ----------------------------------------------------------------------
   NAME:        InitPages
   DESCRIPTION: Returns -1 if there was a error otherwise TRUE;
   ---------------------------------------------------------------------- */

int
InitPages ()
{
   if (user_prefs && user_prefs->logfile == NULL)
	return -1;

   curlog = OpenLogFile (user_prefs->logfile);
   if (curlog == NULL)
      return -1;
   curlog->first_time = TRUE;
   loglist[0] = curlog;
   curlognum = 0;
   numlogs++;
   return TRUE;
}
