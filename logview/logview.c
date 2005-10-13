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
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/goption.h>
#include "logview.h"
#include "log_repaint.h"
#include "logrtns.h"
#include "info.h"
#include "monitor.h"
#include "about.h"
#include "misc.h"
#include "logview-findbar.h"
#include "userprefs.h"
#include "configdata.h"

static GObjectClass *parent_class;
static GSList *logview_windows = NULL;
static GConfClient *client = NULL;
static UserPrefsStruct *user_prefs;
gboolean restoration_complete = FALSE;

#define APP_NAME _("System Log Viewer")
#define GCONF_MONOSPACE_FONT_NAME "/desktop/gnome/interface/monospace_font_name"

/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void destroy (GObject *object, gpointer data);
static void CreateMainWin (LogviewWindow *window_new);
static void LoadLogMenu (GtkAction *action, GtkWidget *parent_window);
static void CloseLogMenu (GtkAction *action, GtkWidget *callback_data);
static void FileSelectResponse (GtkWidget * chooser, gint response, gpointer data);
static void open_databases (void);
static GtkWidget *logview_create_window (void);
static void logview_menu_item_set_state (LogviewWindow *logviewwindow, char *path, gboolean state);
static void toggle_sidebar (GtkAction *action, GtkWidget *callback_data);
static void toggle_calendar (GtkAction *action, GtkWidget *callback_data);
static void logview_collapse_rows (GtkAction *action, GtkWidget *callback_data);
static void toggle_monitor (GtkAction *action, GtkWidget *callback_data);
static void logview_set_fontsize (LogviewWindow *logview);
static void logview_bigger_text (GtkAction *action, GtkWidget *callback_data);
static void logview_smaller_text (GtkAction *action, GtkWidget *callback_data);
static void logview_normal_text (GtkAction *action, GtkWidget *callback_data);
static void logview_calendar_set_state (LogviewWindow *LogviewWindow);
static void logview_menus_set_state (LogviewWindow *logviewwindow);
static void logview_search (GtkAction *action, GtkWidget *callback_data);
static void logview_help (GtkAction *action, GtkWidget *parent_window);
static void logview_copy (GtkAction *action, GtkWidget *callback_data);
static void logview_select_all (GtkAction *action, GtkWidget *callback_data);
static int logview_count_logs (void);
gboolean window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
				 gpointer data);
GType logview_window_get_type (void);
static void loglist_add_log (LogviewWindow *window, Log *log);
Log *logview_find_log_from_name (LogviewWindow *logview, gchar *name);
static void loglist_select_log_from_name (LogviewWindow *logview, gchar *logname);


/*
 *    ,-------.
 *    | Menus |
 *    `-------'
 */

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, N_("_Log"), NULL, NULL, NULL },
	{ "EditMenu", NULL, N_("_Edit"), NULL, NULL, NULL },
	{ "ViewMenu", NULL, N_("_View"), NULL, NULL, NULL },
	{ "HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL },

	{ "OpenLog", GTK_STOCK_OPEN, N_("_Open..."), "<control>O", N_("Open a log from file"), 
	  G_CALLBACK (LoadLogMenu) },
	{ "Properties", GTK_STOCK_PROPERTIES,  N_("_Properties"), "<control>P", N_("Show Log Properties"), 
	  G_CALLBACK (LogInfo) },
	{ "CloseLog", GTK_STOCK_CLOSE, N_("Close"), "<control>W", N_("Close this log"), 
	  G_CALLBACK (CloseLogMenu) },
	{ "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", N_("Quit the log viewer"), 
	  G_CALLBACK (gtk_main_quit) },

	{ "Copy", GTK_STOCK_COPY, N_("Copy"), "<control>C", N_("Copy the selection"),
	  G_CALLBACK (logview_copy) },
	{ "SelectAll", NULL, N_("Select All"), "<Control>A", N_("Select the entire log"),
	  G_CALLBACK (logview_select_all) },
	{ "Search", GTK_STOCK_FIND, N_("_Find..."), "<control>F", N_("Find pattern in logs"),
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
	  G_CALLBACK (AboutShowWindow) },

};

static GtkToggleActionEntry toggle_entries[] = {
	{ "ShowSidebar", NULL, N_("Sidebar"), NULL, N_("Show the sidebar"), 
		G_CALLBACK (toggle_sidebar), TRUE },
	{ "MonitorLogs", NULL, N_("_Monitor"), "<control>M", N_("Monitor Current Log"),
	  G_CALLBACK (toggle_monitor), TRUE },
	{"ShowCalendar", NULL,  N_("Ca_lendar"), "<control>L", N_("Show Calendar Log"), 
	 G_CALLBACK (toggle_calendar), TRUE },
};

