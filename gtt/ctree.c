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

#include "app.h"
#include "ctree.h"
#include "cur-proj.h"
#include "gtt.h"
#include "menucmd.h"
#include "menus.h"
#include "prefs.h"
#include "props-proj.h"
#include "timer.h"
#include "util.h"

/* There is a bug in clist which makes all but the last column headers
 * 0 pixels wide. This hack fixes this. */
// #define CLIST_HEADER_HACK 1

/* column types */
typedef enum {
	NULL_COL = 0,
	TIME_EVER_COL = 1,
	TIME_TODAY_COL,
	TITLE_COL,
	DESC_COL,
	TASK_COL,
} ColType;

#define NCOLS		5


typedef struct ProjTreeNode_s
{
	ProjTreeWindow *ptw;
	GtkCTreeNode *ctnode;
	GttProject *prj;
} ProjTreeNode;

struct ProjTreeWindow_s 
{
	GtkCTree *ctree;
	ColType cols[NCOLS];
	char * col_titles[NCOLS];
	char * col_values[NCOLS];
	GtkJustification col_justify[NCOLS];
	int ncols;
	char ever_timestr[24];
	char day_timestr[24];

	// int clist_header_width_set;
};

static void cupdate_label(ProjTreeNode *ptn, gboolean expand);

/* ============================================================== */

static int
widget_key_event(GtkCTree *ctree, GdkEvent *event, gpointer data)
{
	GtkCTreeNode *rownode;
	GdkEventKey *kev = (GdkEventKey *)event;

	if (event->type != GDK_KEY_RELEASE) return FALSE;
	switch (kev->keyval)
	{
		case GDK_Return:
			rownode = gtk_ctree_node_nth (ctree,  GTK_CLIST(ctree)->focus_row);
			if (rownode)
			{
				ProjTreeNode *ptn;
				ptn = gtk_ctree_node_get_row_data(ctree, rownode);
				if ((ptn->prj == cur_proj) && timer_is_running())
				{
					gtk_ctree_unselect (ctree, rownode);
					cur_proj_set (NULL);
				}
				else
				{
					gtk_ctree_select (ctree, rownode);
					cur_proj_set (ptn->prj);
				}
			}
			return TRUE;
		case GDK_Up:
		case GDK_Down:
			return FALSE;
		case GDK_Left:
			rownode = gtk_ctree_node_nth (ctree,  GTK_CLIST(ctree)->focus_row);
			gtk_ctree_collapse (ctree, rownode);
			return TRUE;
		case GDK_Right:
			rownode = gtk_ctree_node_nth (ctree,  GTK_CLIST(ctree)->focus_row);
			gtk_ctree_expand (ctree, rownode);
			return TRUE;
		default:
			return FALSE;
	}
	return FALSE;
}

static int
widget_button_event(GtkCList *clist, GdkEvent *event, gpointer data)
{
	ProjTreeWindow *ptw = data;
	int row,column;
	GdkEventButton *bevent = (GdkEventButton *)event;
	GtkWidget *menu;
	
	/* The only button event we handle are right-mouse-button,
	 * end double-click-left-mouse-button. */
	if (!((event->type == GDK_2BUTTON_PRESS && bevent->button==1) ||
	      (event->type == GDK_BUTTON_PRESS && bevent->button==3)))
		return FALSE;

	gtk_clist_get_selection_info(clist,bevent->x,bevent->y,&row,&column);
	if (0 > row) return FALSE;
	
	/* change the focus row */
    	gtk_clist_freeze(clist);
	clist->focus_row = row;
    	gtk_clist_thaw(clist);

	if (event->type == GDK_2BUTTON_PRESS) 
	{
		/* double-click left mouse edits the project.
		 * but maybe we want to change double-click to 
		 * something more useful ... */
		GttProject *prj;
		prj = ctree_get_focus_project (ptw);
		prop_dialog_show (prj);
	} 
	else 
	{
		/* right mouse button brings up popup menu */
		menu = menus_get_popup();
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	}
	return TRUE;
}

/* ============================================================== */

GttProject *
ctree_get_focus_project (ProjTreeWindow *ptw)
{
	GttProject * proj = NULL;
	GtkCTreeNode *rownode;
	if (!ptw) return NULL;

	rownode = gtk_ctree_node_nth (ptw->ctree,  GTK_CLIST(ptw->ctree)->focus_row);
	if (rownode)
	{
		ProjTreeNode *ptn;
		ptn = gtk_ctree_node_get_row_data(ptw->ctree, rownode);
		if (ptn) proj = ptn->prj;
	}

	return proj;
}

