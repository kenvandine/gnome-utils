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
#include <gtk/gtk.h>
#include "logview.h"

static GtkTreePath *calendar_day_selected (GtkWidget *widget, LogviewWindow *window);
static void calendar_day_selected_double_click (GtkWidget *widget, LogviewWindow *window);
static void calendar_month_changed (GtkWidget *widget, LogviewWindow *window);

GtkWidget *
calendar_new (void)
{
	PangoFontDescription *fontdesc;
	PangoContext *context;
	GtkCalendar *calendar;
	int size;
	
	calendar = (GtkCalendar *)gtk_calendar_new();
	context = gtk_widget_get_pango_context (GTK_WIDGET(calendar));
	fontdesc = pango_context_get_font_description (context);
	size = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
	pango_font_description_set_size (fontdesc, (size-2)*PANGO_SCALE);
	gtk_widget_modify_font (GTK_WIDGET (calendar), fontdesc);

	gtk_widget_show (GTK_WIDGET (calendar));
    return (GTK_WIDGET(calendar));
}

void 
calendar_init (GtkCalendar *calendar, LogviewWindow *window)
{
	g_signal_connect (G_OBJECT (calendar), "month_changed",
                      G_CALLBACK (calendar_month_changed),
                      window);
	g_signal_connect (G_OBJECT (calendar), "day_selected",
                      G_CALLBACK (calendar_day_selected),
                      window);
	g_signal_connect (G_OBJECT (calendar), "day_selected_double_click",
                      G_CALLBACK (calendar_day_selected_double_click),
                      window);
}

/* ----------------------------------------------------------------------
   NAME:          read_marked_dates
   DESCRIPTION:   Reads all the marked dates for this month and marks 
   		  them in the calendar widget.
   ---------------------------------------------------------------------- */

void
read_marked_dates (CalendarData *cdata, LogviewWindow *window)
{
    GtkCalendar *calendar;
    GList *days;
    Day *day;
    guint month, year;
    
    g_return_if_fail (cdata);
    calendar = GTK_CALENDAR (window->calendar);
    g_return_if_fail (GTK_IS_CALENDAR (calendar));
    
    gtk_calendar_clear_marks (calendar);
    gtk_calendar_get_date (calendar, &year, &month, NULL);

    /* find the current month and year (if there is one)  */
    days = cdata->days;
    while (days != NULL) {
        day = days->data;
        if (month == g_date_get_month (day->date)-1)
            gtk_calendar_mark_day (calendar, g_date_get_day (day->date));
        days = g_list_next (days);
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
        return NULL;
    
    data = window->curlog->caldata;
    if (data == NULL)
        data = (CalendarData*) g_new (CalendarData, 1);
    
    if (data) {
        GList *days;
        Day *day;
        days = window->curlog->days;
        data->days = days;
        day = days->data;
        window->curlog->caldata = data;
        data->first_pass = TRUE;
        
        if (day) {
            gtk_calendar_select_month (GTK_CALENDAR(window->calendar), 
                                       g_date_get_month (day->date)-1,
                                       g_date_get_year (day->date));
            gtk_calendar_select_day (GTK_CALENDAR(window->calendar), 
                                     g_date_get_day (day->date));
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
  Day *day;
  guint iday, month, year;

	if (window->curlog == NULL)
		return;

  calendar = GTK_CALENDAR (window->calendar);
  g_return_if_fail (calendar);

  data = window->curlog->caldata;
  g_return_if_fail (data);

  read_marked_dates (data, window);
}

static Day *
log_find_day (Log *log, int d, int m, int y)
{
    GDate *date;
    GList *days;
    Day *day, *found_day = NULL;
    
    date = g_date_new_dmy (d, m+1, y);
    for (days = log->days; days!=NULL; days=g_list_next(days)) {
        day = days->data;
        if (g_date_compare (day->date, date) == 0) {
            found_day = day;
        }
    }
    return found_day;
}

static GtkTreePath *
calendar_day_selected (GtkWidget *widget, LogviewWindow *window)
{
    guint day, month, year;
    Day *found_day;
    GtkTreePath *path;
    gchar *path_string;
    
	if (window->curlog == NULL)
		return NULL;

    if (window->curlog->caldata->first_pass == TRUE) {
        window->curlog->caldata->first_pass = FALSE;
        return NULL;
    }
  
    gtk_calendar_get_date (GTK_CALENDAR (window->calendar), &year, &month, &day);    
    found_day = log_find_day (window->curlog, day, month, year);
    if (found_day != NULL) {
        path = found_day->path;
        gtk_tree_view_set_cursor (GTK_TREE_VIEW(window->view), path, NULL, FALSE);
        return path;
    } else
        return NULL;
}

void
calendar_day_selected_double_click (GtkWidget *widget, LogviewWindow *window)
{
	GtkTreePath *path;
    path = calendar_day_selected(widget, window);
	if (path)
		gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view), path, FALSE);
}