static const char *ui_description = 
	"<ui>"
	"	<menubar name='LogviewMenu'>"
	"		<menu action='FileMenu'>"
	"			<menuitem action='OpenLog'/>"
	"			<separator/>"
	"			<menuitem action='MonitorLogs'/>"
	"			<separator/>"
	"			<menuitem action='Properties'/>"
	"			<separator/>"
	"			<menuitem action='CloseLog'/>"
	"			<menuitem action='Quit'/>"
	"		</menu>"
	"		<menu action='EditMenu'>"
	"                       <menuitem action='Copy'/>"
	"                       <menuitem action='SelectAll'/>"
	"                       <separator/>"
	"			<menuitem action='Search'/>"
	"		</menu>"	
	"		<menu action='ViewMenu'>"
  "     <menuitem action='ShowSidebar'/>"
	"			<menuitem action='ShowCalendar'/>"
	"                       <separator/>"
	"			<menuitem action='CollapseAll'/>"
  "                       <separator/>"
  "     <menuitem action='ViewZoomIn'/>"
  "     <menuitem action='ViewZoomOut'/>"
  "     <menuitem action='ViewZoom100'/>"
	"		</menu>"
	"		<menu action='HelpMenu'>"
	"			<menuitem action='HelpContents'/>"
	"			<menuitem action='AboutAction'/>"
	"		</menu>"
	"	</menubar>"
	"</ui>";

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */
	
GList *regexp_db = NULL, *descript_db = NULL, *actions_db = NULL;
ConfigData *cfg = NULL;
static gchar *config_prefix = NULL;
static gchar *sm_client_id = NULL;
static int screen = 0;

GOptionContext *context;
GOptionEntry options[] = {
    { "sm-config-prefix", 0, 0, G_OPTION_ARG_STRING, &config_prefix, "", NULL},
    { "sm-client-id", 0, 0, G_OPTION_ARG_STRING, &sm_client_id, "", NULL},
    { "screen", 0, 0, G_OPTION_ARG_INT, &screen, "", NULL},
	{ NULL }
};

void
logview_save_prefs (LogviewWindow *logview)
{
    GList *list;
    Log *log;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    prefs_free_loglist(user_prefs);
    for (list = logview->logs; list != NULL; list = g_list_next (list)) {
        log = list->data;
        user_prefs->logs = g_slist_append (user_prefs->logs, log->name);
	}
    if (logview->curlog)
        user_prefs->logfile = logview->curlog->name;
	user_prefs->fontsize = logview->fontsize;
	if (restoration_complete)
		prefs_save (client, user_prefs);
}

/* ----------------------------------------------------------------------
   NAME:          destroy
   DESCRIPTION:   Exit program.
   ---------------------------------------------------------------------- */

void
destroy (GObject *object, gpointer data)
{
   LogviewWindow *window = data;
   logview_windows = g_slist_remove (logview_windows, window);
   if (window->curlog && window->curlog->monitored)
	   monitor_stop (window->curlog);
   if (logview_windows == NULL) {
	   if (window->curlog && !(window->curlog->display_name))
		   user_prefs->logfile = window->curlog->name;
	   else
		   user_prefs->logfile = NULL;
	   prefs_save (client, user_prefs);
     prefs_free_loglist (user_prefs);
	   gtk_main_quit ();
   }
}

static gint
save_session (GnomeClient *gnome_client, gint phase,
              GnomeRestartStyle save_style, gint shutdown,
              GnomeInteractStyle interact_style, gint fast,
              gpointer client_data)
{
   gchar **argv;
   gint numlogs, i=1;
   GSList *list;
   GList *logs;
   Log *log;

   numlogs = logview_count_logs ();

   argv = g_malloc0 (sizeof (gchar *) * numlogs+1);
   argv[0] = g_get_prgname();

   for (list = logview_windows; list != NULL; list = g_slist_next (list)) {
	   LogviewWindow *w = list->data;
       logs = w->logs;       
       for (logs = w->logs; logs != NULL; logs = g_list_next (logs)) {
           log = logs->data;
		   argv[i++] = g_strdup_printf ("%s", log->name);
       }
   }
   
   gnome_client_set_clone_command (gnome_client, numlogs+1, argv);
   gnome_client_set_restart_command (gnome_client, numlogs+1, argv);

   g_free (argv);

   return TRUE;

}

static gint
die (GnomeClient *gnome_client, gpointer client_data)
{
    gtk_main_quit ();
    return 0;
}

