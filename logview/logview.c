#ifndef GNOMELOCALEDIR
#define GNOMELOCALEDIR "/usr/share/locale"
#endif
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
#include <gconf/gconf-client.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "logview.h"
#include <libgnomeui/gnome-window-icon.h>

/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void repaint (GtkWidget * canvas, GdkRectangle * area);
void CreateMainWin (void);
gboolean log_repaint (GtkWidget * canvas, GdkRectangle * area);
gboolean PointerMoved (GtkWidget * canvas, GdkEventMotion * event);
gboolean HandleLogKeyboard (GtkWidget * win, GdkEventKey * event_key);
gboolean handle_log_mouse_button (GtkWidget * win, GdkEventButton *event);
gboolean handle_log_mouse_scroll (GtkWidget * win, GdkEventScroll *event);
void ExitProg (GtkWidget * widget, gpointer user_data);
void LoadLogMenu (GtkWidget * widget, gpointer user_data);
void CloseLogMenu (GtkWidget * widget, gpointer user_data);
void change_log_menu (GtkWidget * widget, gpointer user_data);
void CalendarMenu (GtkWidget * widget, gpointer user_data);
void MonitorMenu (GtkWidget* widget, gpointer user_data); 
void create_zoom_view (GtkWidget *widget, gpointer data);
void UserPrefsDialog(GtkWidget * widget, gpointer user_data);
void AboutShowWindow (GtkWidget* widget, gpointer user_data);
void CloseApp (void);
void CloseLog (Log *);
void FileSelectCancel (GtkWidget * w, GtkFileSelection * fs);
void FileSelectOk (GtkWidget * w, GtkFileSelection * fs);
void MainWinScrolled (GtkAdjustment *adjustment, GtkRange *);
void CanvasResized (GtkWidget *widget, GtkAllocation *allocation);
gboolean ScrollWin (GtkRange *range, gpointer event);
void LogInfo (GtkWidget * widget, gpointer user_data);
void UpdateStatusArea (void);
void set_scrollbar_size (int);
void change_log (int dir);
void open_databases (void);
void destroy (void);
void InitApp (void);
int InitPages (void);
int RepaintLogInfo (GtkWidget * widget, GdkEventExpose * event);
int read_regexp_db (char *filename, GList **db);
int read_actions_db (char *filename, GList **db);
void print_db (GList *gb);
Log *OpenLogFile (char *);
GtkWidget *new_pixmap_from_data (char  **, GdkWindow *, GdkColor *);
GtkWidget *create_menu (char *item[], int n);
void SaveUserPrefs(UserPrefsStruct *prefs);
void close_zoom_view (GtkWidget *widget, gpointer client_data);

static void toggle_calendar (void);
static void toggle_zoom (void);

/*
 *    ,-------.
 *    | Menus |
 *    `-------'
 */


