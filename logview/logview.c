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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "logview.h"
#include "loglist.h"
#include "log_repaint.h"
#include "logrtns.h"
#include "monitor.h"
#include "about.h"
#include "misc.h"
#include "logview-findbar.h"
#include "userprefs.h"
#include "calendar.h"

#define APP_NAME _("System Log Viewer")

enum {
  PROP_0,
  PROP_DAYS,
};

enum {
  LOG_LINE_TEXT = 0,
  LOG_LINE_POINTER,
  LOG_LINE_WEIGHT,
  LOG_LINE_WEIGHT_SET
};

static GObjectClass *parent_class;

/* Function Prototypes */

GType logview_window_get_type (void);

static void logview_save_prefs (LogviewWindow *logview);
static void logview_add_log (LogviewWindow *logview, Log *log);
static void logview_menu_item_set_state (LogviewWindow *logview, char *path, gboolean state);
static void logview_menu_item_toggle_set_active (LogviewWindow *logview, char *path, gboolean state);
static void logview_update_findbar_visibility (LogviewWindow *logview);

static void logview_open_log (GtkAction *action, LogviewWindow *logview);
static void logview_toggle_sidebar (GtkAction *action, LogviewWindow *logview);
static void logview_toggle_statusbar (GtkAction *action, LogviewWindow *logview);
static void logview_toggle_calendar (GtkAction *action, LogviewWindow *logview);
static void logview_collapse_rows (GtkAction *action, LogviewWindow *logview);
static void logview_toggle_monitor (GtkAction *action, LogviewWindow *logview);
static void logview_close_log (GtkAction *action, LogviewWindow *logview);
static void logview_bigger_text (GtkAction *action, LogviewWindow *logview);
static void logview_smaller_text (GtkAction *action, LogviewWindow *logview);
static void logview_normal_text (GtkAction *action, LogviewWindow *logview);
static void logview_calendar_set_state (LogviewWindow *logview);
static void logview_search (GtkAction *action, LogviewWindow *logview);
static void logview_help (GtkAction *action, GtkWidget *parent_window);
static void logview_copy (GtkAction *action, LogviewWindow *logview);
static void logview_select_all (GtkAction *action, LogviewWindow *logview);
static Log *logview_find_log_from_name (LogviewWindow *logview, gchar *name);

static void logview_window_get_property	(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);

/* Menus */

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, N_("_Log"), NULL, NULL, NULL },
	{ "EditMenu", NULL, N_("_Edit"), NULL, NULL, NULL },
	{ "ViewMenu", NULL, N_("_View"), NULL, NULL, NULL },
	{ "HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL },

	{ "OpenLog", GTK_STOCK_OPEN, N_("_Open..."), "<control>O", N_("Open a log from file"), 
	  G_CALLBACK (logview_open_log) },
	{ "CloseLog", GTK_STOCK_CLOSE, N_("_Close"), "<control>W", N_("Close this log"), 
	  G_CALLBACK (logview_close_log) },
	{ "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", N_("Quit the log viewer"), 
	  G_CALLBACK (gtk_main_quit) },

	{ "Copy", GTK_STOCK_COPY, N_("_Copy"), "<control>C", N_("Copy the selection"),
	  G_CALLBACK (logview_copy) },
	{ "SelectAll", NULL, N_("Select _All"), "<Control>A", N_("Select the entire log"),
	  G_CALLBACK (logview_select_all) },
	{ "Search", GTK_STOCK_FIND, N_("_Filter..."), "<control>F", N_("Filter log"),
	  G_CALLBACK (logview_search) },

	{"ViewZoomIn", GTK_STOCK_ZOOM_IN, NULL, "<control>plus", N_("Bigger text size"),
	 G_CALLBACK (logview_bigger_text)},
	{"ViewZoomOut", GTK_STOCK_ZOOM_OUT, NULL, "<control>minus", N_("Smaller text size"),
	 G_CALLBACK (logview_smaller_text)},
	{"ViewZoom100", GTK_STOCK_ZOOM_100, NULL, "<control>0", N_("Normal text size"),
	 G_CALLBACK (logview_normal_text)},

	{"CollapseAll", NULL, N_("Collapse _All"), NULL, N_("Collapse all the rows"),
	 G_CALLBACK (logview_collapse_rows) },

	{ "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", N_("Open the help contents for the log viewer"), 
	  G_CALLBACK (logview_help) },
	{ "AboutAction", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Show the about dialog for the log viewer"), 
	  G_CALLBACK (logview_about) },

};