void
logview_select_log (LogviewWindow *logview, Log *log)
{
	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
	g_return_if_fail (log);

	logview->curlog = log;
	logview_menus_set_state (logview);
	logview_update_version_bar (logview);
	logview_calendar_set_state (logview);
	log_repaint (logview);
	logview_save_prefs (logview); 
    gtk_widget_grab_focus (logview->view);
} 

void
logview_add_log (LogviewWindow *logview, Log *log)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (log);

    logview->logs = g_list_append (logview->logs, log);
    loglist_add_log (logview, log);
    log->first_time = TRUE;
    log->window = logview;
    
    monitor_start (log);
}

void
logview_add_log_from_name (LogviewWindow *logview, gchar *file)
{
	Log *log;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (file);

	log = OpenLogFile (file, TRUE);
	if (log != NULL)
		  logview_add_log (logview, log);
}

void
logview_add_logs_from_names (LogviewWindow *logview, GSList *lognames)
{
	GSList *list;
	Log *log;
    int n, i=0;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (lognames);

    n = g_slist_length (list);

    gtk_widget_show (logview->progressbar);

	for (list = lognames; list != NULL; list = g_slist_next (list)) {
        i++;
		if (logview_find_log_from_name (logview, list->data) == NULL) {
			log = OpenLogFile (list->data, FALSE);
			if (log != NULL) {
				logview_add_log (logview, log);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (logview->progressbar),
                                               (float) i / (float) n );
                while (gtk_events_pending ())
                    gtk_main_iteration ();
			}
		}
	}
    
    gtk_widget_hide (logview->progressbar);
}

void
logview_remove_log (LogviewWindow *logview, Log *log)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (log);

	CloseLog (log);
	logview->logs = g_list_remove (logview->logs, log);
    log->window = NULL;
    log = NULL;
}

GOptionContext *
logview_init_options ()
{
	GOptionContext *context;
	
	context = g_option_context_new (" - Browse and monitor logs");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_set_help_enabled (context, TRUE);
	g_option_context_set_ignore_unknown_options (context, TRUE);
	return context;
}
	
/* ----------------------------------------------------------------------
   NAME:          main
   DESCRIPTION:   Program entry point.
   ---------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
   GnomeClient *gnome_client;
   GError *error;
   GnomeProgram *program;
	 LogviewWindow *logview;
   int i;

   bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
   bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
   textdomain(GETTEXT_PACKAGE);

   QueueErrMessages (TRUE);

   /*  Initialize gnome & gtk */
   program = gnome_program_init ("gnome-system-log",VERSION, LIBGNOMEUI_MODULE, argc, argv,
				 GNOME_PARAM_APP_DATADIR, DATADIR, 
				 NULL);

   g_set_application_name (_("Log Viewer"));
   gconf_init (argc, argv, NULL);
   client = gconf_client_get_default ();
   gnome_vfs_init ();

   gtk_window_set_default_icon_name ("logviewer");
   cfg = CreateConfig();
   user_prefs = prefs_load (client);
   
   context = logview_init_options ();
   g_option_context_parse (context, &argc, &argv, &error);

   /* Open regular logs and add each log passed as a parameter */

   logview = LOGVIEW_WINDOW(logview_create_window ());
   logview_menus_set_state (logview);
   gtk_widget_set_sensitive (logview->view, FALSE);
   gtk_widget_set_sensitive (logview->loglist, FALSE);
   gtk_widget_show (GTK_WIDGET(logview));
   while (gtk_events_pending ())
       gtk_main_iteration ();
   if (argc == 1) {
       logview_add_logs_from_names (logview, user_prefs->logs);
       loglist_select_log_from_name (logview, user_prefs->logfile);
   } else {
	   for (i=1; i<argc; i++)
           logview_add_log_from_name (logview, argv[i]);
   }
   restoration_complete = TRUE;
   gtk_widget_set_sensitive (logview->view, TRUE);
   gtk_widget_set_sensitive (logview->loglist, TRUE);
   
   gnome_client = gnome_master_client ();

   QueueErrMessages (FALSE);
   ShowQueuedErrMessages ();
   
   g_signal_connect (gnome_client, "save_yourself",
					 G_CALLBACK (save_session), NULL);
   g_signal_connect (gnome_client, "die", G_CALLBACK (die), NULL);

   save_session (gnome_client,0,0,0,0,0,NULL);
   /*  Loop application */
   gtk_main ();

   return 0;
}


/* ----------------------------------------------------------------------
   NAME:        logview_create_window
   DESCRIPTION: Main initialization routine.
   ---------------------------------------------------------------------- */