GnomeUIInfo log_menu[] = {
	GNOMEUIINFO_MENU_OPEN_ITEM(LoadLogMenu, NULL),
	GNOMEUIINFO_MENU_SAVE_AS_ITEM(StubCall, NULL),
	GNOMEUIINFO_SEPARATOR,
        { GNOME_APP_UI_ITEM, N_("S_witch Log"), 
	  N_("Switch log"), change_log_menu, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        { GNOME_APP_UI_ITEM, N_("_Monitor..."), 
	  N_("Monitor Log"), MonitorMenu, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,
        { GNOME_APP_UI_ITEM, N_("_Properties"), 
	  N_("Show Log Properties"), LogInfo, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL, 'I', GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_CLOSE_ITEM(CloseLogMenu, NULL),
	GNOMEUIINFO_MENU_QUIT_ITEM(ExitProg, NULL),
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

GnomeUIInfo view_menu[] = {
        { GNOME_APP_UI_TOGGLEITEM, N_("_Calendar"), N_("Show Calendar Log"), toggle_calendar, 
	  NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, 'L', GDK_CONTROL_MASK, NULL },
        { GNOME_APP_UI_TOGGLEITEM, N_("_Entry Detail"), N_("Show Entry Detail"), toggle_zoom, 
	  NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, 'D', GDK_CONTROL_MASK, NULL },
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};


GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP("gnome-system-log"),
        {GNOME_APP_UI_ITEM, N_("About..."), 
	 N_("Info about logview"), AboutShowWindow,
         NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

GnomeUIInfo main_menu[] = {
        { GNOME_APP_UI_SUBTREE, N_("_Log"), NULL,
          log_menu, NULL, NULL, (GnomeUIPixmapType) 0,
          NULL, 0, (GdkModifierType) 0, NULL },
	GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};
                 

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */


GtkWidget *app = NULL;
GtkWidget *main_win_scrollbar = NULL;
static GtkWidget *viewport;
GtkWidget *main_win_hor_scrollbar = NULL; 
GtkLabel *filename_label = NULL, *date_label = NULL;

GList *regexp_db = NULL, *descript_db = NULL, *actions_db = NULL;
UserPrefsStruct *user_prefs = NULL;
UserPrefsStruct user_prefs_struct = {0};
ConfigData *cfg = NULL;
GtkWidget *open_file_dialog = NULL;

GConfClient *client;
poptContext poptCon;
gint next_opt;

struct poptOption options[] = { {
	"file",
	'f',
	POPT_ARG_STRING,
	NULL,
	1,
	NULL,
	NULL,
} };

extern GtkWidget *CalendarDialog;
extern GtkWidget *zoom_dialog;
extern GdkGC *gc;
extern Log *curlog, *loglist[];
extern int numlogs, curlognum;
extern int loginfovisible, calendarvisible;
extern int cursor_visible;
extern int zoom_visible;
extern PangoLayout *log_layout;

/* ----------------------------------------------------------------------
   NAME:          destroy
   DESCRIPTION:   Exit program.
   ---------------------------------------------------------------------- */

void
destroy (void)
{
   CloseApp ();
}

static gint
save_session (GnomeClient *gclient, gint phase,
              GnomeRestartStyle save_style, gint shutdown,
              GnomeInteractStyle interact_style, gint fast,
              gpointer client_data)
{
   gchar **argv;
   gint i;

   argv = g_malloc0 (sizeof (gchar *) * (numlogs+1));
   argv[0] = (gchar *) client_data;
   for ( i = 1; i <= numlogs; i++) 
       argv[i] = g_strconcat ("--file=", loglist[i-1]->name, NULL);
   
   gnome_client_set_clone_command (gclient, numlogs+1, argv);
   gnome_client_set_restart_command (gclient, numlogs+1, argv);

   g_free (argv);

   return TRUE;
}

static gboolean
restore_session (void)
{
   Log *tl;
   gint i, logcnt = 0;

   /* closing the log file opened by default */
   CloseLog (loglist[0]);
   curlog = NULL;
   loglist[0] = NULL;
   curlognum = 0;
   log_repaint (NULL, NULL);
   if (loginfovisible)
       RepaintLogInfo (NULL, NULL);
   set_scrollbar_size (1);
   numlogs = 0;

   next_opt = poptGetNextOpt (poptCon);

   do {
      if ( next_opt == 1) {
         gchar *f = (gchar *) poptGetOptArg (poptCon);
         if (f != NULL) {
            if ((tl = OpenLogFile (f)) != NULL) {
               curlog = tl;
               loglist[numlogs] = tl;
               numlogs++;
               curlognum = numlogs - 1;
            }
         }
         if (f)
            g_free (f);
      }
   } while ((next_opt = poptGetNextOpt (poptCon)) != -1);
}

static gint
die (GnomeClient *gclient, gpointer client_data)
{
    gtk_main_quit ();
}

/* ----------------------------------------------------------------------
   NAME:          main
   DESCRIPTION:   Program entry point.
   ---------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
  GnomeClient *gclient;

  bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  QueueErrMessages (TRUE);

  /*  Initialize gnome & gtk */
  gnome_program_init ("gnome-system-log",VERSION, LIBGNOMEUI_MODULE, argc, argv,
  		      GNOME_PARAM_APP_DATADIR, DATADIR, NULL);

  gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-log.png");
  
  poptCon = poptGetContext ("gnome-system-log", argc, (const gchar **) argv, 
                            options, 0);  
  gclient = gnome_master_client ();
  g_signal_connect (gclient, "save_yourself",
                    G_CALLBACK (save_session), (gpointer)argv[0]);
  g_signal_connect (gclient, "die", G_CALLBACK (die), NULL);

  gconf_init (argc, argv, NULL);
  client = gconf_client_get_default ();
  
  /*  Load graphics config */
  cfg = CreateConfig();
  
  /*  Show about window */
  /* AboutShowWindow (NULL, NULL); */

  InitApp ();

  QueueErrMessages (FALSE);
  ShowQueuedErrMessages ();
   
  if (gnome_client_get_flags (gclient) & GNOME_CLIENT_RESTORED) {
     restore_session ();
  }

  /*  Loop application */
  gtk_main ();
  
  SaveUserPrefs(user_prefs);

  return 0;
}

/* ----------------------------------------------------------------------
   NAME:        InitApp
   DESCRIPTION: Main initialization routine.
   ---------------------------------------------------------------------- */

void
InitApp ()
{
  /*  Initialize variables */
  loginfovisible = FALSE;
  regexp_db = NULL;
  user_prefs = &user_prefs_struct;
  SetDefaultUserPrefs(user_prefs);

  /*  Display main window */
  CreateMainWin ();

  /* Read databases */
  open_databases ();

  /*  Read files and init data. */
  if (InitPages () < 0)
    ShowErrMessage (_("No log files to open"));
}

/* ----------------------------------------------------------------------
   NAME:        CreateMainWin
   DESCRIPTION: Creates the main window.
   ---------------------------------------------------------------------- */

void
CreateMainWin ()
{
   GtkWidget *canvas;
   GtkWidget *w, *vbox, *table, *hbox, *hbox_date;
   GtkWidget *padding;
   GtkLabel *label;
   GtkObject *adj;
   GtkAllocation req_size;

   /* Create App */

   app = gnome_app_new ("gnome-system-log", _("System Log Viewer"));

   gtk_container_set_border_width ( GTK_CONTAINER (app), 0);
   gtk_window_set_default_size ( GTK_WINDOW (app), LOG_CANVAS_W, LOG_CANVAS_H);
   req_size.x = req_size.y = 0;
   req_size.width = 400;
   req_size.height = 400;
   gtk_widget_size_allocate ( GTK_WIDGET (app), &req_size );
   gtk_signal_connect (GTK_OBJECT (app), "destroy",
		       GTK_SIGNAL_FUNC (destroy), NULL);

   /* Create menus */
   gnome_app_create_menus (GNOME_APP (app), main_menu);

   vbox = gtk_vbox_new (FALSE, 0);
   gnome_app_set_contents (GNOME_APP (app), vbox);

   /* Deactivate unfinished items */
   gtk_widget_set_state (log_menu[1].widget, GTK_STATE_INSENSITIVE);
   if (numlogs < 2)
     gtk_widget_set_state (log_menu[3].widget, GTK_STATE_INSENSITIVE);

   /* Create main canvas and scroll bars */
   table = gtk_table_new (2, 2, FALSE);
   gtk_widget_show (table);

   viewport = gtk_viewport_new (NULL, NULL);
   gtk_widget_set_size_request (viewport, LOG_CANVAS_W, 0); 
   gtk_widget_show (viewport);
               
   canvas = gtk_drawing_area_new ();
   gtk_drawing_area_size (GTK_DRAWING_AREA (canvas), 2*LOG_CANVAS_W,
			  LOG_CANVAS_H); 
   gtk_container_add (GTK_CONTAINER (viewport), canvas);
   gtk_table_attach (GTK_TABLE (table), viewport, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

   if (curlog != NULL)
	   adj = (GtkObject *)gtk_viewport_get_hadjustment (GTK_VIEWPORT (viewport));
   else
	   adj = gtk_adjustment_new (100.0, 0.0, 101.0, 1, 10, 101.0);
   
   main_win_hor_scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj));
   gtk_widget_show(main_win_hor_scrollbar);
   gtk_table_attach (GTK_TABLE (table), main_win_hor_scrollbar, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

   if (curlog != NULL)
     adj = gtk_adjustment_new ( curlog->ln, 0.0,
				curlog->lstats.numlines,
				1.0, 10.0, (float) LINES_P_PAGE);
   else
     adj = gtk_adjustment_new (100.0, 0.0, 101.0, 1, 10, 101.0);

   main_win_scrollbar = (GtkWidget *)gtk_vscrollbar_new (GTK_ADJUSTMENT(adj));
   gtk_range_set_update_policy (GTK_RANGE (main_win_scrollbar), GTK_UPDATE_CONTINUOUS);

   gtk_table_attach (GTK_TABLE (table), main_win_scrollbar, 1, 2, 0, 1,
		     (GtkAttachOptions) (0),
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
   gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		       (GtkSignalFunc) MainWinScrolled,
		       (gpointer) main_win_scrollbar);       
   gtk_widget_show (main_win_scrollbar);  


   gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
   
   /*  Install event handlers */
   gtk_signal_connect (GTK_OBJECT (canvas), "expose_event",
		       GTK_SIGNAL_FUNC (log_repaint), NULL);
   gtk_signal_connect (GTK_OBJECT (app), "key_press_event",
		       GTK_SIGNAL_FUNC (HandleLogKeyboard), NULL);
   gtk_signal_connect (GTK_OBJECT (canvas), "button_press_event",
		       GTK_SIGNAL_FUNC (handle_log_mouse_button), NULL);
   g_signal_connect (G_OBJECT (canvas), "scroll_event", 
   		     G_CALLBACK (handle_log_mouse_scroll), NULL);
   gtk_signal_connect (GTK_OBJECT (canvas), "size_allocate",
		       GTK_SIGNAL_FUNC (CanvasResized), NULL);
   gtk_widget_set_events (canvas, GDK_EXPOSURE_MASK |
			  GDK_BUTTON_PRESS_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_SCROLL_MASK
			  );

   gtk_widget_set_events (app, GDK_KEY_PRESS_MASK);


   gtk_widget_set_style (canvas, cfg->white_bg_style);
   gtk_widget_show (canvas);


   /* Create status area at bottom */
   hbox = gtk_hbox_new (FALSE, 2);
   gtk_container_set_border_width ( GTK_CONTAINER (hbox), 3);

   label = (GtkLabel *)gtk_label_new (_("Filename: "));
   gtk_label_set_justify (label, GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
   gtk_widget_show (GTK_WIDGET (label));  

   filename_label = (GtkLabel *)gtk_label_new ("");
   gtk_widget_show ( GTK_WIDGET (filename_label));  
   gtk_label_set_justify (label, GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (filename_label), 
		       FALSE, FALSE, 0);
   
   hbox_date = gtk_hbox_new (FALSE, 2);
   gtk_container_set_border_width ( GTK_CONTAINER (hbox_date), 3);

   date_label = (GtkLabel *)gtk_label_new ("");
   gtk_widget_show (GTK_WIDGET (date_label));
   /* gtk_widget_set_size_request (GTK_WIDGET (label), 60, -1); */
   gtk_box_pack_end (GTK_BOX (hbox_date), GTK_WIDGET (date_label), 
		     TRUE, TRUE, 0);


   label = (GtkLabel *)gtk_label_new (_("Last Modified: "));
   gtk_label_set_justify (label, GTK_JUSTIFY_RIGHT);
   gtk_box_pack_end (GTK_BOX (hbox_date), GTK_WIDGET (label), 
		     FALSE, FALSE, 0);
   gtk_widget_show (GTK_WIDGET (label));  


   gtk_widget_show (hbox_date);
   gtk_box_pack_end (GTK_BOX (hbox), hbox_date, FALSE, FALSE, 0);
   gtk_widget_show (hbox);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   gtk_widget_show (vbox);
   gtk_widget_show_now (app);

}

/* ----------------------------------------------------------------------
   NAME:          MainScreenResized
   DESCRIPTION:   The main screen was resized.
   ---------------------------------------------------------------------- */

void
CanvasResized (GtkWidget *widget, GtkAllocation *allocation)
{
  DB (
      if (allocation)
      	printf ("Main screen resized!\n New size = (%d,%d)\n", 
		allocation->width, allocation->height);
      else
        printf ("Main screen resized!\nNo allocation :(\n");
	);
}

/* ----------------------------------------------------------------------
   NAME:          ScrollWin
   DESCRIPTION:   When the mouse button is released we scroll the window.
   ---------------------------------------------------------------------- */

gboolean
ScrollWin (GtkRange *range, gpointer event)
{
  int newln;
  
  newln = (int) range->adjustment->value;
  if (curlog == NULL ||
      newln >= curlog->lstats.numlines ||
      newln < 0)
    return FALSE;

  /* evil, yes */
  if (newln == 0)
	  newln = 1;
  
  /* Goto mark */
  MoveToMark (curlog);
  curlog->firstline = 0;
  
  ScrollDown (newln - curlog->curmark->ln);
  
  /* Repaint screen */
  log_repaint(NULL, NULL);

  return FALSE;
}

/* ----------------------------------------------------------------------
   NAME:          MainWinScrolled
   DESCRIPTION:   main window scrolled
   ---------------------------------------------------------------------- */

void
MainWinScrolled (GtkAdjustment *adjustment, GtkRange *range)
{
  int newln, howmuch;
  DateMark *mark;

  newln = (int) range->adjustment->value;

 if (newln < 0 ||
     curlog == NULL)
   return;

 /* evil, yes */
 if (newln == 0)
	 newln = 1;

 if (newln >= curlog->lstats.numlines)
   newln = curlog->lstats.numlines - 1;

  /* Find mark which has this line */
  mark = curlog->curmark;
  if (mark == NULL)
    return;

  if (newln >= mark->ln)
    {
      while( mark->next != NULL)
	{
	  if (newln <= mark->next->ln)
	    break;
	  mark = mark->next;
	}
    }
  else
    {
      while( mark->prev != NULL)
	{
	  if (newln >= mark->ln)
	    break;
	  mark = mark->prev;
	}
    }

  /* Now lets make it the current mark */
  cursor_visible = FALSE;
  howmuch = newln - curlog->ln;
  curlog->ln = newln;
  if (mark != curlog->curmark)
    {
      curlog->curmark = mark;
      MoveToMark (curlog);
      curlog->firstline = 0;
      howmuch = newln - mark->ln;
    }

  /* Update status area */
  UpdateStatusArea ();

  /* Only scroll when the scrollbar is released */
  if (howmuch > 0)
      ScrollDown (howmuch);
  else
      ScrollUp (-1*howmuch);

  if (howmuch != 0)
    log_repaint(NULL, NULL);
}


/* ----------------------------------------------------------------------
   NAME:          set_scrollbar_size
   DESCRIPTION:   Set size of scrollbar acording to file.
   ---------------------------------------------------------------------- */

void set_scrollbar_size (int num_lines)
{
  GtkObject *adj;

  adj = gtk_adjustment_new (-1, 0.0, num_lines,
			    1.0, 10.0, (float) LINES_P_PAGE);
  gtk_range_set_adjustment (GTK_RANGE (main_win_scrollbar),
			    GTK_ADJUSTMENT (adj));
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      (GtkSignalFunc) MainWinScrolled,
		      (gpointer) main_win_scrollbar);       
  if (curlog != NULL )
  {
  	gtk_adjustment_set_value (GTK_ADJUSTMENT(adj), curlog->ln); 
	adj = (GtkObject*)gtk_viewport_get_hadjustment (GTK_VIEWPORT (viewport));
  	gtk_adjustment_set_value (GTK_ADJUSTMENT(adj),0); 
  }
  else
	adj = gtk_adjustment_new (100.0, 0.0, 101.0, 1, 10, 101.0);

  gtk_range_set_adjustment (GTK_RANGE (main_win_hor_scrollbar),GTK_ADJUSTMENT (adj));

  gtk_widget_realize (main_win_scrollbar);
  gtk_widget_realize (main_win_hor_scrollbar);

  gtk_widget_queue_resize (main_win_scrollbar);
  gtk_widget_queue_resize (main_win_hor_scrollbar);
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogMenu
   DESCRIPTION:   Close the current log.
   ---------------------------------------------------------------------- */

void
CloseLogMenu (GtkWidget * widget, gpointer user_data)
{
   int i;

   if (numlogs == 0)
      return;

   CloseLog (curlog);
   numlogs--;
   if (numlogs == 0)
   {
      curlog = NULL;
      loglist[0] = NULL;
      curlognum = 0;
      log_repaint (NULL, NULL);
      if (loginfovisible)
	 RepaintLogInfo (NULL, NULL);
      set_scrollbar_size (1);
      gtk_widget_set_sensitive (log_menu[7].widget, FALSE); 
      gtk_widget_set_sensitive (log_menu[4].widget, FALSE); 
      for ( i = 0; i < 3; i++) 
         gtk_widget_set_sensitive (view_menu[i].widget, FALSE); 
      return;
   }
   for (i = curlognum; i < numlogs; i++)
      loglist[i] = loglist[i + 1];
   loglist[i] = NULL;

   if (curlognum > 0)
      curlognum--;
   curlog = loglist[curlognum];
   log_repaint (NULL, NULL);

   if (loginfovisible)
      RepaintLogInfo (NULL, NULL);

   /* Change menu entry if there is only one log */
   if (numlogs < 2)
     gtk_widget_set_state (log_menu[3].widget, GTK_STATE_INSENSITIVE);

   set_scrollbar_size (curlog->lstats.numlines);
}

/* ----------------------------------------------------------------------
   NAME:          change_log_menu
   DESCRIPTION:   Switch log
   ---------------------------------------------------------------------- */

void
change_log_menu (GtkWidget * widget, gpointer user_data)
{
  change_log (1);
}

/* ----------------------------------------------------------------------
   NAME:          FileSelectOk
   DESCRIPTION:   User selected a file.
   ---------------------------------------------------------------------- */

void
FileSelectOk (GtkWidget * w, GtkFileSelection * fs)
{
   char *f;
   Log *tl;
   gint i;

   /* Check that we haven't opened all logfiles allowed    */
   if (numlogs >= MAX_NUM_LOGS)
     {
       ShowErrMessage (_("Too many open logs. Close one and try again"));
       return;
     }

   f = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));
   gtk_widget_destroy (GTK_WIDGET (fs));

   /* Check whether we are opening the already opened log file */ 
   for ( i = 0; i < numlogs; i++)
   {
      if (strcmp (f, loglist[i]->name) == 0)
      {
         ShowErrMessage (_("File already opened"));
         g_free (f);
         return;
      }
   }      

   if (f != NULL) {
      if ((tl = OpenLogFile (f)) != NULL)
      {
	 curlog = tl;
	 loglist[numlogs] = tl;
	 numlogs++;
	 curlognum = numlogs - 1;

	 /* Clear window */
	 log_repaint (NULL, NULL);
	 if (loginfovisible)
	   RepaintLogInfo (NULL, NULL);
	 if (calendarvisible)
	   init_calendar_data();
	 UpdateStatusArea();

	 /* Set main scrollbar */
	 set_scrollbar_size (curlog->lstats.numlines);

	 if (numlogs)
	 {
	   int i;
	   if (numlogs >= 2)
	     gtk_widget_set_sensitive (log_menu[3].widget, TRUE);
	   gtk_widget_set_sensitive (log_menu[7].widget, TRUE);
	   gtk_widget_set_sensitive (log_menu[4].widget, TRUE);
	   for ( i = 0; i < 3; i++) 
	     gtk_widget_set_sensitive (view_menu[i].widget, TRUE);
	 } 
      }
   }
   g_free (f);

}

/* ----------------------------------------------------------------------
   NAME:          LoadLogMenu
   DESCRIPTION:   Open a new log defined by the user.
   ---------------------------------------------------------------------- */

void
LoadLogMenu (GtkWidget * widget, gpointer user_data)
{
   GtkWidget *filesel = NULL;

   /*  Cannot open more than MAX_NUM_LOGS */
   if (numlogs == MAX_NUM_LOGS)
     { 
       ShowErrMessage (_("Too many open logs. Close one and try again")); 
       return;
     }
   
   /*  Cannot have more than one fileselect window */
   /*  at one time. */
   if (open_file_dialog != NULL) {
	   gtk_widget_show_now (open_file_dialog);
	   gdk_window_raise (open_file_dialog->window);
	   return;
   }


   filesel = gtk_file_selection_new (_("Open new logfile"));
   gtk_window_set_transient_for (GTK_WINDOW (filesel), GTK_WINDOW (app));
   gnome_window_icon_set_from_default (GTK_WINDOW (filesel));

   /* Make window modal */
   gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);

   gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), 
   				    user_prefs->logfile);

   gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
   gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		       "clicked", (GtkSignalFunc) FileSelectOk,
		       filesel);
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			      "clicked", (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT (filesel));

   gtk_signal_connect (GTK_OBJECT (filesel),
		       "destroy", (GtkSignalFunc) gtk_widget_destroyed,
		       &open_file_dialog);

   gtk_widget_show (filesel);

   open_file_dialog = filesel;
}