/* ============================================================== */

static void
tree_select_row(GtkCTree *ctree, GtkCTreeNode* rownode, gint column)
{
	ProjTreeNode *ptn;
	ptn = gtk_ctree_node_get_row_data(ctree, rownode);
	cur_proj_set(ptn->prj);
}



static void
tree_unselect_row(GtkCTree *ctree, GtkCTreeNode* rownode, gint column)
{
	ProjTreeNode *ptn;
	ptn = gtk_ctree_node_get_row_data(ctree, rownode);
	if (ptn->prj != cur_proj) return;
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
click_column(GtkCList *clist, gint col, gpointer data)
{
	ProjTreeWindow *ptw = data;
	ColType ct;

	if ((0 > col) || (NCOLS <= col)) return;
	ct = ptw->cols[col];
	switch (ct)
	{
		case TIME_EVER_COL:
			project_list_sort_total_time();
			break;
		case TIME_TODAY_COL:
			project_list_sort_time();
			break;
		case TITLE_COL:
			project_list_sort_title();
			break;
		case DESC_COL:
			project_list_sort_desc();
			break;
		case TASK_COL:
			break;
		case NULL_COL:
			break;
	}
	
	ctree_setup(ptw);
}

#ifdef CLIST_HEADER_HACK
static void
clist_header_hack(GtkWidget *w)
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
drag_begin (GtkWidget *widget, GdkDragContext *context, gpointer data)
{
	drag_context = context;
}

static void 
drag_drop (GtkWidget *widget, GdkDragContext *context,
           gint x, gint y, guint time, gpointer data)
{
	ProjTreeWindow *ptw = data;
	ProjTreeNode *ptn;
	GttProject *src_prj, *par_prj=NULL, *sib_prj=NULL;
	GttProject *old_parent=NULL;
	GtkCTree *ctree = GTK_CTREE(widget);

	if (!source_ctree_node) return;

	/* sanity check. We expect a source node, and either 
	 * a parent, or a sibling */
	ptn = gtk_ctree_node_get_row_data(ctree, source_ctree_node);
	src_prj = ptn->prj;
	if (!src_prj) return;

	if (parent_ctree_node)
	{
		ptn = gtk_ctree_node_get_row_data(ctree, parent_ctree_node);
		par_prj = ptn->prj;
	}
	if (sibling_ctree_node)
	{
		ptn = gtk_ctree_node_get_row_data(ctree, sibling_ctree_node);
		sib_prj = ptn->prj;
	}

	if (!par_prj && !sib_prj) return;

	old_parent = gtt_project_get_parent (src_prj);

	if (sib_prj)
	{
		gtt_project_insert_before (src_prj, sib_prj);

		/* if collapsed we need to update the time */
		ctree_update_label (ptw, gtt_project_get_parent(src_prj));
	}
	else if (par_prj)
	{
		gtt_project_append_project (par_prj, src_prj);
		ctree_update_label (ptw, par_prj);
	}

	/* Make sure we update the timestamps for the old parent
	 * from which we were cutted. */
	ctree_update_label (ptw, old_parent);

	source_ctree_node = NULL;
	parent_ctree_node = NULL;
	sibling_ctree_node = NULL;
}

/* ============================================================== */

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
	gtk_ctree_node_get_row_data(ctree, source_ctree_node)->prj));
printf ("to parent %s\n", gtt_project_get_desc(
	gtk_ctree_node_get_row_data(ctree, parent_ctree_node)->prj));
printf ("before sibl %s\n\n", gtt_project_get_desc(
	gtk_ctree_node_get_row_data(ctree, sibling_ctree_node)->prj));
}
#endif
	/* Note, we must test for new sibling before new parent,
	 * since there can be a sibling *and* parent */
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
/* note about column layout:
 * We have two ways of doing this: create a column-cell object,
 * and program in an object-oriented styles.  The othe way is to
 * set a column type, and use a big switch statement.  Beacuse this
 * is a small prioject, we'll just use a switch statement.  Seems
 * easier.
 *
 * Note also we have implemented a general system here, that 
 * allows us to show columns in any order, or the same column
 * more than once.   However, we do not actually make use of
 * this anywhere; at the moment, the column locatins are fixed.
 */ 