static GtkWidget *
logview_create_window ()
{
   LogviewWindow *logview;
   GtkWidget *window;

   regexp_db = NULL;

   /*  Display main window */
   window = g_object_new (LOGVIEW_TYPE_WINDOW, NULL);
   logview = LOGVIEW_WINDOW (window);

   logview->sidebar_visible = TRUE;
   logview->calendar_visible = TRUE;
   logview->loginfovisible = FALSE;
   logview->logs = NULL;
   logview->clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (window),
							     GDK_SELECTION_CLIPBOARD);

   /* FIXME : we need to listen to this key, not just read it. */
   gtk_ui_manager_set_add_tearoffs (logview->ui_manager, 
																		gconf_client_get_bool 
																		(client, "/desktop/gnome/interface/menus_have_tearoff", NULL));

   g_signal_connect (GTK_OBJECT (window), "destroy",
										 G_CALLBACK (destroy), logview);

   logview_windows = g_slist_prepend (logview_windows, window);

   return window;
}

Log *
logview_find_log_from_name (LogviewWindow *logview, gchar *name)
{
  GList *list;
  Log *log;

  g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

  for (list = logview->logs; list != NULL; list = g_list_next (list)) {
    log = list->data;
    if (g_ascii_strncasecmp (log->name, name, 255) == 0) {
      return log;
    }
  }
  return NULL;
}

static gchar *
logname_remove_markup (gchar *logname)
{
    gchar *unmarkup;

    g_return_if_fail (logname);

    if (g_str_has_prefix (logname, "<b>")) {
        int n;
        n = strlen (logname);
        unmarkup = g_strndup (logname+3, n-7);
    } else
        unmarkup = g_strdup (logname);
    
    return unmarkup;
}

/* loglist functions
   FIXME : These should be factored out in their own module presumably */

/* loglist_unbold_log is called by a g_timeout */
static gboolean
loglist_unbold_log (gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path;
    Log *log = data;
    LogviewWindow *logview;

    if (log == NULL)
        return FALSE;

    logview = log->window;
    if (logview == NULL)
        return FALSE;

    /* If the log to unbold is not displayed, still wait */
    if (logview->curlog != log)
        return TRUE;

    path = loglist_find_logname (logview, log->name);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (logview->treeview));
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, log->name, -1);
    gtk_tree_path_free (path);
    return FALSE;
}    

static void
loglist_selection_changed (GtkTreeSelection *selection, LogviewWindow *logview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name, *unmarkup;
  Log *log;

  g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
  g_return_if_fail (GTK_IS_TREE_SELECTION (selection));

  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
    /* there is no selected log right now */
    logview->curlog = NULL;
    logview_menus_set_state (logview);
    logview_calendar_set_state (logview);
    log_repaint (logview);
    return;
  }
  
  gtk_tree_model_get (model, &iter, 0, &name, -1);
  unmarkup = logname_remove_markup (name);
  log = logview_find_log_from_name (logview, unmarkup);
  g_return_if_fail (log);

  while (gtk_events_pending ())
      gtk_main_iteration ();

  if ((log != logview->curlog) && 
      gtk_tree_selection_iter_is_selected (selection, &iter)) {
      logview_select_log (logview, log);
      gtk_widget_grab_focus (logview->view);

      /* add a timeout to unbold the log if bolded */
      if (g_str_has_prefix (name, "<b>"))
          g_timeout_add (5000, loglist_unbold_log, log);
  }
  
  g_free (name);
  g_free (unmarkup);
}

GtkTreePath *
loglist_find_logname (LogviewWindow *logview, gchar *logname)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	gchar *name, *unmarkup;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

	model =  gtk_tree_view_get_model (GTK_TREE_VIEW(logview->treeview));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	do {
		gtk_tree_model_get (model, &iter, 0, &name, -1);
        unmarkup = logname_remove_markup (name);
		if (g_ascii_strncasecmp (logname, unmarkup, 255) == 0) {
			g_free (name);
            g_free (unmarkup);
			path = gtk_tree_model_get_path (model, &iter);
			return path;
		}
		g_free (name);
        g_free (unmarkup);
	} while (gtk_tree_model_iter_next (model, &iter));

	return NULL;
}

void
loglist_bold_log (LogviewWindow *logview, Log *log)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path;
    gchar *markup;
    
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    path = loglist_find_logname (logview, log->name);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (logview->treeview));
    gtk_tree_model_get_iter (model, &iter, path);
    markup = g_markup_printf_escaped ("<b>%s</b>", log->name);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, markup, -1);
    
    g_free (markup);
    gtk_tree_path_free (path);

    /* if the log is currently shown, put up a timer to unbold it */
    if (logview->curlog == log)
        g_timeout_add (5000, loglist_unbold_log, log);
}