static GtkToggleActionEntry toggle_entries[] = {
    { "ShowStatusBar", NULL, N_("_Statusbar"), NULL, N_("Show Status Bar"),
      G_CALLBACK (logview_toggle_statusbar), TRUE },
	{ "ShowSidebar", NULL, N_("Side _Pane"), "F9", N_("Show Side Pane"), 
		G_CALLBACK (logview_toggle_sidebar), TRUE },
	{ "MonitorLogs", NULL, N_("_Monitor"), "<control>M", N_("Monitor Current Log"),
	  G_CALLBACK (logview_toggle_monitor), TRUE },
	{"ShowCalendar", NULL,  N_("Ca_lendar"), "<control>L", N_("Show Calendar Log"), 
	 G_CALLBACK (logview_toggle_calendar), TRUE },
};

static const char *ui_description = 
	"<ui>"
	"	<menubar name='LogviewMenu'>"
	"		<menu action='FileMenu'>"
	"			<menuitem action='OpenLog'/>"
	"			<separator/>"
	"			<menuitem action='MonitorLogs'/>"
	"			<separator/>"
	"			<menuitem action='CloseLog'/>"
	"			<menuitem action='Quit'/>"
	"		</menu>"
	"		<menu action='EditMenu'>"
	"       <menuitem action='Copy'/>"
	"       <menuitem action='SelectAll'/>"
	"		</menu>"	
	"		<menu action='ViewMenu'>"
    "     <menuitem action='ShowStatusBar'/>"
    "     <menuitem action='ShowSidebar'/>"
	"	  <menuitem action='ShowCalendar'/>"
	"     <separator/>"
	"	  <menuitem action='Search'/>"
	"	  <menuitem action='CollapseAll'/>"
    "     <separator/>"
    "     <menuitem action='ViewZoomIn'/>"
    "     <menuitem action='ViewZoomOut'/>"
    "     <menuitem action='ViewZoom100'/>"
	"	  </menu>"
	"	  <menu action='HelpMenu'>"
	"		<menuitem action='HelpContents'/>"
	"		<menuitem action='AboutAction'/>"
	"	  </menu>"
	"	</menubar>"
	"</ui>";

/* public interface */

Log *
logview_get_active_log (LogviewWindow *logview)
{
    g_return_val_if_fail (LOGVIEW_IS_WINDOW (logview), NULL);
    return logview->curlog;
}

LogList *
logview_get_loglist (LogviewWindow *logview)
{
    g_return_val_if_fail (LOGVIEW_IS_WINDOW (logview), NULL);
    return LOG_LIST (logview->loglist);
}

int
logview_count_logs (LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));
    return g_slist_length (logview->logs);
}

static void
logview_store_visible_range (LogviewWindow *logview)
{
  GtkTreePath *first, *last;
  Log *log = logview->curlog;
  if (log == NULL)
    return;

  if (log->visible_first)
    gtk_tree_path_free (log->visible_first);

  if (gtk_tree_view_get_visible_range (GTK_TREE_VIEW (logview->view),
                                       &first, NULL))
    log->visible_first = first;
}

void
logview_select_log (LogviewWindow *logview, Log *log)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
  
    logview_store_visible_range (logview);

    logview->curlog = log;
    logview_menus_set_state (logview);
    logview_calendar_set_state (logview);
    logview_repaint (logview);
    logview_update_findbar_visibility (logview);
    
    logview_update_version_bar (logview);
    logview_save_prefs (logview); 
    gtk_widget_grab_focus (logview->view);
} 

void
logview_add_log_from_name (LogviewWindow *logview, gchar *file)
{
	Log *log;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (file);

    log = log_open (file, TRUE);
    if (log != NULL)
      logview_add_log (logview, log);
}

void
logview_add_logs_from_names (LogviewWindow *logview, GSList *lognames, gchar *selected)
{
    GSList *list;
    Log *log, *curlog = NULL;
	
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    if (lognames == NULL)
        return;

    for (list = lognames; list != NULL; list = g_slist_next (list)) {
      log = log_open (list->data, FALSE);
      if (log != NULL) {
	logview_add_log (logview, log);
	if (selected!=NULL && g_strncasecmp (log->name, selected, -1)==0)
	  curlog = log;
      }
    }
        
    if (curlog)
        loglist_select_log (LOG_LIST (logview->loglist), curlog);

    gtk_tree_view_expand_all (GTK_TREE_VIEW (logview->loglist));
}