static void 
ctree_init_cols (ProjTreeWindow *ptw)
{
	int i;

	/* init column types */
	i=0; ptw->cols[i] = TIME_EVER_COL;
	i++; ptw->cols[i] = TIME_TODAY_COL;
	i++; ptw->cols[i] = TITLE_COL;
	i++; ptw->cols[i] = DESC_COL;
	i++; ptw->cols[i] = TASK_COL;
	i++; ptw->cols[i] = NULL_COL;
	ptw->ncols = i;

	/* set column titles */
	for (i=0; NULL_COL != ptw->cols[i]; i++)
	{
		switch (ptw->cols[i])
		{
			case TIME_EVER_COL:
				ptw->col_justify[i] = GTK_JUSTIFY_CENTER;
				ptw->col_titles[i] =  _("Total");
				break;
			case TIME_TODAY_COL:
				ptw->col_justify[i] = GTK_JUSTIFY_CENTER;
				ptw->col_titles[i] =  _("Today");
				break;
			case TITLE_COL:
				ptw->col_justify[i] = GTK_JUSTIFY_LEFT;
				ptw->col_titles[i] =  _("Project Title");
				break;
			case DESC_COL:
				ptw->col_justify[i] = GTK_JUSTIFY_LEFT;
				ptw->col_titles[i] =  _("Description");
				break;
			case TASK_COL:
				ptw->col_justify[i] = GTK_JUSTIFY_LEFT;
				ptw->col_titles[i] =  _("Task");
				break;
			case NULL_COL:
				ptw->col_justify[i] = GTK_JUSTIFY_LEFT;
				ptw->col_titles[i] =  "";
				break;
		}
	}
}

/* ============================================================== */

void 
ctree_update_column_visibility (ProjTreeWindow *ptw)
{
	int i;

	/* set column visibility */
	for (i=0; NULL_COL != ptw->cols[i]; i++)
	{
		switch (ptw->cols[i])
		{
		case TIME_EVER_COL:
			break;
		case TIME_TODAY_COL:
			break;
		case TITLE_COL:
			break;
		case DESC_COL:
		gtk_clist_set_column_visibility (GTK_CLIST(ptw->ctree), i, 
				config_show_title_desc);
			break;
		case TASK_COL:
		gtk_clist_set_column_visibility (GTK_CLIST(ptw->ctree), i, 
				config_show_title_task);
			break;
		case NULL_COL:
			break;
		}
	}
}

/* ============================================================== */

static void 
ctree_col_values (ProjTreeWindow *ptw, GttProject *prj)
{
	int i;

	/* set column values */
	for (i=0; NULL_COL != ptw->cols[i]; i++)
	{
		switch (ptw->cols[i])
		{
			case TIME_EVER_COL:
				ptw->col_values[i] =  ptw->ever_timestr;
				print_hours_elapsed (ptw->ever_timestr, 24, 
					gtt_project_total_secs_ever(prj), 
					config_show_secs);
				break;
			case TIME_TODAY_COL:
				ptw->col_values[i] =  ptw->day_timestr;
				print_hours_elapsed (ptw->day_timestr, 24, 
					gtt_project_total_secs_day(prj), 
					config_show_secs);
				break;
			case TITLE_COL:
				ptw->col_values[i] = 
					(char *) gtt_project_get_title(prj);
				break;
			case DESC_COL:
				ptw->col_values[i] = 
					(char *) gtt_project_get_desc(prj);
				break;
			case TASK_COL:
			{
				GList *leadnode;
				leadnode = gtt_project_get_tasks (prj);
				if (leadnode)
				{
					ptw->col_values[i] =  
					(char *) gtt_task_get_memo (leadnode->data);
				}
				else
				{
					ptw->col_values[i] = "";
				}
				break;
			}
			case NULL_COL:
				ptw->col_values[i] =  "";
				break;
		}
	}
}

/* ============================================================== */
/* redraw handler */

static void
redraw (GttProject *prj, gpointer data)
{
	ProjTreeNode *ptn = data;
	ProjTreeWindow *ptw = ptn->ptw;
	int i;

	ctree_col_values (ptw, prj);
	for (i=0; i<ptw->ncols; i++)
	{
		gtk_ctree_node_set_text(ptw->ctree, ptn->ctnode, 
			i, ptw->col_values[i]);
	}

	cupdate_label (ptn, GTK_CTREE_ROW(ptn->ctnode)->expanded);
}

/* ============================================================== */

GtkWidget *
ctree_get_widget (ProjTreeWindow *ptw)
{
	if (!ptw) return NULL;
	return (GTK_WIDGET (ptw->ctree));
}

