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

#include <glib/gi18n.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "logview.h"
#include "calendar.h"

static GtkTreePath *calendar_day_selected (GtkWidget *widget, LogviewWindow *window);
static void calendar_day_selected_double_click (GtkWidget *widget, LogviewWindow *window);
static void close_calendar (GtkWidget * widget, int arg, gpointer client_data);
static DateMark* find_prev_mark (CalendarData*);
static DateMark* find_next_mark (CalendarData*);
static DateMark* get_mark_from_month (CalendarData *data, gint month, gint year);
static DateMark *get_mark_from_date (CalendarData *, gint, gint, gint);
static void calendar_month_changed (GtkWidget *widget, LogviewWindow *window);

/* ----------------------------------------------------------------------
   NAME:          CalendarMenu
   DESCRIPTION:   Display the calendar.
   ---------------------------------------------------------------------- */

void
CalendarMenu (LogviewWindow *window)
{
	PangoFontDescription *fontdesc;
	PangoContext *context;
	GtkCalendar *calendar;
	int size;
	
	calendar = (GtkCalendar *)gtk_calendar_new();
	
	g_signal_connect (G_OBJECT (calendar), "month_changed",
										G_CALLBACK (calendar_month_changed),
										window);
	g_signal_connect (G_OBJECT (calendar), "day_selected",
										G_CALLBACK (calendar_day_selected),
										window);
	g_signal_connect (G_OBJECT (calendar), "day_selected_double_click",
										G_CALLBACK (calendar_day_selected_double_click),
										window);
	window->calendar = GTK_WIDGET (calendar);
	window->calendar_visible = TRUE;

	context = gtk_widget_get_pango_context (GTK_WIDGET(calendar));
	fontdesc = pango_context_get_font_description (context);
	size = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
	pango_font_description_set_size (fontdesc, (size-2)*PANGO_SCALE);
	gtk_widget_modify_font (GTK_WIDGET (calendar), fontdesc);

	gtk_widget_show (GTK_WIDGET (calendar));
}

/* ----------------------------------------------------------------------
   NAME:          read_marked_dates
   DESCRIPTION:   Reads all the marked dates for this month and marks 
   		  them in the calendar widget.
   ---------------------------------------------------------------------- */

void
read_marked_dates (CalendarData *data, LogviewWindow *window)
{
  GtkCalendar *calendar;
  DateMark *mark;
  gint day, month, year;

  g_return_if_fail (data);
  calendar = GTK_CALENDAR (window->calendar);
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
init_calendar_data (LogviewWindow *window)
{
   CalendarData *data;
	 
	 if (window->curlog == NULL)
		 return;

   data = window->curlog->caldata;
   if (data == NULL)
     data = (CalendarData*) malloc (sizeof (CalendarData));

   if (data) {
		 DateMark *mark;
		 data->curmonthmark = window->curlog->lstats.firstmark;
		 window->curlog->caldata = data;
		 
		 mark = data->curmonthmark;
		 if (mark) {
			 gtk_calendar_select_month (GTK_CALENDAR(window->calendar), 
																	mark->fulldate.tm_mon,
																	mark->fulldate.tm_year + 1900);
			 gtk_calendar_select_day (GTK_CALENDAR(window->calendar), 
																mark->fulldate.tm_mday);
		 }
		 
		 read_marked_dates (data, window);       
	 }

   return data;
}

/* ----------------------------------------------------------------------
   NAME:          calendar_month_changed
   DESCRIPTION:   User changed the month in the calendar.
   ---------------------------------------------------------------------- */

static void
calendar_month_changed (GtkWidget *widget, LogviewWindow *window)
{
  GtkCalendar *calendar;
  CalendarData *data;
  DateMark *mark;
  gint day, month, year;

	if (window->curlog == NULL)
		return;

  calendar = GTK_CALENDAR (window->calendar);
  g_return_if_fail (calendar);

  data = window->curlog->caldata;
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
  read_marked_dates (data, window);
}


/* ----------------------------------------------------------------------
   NAME:          calendar_day_selected
   DESCRIPTION:   User clicked on a calendar entry
   ---------------------------------------------------------------------- */

static GtkTreePath *
calendar_day_selected (GtkWidget *widget, LogviewWindow *window)
{
  /* find the selected day in the current logfile */
  gint day, month, year;
  GtkTreePath *path;

	if (window->curlog == NULL)
		return NULL;

  gtk_calendar_get_date (GTK_CALENDAR (window->calendar), &year, &month, &day);

  path = g_hash_table_lookup (window->curlog->date_headers,
                DATEHASH (month, day));
  if (path != NULL)
		gtk_tree_view_set_cursor (GTK_TREE_VIEW(window->view), path, NULL, FALSE);

	return path;
}

/* ----------------------------------------------------------------------
   NAME:          calendar_day_selected
   DESCRIPTION:   User clicked on a calendar entry
   ---------------------------------------------------------------------- */

void
calendar_day_selected_double_click (GtkWidget *widget, LogviewWindow *window)
{
	GtkTreePath *path;
  path = calendar_day_selected(widget, window);
	if (path)
		gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view), path, FALSE);
}

/* ----------------------------------------------------------------------
   NAME:          close_calendar
   DESCRIPTION:   Callback called when the log info window is closed.
   ---------------------------------------------------------------------- */

void
close_calendar (GtkWidget *widget, int arg, gpointer data)
{
   LogviewWindow *window = LOGVIEW_WINDOW (data);
   if (window->calendar_visible) {
	   GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/ViewMenu/ShowCalendar");
	   gtk_action_activate (action);
   }
   window->calendar_visible = FALSE;
}


/* ----------------------------------------------------------------------
   NAME:        get_mark_from_month
   DESCRIPTION: Move curmonthmark to current month if there is a log 
                entry that month.
   ---------------------------------------------------------------------- */

static DateMark *
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

static DateMark *
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

static DateMark *
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

static DateMark *
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

