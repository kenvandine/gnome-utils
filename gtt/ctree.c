/*   GtkCTree display of projects for the GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001 Linas Vepstas
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

#include "ctree.h"
#include "gtt.h"
#include "menucmd.h"
#include "proj_p.h"
#include "props-proj.h"
#include "util.h"

/* There is a bug in clist which makes all but the last column headers
 * 0 pixels wide. This hack fixes this. */
#define CLIST_HEADER_HACK

#define TOTAL_COL	0
#define TIME_COL	1
#define TITLE_COL	2
#define DESC_COL	3

int clist_header_width_set = 0;

static void cupdate_label(GttProject *p, gboolean expand);

/* ============================================================== */

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
	if (0 > row) return FALSE;
	
	gtk_clist_select_row(clist,row,column);
	if(!cur_proj)
		return FALSE;

	if (event->type == GDK_2BUTTON_PRESS) 
	{
		prop_dialog_show(cur_proj);
		/* hmmm so that the event selects it ... weird*/
		gtk_clist_unselect_row(clist,row,column);

		/* Hmm it would be nice to be able to double-click to edit 
		 * a project, without changing the current project.  But 
		 * no simple way to do this that I can see right now ... */
	} 
	else 
	{
		menu = menus_get_popup();
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	}
	return TRUE;
}


static void
tree_select_row(GtkCTree *ctree, GtkCTreeNode* row, gint column)
{
	cur_proj_set(gtk_ctree_node_get_row_data(ctree, row));
}



static void
tree_unselect_row(GtkCTree *ctree, GtkCTreeNode* row, gint column)
{
	if (gtk_ctree_node_get_row_data(ctree, row) != cur_proj) return;
	cur_proj_set(NULL);
}


static void 
tree_expand (GtkCTree *ctree, GtkCTreeNode *row)
{
	cupdate_label(gtk_ctree_node_get_row_data(ctree, row), TRUE);
}

