/*   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <gnome.h>

#include "gtt.h"
#include "menucmd.h"
#include "proj_p.h"

/* There is a bug in clist which makes all but the last column headers
 * 0 pixels wide. This hack fixes this. */
#define CLIST_HEADER_HACK

#define TOTAL_COL	0
#define TIME_COL	1
#define TITLE_COL	2
#define DESC_COL	3

int clist_header_width_set = 0;

static int
clist_event(GtkCList *clist, GdkEvent *event, gpointer data)
{
	int row,column;
	GdkEventButton *bevent = (GdkEventButton *)event;
	GtkWidget *menu;
	
	if (!((event->type == GDK_2BUTTON_PRESS && bevent->button==1) ||
	      (event->type == GDK_BUTTON_PRESS && bevent->button==3)))
		return FALSE;

	gtk_clist_get_selection_info(clist,bevent->x,bevent->y,&row,&column);
	if(row<0)
		return FALSE;
	
	gtk_clist_select_row(clist,row,column);
	if(!cur_proj)
		return FALSE;

	if (event->type == GDK_2BUTTON_PRESS) {
		prop_dialog(cur_proj);
		/*hmmm so that the event selects it ... weird*/
		gtk_clist_unselect_row(clist,row,column);
	} else {
		menu = menus_get_popup();
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	}
	return TRUE;
}
	

static void
select_row(GtkCList *clist, gint row, gint column, GdkEventButton *event)
{
#if 0
	GtkWidget *menu;

	if (!event) return;
	if (event->button != 3) {
#endif
		cur_proj_set(gtk_clist_get_row_data(clist, row));
#if 0
		if ((event->type == GDK_2BUTTON_PRESS) &&
		    (cur_proj)) {
			prop_dialog(cur_proj);
			return;
		}
		return;
	}
	/* TODO: workaround -- should be removed when the bug in clist is fixed.
	 * possibly unselect cur_proj */
	if ((gtk_clist_get_row_data(clist, row) != cur_proj) && (cur_proj))
		gtk_clist_unselect_row(GTK_CLIST(glist), cur_proj->row, column);
	cur_proj_set(gtk_clist_get_row_data(clist, row));
	menu = menus_get_popup();
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, event->time);
#endif
}



static void
unselect_row(GtkCList *clist, gint row, gint column, GdkEventButton *event)
{
	GtkWidget *menu;

	if (gtk_clist_get_row_data(clist, row) != cur_proj) return;
	if (event) {
		if (event->button == 3) {
			/* make sure the project keeps selection */
			gtk_clist_select_row(GTK_CLIST(glist), row, column);
			menu = menus_get_popup();
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
				       3, event->time);
			return;
		}
		if (event->type == GDK_2BUTTON_PRESS) {
			cur_proj_set(gtk_clist_get_row_data(clist, row));
			gtk_clist_select_row(GTK_CLIST(glist), cur_proj->row,
					     column);
			prop_dialog(cur_proj);
			return;
		}
	}
	cur_proj_set(NULL);
}



static void
click_column(GtkCList *clist, gint col)
{
	if (col == TOTAL_COL)
		project_list_sort_total_time();
	else if (col == TIME_COL)
		project_list_sort_time();
	else if (col == TITLE_COL)
		project_list_sort_title();
	else if (col == DESC_COL)
		project_list_sort_desc();
	setup_clist();
}

#ifdef CLIST_HEADER_HACK
static void
clist_header_hack(GtkWidget *window, GtkWidget *w)
{
	GtkStyle *style;
	GdkGCValues vals;
	int width;

	if (clist_header_width_set) return;
	clist_header_width_set = 1;
	style = gtk_widget_get_style(w);
	g_return_if_fail(style != NULL);
	if (!style->fg_gc[0]) {
		/* Fallback if the GC still isn't available */
		width = 50;
	} else {
		gdk_gc_get_values(style->fg_gc[0], &vals);
		width = gdk_string_width(vals.font, "00:00:00");
	}
	gtk_clist_set_column_width(GTK_CLIST(w), TOTAL_COL, width);
	gtk_clist_set_column_width(GTK_CLIST(w), TIME_COL, width);
	gtk_clist_set_column_width(GTK_CLIST(w), TITLE_COL, 120);
}
#endif /* CLIST_HEADER_HACK */