static void
loglist_select_log_from_name (LogviewWindow *logview, gchar *logname)
{
	GtkTreePath *path;
	path = loglist_find_logname (logview, logname);
	if (path) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->treeview));
		gtk_tree_selection_select_path (selection, path);
	}
}

static void 
loglist_add_log (LogviewWindow *logview, Log *log)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (logview->treeview));
    
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, log->name, -1);

	if (restoration_complete) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->treeview));
		gtk_tree_selection_select_iter (selection, &iter);
	}
}

static void
loglist_remove_selected_log (LogviewWindow *logview)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gchar *name, *unmarkup;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->treeview));
	gtk_tree_selection_get_selected (selection, &model, &iter);

	gtk_tree_model_get (model, &iter, 0, &name, -1);
    unmarkup = logname_remove_markup (name);
	
	if (unmarkup) {
		Log *log;
		
		log = logview_find_log_from_name (logview, unmarkup);
		logview_remove_log (logview, log);
        logview->curlog = NULL;

        if (gtk_list_store_remove (GTK_LIST_STORE (model), &iter))
            gtk_tree_selection_select_iter (selection, &iter);
	}

    g_free (unmarkup);
    g_free (name);
}

GtkWidget *
loglist_create (LogviewWindow *window)
{
  GtkWidget *label;
  GtkWidget *scrolled;
  GtkWidget *vbox;
  GtkWidget *treeview;
  GtkListStore *model;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkCellRenderer *cell;
    
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (vbox), 3);
  
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_ETCHED_IN);
  
  model = gtk_list_store_new (1, G_TYPE_STRING);
  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  g_object_unref (G_OBJECT (model));
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (loglist_selection_changed), window);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("words", cell, "markup", 0, NULL);
  
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), -1);
  
  gtk_container_add (GTK_CONTAINER (scrolled), treeview);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  window->treeview = treeview;
  gtk_widget_show (vbox);
  return vbox;
}

