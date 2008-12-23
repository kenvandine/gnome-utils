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
#include "logview-loglist.h"
#include "misc.h"

/*
#include "logview-findbar.h"
*/
#include "logview-about.h"
#include "logview-prefs.h"
#include "logview-manager.h"

#define APP_NAME _("System Log Viewer")

enum {
  LOG_LINE_TEXT = 0,
  LOG_LINE_POINTER,
  LOG_LINE_WEIGHT,
  LOG_LINE_WEIGHT_SET
};

struct _LogviewWindowPrivate {
  GtkWidget *statusbar;
  GtkUIManager *ui_manager;

  GtkWidget *calendar;
  GtkWidget *find_bar;
  GtkWidget *loglist;
  GtkWidget *sidebar; 
  GtkWidget *version_bar;
  GtkWidget *version_selector;
  GtkWidget *hpaned;
  GtkWidget *text_view;

  GtkTextTagTable *tag_table;

  int original_fontsize, fontsize;

  LogviewPrefs *prefs;
  LogviewManager *manager;

  gulong monitor_id;
};

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_WINDOW, LogviewWindowPrivate))

G_DEFINE_TYPE (LogviewWindow, logview_window, GTK_TYPE_WINDOW);

/* Function Prototypes */
static void logview_update_findbar_visibility (LogviewWindow *logview);
static void logview_calendar_set_state (LogviewWindow *logview);
static void logview_search (GtkAction *action, LogviewWindow *logview);

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
	"			<menuitem action='ShowStatusBar'/>"
	"			<menuitem action='ShowSidebar'/>"
	"			<menuitem action='ShowCalendar'/>"
	"			<separator/>"
	"			<menuitem action='Search'/>"
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

/* private functions */

#if 0

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

#endif

static void
logview_version_selector_changed (GtkComboBox *version_selector, gpointer user_data)
{

}
#if 0
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

#endif

/* private helpers */

static void
populate_tag_table (GtkTextTagTable *tag_table)
{
  GtkTextTag *tag;

  tag = gtk_text_tag_new ("bold");
  g_object_set (tag, "weight", PANGO_WEIGHT_BOLD,
                "weight-set", TRUE, NULL);
  gtk_text_tag_table_add (tag_table, tag);
}

static void
logview_update_statusbar (LogviewWindow *logview, LogviewLog *active)
{
  char *statusbar_text;
  char *size, *modified, *index;
  gulong timestamp;

  g_assert (LOGVIEW_IS_WINDOW (logview));

  if (active == NULL) {
    gtk_statusbar_pop (GTK_STATUSBAR (logview->priv->statusbar), 0);
    return;
  }

  /* ctime returned string has "\n\0" causes statusbar display a invalid char */
  timestamp = logview_log_get_timestamp (active);
  modified = ctime (&timestamp);
  index = strrchr (modified, '\n');
  if (index && *index != '\0')
    *index = '\0';

  modified = g_strdup_printf (_("last update: %s"), modified);

  size = g_format_size_for_display (logview_log_get_file_size (active));
  statusbar_text = g_strdup_printf (_("%d lines (%s) - %s"), 
                                    logview_log_get_cached_lines_number (active),
                                    size, modified);

  if (statusbar_text) {
    gtk_statusbar_pop (GTK_STATUSBAR (logview->priv->statusbar), 0);
    gtk_statusbar_push (GTK_STATUSBAR (logview->priv->statusbar), 0, statusbar_text);
    g_free (size);
    g_free (modified);
    g_free (statusbar_text);
  }
}

#define DEFAULT_LOGVIEW_FONT "Monospace 10"

static void
logview_set_font (LogviewWindow *logview,
                  const char    *fontname)
{
  PangoFontDescription *font_desc;

  g_assert (LOGVIEW_IS_WINDOW (logview));

  if (fontname == NULL)
    fontname = DEFAULT_LOGVIEW_FONT;

  font_desc = pango_font_description_from_string (fontname);
  if (font_desc) {
    gtk_widget_modify_font (logview->priv->text_view, font_desc);
    pango_font_description_free (font_desc);
  }
}

static void
logview_set_fontsize (LogviewWindow *logview)
{
  PangoFontDescription *fontdesc;
  PangoContext *context;
  LogviewWindowPrivate *priv = logview->priv;
	
  g_assert (LOGVIEW_IS_WINDOW (logview));

  context = gtk_widget_get_pango_context (priv->text_view);
  fontdesc = pango_context_get_font_description (context);
  pango_font_description_set_size (fontdesc, (priv->fontsize) * PANGO_SCALE);
  gtk_widget_modify_font (priv->text_view, fontdesc);

  logview_prefs_store_fontsize (logview->priv->prefs, priv->fontsize);
}

