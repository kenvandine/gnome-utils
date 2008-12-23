/* logview-window.c - main window of logview
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
 * Copyright (C) 2008  Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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
#include "logview-prefs.h"
#include "logview-manager.h"
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

struct _LogviewWindowPrivate {
  GtkWidget *view;		
  GtkWidget *statusbar;
  GtkUIManager *ui_manager;

  GtkWidget *calendar;
  GtkWidget *find_bar;
  GtkWidget *loglist;
  GtkWidget *sidebar; 
  GtkWidget *version_bar;
  GtkWidget *version_selector;
  GtkWidget *hpaned;

  int original_fontsize, fontsize;

  LogviewPrefs *prefs;
  LogviewManager *manager;
};

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_WINDOW, LogviewWindowPrivate))

G_DEFINE_TYPE (LogviewWindow, logview_window, GTK_TYPE_WINDOW);

/* Function Prototypes */

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
	{ "FileMenu", NULL, N_("_File"), NULL, NULL, NULL },
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
	"			<menuitem action='CloseLog'/>"
	"			<menuitem action='Quit'/>"
	"		</menu>"
	"		<menu action='EditMenu'>"
	"			<menuitem action='Copy'/>"
	"	    	<menuitem action='SelectAll'/>"
	"		</menu>"
	"		<menu action='ViewMenu'>"
	"			<menuitem action='MonitorLogs'/>"
	"			<separator/>"
	"			<menuitem action='ShowStatusBar'/>"
	"			<menuitem action='ShowSidebar'/>"
	"			<menuitem action='ShowCalendar'/>"
	"			<separator/>"
	"			<menuitem action='Search'/>"
	"			<menuitem action='CollapseAll'/>"
	"			<separator/>"
	"			<menuitem action='ViewZoomIn'/>"
	"			<menuitem action='ViewZoomOut'/>"
	"			<menuitem action='ViewZoom100'/>"
	"		</menu>"
	"		<menu action='HelpMenu'>"
	"			<menuitem action='HelpContents'/>"
	"			<menuitem action='AboutAction'/>"
	"		</menu>"
	"	</menubar>"
	"</ui>";

/* public interface */

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
        logview_menu_item_toggle_set_active (logview, "/LogviewMenu/ViewMenu/MonitorLogs", logview->curlog->monitored);
    }
    
    logview_menu_item_set_state (logview, "/LogviewMenu/ViewMenu/MonitorLogs", monitor_active);
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
        GtkAction *action = gtk_ui_manager_get_action (logview->ui_manager, "/LogviewMenu/ViewMenu/MonitorLogs");
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

#define DEFAULT_LOGVIEW_FONT "Monospace 10"

void
logview_set_font (LogviewWindow *logview,
                  const gchar   *fontname)
{
	PangoFontDescription *font_desc;

	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

	if (fontname == NULL)
		fontname = DEFAULT_LOGVIEW_FONT;

	font_desc = pango_font_description_from_string (fontname);
	if (font_desc) {
		gtk_widget_modify_font (logview->view, font_desc);
		pango_font_description_free (font_desc);
	}
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
    GtkClipboard *clipboard;

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

    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (logview->view),
                                          GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clipboard, text, -1);

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

/* actions callbacks */

static void
open_file_selected_cb (GtkWidget *chooser, gint response, LogviewWindow *logview)
{
  char *f;
  LogviewLog *log;

  g_assert (LOGVIEW_IS_WINDOW (logview));

  gtk_widget_hide (GTK_WIDGET (chooser));
  if (response != GTK_RESPONSE_OK) {
	  return;
  }

  f = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

  log = logview_manager_get_if_loaded (logview->priv->manager, f);

  if (log) {
    logview_manager_set_active_log (log);
    g_object_unref (log);
    goto out;
  }

  logview_manager_add_log_from_name (logview->priv->manager, f);

out:
  g_free (f);
}

static void
logview_open_log (GtkAction *action, LogviewWindow *logview)
{
  static GtkWidget *chooser = NULL;
  char *active;

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
                      G_CALLBACK (open_file_selected_cb), logview);
    g_signal_connect (G_OBJECT (chooser), "destroy",
                      G_CALLBACK (gtk_widget_destroyed), &chooser);
    active = logview_prefs_get_active_logfile (logview->priv->prefs);
    if (active != NULL) {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), active);
      g_free (active);
    }
  }

  gtk_window_present (GTK_WINDOW (chooser));
}

static void
logview_help (GtkAction *action, GtkWidget *parent_window)
{
  GError *error = NULL;

  gtk_show_uri (gtk_widget_get_screen (parent_window),
                "ghelp:gnome-system-log", gtk_get_current_event_time (),
                &error);

  if (error) {
    error_dialog_show (parent_window, _("There was an error displaying help."), error->message);
    g_error_free (error);
  }
}

static gboolean 
window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
                        gpointer data)
{
  logview_prefs_store_window_size (event->width, event->height);

  return FALSE;
}

static void
logview_window_finalize (GObject *object)
{
  LogviewWindow *logview = LOGVIEW_WINDOW (object);

  g_object_unref (logview->priv->ui_manager);
  G_OBJECT_CLASS (logview_window_parent_class)->finalize (object);
}