void
logview_menus_set_state (LogviewWindow *logview)
{
    Log *log;
    gboolean calendar_active = FALSE, monitor_active = FALSE;
    GtkWidget *widget;

    g_assert (LOGVIEW_IS_WINDOW (logview));
    log = logview->curlog;

    if (log) {
        monitor_active = (log->display_name == NULL);
        calendar_active = (log->days != NULL);
        logview_menu_item_toggle_set_active (logview, "/LogviewMenu/FileMenu/MonitorLogs", logview->curlog->monitored);
    }
    
    logview_menu_item_set_state (logview, "/LogviewMenu/FileMenu/MonitorLogs", monitor_active);
    logview_menu_item_set_state (logview, "/LogviewMenu/ViewMenu/ShowCalendar", calendar_active);
    logview_menu_item_set_state (logview, "/LogviewMenu/FileMenu/CloseLog", (log != NULL));
    logview_menu_item_set_state (logview, "/LogviewMenu/ViewMenu/CollapseAll", calendar_active);
    logview_menu_item_set_state (logview, "/LogviewMenu/ViewMenu/Search", (log != NULL));
    logview_menu_item_set_state (logview, "/LogviewMenu/EditMenu/Copy", (log != NULL));
    logview_menu_item_set_state (logview, "/LogviewMenu/EditMenu/SelectAll", (log != NULL));
}

void
logview_set_window_title (LogviewWindow *logview)
{
    gchar *window_title;
    gchar *logname;
    Log *log;

    g_assert (LOGVIEW_IS_WINDOW (logview));

    log = logview->curlog;
    if (log == NULL)
      return;

    if (log->name != NULL) {

      if (log->display_name != NULL)
	logname = log->display_name;
      else
	logname = log->name;
      
      if (log->monitored) 
	window_title = g_strdup_printf (_("%s (monitored) - %s"), logname, APP_NAME);
      else
	window_title = g_strdup_printf ("%s - %s", logname, APP_NAME);

    }
    else
      window_title = g_strdup_printf (APP_NAME);

    gtk_window_set_title (GTK_WINDOW (logview), window_title);
    g_free (window_title);
}

void
logview_show_main_content (LogviewWindow *logview)
{
  g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
  gtk_widget_show (logview->calendar);
  gtk_widget_show (logview->loglist);
  gtk_widget_show (logview->sidebar);
  gtk_widget_show (logview->view);
  gtk_widget_show (logview->hpaned);
  gtk_widget_show (logview->statusbar);
}

/* private functions */

static void
logview_update_findbar_visibility (LogviewWindow *logview)
{
	Log *log = logview->curlog;
    if (log == NULL) {
        gtk_widget_hide (logview->find_bar);
        return;
    }

	if (log->filter != NULL)
		gtk_widget_show (logview->find_bar);
	else
		gtk_widget_hide (logview->find_bar);
}

static void
logview_save_prefs (LogviewWindow *logview)
{
    GSList *list;
    Log *log;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    if (GTK_WIDGET_VISIBLE (GTK_WIDGET (logview->loglist))) {
        prefs_free_loglist();
        for (list = logview->logs; list != NULL; list = g_slist_next (list)) {
            log = list->data;
            prefs_store_log (log->name);
        }
        
        if (logview->curlog) {
	  if (logview->curlog->parent_log)
            prefs_store_active_log (logview->curlog->parent_log->name);
	  else
	    prefs_store_active_log (logview->curlog->name);
	}
        prefs_store_fontsize (logview->fontsize);
	prefs_save ();
    }
}

static void
logview_destroy (GObject *object, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

    if (logview->curlog) {
        if (logview->curlog->monitored)
            monitor_stop (logview->curlog);
    }

    logview_save_prefs (logview);
    prefs_free_loglist ();

    if (gtk_main_level() > 0)
      gtk_main_quit ();
    else
      exit (0);
}

static void
logview_add_log (LogviewWindow *logview, Log *log)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (log);

    logview->logs = g_slist_append (logview->logs, log);
    monitor_start (log);
    loglist_add_log (LOG_LIST(logview->loglist), log);
    log->window = logview;    
}