/* ----------------------------------------------------------------------
   NAME:          ExitProg
   DESCRIPTION:   Callback to call when program exits.
   ---------------------------------------------------------------------- */

void 
ExitProg (GtkWidget * widget, gpointer user_data)
{
   CloseApp ();
}

/* ----------------------------------------------------------------------
   NAME:          CloseApp
   DESCRIPTION:   Close everything and exit.
   ---------------------------------------------------------------------- */

void 
CloseApp (void)
{
   int i;

   for (i = 0; i < numlogs; i++)
      CloseLog (loglist[i]);

   numlogs = 0;

   if (G_IS_OBJECT (log_layout)) {
      g_object_unref (G_OBJECT (log_layout));
      log_layout = NULL;
   }

   gtk_main_quit ();   
}

/* ----------------------------------------------------------------------
   NAME:          open_databases
   DESCRIPTION:   Try to locate regexp and descript databases and load
   	          them.
   ---------------------------------------------------------------------- */

void
open_databases (void)
{
	char full_name[1024];
	gboolean found;

	/* Find regexp DB -----------------------------------------------------  */
	found = FALSE;
	if (cfg->regexp_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-regexp.db", cfg->regexp_db_path);
		DB (fprintf (stderr, "Looking for database in [%s]\n", cfg->regexp_db_path));
		if (access (full_name, R_OK) == 0)  {
			found = TRUE;
			read_regexp_db (full_name, &regexp_db);
		}
	}

	if ( ! found) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/share/gnome-system-log/gnome-system-log-regexp.db", LOGVIEWINSTALLPREFIX);
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			g_free (cfg->regexp_db_path);
			cfg->regexp_db_path = g_strdup (full_name);
			read_regexp_db (full_name, &regexp_db);
		}
	}

	/* Find description DB ------------------------------------------------  */
	found = FALSE;
	if (cfg->descript_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-descript.db", cfg->descript_db_path);
		DB (fprintf (stderr, "Looking for database in [%s]\n", cfg->descript_db_path));
		if (access (full_name, R_OK) == 0) {
			read_descript_db (full_name, &descript_db);
			found = TRUE;
		}
	}

	if ( ! found) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/share/gnome-system-log/gnome-system-log-descript.db", LOGVIEWINSTALLPREFIX);
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			g_free (cfg->descript_db_path);
			cfg->descript_db_path = g_strdup (full_name);
			read_descript_db (full_name, &descript_db);
		}
	}


	/* Find action DB ------------------------------------------------  */
	found = FALSE;
	g_snprintf (full_name, sizeof (full_name),
		    "%s/.gnome/gnome-system-log-actions.db", g_get_home_dir ());
	DB (fprintf (stderr, "Looking for database in [%s/.gnome]\n",
		     g_get_home_dir ()));
	if (access (full_name, R_OK) == 0) {
		found = TRUE;
		read_actions_db (full_name, &actions_db);
	}

	if ( ! found && cfg->action_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-actions.db", cfg->action_db_path);
		DB (fprintf (stderr, "Looking for database in [%s]\n", cfg->action_db_path));
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			read_actions_db (full_name, &actions_db);
		}
	}


	if ( ! found) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/share/gnome-system-log/gnome-system-log-actions.db", LOGVIEWINSTALLPREFIX);
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			g_free (cfg->action_db_path);
			cfg->action_db_path = g_strdup (full_name);
			read_actions_db (full_name, &actions_db);
		}
	}

	/* If debugging then print DB */
	DB (print_db (regexp_db));
}