static void
logview_set_window_title (LogviewWindow *logview, const char * log_name)
{
  char *window_title;

  g_assert (LOGVIEW_IS_WINDOW (logview));

  if (log_name) {
    window_title = g_strdup_printf ("%s - %s", log_name, APP_NAME);
  } else {
    window_title = g_strdup_printf (APP_NAME);
  }

  gtk_window_set_title (GTK_WINDOW (logview), window_title);

  g_free (window_title);
}

static void
logview_menu_item_set_state (LogviewWindow *logview, char *path, gboolean state)
{
  GtkAction *action;

  g_assert (path);

  action = gtk_ui_manager_get_action (logview->priv->ui_manager, path);
  gtk_action_set_sensitive (action, state);
}

static void
logview_window_menus_set_state (LogviewWindow *logview)
{
  LogviewLog *log;
  gboolean calendar_active = FALSE;
  GtkWidget *widget;

  g_assert (LOGVIEW_IS_WINDOW (logview));

  log = logview_manager_get_active_log (logview->priv->manager);

  if (log) {
    calendar_active = (logview_log_get_days_for_cached_lines (log) != NULL);
  }

  logview_menu_item_set_state (logview, "/LogviewMenu/ViewMenu/ShowCalendar", calendar_active);
  logview_menu_item_set_state (logview, "/LogviewMenu/FileMenu/CloseLog", (log != NULL));
  logview_menu_item_set_state (logview, "/LogviewMenu/ViewMenu/Search", (log != NULL));
  logview_menu_item_set_state (logview, "/LogviewMenu/EditMenu/Copy", (log != NULL));
  logview_menu_item_set_state (logview, "/LogviewMenu/EditMenu/SelectAll", (log != NULL));

  g_object_unref (log);
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

  f = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser));

  log = logview_manager_get_if_loaded (logview->priv->manager, f);

  if (log) {
    logview_manager_set_active_log (logview->priv->manager, log);
    g_object_unref (log);
    goto out;
  }

  logview_manager_add_log_from_name (logview->priv->manager, f, TRUE);

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
logview_close_log (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  /*
  gtk_widget_hide (logview->priv->find_bar);
   */
  logview_manager_close_active_log (logview->priv->manager);
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

static void
logview_bigger_text (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  logview->priv->fontsize = MIN (logview->priv->fontsize + 1, 24);
  logview_set_fontsize (logview);
}	

static void
logview_smaller_text (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  logview->priv->fontsize = MAX (logview->priv->fontsize-1, 6);
  logview_set_fontsize (logview);
}	

static void
logview_normal_text (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  logview->priv->fontsize = logview->priv->original_fontsize;
  logview_set_fontsize (logview);
}

static void
logview_select_all (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  /* TODO: implement */
}

static void
logview_copy (GtkAction *action, LogviewWindow *logview)
{
  /* TODO: implement */
}

static void
logview_search (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));
/*
  gtk_widget_show_all (logview->priv->find_bar);
  logview_findbar_grab_focus (LOGVIEW_FINDBAR (logview->priv->find_bar));
*/
}

void
logview_about (GtkWidget *widget, GtkWidget *window)
{
  g_return_if_fail (GTK_IS_WINDOW (window));

  char *license_trans = g_strjoin ("\n\n", _(logview_about_license[0]),
                                   _(logview_about_license[1]),
                                   _(logview_about_license[2]), NULL);

  gtk_show_about_dialog (GTK_WINDOW (window),
                         "name",  _("System Log Viewer"),
                         "version", VERSION,
                         "copyright", "Copyright \xc2\xa9 1998-2008 Free Software Foundation, Inc.",
                         "license", license_trans,
                         "wrap-license", TRUE,
                         "comments", _("A system log viewer for GNOME."),
                         "authors", logview_about_authors,
                         "documenters", logview_about_documenters,
                         "translator_credits", strcmp (logview_about_translator_credits,
                                                       "translator-credits") != 0 ?
                                               logview_about_translator_credits : NULL,
                         "logo_icon_name", "logviewer",
                         NULL);
  g_free (license_trans);

  return;
}

static void
logview_toggle_statusbar (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  if (GTK_WIDGET_VISIBLE (logview->priv->statusbar))
    gtk_widget_hide (logview->priv->statusbar);
  else
    gtk_widget_show (logview->priv->statusbar);
}

static void
logview_toggle_sidebar (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  if (GTK_WIDGET_VISIBLE (logview->priv->sidebar))
    gtk_widget_hide (logview->priv->sidebar);
  else
    gtk_widget_show (logview->priv->sidebar);
}