static Log *
logview_find_log_from_name (LogviewWindow *logview, gchar *name)
{
  GSList *list;
  Log *log;

  if (logview == NULL || name == NULL)
      return NULL;

  for (list = logview->logs; list != NULL; list = g_slist_next (list)) {
    log = list->data;
    if (g_ascii_strncasecmp (log->name, name, 255) == 0) {
      return log;
    }
  }
  return NULL;
}

static void
logview_version_selector_changed (GtkComboBox *version_selector, gpointer user_data)
{
	LogviewWindow *logview = user_data;
	Log *log = logview->curlog;
	int selected;

    g_assert (LOGVIEW_IS_WINDOW (logview));

	selected = gtk_combo_box_get_active (version_selector);

	if (selected == log->current_version)
		return;

	/* select a new version */
	if (selected == 0) {
		logview_select_log (logview, log->parent_log);
	} else {
		Log *new;
		if (log->parent_log) {
			new = log->parent_log->older_logs[selected];
		} else {
			new = log->older_logs[selected];
		}

		logview_select_log (logview, new);
	}
}

static void
logview_close_log (GtkAction *action, LogviewWindow *logview)
{
    Log *log;
    g_assert (LOGVIEW_IS_WINDOW (logview));

    if (logview->curlog == NULL)
        return;

    if (logview->curlog->monitored) {
        GtkAction *action = gtk_ui_manager_get_action (logview->ui_manager, "/LogviewMenu/FileMenu/MonitorLogs");
        gtk_action_activate (action);
    }
    
    gtk_widget_hide (logview->find_bar);

    log = logview->curlog;
    logview->curlog = NULL;

    logview->logs = g_slist_remove (logview->logs, log);
    log_close (log);
    loglist_remove_log (LOG_LIST (logview->loglist), log);
}

static void
logview_file_selected_cb (GtkWidget *chooser, gint response, LogviewWindow *logview)
{
   char *f;

   g_assert (LOGVIEW_IS_WINDOW (logview));

   gtk_widget_hide (GTK_WIDGET (chooser));
   if (response != GTK_RESPONSE_OK)
	   return;
   
   f = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

   if (f != NULL) {
       /* Check if the log is not already opened */
       GSList *list;
       Log *log, *tl;
	   for (list = logview->logs; list != NULL; list = g_slist_next (list)) {
		   log = list->data;
		   if (g_ascii_strncasecmp (log->name, f, 255) == 0) {
		     loglist_select_log (LOG_LIST (logview->loglist), log);
		     return;
		   }
	   }

	   if ((tl = log_open (f, TRUE)) != NULL)
	     logview_add_log (logview, tl);
   }
   
   g_free (f);

}

static void
logview_open_log (GtkAction *action, LogviewWindow *logview)
{
   static GtkWidget *chooser = NULL;

   g_assert (LOGVIEW_IS_WINDOW (logview));
   
   if (chooser == NULL) {
	   chooser = gtk_file_chooser_dialog_new (_("Open Log"),
						  GTK_WINDOW (logview),
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);
	   gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);
	   gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
	   g_signal_connect (G_OBJECT (chooser), "response",
			     G_CALLBACK (logview_file_selected_cb), logview);
	   g_signal_connect (G_OBJECT (chooser), "destroy",
			     G_CALLBACK (gtk_widget_destroyed), &chooser);
	   if (prefs_get_active_log () != NULL)
		   gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), 
						  prefs_get_active_log ());
   }

   gtk_window_present (GTK_WINDOW (chooser));
}

static void
logview_toggle_statusbar (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

    if (GTK_WIDGET_VISIBLE (logview->statusbar))
        gtk_widget_hide (logview->statusbar);
    else
        gtk_widget_show (logview->statusbar);
}

static void
logview_toggle_sidebar (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

	if (GTK_WIDGET_VISIBLE (logview->sidebar))
		gtk_widget_hide (logview->sidebar);
	else
		gtk_widget_show (logview->sidebar);
}

static void 
logview_toggle_calendar (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

	if (GTK_WIDGET_VISIBLE (logview->calendar))
		gtk_widget_hide (logview->calendar);
	else {
		gtk_widget_show (logview->calendar);
		if (!GTK_WIDGET_VISIBLE (logview->sidebar)) {
			GtkAction *action = gtk_ui_manager_get_action (logview->ui_manager, "/LogviewMenu/ViewMenu/ShowSidebar");
			gtk_action_activate (action);
		}
	}
}

