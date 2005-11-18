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
#include <gtk/gtk.h>
#include <stdlib.h>
#include "logview.h"
#include "calendar.h"

static GObjectClass *parent_class;
GType calendar_get_type (void);

struct CalendarPriv
{
    GList *days;
	gboolean first_pass;
};

static void
calendar_mark_dates (Calendar *calendar)
{
    GList *days;
    Day *day;
    guint month, year;

    g_assert (IS_CALENDAR (calendar));
    
    gtk_calendar_clear_marks (GTK_CALENDAR (calendar));
    gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, NULL);

    for (days = calendar->priv->days; days != NULL; days = g_list_next(days)) {
        day = days->data;
        if (month == g_date_get_month (day->date)-1)
            gtk_calendar_mark_day (GTK_CALENDAR(calendar), g_date_get_day (day->date));
    }
}

        
/* 
   init_calendar_data
   Sets up calendar data asociated with this log.
*/

void
calendar_init_data (Calendar *calendar, LogviewWindow *logview)
{
    g_return_if_fail (IS_CALENDAR (calendar));
	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

	g_object_get (G_OBJECT (logview), "days", &(calendar->priv->days), NULL);
    calendar->priv->first_pass = TRUE;

    calendar_mark_dates (calendar);
}

static void
calendar_month_changed (GtkWidget *widget, gpointer data)
{
    g_assert (IS_CALENDAR (widget));
    calendar_mark_dates (CALENDAR (widget));
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

static void
calendar_day_selected (GtkWidget *widget, LogviewWindow *logview)
{
    Calendar *calendar;
    guint day, month, year;
    Day *found_day;
    GtkTreePath *path;

    calendar = CALENDAR (widget);

    if (logview->curlog == NULL)
      return;

    if (calendar->priv->first_pass == TRUE) {
        calendar->priv->first_pass = FALSE;
        return;
    }
  
    gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, &day);    
    found_day = log_find_day (logview->curlog, day, month, year);
    if (found_day != NULL) {
        path = found_day->path;
        if ((gtk_tree_path_compare (path, logview->curlog->selected_range.first) != 0) &&
            (!gtk_tree_path_is_descendant (logview->curlog->selected_range.first, path)))
            gtk_tree_view_set_cursor (GTK_TREE_VIEW(logview->view), path, NULL, FALSE);
    }
}

void
calendar_select_date (Calendar *calendar, GDate *date)
{
    guint y, m, d;

    gtk_calendar_get_date (GTK_CALENDAR (calendar), &y, &m, &d);
    m++;

    if (g_date_get_day (date) == d && g_date_get_month (date) == m &&
        g_date_get_year (date) == y)
        return;
    calendar->priv->first_pass = TRUE;

    if (g_date_get_year (date) != y || g_date_get_month (date) != m)
        gtk_calendar_select_month (GTK_CALENDAR (calendar), g_date_get_month(date)-1, 
                                   g_date_get_year (date));
    if (g_date_get_day (date) != d)
        gtk_calendar_select_day (GTK_CALENDAR (calendar), g_date_get_day(date));
}

void
calendar_connect (Calendar *calendar, LogviewWindow *logview)
{
    g_return_if_fail (IS_CALENDAR (calendar));
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    g_signal_connect (G_OBJECT (calendar), "month_changed",
                      G_CALLBACK (calendar_month_changed),
                      logview);
    g_signal_connect (G_OBJECT (calendar), "day_selected",
                      G_CALLBACK (calendar_day_selected),
                      logview);
}

static void 
calendar_init (GtkCalendar *widget)
{
    Calendar *calendar = CALENDAR (widget);
    PangoFontDescription *fontdesc;
    PangoContext *context;
    int size;

    g_assert (IS_CALENDAR (calendar));

    calendar->priv = g_new0 (CalendarPriv, 1);
    
    context = gtk_widget_get_pango_context (GTK_WIDGET(widget));
    fontdesc = pango_context_get_font_description (context);
    size = pango_font_description_get_size (fontdesc) / PANGO_SCALE;
    pango_font_description_set_size (fontdesc, (size-2)*PANGO_SCALE);
    gtk_widget_modify_font (GTK_WIDGET (widget), fontdesc);
}

static void
calendar_class_finalize (GObject *object)
{
    Calendar *calendar = CALENDAR (object);
    g_free (calendar->priv);
	parent_class->finalize (object);
}

static void
calendar_class_init (CalendarClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
    object_class->finalize = calendar_class_finalize;
	parent_class = g_type_class_peek_parent (klass);
}

GType
calendar_get_type (void)
{
	static GType object_type = 0;
	
	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (CalendarClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) calendar_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (Calendar),
			0,              /* n_preallocs */
			(GInstanceInitFunc) calendar_init
		};

		object_type = g_type_register_static (GTK_TYPE_CALENDAR, "Calendar", &object_info, 0);
	}

	return object_type;
}

GtkWidget *
calendar_new (void)
{
    GtkWidget *widget;
    widget = g_object_new (CALENDAR_TYPE, NULL);
    return widget;
}