static void 
logview_toggle_calendar (GtkAction *action, LogviewWindow *logview)
{
  g_assert (LOGVIEW_IS_WINDOW (logview));

  if (GTK_WIDGET_VISIBLE (logview->priv->calendar))
    gtk_widget_hide (logview->priv->calendar);
  else {
    gtk_widget_show (logview->priv->calendar);
    if (!GTK_WIDGET_VISIBLE (logview->priv->sidebar)) {
      GtkAction *action = gtk_ui_manager_get_action (logview->priv->ui_manager,
                                                     "/LogviewMenu/ViewMenu/ShowSidebar");
      gtk_action_activate (action);
    }
  }
}

/* GObject functions */

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
    {"ShowCalendar", NULL,  N_("Ca_lendar"), "<control>L", N_("Show Calendar Log"), 
      G_CALLBACK (logview_toggle_calendar), TRUE },
};

static gboolean 
window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
                        gpointer data)
{
  LogviewWindow *window = data;

  logview_prefs_store_window_size (window->priv->prefs,
                                   event->width, event->height);

  return FALSE;
}

static void read_new_lines_cb (LogviewLog *log,
                               const char **lines,
                               GError **error,
                               gpointer user_data);

static void
log_monitor_changed_cb (LogviewLog *log,
                        gpointer user_data)
{
  /* reschedule a read */
  logview_log_read_new_lines (log, (LogviewNewLinesCallback) read_new_lines_cb,
                              user_data);
}

static void
read_new_lines_cb (LogviewLog *log,
                   const char **lines,
                   GError **error,
                   gpointer user_data)
{
  LogviewWindow *window = user_data;
  GtkTextBuffer *buffer;
  gboolean boldify = FALSE;
  int i;
  GtkTextIter iter, start;
  GtkTextMark *mark;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->priv->text_view));

  if (gtk_text_buffer_get_char_count (buffer) != 0) {
    boldify = TRUE;
  }

  gtk_text_buffer_get_end_iter (buffer, &iter);

  if (boldify) {
    mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
  }

  for (i = 0; lines[i]; i++) {
    gtk_text_buffer_insert (buffer, &iter, lines[i], strlen (lines[i]));
    gtk_text_iter_forward_to_end (&iter);
    gtk_text_buffer_insert (buffer, &iter, "\n", 1);
    gtk_text_iter_forward_char (&iter);
  }

  if (boldify) {
    gtk_text_buffer_get_iter_at_mark (buffer, &start, mark);
    gtk_text_buffer_apply_tag_by_name (buffer, "bold", &start, &iter);
    gtk_text_buffer_delete_mark (buffer, mark);
  }

  if (window->priv->monitor_id == 0) {
    window->priv->monitor_id = g_signal_connect (log, "log-changed",
                                                 G_CALLBACK (log_monitor_changed_cb), window);
  }
}

static void
active_log_changed_cb (LogviewManager *manager,
                       LogviewLog *log,
                       LogviewLog *old_log,
                       gpointer data)
{
  LogviewWindow *window = data;
  const char **lines;
  GtkTextBuffer *buffer;

  if (window->priv->monitor_id) {
    g_signal_handler_disconnect (old_log, window->priv->monitor_id);
    window->priv->monitor_id = 0;
  }

  lines = logview_log_get_cached_lines (log);
  buffer = gtk_text_buffer_new (window->priv->tag_table);

  if (lines != NULL) {
    int i;
    GtkTextIter iter;

    /* update the text view to show the current lines */
    gtk_text_buffer_get_end_iter (buffer, &iter);

    for (i = 0; lines[i]; i++) {
      gtk_text_buffer_insert (buffer, &iter, lines[i], strlen (lines[i]));
      gtk_text_iter_forward_to_end (&iter);
      gtk_text_buffer_insert (buffer, &iter, "\n", 1);
      gtk_text_iter_forward_char (&iter);
    }
  }

  /* we set the buffer to the view anyway;
   * if there are no lines it will be empty for the duration of the thread
   * and will help us to distinguish the two cases of the following if
   * cause in the callback.
   */
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (window->priv->text_view), buffer);
  g_object_unref (buffer);

  if (lines == NULL || logview_log_has_new_lines (log)) {
    /* read the new lines */
    logview_log_read_new_lines (log, (LogviewNewLinesCallback) read_new_lines_cb, window);
  } else {
    /* start now monitoring the log for changes */
    window->priv->monitor_id = g_signal_connect (log, "log-changed",
                                                 G_CALLBACK (log_monitor_changed_cb), window);
  }
}

static void
font_changed_cb (LogviewPrefs *prefs,
                 const char *font_name,
                 gpointer user_data)
{
  LogviewWindow *window = user_data;

  logview_set_font (window, font_name);
}