static void 
tree_collapse (GtkCTree *ctree, GtkCTreeNode *row)
{
	cupdate_label(gtk_ctree_node_get_row_data(ctree, row), FALSE);
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
	setup_ctree();
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

/* ============================================================== */
/* Attempt to change pixmaps to indicate whether dragged project
 * will be a peer, or a child, of the project its dragged onto.
 * Right now, this code ain't pretty; there must be a better way ...  
 */
static GtkWidget *parent_pixmap = NULL;
static GtkWidget *sibling_pixmap = NULL;
static GdkDragContext *drag_context = NULL;

static GtkCTreeNode *source_ctree_node = NULL;
static GtkCTreeNode *parent_ctree_node = NULL;
static GtkCTreeNode *sibling_ctree_node = NULL;

/* we need to drag context to flip the pixmaps */
static void
drag_begin (GtkWidget *widget, GdkDragContext *context)
{
	drag_context = context;
}

static void 
drag_drop (GtkWidget *widget, GdkDragContext *context,
           gint x, gint y, guint time)
{
	GttProject *src_prj, *par_prj=NULL, *sib_prj=NULL;
	GtkCTree *ctree = GTK_CTREE(widget);

	if (!source_ctree_node) return;

	/* sanity check. We expect a source node, and either 
	 * a parent, or a sibling */
	src_prj = gtk_ctree_node_get_row_data(ctree, source_ctree_node);
	if (!src_prj) return;

	if (parent_ctree_node)
		par_prj = gtk_ctree_node_get_row_data(ctree, parent_ctree_node);
	if (sibling_ctree_node)
		sib_prj = gtk_ctree_node_get_row_data(ctree, sibling_ctree_node);
	if (!par_prj && !sib_prj) return;

	if (sib_prj)
	{
		gtt_project_insert_before (src_prj, sib_prj);

		/* if collapsed wwe need to update the time */
		ctree_update_label (src_prj->parent);
	}
	else if (par_prj)
	{
		gtt_project_append_project (par_prj, src_prj);
		ctree_update_label (par_prj);
	}

	source_ctree_node = NULL;
	parent_ctree_node = NULL;
	sibling_ctree_node = NULL;
}

static gboolean 
ctree_drag (GtkCTree *ctree, GtkCTreeNode *source_node, 
                             GtkCTreeNode *new_parent,
                             GtkCTreeNode *new_sibling)
{
	if (!source_node) return FALSE;

	if (!parent_pixmap->window) 
	{
		gtk_widget_realize (parent_pixmap);
		gtk_widget_realize (sibling_pixmap);
	}

	/* Record the values. We don't actually reparent anything
	 * until the drag has completed. */
	source_ctree_node = source_node;
	parent_ctree_node = new_parent;
	sibling_ctree_node = new_sibling;

#if 0
{
printf ("draging %s\n", gtt_project_get_desc(
	gtk_ctree_node_get_row_data(ctree, source_ctree_node)));
printf ("to parent %s\n", gtt_project_get_desc(
	gtk_ctree_node_get_row_data(ctree, parent_ctree_node)));
printf ("before sibl %s\n\n", gtt_project_get_desc(
	gtk_ctree_node_get_row_data(ctree, sibling_ctree_node)));
}
#endif
	/* note, we must test for new sibling before new parent,
	 * since tehre can eb a ibling *and* parent */
	if (new_sibling) 
	{
		/* if we have a sibling and a parent, and the parent
		 * is not expanded, then the visually correct thing 
		 * is to show 'subproject of parent; (i.e. downarrow)
		 */
		if (new_parent && (0 == GTK_CTREE_ROW(new_parent)->expanded)) 
		{
			/* down arrow means 'child of parent' */
			gtk_drag_set_icon_widget (drag_context, parent_pixmap, 10, 10);
			return TRUE;
		}

		/* left arrow means 'peer' (specifcally, 'insert before
		 * sibling') */
		gtk_drag_set_icon_widget (drag_context, sibling_pixmap, 10, 10);
		return TRUE;
	}

	if (new_parent) 
	{
		/* down arrow means 'child of parent' */
		gtk_drag_set_icon_widget (drag_context, parent_pixmap, 10, 10);
		return TRUE;
	}

	return FALSE;
}

/* ============================================================== */


GtkWidget *
create_ctree(void)
{
	GtkWidget *wimg;
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
	w = gtk_ctree_new_with_titles(4, 2, tmp);

	gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_justification(GTK_CLIST(w), TOTAL_COL, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(w), TIME_COL,  GTK_JUSTIFY_CENTER);
	gtk_clist_column_titles_active(GTK_CLIST(w));

	/* create the top-level window to hold the c-tree */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (sw), w);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (sw),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show_all (sw);

	gtk_signal_connect(GTK_OBJECT(w), "event",
			   GTK_SIGNAL_FUNC(clist_event), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "tree_select_row",
			   GTK_SIGNAL_FUNC(tree_select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "click_column",
			   GTK_SIGNAL_FUNC(click_column), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "tree_unselect_row",
			   GTK_SIGNAL_FUNC(tree_unselect_row), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "tree_expand",
			   GTK_SIGNAL_FUNC(tree_expand), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "tree_collapse",
			   GTK_SIGNAL_FUNC(tree_collapse), NULL);

	gtk_signal_connect(GTK_OBJECT(w), "drag_begin",
			   GTK_SIGNAL_FUNC(drag_begin), NULL);

	gtk_signal_connect(GTK_OBJECT(w), "drag_drop",
			   GTK_SIGNAL_FUNC(drag_drop), NULL);

	/* allow projects to be re-arranged by dragging */
	gtk_clist_set_reorderable(GTK_CLIST(w), TRUE);
	gtk_clist_set_use_drag_icons (GTK_CLIST(w), TRUE);
	gtk_ctree_set_drag_compare_func (GTK_CTREE(w), ctree_drag);

	/* desperate attempt to clue the user into how the 
	 * dragged project will be reparented. We show left or
	 * down arrows, respecitvely.  If anyone can come up with
	 * something more elegant, then please ...  */
	parent_pixmap = gtk_window_new (GTK_WINDOW_POPUP);
	sibling_pixmap = gtk_window_new (GTK_WINDOW_POPUP);

	{
		#include "down.xpm"
		wimg = gnome_pixmap_new_from_xpm_d (down);
		gtk_widget_ref(wimg);
		gtk_widget_show(wimg);
		gtk_container_add (GTK_CONTAINER(parent_pixmap), wimg);
	}
	{
		#include "left.xpm"
		wimg = gnome_pixmap_new_from_xpm_d (left);
		gtk_widget_ref(wimg);
		gtk_widget_show(wimg);
		gtk_container_add (GTK_CONTAINER(sibling_pixmap), wimg);
	}

	return w;
}



/* setup_ctree_w references no widget globals ... */

static void
setup_ctree_w (GtkWidget *top_win, GtkCTree *tree_w)
{
	GList *node, *prjlist;
	int timer_running;

	timer_running = timer_is_running();
	stop_timer();

	/* first, add all projects to the ctree */
	prjlist = gtt_get_project_list();
	if (prjlist) {
		gtk_clist_freeze(GTK_CLIST(tree_w));
		gtk_clist_clear(GTK_CLIST(tree_w));
		for (node = prjlist; node; node = node->next) 
		{
			GttProject *prj = node->data;
			ctree_add(prj, NULL);
		}
		gtk_clist_thaw(GTK_CLIST(tree_w));
	} else {
		gtk_clist_clear(GTK_CLIST(tree_w));
	}

	/* next, highlight the current project, and expand 
	 * the tree branches to it, if needed */
	if (cur_proj) 
	{
		GttProject *parent;
		gtk_ctree_select(tree_w, cur_proj->trow);
		parent = gtt_project_get_parent (cur_proj);
		while (parent) 
		{
			gtk_ctree_expand(tree_w, parent->trow);
			parent = gtt_project_get_parent (parent);
		}
	}

	err_init();
	if (!GTK_WIDGET_MAPPED(top_win)) {
		gtk_widget_show(top_win);
#ifdef CLIST_HEADER_HACK
		clist_header_hack(top_win, GTK_WIDGET(tree_w));
#endif /* CLIST_HEADER_HACK */
	}
	if (timer_running) start_timer();
	menu_set_states();

	if (config_show_clist_titles)
	{
		gtk_clist_column_titles_show(GTK_CLIST(tree_w));
	}
	else
	{
		gtk_clist_column_titles_hide(GTK_CLIST(tree_w));
	}

	if (config_show_subprojects)
	{
		gtk_ctree_set_line_style(tree_w, GTK_CTREE_LINES_SOLID);
		gtk_ctree_set_expander_style(tree_w, GTK_CTREE_EXPANDER_SQUARE);
	}
	else
	{
		gtk_ctree_set_line_style(tree_w, GTK_CTREE_LINES_NONE);
		gtk_ctree_set_expander_style(tree_w, GTK_CTREE_EXPANDER_NONE);
	}

	if (cur_proj) 
	{
		gtk_ctree_node_moveto(tree_w, cur_proj->trow, -1,
				 0.5, 0.0);
	}
	gtk_widget_queue_draw(GTK_WIDGET(tree_w));
}

void
setup_ctree(void)
{
	/* suck in globals */
	setup_ctree_w (window, GTK_CTREE(glist));
}

/* ============================================================== */

void
ctree_add (GttProject *p, GtkCTreeNode *parent)
{
	char ever_timestr[24], day_timestr[24];
	GList *n;
	GtkCTreeNode *treenode;
	char *tmp[4];

	print_hours_elapsed (ever_timestr, 24, gtt_project_total_secs_ever(p), 
			config_show_secs);
	print_hours_elapsed (day_timestr, 24, gtt_project_total_secs_day(p), 
			config_show_secs);
	tmp[TOTAL_COL] = ever_timestr;
	tmp[TIME_COL]  = day_timestr;
	tmp[TITLE_COL] = (char *) gtt_project_get_title(p);
	tmp[DESC_COL]  = (char *) gtt_project_get_desc(p);

	treenode = gtk_ctree_insert_node (GTK_CTREE(glist),  parent, NULL,
                               tmp, 0, NULL, NULL, NULL, NULL,
                               FALSE, FALSE);

	gtk_ctree_node_set_row_data(GTK_CTREE(glist), treenode, p);
	p->trow = treenode;

	for (n=gtt_project_get_children(p); n; n=n->next)
	{
		GttProject *sub_prj = n->data;
		ctree_add (sub_prj, treenode);
	}
}



void
ctree_insert_before (GttProject *p, GttProject *sibling)
{
	char ever_timestr[24], day_timestr[24];
	GtkCTreeNode *treenode, *sibnode=NULL;
	GtkCTreeNode *parentnode=NULL;
	char *tmp[4];

	print_hours_elapsed (ever_timestr, 24, gtt_project_total_secs_ever(p), 
			config_show_secs);
	print_hours_elapsed (day_timestr, 24, gtt_project_total_secs_day(p), 
			config_show_secs);
	tmp[TOTAL_COL] = ever_timestr;
	tmp[TIME_COL]  = day_timestr;
	tmp[TITLE_COL] = (char *) gtt_project_get_title(p);
	tmp[DESC_COL]  = (char *) gtt_project_get_desc(p);

	if (sibling)
	{
		if (sibling->parent) parentnode = sibling->parent->trow;
		sibnode = sibling->trow;
	}
	treenode = gtk_ctree_insert_node (GTK_CTREE(glist),  
                               parentnode, sibnode,
                               tmp, 0, NULL, NULL, NULL, NULL,
                               FALSE, FALSE);

	gtk_ctree_node_set_row_data(GTK_CTREE(glist), treenode, p);

	p->trow = treenode;
}

void
ctree_insert_after (GttProject *p, GttProject *sibling)
{
	char ever_timestr[24], day_timestr[24];
	GtkCTreeNode *treenode;
	GtkCTreeNode *parentnode=NULL;
	GtkCTreeNode *next_sibling=NULL;
	char *tmp[4];

	print_hours_elapsed (ever_timestr, 24, gtt_project_total_secs_ever(p), 
			config_show_secs);
	print_hours_elapsed (day_timestr, 24, gtt_project_total_secs_day(p), 
			config_show_secs);
	tmp[TOTAL_COL] = ever_timestr;
	tmp[TIME_COL]  = day_timestr;
	tmp[TITLE_COL] = (char *) gtt_project_get_title(p);
	tmp[DESC_COL]  = (char *) gtt_project_get_desc(p);

	if (sibling)
	{
		if (sibling->parent) parentnode = sibling->parent->trow;
		next_sibling = GTK_CTREE_NODE_NEXT(sibling->trow);

		/* weird math: if this is the last leaf on this
		 * branch, then next_sibling must be null to become 
		 * the new last leaf. Unfortunately, GTK_CTREE_NODE_NEXT
		 * doesn't give null, it just gives the next branch */
		if (next_sibling &&  
		   (GTK_CTREE_ROW(next_sibling)->parent != parentnode)) next_sibling = NULL;
	}
	treenode = gtk_ctree_insert_node (GTK_CTREE(glist),  
                               parentnode, next_sibling,
                               tmp, 0, NULL, NULL, NULL, NULL,
                               FALSE, FALSE);

	gtk_ctree_node_set_row_data(GTK_CTREE(glist), treenode, p);

	p->trow = treenode;
}



void
ctree_remove(GttProject *p)
{
	gtk_ctree_remove_node(GTK_CTREE(glist), p->trow);
	p->trow = NULL;
}



static void
cupdate_label(GttProject *p, gboolean expand)
{
	int secs_ever, secs_day;
	char ever_timestr[24], day_timestr[24];

	if (expand)
	{
		secs_ever = gtt_project_get_secs_ever (p);
		secs_day = gtt_project_get_secs_day (p);
	}
	else
	{
		secs_ever = gtt_project_total_secs_ever (p);
		secs_day = gtt_project_total_secs_day (p);
	}

	print_hours_elapsed (ever_timestr, 24, secs_ever, config_show_secs);
	print_hours_elapsed (day_timestr, 24, secs_day, config_show_secs);

	gtk_ctree_node_set_text(GTK_CTREE(glist), p->trow, TOTAL_COL,
			   ever_timestr);
	gtk_ctree_node_set_text(GTK_CTREE(glist), p->trow, TIME_COL,
			   day_timestr);
	update_status_bar();
}


void
ctree_update_label(GttProject *p)
{
	if (!p || !p->trow) return;
	cupdate_label (p, GTK_CTREE_ROW(p->trow)->expanded);
}

void
ctree_update_title(GttProject *p)
{
	if (NULL == p->trow) return;
	gtk_ctree_node_set_text(GTK_CTREE(glist), p->trow, TITLE_COL,
			   gtt_project_get_title(p));
}


void
ctree_update_desc(GttProject *p)
{
	if (NULL == p->trow) return;
	gtk_ctree_node_set_text(GTK_CTREE(glist), p->trow, DESC_COL,
			   gtt_project_get_desc(p));
}


/* ===================== END OF FILE ==============================  */