GtkWidget *
create_clist(void)
{
	GtkWidget *w, *sw;
	char *titles[4] = {
		N_("Total"),
		N_("Today"),
		N_("Project Title"),
		N_("Description")
	};
	char *tmp[4];

	tmp[TOTAL_COL] = _(titles[0]);
	tmp[TIME_COL]  = _(titles[1]);
	tmp[TITLE_COL] = _(titles[2]);
	tmp[DESC_COL]  = _(titles[3]);
	w = gtk_clist_new_with_titles(4, tmp);

	gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_justification(GTK_CLIST(w), TOTAL_COL, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(w), TIME_COL,  GTK_JUSTIFY_CENTER);
	gtk_clist_column_titles_active(GTK_CLIST(w));

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (sw), w);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (sw),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show_all (sw);

	gtk_signal_connect(GTK_OBJECT(w), "event",
			   GTK_SIGNAL_FUNC(clist_event), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "select_row",
			   GTK_SIGNAL_FUNC(select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "click_column",
			   GTK_SIGNAL_FUNC(click_column), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "unselect_row",
			   GTK_SIGNAL_FUNC(unselect_row), NULL);
	return w;
}



void
setup_clist(void)
{
	GList *node, *plist;
	int timer_running, cp_found = 0;

	timer_running = (main_timer != 0);
	stop_timer();

	plist = gtt_get_project_list();
	if (plist) {
		gtk_clist_freeze(GTK_CLIST(glist));
		gtk_clist_clear(GTK_CLIST(glist));
		for (node = plist; node; node = node->next) 
		{
			GttProject *prj = node->data;
			clist_add(prj);
			if (prj == cur_proj) cp_found = 1;
		}
		gtk_clist_thaw(GTK_CLIST(glist));
	} else {
		gtk_clist_clear(GTK_CLIST(glist));
	}
	if (!cp_found) {
		cur_proj_set(NULL);
	} else if (cur_proj) {
		gtk_clist_select_row(GTK_CLIST(glist), cur_proj->row, 0);
	}
	err_init();
	if (!GTK_WIDGET_MAPPED(window)) {
		gtk_widget_show(window);
#ifdef CLIST_HEADER_HACK
		clist_header_hack(window, glist);
#endif /* CLIST_HEADER_HACK */
	}
	if (timer_running) start_timer();
	menu_set_states();
	if (config_show_clist_titles)
		gtk_clist_column_titles_show(GTK_CLIST(glist));
	else
		gtk_clist_column_titles_hide(GTK_CLIST(glist));
	if (cur_proj) {
		gtk_clist_moveto(GTK_CLIST(glist), cur_proj->row, -1,
				 0.5, 0.0);
	}
	gtk_widget_queue_draw(GTK_WIDGET(glist));
}



void
clist_add(GttProject *p)
{
	char *tmp[4];

	tmp[TOTAL_COL] = project_get_total_timestr(p, config_show_secs);
	tmp[TIME_COL]  = project_get_timestr(p, config_show_secs);
	tmp[TITLE_COL] = (char *) gtt_project_get_title(p);
	tmp[DESC_COL]  = (char *) gtt_project_get_desc(p);
	p->row = gtk_clist_append(GTK_CLIST(glist), tmp);
	gtk_clist_set_row_data(GTK_CLIST(glist), p->row, p);
}



void
clist_insert(GttProject *p, gint pos)
{
	GList *node;
	char *tmp[4];

	tmp[TOTAL_COL] = project_get_total_timestr(p, config_show_secs);
	tmp[TIME_COL]  = project_get_timestr(p, config_show_secs);
	tmp[TITLE_COL] = (char *) gtt_project_get_title(p);
	tmp[DESC_COL]  = (char *) gtt_project_get_desc(p);
	gtk_clist_insert(GTK_CLIST(glist), pos, tmp);
	gtk_clist_set_row_data(GTK_CLIST(glist), pos, p);
	for (node = gtt_get_project_list(); node; node = node->next) 
	{
		GttProject *prj = node->data;
		if (prj->row >= pos) prj->row++;
	}
	p->row = pos;
}



void
clist_remove(GttProject *p)
{
	GList *node;

	gtk_clist_remove(GTK_CLIST(glist), p->row);
	for (node = gtt_get_project_list(); node; node = node->next) 
	{
		GttProject *prj = node->data;
		if (prj->row >= p->row) prj->row--;
	}
	p->row = -1;
}



void
clist_update_label(GttProject *p)
{
	g_return_if_fail(p->row != -1);
	gtk_clist_set_text(GTK_CLIST(glist), p->row, TOTAL_COL,
			   project_get_total_timestr(p, config_show_secs));
	gtk_clist_set_text(GTK_CLIST(glist), p->row, TIME_COL,
			   project_get_timestr(p, config_show_secs));
	update_status_bar();
}



void
clist_update_title(GttProject *p)
{
	g_return_if_fail(p->row != -1);
	gtk_clist_set_text(GTK_CLIST(glist), p->row, TITLE_COL,
			   gtt_project_get_title(p));
}


void
clist_update_desc(GttProject *p)
{
	g_return_if_fail(p->row != -1);
	gtk_clist_set_text(GTK_CLIST(glist), p->row, DESC_COL,
			   p->desc);
}