void
logview_version_selector_changed (GtkComboBox *version_selector, gpointer user_data)
{
	LogviewWindow *logview = user_data;
	Log *log = logview->curlog;
	int selected;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

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
logview_monospace_font_changed (GConfClient *client, guint id, 
                                GConfEntry *entry, gpointer data)
{
  LogviewWindow *logview = data;
  gchar *monospace_font_name;
  PangoFontDescription *fontdesc;

  monospace_font_name = gconf_client_get_string (client, GCONF_MONOSPACE_FONT_NAME, NULL);
  fontdesc = pango_font_description_from_string (monospace_font_name);
  if (pango_font_description_get_family (fontdesc) != NULL)
    gtk_widget_modify_font (GTK_WIDGET(logview->view), fontdesc);
  g_free (fontdesc);
  g_free (monospace_font_name);
}


/* ----------------------------------------------------------------------
   NAME:        CreateMainWin
   DESCRIPTION: Creates the main window.
   ---------------------------------------------------------------------- */

static void
CreateMainWin (LogviewWindow *window)
{
   GtkWidget *vbox, *hbox;
   GtkTreeStore *tree_store;
   GtkTreeSelection *selection;
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;
   gint i;
   GtkActionGroup *action_group;
   GtkAccelGroup *accel_group;
   GError *error = NULL;
   GtkWidget *menubar;
   GtkWidget *loglist;
   GtkWidget *hpaned;
   GtkWidget *label;
   GtkWidget *main_view;
   GtkWidget *scrolled;
   PangoFontDescription *fontdesc;
   PangoContext *context;
   gchar *monospace_font_name;
   const gchar *column_titles[] = { N_("Date"), N_("Host Name"),
                                    N_("Process"), N_("Message"), NULL };
   const gint column_widths[] = {150,100,200,400};

   gtk_window_set_default_size (GTK_WINDOW (window), user_prefs->width, user_prefs->height);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (window), vbox);

   /* Create menus */
   action_group = gtk_action_group_new ("LogviewMenuActions");
   gtk_action_group_set_translation_domain (action_group, NULL);
   gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), window);
   gtk_action_group_add_toggle_actions(action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), window);

   window->ui_manager = gtk_ui_manager_new ();

   gtk_ui_manager_insert_action_group (window->ui_manager, action_group, 0);
   
   accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
   gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
   
   if (!gtk_ui_manager_add_ui_from_string (window->ui_manager, ui_description, -1, &error)) {
	   g_message ("Building menus failed: %s", error->message);
	   g_error_free (error);
	   exit (1);
   }
   
   menubar = gtk_ui_manager_get_widget (window->ui_manager, "/LogviewMenu");
   gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

   /* panes */
   hpaned = gtk_hpaned_new ();
   gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);

   /* First pane : sidebar (list of logs + calendar) */
	 window->sidebar = gtk_vbox_new (FALSE, 0);

	 CalendarMenu (window);
	 gtk_box_pack_end (GTK_BOX (window->sidebar), GTK_WIDGET(window->calendar), FALSE, FALSE, 0);

	 window->loglist = loglist_create (window);
	 gtk_box_pack_start (GTK_BOX (window->sidebar), window->loglist, TRUE, TRUE, 0);

	 gtk_paned_pack1 (GTK_PANED (hpaned), window->sidebar, FALSE, FALSE);

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
   window->view = gtk_tree_view_new ();
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (window->view), TRUE); 
   gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (window->view), TRUE);

   /* Use the desktop monospace font */
   monospace_font_name = gconf_client_get_string (client, GCONF_MONOSPACE_FONT_NAME, NULL);
   fontdesc = pango_font_description_from_string (monospace_font_name);
   if (pango_font_description_get_family (fontdesc) != NULL)
     gtk_widget_modify_font (GTK_WIDGET(window->view), fontdesc);
   g_free (monospace_font_name);
   g_free (fontdesc);
   gconf_client_notify_add (client, GCONF_MONOSPACE_FONT_NAME, 
                            logview_monospace_font_changed, window, NULL, NULL);
  
   for (i = 0; column_titles[i]; i++) {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_(column_titles[i]),
                    renderer, "text", i, NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, column_widths[i]);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (window->view), column);
   }
   gtk_tree_view_set_search_column (GTK_TREE_VIEW (window->view), 3);

	 /* Version selector */
	 window->version_bar = gtk_hbox_new (FALSE, 0);
	 gtk_container_set_border_width (GTK_CONTAINER (window->version_bar), 3);
	 window->version_selector = gtk_combo_box_new_text ();
	 g_signal_connect (G_OBJECT (window->version_selector), "changed",
										 G_CALLBACK (logview_version_selector_changed), window);
	 label = gtk_label_new (_("Version: "));

	 gtk_box_pack_end (GTK_BOX(window->version_bar), window->version_selector, FALSE, FALSE, 0);
	 gtk_box_pack_end (GTK_BOX(window->version_bar), label, FALSE, FALSE, 0);
	 gtk_box_pack_end (GTK_BOX(main_view), window->version_bar, FALSE, FALSE, 0);

	 /* Remember the original font size */
	 context = gtk_widget_get_pango_context (window->view);
	 fontdesc = pango_context_get_font_description (context);
	 window->original_fontsize = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
	 window->fontsize = window->original_fontsize;

   gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (window->view));
   gtk_widget_show_all (scrolled);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

   /* Add signal handlers */
   g_signal_connect (G_OBJECT (selection), "changed",
                     G_CALLBACK (handle_selection_changed_cb), window);
   g_signal_connect (G_OBJECT (window->view), "row-expanded",
                     G_CALLBACK (row_toggled_cb), window);
   g_signal_connect (G_OBJECT (window->view), "row-collapsed",
                     G_CALLBACK (row_toggled_cb), window);
   g_signal_connect (G_OBJECT (window), "configure_event",
                     G_CALLBACK (window_size_changed_cb), window);

   window->find_bar = logview_findbar_new (window);
   gtk_box_pack_start (GTK_BOX (vbox), window->find_bar, FALSE, FALSE, 0);
   gtk_widget_show (window->find_bar);

   /* Status area at bottom */
   hbox = gtk_hbox_new (FALSE, 0);   
   window->statusbar = gtk_statusbar_new ();
   gtk_box_pack_start (GTK_BOX (hbox), window->statusbar, TRUE, TRUE, 0);
   window->progressbar = gtk_progress_bar_new ();
   gtk_box_pack_start (GTK_BOX (hbox), window->progressbar, FALSE, FALSE, 0);

   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   gtk_widget_show_all (vbox);
   gtk_widget_hide (window->find_bar);
   gtk_widget_hide (window->version_bar);
   gtk_widget_hide (window->progressbar);
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogMenu
   DESCRIPTION:   Close the current log.
   ---------------------------------------------------------------------- */

static void
CloseLogMenu (GtkAction *action, GtkWidget *callback_data)
{
   LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

   g_return_if_fail (window->curlog);

   if (window->curlog->monitored) {
	   GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/FileMenu/MonitorLogs");
	   gtk_action_activate (action);
   }

   gtk_widget_hide (window->find_bar);
	 loglist_remove_selected_log (window);
}