/* ----------------------------------------------------------------------
   NAME:          IsLeapYear
   DESCRIPTION:   Return TRUE if year is a leap year.
   ---------------------------------------------------------------------- */
int
IsLeapYear (int year)
{
   if ((1900 + year) % 4 == 0)
      return TRUE;
   else
      return FALSE;
}

void SetDefaultUserPrefs(UserPrefsStruct *prefs)
{
	/* Make defaults configurable later */
	/* Will have to save prefs. eventually too*/
	gchar *logfile = NULL;
	struct stat filestat;
	
	/* FIXME: load saved values. Should install gconf schemas file for
	** defaults 
	*/
	prefs->process_column_width = 12;
	prefs->hostname_column_width = 15;
	prefs->message_column_width = 24;
	
	logfile = gconf_client_get_string (client, "/apps/logview/logfile", NULL);
	if (logfile != NULL) {
		prefs->logfile = g_strdup (logfile);
		g_free (logfile);
	}
	else {
		/* First time running logview. Try to find the logfile */
		if (lstat("/var/adm/messages", &filestat) == 0) 
			prefs->logfile = g_strdup ("/var/adm/messages");
		else if (lstat("/var/log/messages", &filestat) == 0) 
			prefs->logfile = g_strdup ("/var/log/messages");
		else
			prefs->logfile = NULL;
	}
	
}

void SaveUserPrefs(UserPrefsStruct *prefs)
{
    gconf_client_set_string (client, "/apps/logview/logfile", prefs->logfile, NULL);
    gconf_client_set_int (client, "/apps/logview/process_column_width",
    			  prefs->process_column_width, NULL);
    gconf_client_set_int (client, "/apps/logview/hostname_column_width",
    			  prefs->hostname_column_width, NULL);
    gconf_client_set_int (client, "/apps/logview/message_column_width",
    			  prefs->message_column_width, NULL);

}

static void 
toggle_calendar (void)
{
    if (calendarvisible) {
	calendarvisible = FALSE;
	gtk_widget_hide (CalendarDialog);
    }
    else
	CalendarMenu (app, NULL);
}

static void
toggle_zoom (void)
{
    if (zoom_visible) {
	close_zoom_view (app, NULL);
    }
    else
	create_zoom_view (NULL, NULL);
}