static void 
logview_collapse_rows (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

    gtk_tree_view_collapse_all (GTK_TREE_VIEW (logview->view));
}

static void
logview_toggle_monitor (GtkAction *action, LogviewWindow *logview)
{
    Log *log;

    g_assert (LOGVIEW_IS_WINDOW (logview));

    log = logview->curlog;
    if ((log == NULL) || (log->display_name))
        return;

    if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) &&
        (log->monitored))
        return;

    if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action))==FALSE &&
        (!log->monitored))
        return;

    if (log->monitored)
        monitor_stop (log);
    else
        monitor_start (log);
    
    logview_set_window_title (logview);
    logview_menus_set_state (logview);
}

static void
logview_set_fontsize (LogviewWindow *logview)
{
	PangoFontDescription *fontdesc;
	PangoContext *context;
	
    g_assert (LOGVIEW_IS_WINDOW (logview));

	context = gtk_widget_get_pango_context (logview->view);
	fontdesc = pango_context_get_font_description (context);
	pango_font_description_set_size (fontdesc, (logview->fontsize)*PANGO_SCALE);
	gtk_widget_modify_font (logview->view, fontdesc);
	logview_save_prefs (logview);
}	

static void
logview_bigger_text (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

	logview->fontsize = MIN (logview->fontsize + 1, 24);
	logview_set_fontsize (logview);
}	

static void
logview_smaller_text (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

	logview->fontsize = MAX (logview->fontsize-1, 6);
	logview_set_fontsize (logview);
}	

static void
logview_normal_text (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

	logview->fontsize = logview->original_fontsize;
	logview_set_fontsize (logview);
}
	
static void
logview_search (GtkAction *action, LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));
    
    gtk_widget_show_all (logview->find_bar);
    logview_findbar_grab_focus (LOGVIEW_FINDBAR (logview->find_bar));
}

static void
logview_copy (GtkAction *action, LogviewWindow *logview)
{
	gchar *text, **lines;
	int nline, i, l1, l2;
    gchar *line;
    Log *log;

    g_assert (LOGVIEW_IS_WINDOW (logview));

    log = logview->curlog;
    if (log == NULL)
        return;

    l1 = log->selected_line_first;
    l2 = log->selected_line_last;
    if (l1 == -1 || l2 == -1)
        return;

    nline = l2 - l1 + 1;
    lines = g_new0(gchar *, (nline + 1));
    for (i=0; i<=nline; i++) {
        line = log->lines[l1+i];
        lines[i] = g_strdup (line);
    }
    lines[nline] = NULL;
    text = g_strjoinv ("\n", lines);
    gtk_clipboard_set_text (logview->clipboard, text, -1);
    g_free (text);
    g_strfreev (lines);
}

static void
logview_select_all (GtkAction *action, LogviewWindow *logview)
{
	GtkTreeSelection *selection;

    g_assert (LOGVIEW_IS_WINDOW (logview));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->view));
	gtk_tree_selection_select_all (selection);
}

static void
logview_menu_item_toggle_set_active (LogviewWindow *logview, char *path, gboolean state)
{
    GtkToggleAction *action;
    
    g_assert (path);
    
    action = GTK_TOGGLE_ACTION (gtk_ui_manager_get_action (logview->ui_manager, path));
    gtk_toggle_action_set_active (action, state);
}

static void
logview_menu_item_set_state (LogviewWindow *logview, char *path, gboolean state)
{
	GtkAction *action;

	g_assert (path);

	action = gtk_ui_manager_get_action (logview->ui_manager, path);
	gtk_action_set_sensitive (action, state);
}

static void
logview_calendar_set_state (LogviewWindow *logview)
{
    g_assert (LOGVIEW_IS_WINDOW (logview));

    if (logview->curlog) {
        if (logview->curlog->days != NULL)
            calendar_init_data (CALENDAR (logview->calendar), logview);
        gtk_widget_set_sensitive (logview->calendar, (logview->curlog->days != NULL));
    } else
        gtk_widget_set_sensitive (logview->calendar, FALSE);
}