ProjTreeWindow *
ctree_new(void)
{
	ProjTreeWindow *ptw;
	GtkWidget *wimg;
	GtkWidget *w, *sw;
	int i;

	ptw = g_new0 (ProjTreeWindow, 1);

	ctree_init_cols (ptw);

	w = gtk_ctree_new_with_titles(ptw->ncols, 2, ptw->col_titles);
	ptw->ctree = GTK_CTREE(w);

	for (i=0; i<ptw->ncols; i++)
	{
		gtk_clist_set_column_justification(GTK_CLIST(w), 
			i, ptw->col_justify[i]);
	}
	gtk_clist_column_titles_active(GTK_CLIST(w));
	gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_SINGLE);

	gtk_widget_set_usize(w, -1, 120);
	ctree_update_column_visibility (ptw);
	gtk_ctree_set_show_stub(GTK_CTREE(w), FALSE);

	/* create the top-level window to hold the c-tree */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (sw), w);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (sw),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show_all (sw);

	gtk_signal_connect(GTK_OBJECT(w), "button_press_event",
			   GTK_SIGNAL_FUNC(widget_button_event), ptw);
	gtk_signal_connect(GTK_OBJECT(w), "key_release_event",
			   GTK_SIGNAL_FUNC(widget_key_event), ptw);
	gtk_signal_connect(GTK_OBJECT(w), "tree_select_row",
			   GTK_SIGNAL_FUNC(tree_select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "click_column",
			   GTK_SIGNAL_FUNC(click_column), ptw);
	gtk_signal_connect(GTK_OBJECT(w), "tree_unselect_row",
			   GTK_SIGNAL_FUNC(tree_unselect_row), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "tree_expand",
			   GTK_SIGNAL_FUNC(tree_expand), NULL);
	gtk_signal_connect(GTK_OBJECT(w), "tree_collapse",
			   GTK_SIGNAL_FUNC(tree_collapse), NULL);

	gtk_signal_connect(GTK_OBJECT(w), "drag_begin",
			   GTK_SIGNAL_FUNC(drag_begin), ptw);

	gtk_signal_connect(GTK_OBJECT(w), "drag_drop",
			   GTK_SIGNAL_FUNC(drag_drop), ptw);

	/* allow projects to be re-arranged by dragging */
	gtk_clist_set_reorderable(GTK_CLIST(w), TRUE);
	gtk_clist_set_use_drag_icons (GTK_CLIST(w), TRUE);
	gtk_ctree_set_drag_compare_func (GTK_CTREE(w), ctree_drag);

	/* Desperate attempt to clue the user into how the 
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

	return ptw;
}


/* ========================================================= */

void
ctree_setup (ProjTreeWindow *ptw)
{
	GtkCTree *tree_w = ptw->ctree;
	GList *node, *prjlist;
	GttProject *running_project = cur_proj;

	cur_proj_set (NULL);

	/* first, add all projects to the ctree */
	prjlist = gtt_get_project_list();
	if (prjlist) {
		gtk_clist_freeze(GTK_CLIST(tree_w));
		gtk_clist_clear(GTK_CLIST(tree_w));
		for (node = prjlist; node; node = node->next) 
		{
			GttProject *prj = node->data;
			ctree_add(ptw, prj, NULL);
		}
		gtk_clist_thaw(GTK_CLIST(tree_w));
	} else {
		gtk_clist_clear(GTK_CLIST(tree_w));
	}

	/* next, highlight the current project, and expand 
	 * the tree branches to it, if needed */
	if (running_project) cur_proj_set (running_project);
	if (cur_proj) 
	{
		GttProject *parent;
		ProjTreeNode *ptn;

		ptn = gtt_project_get_private_data (cur_proj);
		gtk_ctree_select(tree_w, ptn->ctnode);
		parent = gtt_project_get_parent (cur_proj);
		while (parent) 
		{
			ptn = gtt_project_get_private_data (parent);
			gtk_ctree_expand(tree_w, ptn->ctnode);
			parent = gtt_project_get_parent (parent);
		}
	}

#ifdef CLIST_HEADER_HACK
	clist_header_hack(GTK_WIDGET(tree_w));
#endif /* CLIST_HEADER_HACK */

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
		ProjTreeNode *ptn;
		ptn = gtt_project_get_private_data (cur_proj);
		gtk_ctree_node_moveto(tree_w, ptn->ctnode, -1,
				 0.5, 0.0);
		/* hack alert -- we should set the focus row 
		 * here as well */
	}
	gtk_widget_queue_draw(GTK_WIDGET(tree_w));
}

/* ========================================================= */