static void
tearoff_changed_cb (LogviewPrefs *prefs,
                    gboolean have_tearoffs,
                    gpointer user_data)
{
  LogviewWindow *window = user_data;

  gtk_ui_manager_set_add_tearoffs (window->priv->ui_manager, have_tearoffs);
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
  gint i;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GError *error = NULL;
  GtkWidget *hpaned, *main_view, *scrolled, *vbox, *w;
  PangoContext *context;
  PangoFontDescription *fontdesc;
  gchar *monospace_font_name;
  LogviewWindowPrivate *priv;
  int width, height;

  priv = logview->priv = GET_PRIVATE (logview);
  priv->prefs = logview_prefs_get ();
  priv->manager = logview_manager_get ();
  priv->monitor_id = 0;

  logview_prefs_get_stored_window_size (priv->prefs, &width, &height);
  gtk_window_set_default_size (GTK_WINDOW (logview), width, height);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (logview), vbox);

  /* create menus */
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
  priv->hpaned = hpaned;
  gtk_widget_show (hpaned);

  /* first pane : sidebar (list of logs + calendar) */
  priv->sidebar = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (priv->sidebar);

  /* first pane: log list */
  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w),
                                       GTK_SHADOW_ETCHED_IN);

  priv->loglist = logview_loglist_new ();
  gtk_container_add (GTK_CONTAINER (w), priv->loglist);
  gtk_box_pack_start (GTK_BOX (priv->sidebar), w, TRUE, TRUE, 0);
  gtk_paned_pack1 (GTK_PANED (hpaned), priv->sidebar, FALSE, FALSE);
  gtk_widget_show (w);
  gtk_widget_show (priv->loglist);

  /* second pane : log */
  main_view = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack2 (GTK_PANED (hpaned), main_view, TRUE, TRUE);

  /* second pane: text view */
  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (main_view), w, TRUE, TRUE, 0);
  gtk_widget_show (w);

  priv->tag_table = gtk_text_tag_table_new ();
  populate_tag_table (priv->tag_table);
  priv->text_view = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (w), priv->text_view);
  gtk_widget_show (priv->text_view);

  /* use the desktop monospace font */
  monospace_font_name = logview_prefs_get_monospace_font_name (priv->prefs);
  logview_set_font (logview, monospace_font_name);
  g_free (monospace_font_name);

  /* version selector */
  priv->version_bar = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (priv->version_bar), 3);
  priv->version_selector = gtk_combo_box_new_text ();
  g_signal_connect (priv->version_selector, "changed",
                    G_CALLBACK (logview_version_selector_changed), logview);
  w = gtk_label_new (_("Version: "));

  gtk_box_pack_end (GTK_BOX (priv->version_bar), priv->version_selector, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (priv->version_bar), w, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_view), priv->version_bar, FALSE, FALSE, 0);

/*
  priv->find_bar = logview_findbar_new ();
  gtk_box_pack_end (GTK_BOX (main_view), priv->find_bar, FALSE, FALSE, 0);
  logview_findbar_connect (LOGVIEW_FINDBAR (priv->find_bar), logview);
*/
  /* remember the original font size */
  context = gtk_widget_get_pango_context (priv->text_view);
  fontdesc = pango_context_get_font_description (context);
  priv->original_fontsize = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
  priv->fontsize = priv->original_fontsize;

  /* signal handlers
   * - first is used to remember/restore the window size on quit.
   */
  g_signal_connect (logview, "configure_event",
                    G_CALLBACK (window_size_changed_cb), logview);
  g_signal_connect (priv->prefs, "system-font-changed",
                    G_CALLBACK (font_changed_cb), logview);
  g_signal_connect (priv->prefs, "have-tearoff-changed",
                    G_CALLBACK (tearoff_changed_cb), logview);
  g_signal_connect (priv->manager, "active-changed",
                    G_CALLBACK (active_log_changed_cb), logview);

  /* status area at bottom */
  priv->statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), priv->statusbar, FALSE, FALSE, 0);
  gtk_widget_show (priv->statusbar);

  gtk_widget_show (vbox);
  gtk_widget_show (main_view);
}

static void
logview_window_class_init (LogviewWindowClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = logview_window_finalize;

  g_type_class_add_private (klass, sizeof (LogviewWindowPrivate));
}

/* public methods */

GtkWidget *
logview_window_new ()
{
  LogviewWindow *logview;

  logview = g_object_new (LOGVIEW_TYPE_WINDOW, NULL);

  if (logview->priv->ui_manager == NULL) {
    return NULL;
  }

  return GTK_WIDGET (logview);
}