static void
logview_help (GtkAction *action, GtkWidget *parent_window)
{
    GError *error = NULL;                                                                                
    gnome_help_display_desktop_on_screen (NULL, "gnome-system-log", "gnome-system-log", NULL,
                                          gtk_widget_get_screen (GTK_WIDGET(parent_window)), &error);
	if (error) {
		error_dialog_show (GTK_WIDGET(parent_window), _("There was an error displaying help."), error->message);
		g_error_free (error);
	}
}

static gboolean 
window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
				 gpointer data)
{
    LogviewWindow *logview = LOGVIEW_WINDOW (data);
    
    g_assert (LOGVIEW_IS_WINDOW (logview));
    prefs_store_window_size (GTK_WIDGET(logview));
    return FALSE;
}

static void
logview_window_finalize (GObject *object)
{
    LogviewWindow *logview = LOGVIEW_WINDOW(object);

    g_object_unref (logview->ui_manager);
    parent_class->finalize (object);
}

static void
logview_init (LogviewWindow *logview)
{
   GtkWidget *vbox;
   GtkTreeStore *tree_store;
   GtkTreeSelection *selection;
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;
   gint i;
   GtkActionGroup *action_group;
   GtkAccelGroup *accel_group;
   GError *error = NULL;
   GtkWidget *menubar;
   GtkWidget *hpaned;
   GtkWidget *label;
   GtkWidget *main_view;
   GtkWidget *loglist_scrolled, *scrolled;
   PangoContext *context;
   PangoFontDescription *fontdesc;
   gchar *monospace_font_name;

   gtk_window_set_default_size (GTK_WINDOW (logview), prefs_get_width (), prefs_get_height ());

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (logview), vbox);

   /* Create menus */
   action_group = gtk_action_group_new ("LogviewMenuActions");
   gtk_action_group_set_translation_domain (action_group, NULL);
   gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), logview);
   gtk_action_group_add_toggle_actions(action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), logview);

   logview->ui_manager = gtk_ui_manager_new ();

   gtk_ui_manager_insert_action_group (logview->ui_manager, action_group, 0);
   
   accel_group = gtk_ui_manager_get_accel_group (logview->ui_manager);
   gtk_window_add_accel_group (GTK_WINDOW (logview), accel_group);
   
   if (!gtk_ui_manager_add_ui_from_string (logview->ui_manager, ui_description, -1, &error)) {
     logview->ui_manager = NULL;
     return;
   }
   
   menubar = gtk_ui_manager_get_widget (logview->ui_manager, "/LogviewMenu");
   gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

   /* panes */
   hpaned = gtk_hpaned_new ();
   gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);

   /* First pane : sidebar (list of logs + calendar) */
   logview->sidebar = gtk_vbox_new (FALSE, 0);
   
   logview->calendar = calendar_new ();
   calendar_connect (CALENDAR (logview->calendar), logview);
   gtk_box_pack_end (GTK_BOX (logview->sidebar), GTK_WIDGET(logview->calendar), FALSE, FALSE, 0);
   
   /* log list */
   loglist_scrolled = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (loglist_scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (loglist_scrolled),
                                        GTK_SHADOW_ETCHED_IN);
   logview->loglist = loglist_new ();
   gtk_container_add (GTK_CONTAINER (loglist_scrolled), logview->loglist);
   gtk_box_pack_start (GTK_BOX (logview->sidebar), loglist_scrolled, TRUE, TRUE, 0);
   gtk_paned_pack1 (GTK_PANED (hpaned), logview->sidebar, FALSE, FALSE);
   loglist_connect (LOG_LIST(logview->loglist), logview);

   /* Second pane : log */
   main_view = gtk_vbox_new (FALSE, 0);
   gtk_paned_pack2 (GTK_PANED (hpaned), GTK_WIDGET (main_view), TRUE, TRUE);

   /* Scrolled window for the main view */
   scrolled = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
               GTK_POLICY_AUTOMATIC,
               GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start (GTK_BOX(main_view), scrolled, TRUE, TRUE, 0);

   /* Main Tree View */
   logview->view = gtk_tree_view_new ();
   gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (logview->view), TRUE);
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (logview->view), FALSE);

   /* Use the desktop monospace font */
   monospace_font_name = prefs_get_monospace ();
   widget_set_font (GTK_WIDGET (logview->view), monospace_font_name);
   g_free (monospace_font_name);
  
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new ();
   gtk_tree_view_column_pack_start (column, renderer, TRUE);
   gtk_tree_view_column_set_attributes (column, renderer, 
                                        "text", LOG_LINE_TEXT, 
                                        "weight", LOG_LINE_WEIGHT,
                                        "weight-set", LOG_LINE_WEIGHT_SET,
                                        NULL);
   gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
   gtk_tree_view_append_column (GTK_TREE_VIEW (logview->view), column);

   /* Version selector */
   logview->version_bar = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (logview->version_bar), 3);
   logview->version_selector = gtk_combo_box_new_text ();
   g_signal_connect (G_OBJECT (logview->version_selector), "changed",
                     G_CALLBACK (logview_version_selector_changed), logview);
   label = gtk_label_new (_("Version: "));
   
   gtk_box_pack_end (GTK_BOX(logview->version_bar), logview->version_selector, FALSE, FALSE, 0);
   gtk_box_pack_end (GTK_BOX(logview->version_bar), label, FALSE, FALSE, 0);
   gtk_box_pack_end (GTK_BOX(main_view), logview->version_bar, FALSE, FALSE, 0);
   
   logview->find_bar = logview_findbar_new ();
   gtk_box_pack_end (GTK_BOX (main_view), logview->find_bar, FALSE, FALSE, 0);
   logview_findbar_connect (LOGVIEW_FINDBAR (logview->find_bar), logview);

   /* Remember the original font size */
   context = gtk_widget_get_pango_context (logview->view);
   fontdesc = pango_context_get_font_description (context);
   logview->original_fontsize = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
   logview->fontsize = logview->original_fontsize;

   gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (logview->view));
   gtk_widget_show_all (scrolled);
   
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->view));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

   /* Add signal handlers */
   g_signal_connect (G_OBJECT (selection), "changed",
                     G_CALLBACK (selection_changed_cb), logview);
   g_signal_connect (G_OBJECT (logview->view), "row-expanded",
                     G_CALLBACK (row_toggled_cb), logview);
   g_signal_connect (G_OBJECT (logview->view), "row-collapsed",
                     G_CALLBACK (row_toggled_cb), logview);
   g_signal_connect (G_OBJECT (logview), "configure_event",
                     G_CALLBACK (window_size_changed_cb), logview);

   /* Status area at bottom */
   logview->statusbar = gtk_statusbar_new ();
   gtk_box_pack_start (GTK_BOX (vbox), logview->statusbar, FALSE, FALSE, 0);

   gtk_widget_show (menubar);
   gtk_widget_show (loglist_scrolled);
   gtk_widget_show (main_view);
   gtk_widget_show (vbox);
   logview->hpaned = hpaned;
}