void
ctree_destroy (ProjTreeWindow *ptw)
{

	/* hack alert -- this function is not usable in its
	 * current form */
	g_warning ("incompletely implemented.  should probably "
		"traverse and destroy ptn's\n");
	// ctree_remove (ptw, ptn->prj);

	gtk_widget_destroy (GTK_WIDGET(ptw->ctree));
	ptw->ctree = NULL;
	ptw->ncols = 0;
	g_free (ptw);
}

/* ============================================================== */

void
ctree_add (ProjTreeWindow *ptw, GttProject *p, GtkCTreeNode *parent)
{
	ProjTreeNode *ptn;
	GList *n;

	ctree_col_values (ptw, p);

	ptn = gtt_project_get_private_data (p);
	if (!ptn)
	{
		ptn = g_new0 (ProjTreeNode, 1);
		ptn->ptw = ptw;
		ptn->prj = p;
		gtt_project_set_private_data (p, ptn);
		gtt_project_add_notifier (p, redraw, ptn);
	}
	ptn->ctnode = gtk_ctree_insert_node (ptw->ctree,  parent, NULL,
                               ptw->col_values, 0, NULL, NULL, NULL, NULL,
                               FALSE, FALSE);

	gtk_ctree_node_set_row_data(ptw->ctree, ptn->ctnode, ptn);

	/* make sure children get moved over also */
	for (n=gtt_project_get_children(p); n; n=n->next)
	{
		GttProject *sub_prj = n->data;
		ctree_add (ptw, sub_prj, ptn->ctnode);
	}
}

/* ============================================================== */

void
ctree_insert_before (ProjTreeWindow *ptw, GttProject *p, GttProject *sibling)
{
	ProjTreeNode *ptn;
	GtkCTreeNode *sibnode=NULL;
	GtkCTreeNode *parentnode=NULL;

	ctree_col_values (ptw, p);

	if (sibling)
	{
		GttProject *parent = gtt_project_get_parent (sibling);
		if (parent) 
		{
			ptn = gtt_project_get_private_data (parent);
			parentnode = ptn->ctnode;
		}
		ptn = gtt_project_get_private_data (sibling);
		sibnode = ptn->ctnode;
	}

	ptn = gtt_project_get_private_data (p);
	if (!ptn)
	{
		ptn = g_new0 (ProjTreeNode, 1);
		ptn->ptw = ptw;
		ptn->prj = p;
		gtt_project_set_private_data (p, ptn);
		gtt_project_add_notifier (p, redraw, ptn);
	}
	ptn->ctnode = gtk_ctree_insert_node (ptw->ctree,  
                               parentnode, sibnode,
                               ptw->col_values, 0, NULL, NULL, NULL, NULL,
                               FALSE, FALSE);

	gtk_ctree_node_set_row_data(ptw->ctree, ptn->ctnode, ptn);
}

/* ============================================================== */

void
ctree_insert_after (ProjTreeWindow *ptw, GttProject *p, GttProject *sibling)
{
	ProjTreeNode *ptn;
	GtkCTreeNode *parentnode=NULL;
	GtkCTreeNode *next_sibling=NULL;

	ctree_col_values (ptw, p);

	if (sibling)
	{
		GttProject *parent = gtt_project_get_parent (sibling);
		if (parent) 
		{
			ptn = gtt_project_get_private_data (parent);
			parentnode = ptn->ctnode;
		}
		ptn = gtt_project_get_private_data (sibling);
		next_sibling = GTK_CTREE_NODE_NEXT(ptn->ctnode);

		/* weird math: if this is the last leaf on this
		 * branch, then next_sibling must be null to become 
		 * the new last leaf. Unfortunately, GTK_CTREE_NODE_NEXT
		 * doesn't give null, it just gives the next branch */
		if (next_sibling &&  
		   (GTK_CTREE_ROW(next_sibling)->parent != parentnode)) next_sibling = NULL;
	}

	ptn = gtt_project_get_private_data (p);
	if (!ptn)
	{
		ptn = g_new0 (ProjTreeNode, 1);
		ptn->ptw = ptw;
		ptn->prj = p;
		gtt_project_set_private_data (p, ptn);
		gtt_project_add_notifier (p, redraw, ptn);
	}
	ptn->ctnode = gtk_ctree_insert_node (ptw->ctree,  
                               parentnode, next_sibling,
                               ptw->col_values, 0, NULL, NULL, NULL, NULL,
                               FALSE, FALSE);

	gtk_ctree_node_set_row_data(ptw->ctree, ptn->ctnode, ptn);
}

