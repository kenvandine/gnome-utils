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
#include <time.h>
#include <gnome.h>
#include "gtk/gtk.h"
#include "logview.h"

#define CALENDAR_WIDTH           180
#define CALENDAR_HEIGHT          150

#define SUNDAY                   0
#define MONDAY                   1
#define FRIDAY                   5
#define SATURDAY                 6
#define FEBRUARY                 1
#define XSEP                     3
#define CALLEFTMARGIN            5
#define BASELINESKIP             4

#define THISMONTH                1
#define OTHERMONTH               2
#define MARKEDDATE               3

/*
 *       --------
 *       Typedefs
 *       --------
 */


/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void CalendarMenu (GtkWidget *app);
void close_calendar (GtkWidget * widget, gpointer client_data);
void set_scrollbar_size (int num_lines);
void calendar_month_changed (GtkWidget *widget, gpointer data);
void read_marked_dates (CalendarData *data);
void calendar_month_changed (GtkWidget *widget, gpointer data);
void calendar_day_selected (GtkWidget *widget, gpointer unused_data);
void calendar_day_selected_double_click (GtkWidget *widget, gpointer data);
CalendarData* init_calendar_data ();
DateMark* find_prev_mark (CalendarData*);
DateMark* find_next_mark (CalendarData*);
DateMark* get_mark_from_month (CalendarData *data, gint month, gint year);
DateMark *get_mark_from_date (CalendarData *, gint, gint, gint);
void log_repaint (GtkWidget * canvas, GdkRectangle * area);

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */

extern ConfigData *cfg;
extern GdkGC *gc;
extern Log *curlog, *loglist[];
extern int numlogs, curlognum;
extern char *month[12];
extern GtkWidget *view;
extern GtkUIManager *ui_manager;
GtkWidget *CalendarDialog = NULL;
GtkWidget *CalendarWidget;
int calendarvisible;



/* ----------------------------------------------------------------------
   NAME:          CalendarMenu
   DESCRIPTION:   Display the calendar.
   ---------------------------------------------------------------------- */

void
CalendarMenu (GtkWidget *window)
{
   GtkCalendar *calendar;
   GtkWidget *vbox;

   if (curlog == NULL || calendarvisible)
      return;

   if (CalendarDialog == NULL)
   {
      CalendarDialog = gtk_dialog_new ();

      gtk_window_set_title (GTK_WINDOW (CalendarDialog), _("Calendar"));
      gtk_window_set_resizable (GTK_WINDOW (CalendarDialog), FALSE);
      gtk_dialog_set_has_separator (GTK_DIALOG(CalendarDialog), FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (CalendarDialog)), 5);

      gtk_dialog_add_button (GTK_DIALOG (CalendarDialog), 
			     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
      g_signal_connect (G_OBJECT (CalendarDialog), "response",
		        G_CALLBACK (close_calendar),
			&CalendarDialog);
      g_signal_connect (G_OBJECT (CalendarDialog), "destroy",
			G_CALLBACK (close_calendar),
			NULL);
      g_signal_connect (G_OBJECT (CalendarDialog), "delete_event",
			G_CALLBACK (gtk_true),
			NULL);

      vbox = gtk_vbox_new (FALSE, 6);

      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (CalendarDialog)->vbox), vbox, 
			  TRUE, TRUE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

      calendar = (GtkCalendar *)gtk_calendar_new();

      gtk_signal_connect (GTK_OBJECT (calendar), "month_changed",
			  GTK_SIGNAL_FUNC (calendar_month_changed),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (calendar), "day_selected",
			  GTK_SIGNAL_FUNC (calendar_day_selected),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (calendar), "day_selected_double_click",
			  GTK_SIGNAL_FUNC (calendar_day_selected_double_click),
			  NULL);
      gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (calendar), TRUE, TRUE, 0);
      gtk_widget_show (GTK_WIDGET (calendar));

      gtk_dialog_set_default_response (GTK_DIALOG (CalendarDialog), GTK_RESPONSE_CLOSE); 
   
      gtk_widget_show (vbox);
      
      CalendarWidget = GTK_WIDGET (calendar);
      init_calendar_data ();
   }
   calendarvisible = TRUE;

   gtk_widget_show (CalendarDialog);

}

/* ----------------------------------------------------------------------
   NAME:          read_marked_dates
   DESCRIPTION:   Reads all the marked dates for this month and marks 
   		  them in the calendar widget.
   ---------------------------------------------------------------------- */

void
read_marked_dates (CalendarData *data)
{
  GtkCalendar *calendar;
  DateMark *mark;
  gint day, month, year;

  g_return_if_fail (data);
  calendar = GTK_CALENDAR (CalendarWidget);
  g_return_if_fail (calendar);

  gtk_calendar_clear_marks (calendar);
  gtk_calendar_get_date (calendar, &year, &month, &day);
  year -= 1900;

  /* find the current month and year (if there is one)  */
  mark = data->curmonthmark;

  while (mark)
    {
      if (mark->fulldate.tm_mon != month || 
	  mark->fulldate.tm_year != year)
	break;
      gtk_calendar_mark_day (calendar, mark->fulldate.tm_mday);
      mark = mark->next;
    }

  return;
}

        
/* ----------------------------------------------------------------------
   NAME:          init_calendar_data
   DESCRIPTION:   Sets up calendar data asociated with this log.
   ---------------------------------------------------------------------- */