static void
logview_window_init (LogviewWindow *logview)
{
  GtkTreeStore *tree_store;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  gint i;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GError *error = NULL;
  GtkWidget *hpaned, *main_view, *loglist_scrolled, *scrolled, *vbox;
  PangoContext *context;
  PangoFontDescription *fontdesc;
  gchar *monospace_font_name;
  LogviewWindowPrivate *priv = logview->priv;
  int width, height;

  priv = GET_PRIVATE (logview);
  priv->prefs = logview_prefs_get ();
  priv->manager = logview_manager_get ();

  logview_prefs_get_stored_window_size (prefs, &width, &height);
  gtk_window_set_default_size (GTK_WINDOW (logview), width, height);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (logview), vbox);

  /* Create menus */
  action_group = gtk_action_group_new ("LogviewMenuActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), logview);
  gtk_action_group_add_toggle_actions(action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), logview);

  priv->ui_manager = gtk_ui_manager_new ();

  gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group (priv->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (logview), accel_group);

  if (!gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_description, -1, &error)) {
    priv->ui_manager = NULL;
    g_critical ("Can't load the UI description: %s", error->message);
    g_error_free (error);
    return;
  }

  gtk_ui_manager_set_add_tearoffs (priv->ui_manager,
                                   logview_prefs_get_have_tearoff (priv->prefs));

  w = gtk_ui_manager_get_widget (priv->ui_manager, "/LogviewMenu");
  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
  gtk_widget_show (w);

  /* panes */
  hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);

  /* First pane : sidebar (list of logs + calendar) */
  priv->sidebar = gtk_vbox_new (FALSE, 0);

  priv->calendar = calendar_new ();
  calendar_connect (CALENDAR (priv->calendar), logview);
  gtk_box_pack_end (GTK_BOX (priv->sidebar), priv->calendar, FALSE, FALSE, 0);

  /* log list */
  loglist_scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (loglist_scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (loglist_scrolled),
                                       GTK_SHADOW_ETCHED_IN);
  priv->loglist = loglist_new ();
  gtk_container_add (GTK_CONTAINER (loglist_scrolled), priv->loglist);
  gtk_box_pack_start (GTK_BOX (priv->sidebar), loglist_scrolled, TRUE, TRUE, 0);
  gtk_paned_pack1 (GTK_PANED (hpaned), priv->sidebar, FALSE, FALSE);
  loglist_connect (LOG_LIST (priv->loglist), logview);

  /* Second pane : log */
  main_view = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack2 (GTK_PANED (hpaned), main_view, TRUE, TRUE);

  /* Scrolled window for the main view */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (main_view), scrolled, TRUE, TRUE, 0);

  /* Main Tree View */
  priv->view = gtk_tree_view_new ();
  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (priv->view), FALSE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->view), FALSE);

  /* Use the desktop monospace font */
  monospace_font_name = prefs_get_monospace ();
  logview_set_font (logview, monospace_font_name);
  g_free (monospace_font_name);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer, 
                                       "text", LOG_LINE_TEXT, 
                                       "weight", LOG_LINE_WEIGHT,
                                       "weight-set", LOG_LINE_WEIGHT_SET,
                                       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

  /* Version selector */
  priv->version_bar = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (priv->version_bar), 3);
  priv->version_selector = gtk_combo_box_new_text ();
  g_signal_connect (priv->version_selector, "changed",
                    G_CALLBACK (logview_version_selector_changed), logview);
  w = gtk_label_new (_("Version: "));

  gtk_box_pack_end (GTK_BOX (priv->version_bar), priv->version_selector, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (priv->version_bar), w, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_view), priv->version_bar, FALSE, FALSE, 0);

  priv->find_bar = logview_findbar_new ();
  gtk_box_pack_end (GTK_BOX (main_view), priv->find_bar, FALSE, FALSE, 0);
  logview_findbar_connect (LOGVIEW_FINDBAR (priv->find_bar), logview);

  /* Remember the original font size */
  context = gtk_widget_get_pango_context (priv->view);
  fontdesc = pango_context_get_font_description (context);
  priv->original_fontsize = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
  priv->fontsize = priv->original_fontsize;

  gtk_container_add (GTK_CONTAINER (scrolled), priv->view);
  gtk_widget_show_all (scrolled);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  /* Add signal handlers */
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed_cb), logview);
  g_signal_connect (priv->view, "row-expanded",
                    G_CALLBACK (row_toggled_cb), logview);
  g_signal_connect (priv->view, "row-collapsed",
                    G_CALLBACK (row_toggled_cb), logview);
  g_signal_connect (logview, "configure_event",
                    G_CALLBACK (window_size_changed_cb), logview);
  g_signal_connect (priv->prefs, "system-font-changed",
                    G_CALLBACK (font_changed_cb), logview);
  g_signal_connect (priv->prefs, "have-tearoff-changed",
                    G_CALLBACK (tearoff_changed_cb), logview);
  g_signal_connect (priv->manager, "log-added",
                    G_CALLBACK (log_added_cb), logview);
  g_signal_connect (priv->manager, "active-changed",
                    G_CALLBACK (active_log_changed_cb), logview);

  /* Status area at bottom */
  priv->statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), priv->statusbar, FALSE, FALSE, 0);

  gtk_widget_show (loglist_scrolled);
  gtk_widget_show (main_view);
  gtk_widget_show (vbox);

  priv->hpaned = hpaned;
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

	g_object_class_install_property (object_class, PROP_DAYS,
					 g_param_spec_pointer ("days",
					 _("Days"),
					 _("Pointer towards a GSList of days for the current log."),
					 (G_PARAM_READABLE)));

  g_type_class_add_private (klass, sizeof (LogviewWindowPrivate));
}

/* public methods */

GtkWidget *
logview_window_new ()
{
  LogviewWindow *logview;

  logview = g_object_new (LOGVIEW_TYPE_WINDOW, NULL);

  if (priv->ui_manager == NULL) {
    return NULL;
  }

  return window;
}