/* ============================================================== */

void
ctree_remove(ProjTreeWindow *ptw, GttProject *p)
{
	ProjTreeNode *ptn;
	if (!ptw || !p) return;

	ptn = gtt_project_get_private_data (p);
	g_return_if_fail (NULL != ptn);
	gtt_project_remove_notifier (p, redraw, ptn);
	gtk_ctree_node_set_row_data(ptw->ctree, ptn->ctnode, NULL);
	gtk_ctree_remove_node(ptw->ctree, ptn->ctnode);
	ptn->prj = NULL;
	ptn->ctnode = NULL;
	ptn->ptw = NULL;
	g_free (ptn);
	gtt_project_set_private_data (p, NULL);
}


/* ============================================================== */

static void
cupdate_label(ProjTreeNode *ptn, gboolean expand)
{
	GttProject *p = ptn->prj;
	int secs_ever, secs_day;
	char ever_timestr[24], day_timestr[24];
	int ever_col=-1, day_col=-1;
	int i;

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

	for (i=0; i<ptn->ptw->ncols; i++)
	{
		if (TIME_EVER_COL == ptn->ptw->cols[i]) ever_col = i;
		if (TIME_TODAY_COL == ptn->ptw->cols[i]) day_col = i;
	}

	gtk_ctree_node_set_text(ptn->ptw->ctree, ptn->ctnode, ever_col,
			   ever_timestr);
	gtk_ctree_node_set_text(ptn->ptw->ctree, ptn->ctnode, day_col,
			   day_timestr);
	update_status_bar();
}

/* ============================================================== */

void
ctree_update_label(ProjTreeWindow *ptw, GttProject *p)
{
	ProjTreeNode *ptn;
	if (!ptw || !p) return;
	ptn = gtt_project_get_private_data (p);
	g_return_if_fail (NULL != ptn);
	cupdate_label (ptn, GTK_CTREE_ROW(ptn->ctnode)->expanded);
}

/* ============================================================== */

void 
ctree_unselect (ProjTreeWindow *ptw, GttProject *p)
{
	ProjTreeNode *ptn;
	if (!ptw || !p) return;
	ptn = gtt_project_get_private_data (p);
	if (!ptn) return;

	gtk_ctree_unselect(ptw->ctree, ptn->ctnode);
}

void 
ctree_select (ProjTreeWindow *ptw, GttProject *p)
{
	ProjTreeNode *ptn;
	if (!ptw || !p) return;
	ptn = gtt_project_get_private_data (p);
	if (!ptn) return;

	gtk_ctree_select(ptw->ctree, ptn->ctnode);
}

/* ============================================================== */

void 
ctree_titles_show (ProjTreeWindow *ptw)
{
	if (!ptw) return;
	gtk_clist_column_titles_show(GTK_CLIST(ptw->ctree));
}

void 
ctree_titles_hide (ProjTreeWindow *ptw)
{
	if (!ptw) return;
	gtk_clist_column_titles_hide(GTK_CLIST(ptw->ctree));
}

void
ctree_subproj_show (ProjTreeWindow *ptw)
{
	if (!ptw) return;
	gtk_ctree_set_show_stub(ptw->ctree, FALSE);
	gtk_ctree_set_line_style(ptw->ctree, GTK_CTREE_LINES_SOLID);
	gtk_ctree_set_expander_style(ptw->ctree,GTK_CTREE_EXPANDER_SQUARE);
}

void
ctree_subproj_hide (ProjTreeWindow *ptw)
{
	if (!ptw) return;
	gtk_ctree_set_show_stub(ptw->ctree, FALSE);
	gtk_ctree_set_line_style(ptw->ctree, GTK_CTREE_LINES_NONE);
	gtk_ctree_set_expander_style(ptw->ctree,GTK_CTREE_EXPANDER_NONE);
}

/* ============================================================== */

void
ctree_set_col_width (ProjTreeWindow *ptw, int col, int width)
{
	if (!ptw) return;
	gtk_clist_set_column_width(GTK_CLIST(ptw->ctree), col, width);
	// ptw->clist_header_width_set = 1;
}

int
ctree_get_col_width (ProjTreeWindow *ptw, int col)
{
	int width;
	if (!ptw) return -1;
	if (0 > col) return -1;
	if (col >= GTK_CLIST(ptw->ctree)->columns) return -1;
	width = GTK_CLIST(ptw->ctree)->column[col].width;
	return width;
}

/* ===================== END OF FILE ==============================  */
