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
#include <gtk/gtk.h>

#include "gtt-features.h"
#ifdef GTK_USE_CLIST
#include <libintl.h>

#include "gtt.h"



static void
select_row(GtkCList *clist, gint row, gint column, GdkEventButton *event)
{
        GtkWidget *menu;

        if (!event) return;
        if (event->button != 3) {
                if (cur_proj) {
                        if (cur_proj->row == row) {
                                cur_proj_set(NULL);
                                return;
                        }
                }
                cur_proj_set(gtk_clist_get_row_data(clist, row));
                return;
        }
        /* make sure, project keeps selection */
        if (gtk_clist_get_row_data(clist, row) == cur_proj)
                gtk_clist_select_row(GTK_CLIST(glist), row, column);
        cur_proj_set(gtk_clist_get_row_data(clist, row));
        get_menubar(&menu, NULL, MENU_POPUP);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, 0);
}


/* TODO: maybe I can skip this alltogether - have to test it a bit */
#undef NEED_UNSEL

#ifdef NEED_UNSEL
static void
unselect_row(GtkCList *clist, gint row, gint column, GdkEventButton *event)
{
#if 0
        g_print("unselect_row: row=%d, col=%d, button=%d\n",
                row, column, (event)?event->button:-1);
#endif
        if (gtk_clist_get_row_data(clist, row) != cur_proj) return;
        /* g_return_if_fail(cur_proj == NULL); */
        cur_proj_set(NULL);
}
#endif /* NEED_UNSEL */



static void
click_column(GtkCList *clist, gint col)
{
        if (col == 0)
                project_list_sort_time();
        else
                project_list_sort_title();
        setup_clist();
}



GtkWidget *
create_clist(void)
{
        GtkWidget *w;
        char *titles[2] = {
                "00:00:00", /* to make room */
                N_("Project Title")
        };

        titles[1] = _(titles[1]);
        w = gtk_clist_new_with_titles(2, titles);
	gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_SINGLE);
        gtk_clist_set_column_title(GTK_CLIST(w), 0, _("Time"));
        gtk_clist_set_column_justification(GTK_CLIST(w), 0, GTK_JUSTIFY_CENTER);
        gtk_clist_column_titles_active(GTK_CLIST(w));
        gtk_clist_set_policy(GTK_CLIST(w),
                             GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_signal_connect(GTK_OBJECT(w), "select_row",
                           GTK_SIGNAL_FUNC(select_row), NULL);
        gtk_signal_connect(GTK_OBJECT(w), "click_column",
                           GTK_SIGNAL_FUNC(click_column), NULL);
#ifdef NEED_UNSEL
        gtk_signal_connect(GTK_OBJECT(w), "unselect_row",
                           GTK_SIGNAL_FUNC(unselect_row), NULL);
#endif /* NEED_UNSEL */
        return w;
}



void
setup_clist(void)
{
	project_list *pl;
	int timer_running, cp_found = 0;

	timer_running = (main_timer != 0);
	stop_timer();
        gtk_clist_freeze(GTK_CLIST(glist));
        gtk_clist_clear(GTK_CLIST(glist));
	if (!plist) {
		/* define one empty project on the first start */
		project_list_add(project_new_title(_("empty")));
	}

	for (pl = plist; pl; pl = pl->next) {
		clist_add(pl->proj);
		if (pl->proj == cur_proj) cp_found = 1;
	}

        gtk_clist_thaw(GTK_CLIST(glist));
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
        char *tmp[2];

        tmp[0] = project_get_timestr(p, config_show_secs);
        tmp[1] = p->title;
        p->row = gtk_clist_append(GTK_CLIST(glist), tmp);
        gtk_clist_set_row_data(GTK_CLIST(glist), p->row, p);
}



void
clist_insert(project *p, gint pos)
{
        project_list *pl;
        char *tmp[2];

        tmp[0] = project_get_timestr(p, config_show_secs);
        tmp[1] = p->title;
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
        gtk_clist_set_text(GTK_CLIST(glist), p->row, 0,
                           project_get_timestr(p, config_show_secs));
        update_status_bar();
}



void
clist_update_title(project *p)
{
        g_return_if_fail(p->row != -1);
        gtk_clist_set_text(GTK_CLIST(glist), p->row, 1,
                           p->title);
}



#endif /* GTK_USE_CLIST */