static void
logview_window_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
  LogviewWindow *logview = LOGVIEW_WINDOW (object);
  
  switch (param_id) {
  case PROP_DAYS:
    g_value_set_pointer (value, logview->curlog->days);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  }
}

static void
logview_window_class_init (LogviewWindowClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = logview_window_finalize;
	object_class->get_property = logview_window_get_property;
	parent_class = g_type_class_peek_parent (klass);

	g_object_class_install_property (object_class, PROP_DAYS,
					 g_param_spec_pointer ("days",
					 _("Days"),
					 _("Pointer towards a GSList of days for the current log."),
					 (G_PARAM_READABLE)));
}

GType
logview_window_get_type (void)
{
	static GType object_type = 0;
	
	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (LogviewWindowClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) logview_window_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (LogviewWindow),
			0,              /* n_preallocs */
			(GInstanceInitFunc) logview_init
		};

		object_type = g_type_register_static (GTK_TYPE_WINDOW, "LogviewWindow", &object_info, 0);
	}

	return object_type;
}

GtkWidget *
logview_window_new ()
{
   LogviewWindow *logview;
   GtkWidget *window;

   window = g_object_new (LOGVIEW_TYPE_WINDOW, NULL);
   logview = LOGVIEW_WINDOW (window);
   if (logview->ui_manager == NULL)
     return NULL;

   logview->logs = NULL;
   logview->clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (window),
							     GDK_SELECTION_CLIPBOARD);
   gtk_ui_manager_set_add_tearoffs (logview->ui_manager, 
                                    prefs_get_have_tearoff ());

   g_signal_connect (GTK_OBJECT (window), "destroy",
                     G_CALLBACK (logview_destroy), logview);

   return window;
}