/* ----------------------------------------------------------------------
   NAME:          FileSelectResponse
   DESCRIPTION:   User selected a file.
   ---------------------------------------------------------------------- */

static void
FileSelectResponse (GtkWidget * chooser, gint response, gpointer data)
{
   char *f;
   LogviewWindow *logview = data;

   gtk_widget_hide (GTK_WIDGET (chooser));
   if (response != GTK_RESPONSE_OK)
	   return;
   
   f = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

   if (f != NULL) {
	   /* Check if the log is not already opened */
	   GList *list;
		 Log *log, *tl;
	   for (list = logview->logs; list != NULL; list = g_list_next (list)) {
		   log = list->data;
		   if (g_ascii_strncasecmp (log->name, f, 255) == 0) {
				 loglist_select_log_from_name (logview, log->name);
			   return;
		   }
	   }

	   if ((tl = OpenLogFile (f, TRUE)) != NULL)
	     logview_add_log (logview, tl);
   }
   
   g_free (f);

}

/* ----------------------------------------------------------------------
   NAME:          LoadLogMenu
   DESCRIPTION:   Open a new log defined by the user.
   ---------------------------------------------------------------------- */

static void
LoadLogMenu (GtkAction *action, GtkWidget *parent_window)
{
   static GtkWidget *chooser = NULL;
   
   if (chooser == NULL) {
	   chooser = gtk_file_chooser_dialog_new (_("Open new logfile"),
						  GTK_WINDOW (parent_window),
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);
	   gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);
	   gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
	   g_signal_connect (G_OBJECT (chooser), "response",
			     G_CALLBACK (FileSelectResponse), parent_window);
	   g_signal_connect (G_OBJECT (chooser), "destroy",
			     G_CALLBACK (gtk_widget_destroyed), &chooser);
	   if (user_prefs->logfile != NULL)
		   gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), 
						  user_prefs->logfile);
   }

   gtk_window_present (GTK_WINDOW (chooser));
}

static void
toggle_sidebar (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

	if (window->sidebar_visible)
		gtk_widget_hide (window->sidebar);
	else
		gtk_widget_show (window->sidebar);
	window->sidebar_visible = !(window->sidebar_visible);
}

static void 
toggle_calendar (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

	if (window->calendar_visible)
		gtk_widget_hide (window->calendar);
	else {
		gtk_widget_show (window->calendar);
		if (!window->sidebar_visible) {
			GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/ViewMenu/ShowSidebar");
			gtk_action_activate (action);
		}
	}
	window->calendar_visible = !(window->calendar_visible);
}

static void 
logview_collapse_rows (GtkAction *action, GtkWidget *callback_data)
{
    LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
    gtk_tree_view_collapse_all (GTK_TREE_VIEW (window->view));
}

static void
toggle_monitor (GtkAction *action, GtkWidget *callback_data)
{
    LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
    g_return_if_fail (window->curlog);
    if (!window->curlog->display_name) {
	    if (window->curlog->monitored)
		    monitor_stop (window->curlog);
	    else
		    monitor_start (window->curlog);

	    logview_set_window_title (window);
	    logview_menus_set_state (window);
    }
}

static void
logview_set_fontsize (LogviewWindow *logview)
{
	PangoFontDescription *fontdesc;
	PangoContext *context;
	int i;
	
	context = gtk_widget_get_pango_context (logview->view);
	fontdesc = pango_context_get_font_description (context);
	pango_font_description_set_size (fontdesc, (logview->fontsize)*PANGO_SCALE);
	gtk_widget_modify_font (logview->view, fontdesc);
	logview_save_prefs (logview);
}	

static void
logview_bigger_text (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (callback_data);
	logview->fontsize = MIN (logview->fontsize + 1, 24);
	logview_set_fontsize (logview);
}	

static void
logview_smaller_text (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (callback_data);
	logview->fontsize = MAX (logview->fontsize-1, 6);
	logview_set_fontsize (logview);
}	

static void
logview_normal_text (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (callback_data);
	logview->fontsize = logview->original_fontsize;
	logview_set_fontsize (logview);
}
	
static void
logview_search (GtkAction *action,GtkWidget *callback_data)
{
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

	gtk_widget_show (window->find_bar);
	gtk_widget_grab_focus (window->find_entry);
}

