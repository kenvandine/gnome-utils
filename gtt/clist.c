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

/* There is a bug in clist which makes all but the last column headers
   0 pixels wide. This hack fixes this. */
#define CLIST_HEADER_HACK

#define TOTAL_COL	0
#define TIME_COL	1
#define TITLE_COL	2
#define DESC_COL	3

static void
select_row(GtkCList *clist, gint row, gint column, GdkEventButton *event)
{
	GtkWidget *menu;

	if (!event) return;
	if (event->button != 3) {
		cur_proj_set(gtk_clist_get_row_data(clist, row));
		return;
	}
	/* TODO: workaround -- should be removed when the bug in clist is fixed.
	 * possibly unselect cur_proj */
	if ((gtk_clist_get_row_data(clist, row) != cur_proj) && (cur_proj))
		gtk_clist_unselect_row(GTK_CLIST(glist), cur_proj->row, column);
	cur_proj_set(gtk_clist_get_row_data(clist, row));
	menu = menus_get_popup();
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, 0);
}



static void
unselect_row(GtkCList *clist, gint row, gint column, GdkEventButton *event)
{
	GtkWidget *menu;

	if (gtk_clist_get_row_data(clist, row) != cur_proj) return;
	if ((!event) || (event->button != 3)) {
		cur_proj_set(NULL);
		return;
	}
	/* make sure the project keeps selection */
	gtk_clist_select_row(GTK_CLIST(glist), row, column);
	menu = menus_get_popup();
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, 0);
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



GtkWidget *
create_clist(void)
{
	GtkWidget *w;
#ifdef CLIST_HEADER_HACK
	GtkStyle *style;
	GdkGCValues vals;
#endif
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
#ifdef CLIST_HEADER_HACK
	style = gtk_widget_get_style(w);
	g_return_val_if_fail(style != NULL, NULL);
	gdk_gc_get_values(style->fg_gc[0], &vals);
	{
	    int wid = gdk_string_width(vals.font, "00:00:00");

	    gtk_clist_set_column_width(GTK_CLIST(w), TOTAL_COL, wid);
	    gtk_clist_set_column_width(GTK_CLIST(w), TIME_COL,	wid);
	}
	gtk_clist_set_column_width(GTK_CLIST(w), TITLE_COL, 120);
#endif
	gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_justification(GTK_CLIST(w), TOTAL_COL, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(w), TIME_COL,  GTK_JUSTIFY_CENTER);
	gtk_clist_column_titles_active(GTK_CLIST(w));
	gtk_clist_set_policy(GTK_CLIST(w),
			     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
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
	project_list *pl;
	int timer_running, cp_found = 0;

	timer_running = (main_timer != 0);
	stop_timer();
	if (plist) {
		gtk_clist_freeze(GTK_CLIST(glist));
		gtk_clist_clear(GTK_CLIST(glist));
		for (pl = plist; pl; pl = pl->next) {
			clist_add(pl->proj);
			if (pl->proj == cur_proj) cp_found = 1;
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
	gtk_widget_show(window);
	if (timer_running) start_timer();
	menu_set_states();
	if (config_show_clist_titles)
		gtk_clist_column_titles_show(GTK_CLIST(glist));
	else
		gtk_clist_column_titles_hide(GTK_CLIST(glist));
}



void
clist_add(project *p)
{
	char *tmp[4];

	tmp[TOTAL_COL] = project_get_total_timestr(p, config_show_secs);
	tmp[TIME_COL]  = project_get_timestr(p, config_show_secs);
	tmp[TITLE_COL] = p->title;
	tmp[DESC_COL]  = p->desc;
	p->row = gtk_clist_append(GTK_CLIST(glist), tmp);
	gtk_clist_set_row_data(GTK_CLIST(glist), p->row, p);
}



void
clist_insert(project *p, gint pos)
{
	project_list *pl;
	char *tmp[4];

	tmp[TOTAL_COL] = project_get_total_timestr(p, config_show_secs);
	tmp[TIME_COL]  = project_get_timestr(p, config_show_secs);
	tmp[TITLE_COL] = p->title;
	tmp[DESC_COL]  = p->desc;
	gtk_clist_insert(GTK_CLIST(glist), pos, tmp);
	gtk_clist_set_row_data(GTK_CLIST(glist), pos, p);
	for (pl = plist; pl; pl = pl->next) {
		if (pl->proj->row >= pos)
			pl->proj->row++;
	}
	p->row = pos;
}



void
clist_remove(project *p)
{
	project_list *pl;

	gtk_clist_remove(GTK_CLIST(glist), p->row);
	for (pl = plist; pl; pl = pl->next) {
		if (pl->proj->row >= p->row)
			pl->proj->row--;
	}
	p->row = -1;
}



void
clist_update_label(project *p)
{
	g_return_if_fail(p->row != -1);
	gtk_clist_set_text(GTK_CLIST(glist), p->row, TOTAL_COL,
			   project_get_total_timestr(p, config_show_secs));
	gtk_clist_set_text(GTK_CLIST(glist), p->row, TIME_COL,
			   project_get_timestr(p, config_show_secs));
	update_status_bar();
}



void
clist_update_title(project *p)
{
	g_return_if_fail(p->row != -1);
	gtk_clist_set_text(GTK_CLIST(glist), p->row, TITLE_COL,
			   p->title);
}


void
clist_update_desc(project *p)
{
	g_return_if_fail(p->row != -1);
	gtk_clist_set_text(GTK_CLIST(glist), p->row, DESC_COL,
			   p->desc);
}