CalendarData*
init_calendar_data ()
{
   CalendarData *data;

   data = curlog->caldata;
   if (data == NULL)
     data = (CalendarData*) malloc (sizeof (CalendarData));

   if (data)
     {
       data->curmonthmark = curlog->lstats.firstmark;
       curlog->caldata = data;

#if 0
       /* Move mark to first marked date in this month */
       data->curmonthmark = 
	 get_mark_from_month (data, curlog->curmark->fulldate.tm_mon,
			      curlog->curmark->fulldate.tm_year);
#endif
       
       /* signal a redraw event */
       gtk_signal_emit_by_name (GTK_OBJECT (CalendarWidget), "month_changed");
       
     }

   return data;
}

/* ----------------------------------------------------------------------
   NAME:          calendar_month_changed
   DESCRIPTION:   User changed the month in the calendar.
   ---------------------------------------------------------------------- */

void
calendar_month_changed (GtkWidget *widget, gpointer unused_data)
{
  GtkCalendar *calendar;
  CalendarData *data;
  DateMark *mark;
  gint day, month, year;

  calendar = GTK_CALENDAR (CalendarWidget);
  g_return_if_fail (calendar);

  data = curlog->caldata;
  g_return_if_fail (data);

  /* Get current date */
  gtk_calendar_get_date (calendar, &year, &month, &day);
  /* This is Y2K compatible but internally we keep track of years
     starting from 1900. This is because of Unix and not because 
     I like it!! */
  year -= 1900;
  mark = get_mark_from_month (data, month, year);
  if (mark)
    data->curmonthmark = mark;
  read_marked_dates (data);
}


/* ----------------------------------------------------------------------
   NAME:          calendar_day_selected
   DESCRIPTION:   User clicked on a calendar entry
   ---------------------------------------------------------------------- */

void
calendar_day_selected (GtkWidget *widget, gpointer unused_data)
{
  /* find the selected day in the current logfile */
  gint day, month, year;
  GtkTreePath *path;

  gtk_calendar_get_date (GTK_CALENDAR (CalendarWidget), &year, &month, &day);

  path = g_hash_table_lookup (curlog->date_headers,
                DATEHASH (month, day));
  if (path != NULL)
    {
      gtk_tree_view_set_cursor (view, path, NULL, FALSE);
    }
}

/* ----------------------------------------------------------------------
   NAME:          calendar_day_selected
   DESCRIPTION:   User clicked on a calendar entry
   ---------------------------------------------------------------------- */

void
calendar_day_selected_double_click (GtkWidget *widget, gpointer data)
{
  calendar_day_selected(widget, data);
}

/* ----------------------------------------------------------------------
   NAME:          close_calendar
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
close_calendar (GtkWidget *widget, gpointer client_data)
{
   if (calendarvisible) {
      gtk_widget_hide (CalendarDialog);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
				      (gtk_ui_manager_get_widget(ui_manager, "/LogviewMenu/ViewMenu/ShowCalendar")),
				      FALSE);
   }
   calendarvisible = FALSE;
}


/* ----------------------------------------------------------------------
   NAME:        get_mark_from_month
   DESCRIPTION: Move curmonthmark to current month if there is a log 
                entry that month.
   ---------------------------------------------------------------------- */

DateMark *
get_mark_from_date (CalendarData *data, gint day, gint month, gint year)
{
  DateMark *mark;

  g_return_val_if_fail (data, NULL);
  mark = get_mark_from_month (data, month, year);
  
  while (mark)
    {
      if (mark->fulldate.tm_mday == day)
	return mark;
      mark = mark->next;
    }

  return mark;
}

/* ----------------------------------------------------------------------
   NAME:        get_mark_from_month
   DESCRIPTION: Move curmonthmark to current month if there is a log 
                entry that month.
   ---------------------------------------------------------------------- */

DateMark *
get_mark_from_month (CalendarData *data, gint month, gint year)
{
   DateMark *mark = data->curmonthmark;

   if (mark == NULL)
      return mark;

   /* Are we on top of the mark: return if so      */
   if (month == mark->fulldate.tm_mon &&
       year == mark->fulldate.tm_year)
      return mark;

   /* Check if we are on the next or previous mark */
   if (year > mark->fulldate.tm_year)
      mark = find_next_mark (data);
   else if (year < mark->fulldate.tm_year)
      mark = find_prev_mark (data);
   else if (year == mark->fulldate.tm_year) {
      if (month > mark->fulldate.tm_mon)
	 mark = find_next_mark (data);
      else
	 mark = find_prev_mark (data);
   }

   if (mark == NULL)
      return NULL;

   if (month == mark->fulldate.tm_mon && 
       year == mark->fulldate.tm_year)
     return mark;

   return NULL;
}

/* ----------------------------------------------------------------------
   NAME:          find_prev_mark
   DESCRIPTION:   Returns the previous mark with a different month.
   ---------------------------------------------------------------------- */

DateMark *
find_prev_mark (CalendarData *data)
{
   DateMark *mark = data->curmonthmark;

   if (mark->prev == NULL)
      return NULL;
   mark = mark->prev;
   while (TRUE)
   {
      if (mark->prev == NULL)
	 break;
      mark = mark->prev;
      if (mark->fulldate.tm_mon != mark->next->fulldate.tm_mon ||
	  mark->fulldate.tm_year != mark->next->fulldate.tm_year)
      {
	 mark = mark->next;
	 break;
      }
   }

   return mark;
}

/* ----------------------------------------------------------------------
   NAME:          find_next_mark
   DESCRIPTION:   Returns the next mark with a different month.
   ---------------------------------------------------------------------- */

DateMark *
find_next_mark (CalendarData *data)
{
   DateMark *mark = data->curmonthmark;

   while (TRUE)
   {
      if (mark->next == NULL)
	 return NULL;
      mark = mark->next;
      if (mark->fulldate.tm_mon != mark->prev->fulldate.tm_mon ||
	  mark->fulldate.tm_year != mark->prev->fulldate.tm_year)
	 break;
   }

   return mark;
}