static void
logview_copy (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
	LogLine *line;
	int nline, i;
	gchar *text, **lines;

	if (window->curlog->selected_line_first > -1 && window->curlog->selected_line_last > -1) {
		nline = window->curlog->selected_line_last - window->curlog->selected_line_first + 1;
		lines = g_new0(gchar *, (nline + 1));
		for (i=0; i<=nline; i++) {
			if (window->curlog->selected_line_first + i < window->curlog->total_lines) {
				line = (window->curlog->lines)[window->curlog->selected_line_first + i];
				if (line && line->hostname && line->process && line->message)
					lines[i] = g_strdup_printf ("%s %s %s", line->hostname, line->process, line->message);
			}
		}
		lines[nline] = NULL;
		text = g_strjoinv ("\n", lines);
		gtk_clipboard_set_text (window->clipboard, LocaleToUTF8(text), -1);
		g_free (text);
		g_strfreev (lines);
	}
}

static void
logview_select_all (GtkAction *action, GtkWidget *callback_data)
{
	GtkTreeSelection *selection;
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
	gtk_tree_selection_select_all (selection);
}

static void
logview_menu_item_set_state (LogviewWindow *logviewwindow, char *path, gboolean state)
{
	GtkAction *action;
	g_return_if_fail (path);
	action = gtk_ui_manager_get_action (logviewwindow->ui_manager, path);
	gtk_action_set_sensitive (action, state);
}

static void
logview_calendar_set_state (LogviewWindow *logview)
{
    if (logview->curlog) {
        if (logview->curlog->has_date)
            init_calendar_data (logview);
        gtk_widget_set_sensitive (logview->calendar, logview->curlog->has_date);
    } else
        gtk_widget_set_sensitive (logview->calendar, FALSE);
}

static void
logview_menus_set_state (LogviewWindow *window)
{
  g_return_if_fail (window);

  if (window->curlog) {
    if (window->curlog->display_name)
      logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/MonitorLogs", FALSE);
    else
      logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/MonitorLogs", TRUE);
    
    if (window->curlog->has_date)
      logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowCalendar", TRUE);
    else
      logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowCalendar", FALSE);
    
  } else {
    logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/MonitorLogs", FALSE);
    logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowCalendar", FALSE);
  }
  
  logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/Properties", (window->curlog != NULL));
  logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/CloseLog", (window->curlog != NULL));
  logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/CollapseAll", (window->curlog != NULL));
  logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/Search", (window->curlog != NULL));
  logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/Copy", (window->curlog != NULL));
  logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/SelectAll", (window->curlog != NULL));
}

static int
logview_count_logs (void)
{
	GSList *list;
    GList *logs;
	LogviewWindow *window;
	int numlogs = 0;
	for (list = logview_windows; list != NULL; list = g_slist_next (list)) {
		window = list->data;
        for (logs = window->logs; logs != NULL; logs = g_list_next (logs))
            numlogs ++;
	}
	return numlogs;
}

static void
logview_help (GtkAction *action, GtkWidget *parent_window)
{
        GError *error = NULL;                                                                                
        gnome_help_display_desktop_on_screen (NULL, "gnome-system-log", "gnome-system-log", NULL,
					      gtk_widget_get_screen (GTK_WIDGET(parent_window)), &error);
	if (error) {
		ShowErrMessage (GTK_WIDGET(parent_window), _("There was an error displaying help."), error->message);
		g_error_free (error);
	}
}

void
logview_set_window_title (LogviewWindow *window)
{
	gchar *window_title;
	gchar *logname;

	if ((window->curlog != NULL) && (window->curlog->name != NULL)) {
		if (window->curlog->display_name != NULL)
			logname = window->curlog->display_name;
		else
			logname = window->curlog->name;
		
		if (window->curlog->monitored) 
			window_title = g_strdup_printf (_("%s (monitored) - %s"), logname, APP_NAME);
		else
			window_title = g_strdup_printf ("%s - %s", logname, APP_NAME);
	}
	else
		window_title = g_strdup_printf (APP_NAME);
	gtk_window_set_title (GTK_WINDOW (window), window_title);
	g_free (window_title);
}

static void
logview_window_finalize (GObject *object)
{
        LogviewWindow *window = (LogviewWindow *) object;

	g_object_unref (window->ui_manager);
        parent_class->finalize (object);
}

static void
logview_window_class_init (LogviewWindowClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = logview_window_finalize;

	parent_class = g_type_class_peek_parent (klass);
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
			(GInstanceInitFunc) CreateMainWin
		};

		object_type = g_type_register_static (GTK_TYPE_WINDOW, "LogviewWindow", &object_info, 0);
	}

	return object_type;
}

gboolean 
window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
				 gpointer data)
{
	LogviewWindow *window = data;

	prefs_store_size (GTK_WIDGET(window), user_prefs);
	return FALSE;
}
