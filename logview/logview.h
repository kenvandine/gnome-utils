
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


#include <time.h>
#include "gtk/gtk.h"
#include <stdio.h>

#ifndef __LOGVIEW_H__
#define __LOGVIEW_H__

/* #define DEBUG 1 */

#ifdef DEBUG
#define DB(x) x
#else
#define DB(x) while (0) { ; }
#endif

#define LOG_LINESEP              15

/* FIXME: this is wrong, this needs to be recalculated all the time
 * based on the current page, all the math is utterly wrong here
 * it just sucks.  This is bug #58435 */
#define LINES_P_PAGE             10
#define NUM_PAGES                5 
#define MAX_WIDTH                240
#define MAX_HOSTNAME_WIDTH       257	/* Need authoritative answer on this value. */
#define MAX_PROC_WIDTH           60
#define NUM_LOGS                 2
#define R_BUF_SIZE               1024	/* Size of read buffer */
#define MAX_NUM_LOGS             10


#define SPARE_LINES              5
#define LOG_CANVAS_H             400
#define LOG_CANVAS_W             600
#define LOG_BOTTOM_MARGIN        200

#define LOG_WINDOW_W             LOG_CANVAS_W+15
#define LOG_WINDOW_H             LOG_CANVAS_H+LOG_BOTTOM_MARGIN


/*
 *    ,----------.
 *    | Typedefs |
 *    `----------'
 */

typedef void (*MenuCallback) (GtkWidget * widget, gpointer user_data);
typedef struct __menu_item MenuItem;

typedef struct
{
  /* Paths ----------------------------------------------------- */
  char *regexp_db_path, *descript_db_path, *action_db_path;

} ConfigData;

struct __menu_item
{
   char *name;
   char accel;
   MenuCallback callback;
   MenuItem *submenu;
};

struct __datemark
{
  time_t time;		/* date           */
  struct tm fulldate;
  long offset;		/* offset in file */
  char year;		/* to correct for logfiles with many years */
  long ln;		/* Line # from begining of file */
  struct __datemark *next, *prev;
};

typedef struct __datemark DateMark;

typedef struct
{
  time_t startdate, enddate;
  time_t mtime;
  long numlines;
  long size;
  DateMark *firstmark, *lastmark;
}
LogStats;

typedef struct {
  GtkWidget *calendar;
  DateMark *curmonthmark;
} CalendarData;

typedef struct
{
  char tag[50];
  char *regexp;
  char *description;
  int level;
} Description;

typedef struct
{
  char *regexp;
  GList *matching;
} ProcessDB;

typedef struct
{
  char *text;
  char tag[50];
} DescriptionEntry;
  
typedef struct
{
  char tag[50];
  char *log_name;
  char *process;
  char *message;
  char *description;
  char *action;
} Action;

typedef struct
{
  char message[MAX_WIDTH];
  char process[MAX_PROC_WIDTH];
  char hostname[MAX_HOSTNAME_WIDTH];
  signed char month;
  signed char date;
  signed char hour;
  signed char min;
  signed char sec;
  Description *description;
} LogLine;

typedef void (*MonActions)();

typedef struct
{
	/* Negative column width indicates don't show */
	int process_column_width;
	int hostname_column_width;
	int message_column_width;
	/* FIXME: This should perhaps be a glist of logfiles to
	** open on startup. Also one should be able to set the value in
	** the prefs dialog
	*/
	gchar *logfile;
} UserPrefsStruct;

typedef struct
{
  FILE *fp;
  DateMark *curmark;
  char name[255];
  CalendarData *caldata;
  LogStats lstats;
  gint current_line_no; /* indicates the line that is selected */
  gint total_lines; /* no of lines in the file */
  LogLine **lines; /* actual lines */
  GtkTreePath *expand_paths[32];

  /* Monitor info */
  GtkWidget *mon_lines;
  MonActions alert;
  long offset_end;
  int mon_on;		/* Flag set if we are monitoring this log          */
  int mon_numlines;
}
Log;


/*
 *    ,---------------------.
 *    | Function Prototypes |
 *    `---------------------'
 */

void StubCall (GtkWidget *, gpointer);
void ShowErrMessage (const char *msg);
void QueueErrMessages (gboolean do_queue);
void ShowQueuedErrMessages (void);
GtkWidget *AddMenu (MenuItem * items);
ConfigData *CreateConfig(void);
int repaint_zoom (void);
void MoveToMark (Log *log);
CalendarData* init_calendar_data (void);
int RepaintCalendar (GtkWidget * widget, GdkEventExpose * event);
int read_descript_db (char *filename, GList **db);
int find_tag_in_db (LogLine *line, GList *db);
int IsLeapYear (int year);
void SetDefaultUserPrefs(UserPrefsStruct *prefs);
int exec_action_in_db (Log *log, LogLine *line, GList *db);

char *LocaleToUTF8 (const char *in);

#define sure_string ((x)?(x):"")

#endif /* __LOGVIEW_H__ */
