#include <config.h>
#include <gnome.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <stdio.h>

#include "../versit/vcc.h"
#include "card.h"
#include "canvas.h"
#include "del.h"
#include "dialog.h"
#include "gnomecard.h"
#include "init.h"
#include "my.h"
#include "popup-menu.h"
#include "sort.h"
#include "list.h"

#include "columnhdrs.h" /* only used because I'm hardcoding col hdrs 4 now */

#define NAME_COL_WIDTH 100
#define ORG_COL_WIDTH 100
#define PHONE_COL_WIDTH 100
#define EMAIL_COL_WIDTH 100

#define LIST_WIDTH 450
#define LIST_HEIGHT 320

#define NONE  0
#define FNAME 1

#define IDENT 0
#define GEO 1
#define ORG 2
#define EXP 3
#define SEC 4

GtkWidget *gnomecard_window;

GtkCList *gnomecard_list=NULL;
gint     gnomecard_selected_row=0;

GtkWidget *tb_next, *tb_prev, *tb_first, *tb_last;
GtkWidget *tb_edit, *tb_find, *tb_save, *tb_del;
GtkWidget *menu_next, *menu_prev, *menu_first, *menu_last;
GtkWidget *menu_edit, *menu_find, *menu_save, *menu_del;

/* NOT USED GnomeUIInfo *add_menu;  */

GList *gnomecard_crds=NULL;
GList *gnomecard_curr_crd=NULL;

char *gnomecard_fname;
char *gnomecard_find_str;
gboolean gnomecard_find_sens;
gboolean gnomecard_find_back;
gint gnomecard_def_data;

gboolean gnomecard_changed;
gboolean gnomecard_found;                 /* yeah... pretty messy. (fixme) */


static void gnomecard_toggle_card_view(GtkWidget *w, gpointer data);
static void gnomecard_set_next(gboolean state);
static void gnomecard_set_prev(gboolean state);
static void gnomecard_list_button_press(GtkCList *list, GdkEventButton *event, 
					gpointer data);
static gboolean gnomecard_cards_blocked(void);
static void gnomecard_new_card(GtkWidget *widget, gpointer data);
static void gnomecard_list_selected(GtkCList *list, gint row, gint column,
				    GdkEventButton *event, gpointer data);


gchar
*gnomecard_join_name (char *pre, char *given, char *add, char *fam, char *suf)
{
    char *name;
    
    name = g_malloc(MY_STRLEN(given) + MY_STRLEN(add) + MY_STRLEN(fam) +
		    MY_STRLEN(pre) + MY_STRLEN(suf) + 5);
    
    *name = 0;
    if (pre && *pre) { strcpy(name, pre);   strcat(name, " "); }
    if (given && *given) { strcat(name, given); strcat(name, " "); }
    if (add && *add) { strcat(name, add);   strcat(name, " "); }
    if (fam && *fam) { strcat(name, fam);   strcat(name, " "); }
    if (suf && *suf)     
	strcat(name, suf);
    else
	if (*name)
	    name[strlen(name) - 1] = 0;
    
    return name;
}

void
gnomecard_set_changed(gboolean val)
{
    gnomecard_changed = val;
    gtk_widget_set_sensitive(tb_save, val);
    gtk_widget_set_sensitive(menu_save, val);
}

static void
gnomecard_toggle_card_view(GtkWidget *w, gpointer data)
{
    if (GTK_CHECK_MENU_ITEM(w)->active)
	gtk_widget_hide(gnomecard_canvas);
    else
	gtk_widget_show(gnomecard_canvas);
}

static void
gnomecard_set_next(gboolean state)
{
    gtk_widget_set_sensitive(tb_next, state);
    gtk_widget_set_sensitive(menu_next, state);
    gtk_widget_set_sensitive(tb_last, state);
    gtk_widget_set_sensitive(menu_last, state);
}

static void
gnomecard_set_prev(gboolean state)
{
    gtk_widget_set_sensitive(tb_prev, state);
    gtk_widget_set_sensitive(menu_prev, state);
    gtk_widget_set_sensitive(tb_first, state);
    gtk_widget_set_sensitive(menu_first, state);
}

/* NOT USED
extern void gnomecard_set_add(gboolean state)
{
	int i;
	
	for (i = 2; add_menu[i].type != GNOME_APP_UI_ENDOFINFO; i++)
		if (add_menu[i].type == GNOME_APP_UI_ITEM)
			gtk_widget_set_sensitive(GTK_WIDGET(add_menu[i].widget), state);
}
*/

void
gnomecard_set_edit_del(gboolean state)
{
    gtk_widget_set_sensitive(tb_edit, state);
    gtk_widget_set_sensitive(menu_edit, state);
    gtk_widget_set_sensitive(tb_del, state);
    gtk_widget_set_sensitive(menu_del, state);
    gtk_widget_set_sensitive(tb_find, state);
    gtk_widget_set_sensitive(menu_find, state);
}

void
gnomecard_set_curr(GList *node)
{
    gnomecard_curr_crd = node;
    
    if (gnomecard_curr_crd) {
	gnomecard_update_canvas(gnomecard_curr_crd->data);
	
	if (!((Card *) gnomecard_curr_crd->data)->flag) {
	    gnomecard_set_edit_del(TRUE);
	    /*gnomecard_set_add(TRUE);*/
	} else { 
	    gnomecard_set_edit_del(FALSE);
	}
	
	if (gnomecard_curr_crd->next)
	    gnomecard_set_next(TRUE);
	else
	    gnomecard_set_next(FALSE);
	
	if (gnomecard_curr_crd->prev)
	    gnomecard_set_prev(TRUE);
	else
	    gnomecard_set_prev(FALSE);
	
    } else {
	gnomecard_canvas_text_item_set(_("No cards, yet."));
	
	gnomecard_set_edit_del(FALSE);
	/*gnomecard_set_add(FALSE);*/
	
	gnomecard_set_next(FALSE);
	gnomecard_set_prev(FALSE);
    }
}

static void
gnomecard_list_button_press(GtkCList *list, GdkEventButton *event, 
			    gpointer data)
{
    static struct menu_item items[] = {
	{ N_("Edit this item..."), (GtkSignalFunc) gnomecard_edit_card, NULL,
	  TRUE },
	{ N_("Delete this item"), (GtkSignalFunc)gnomecard_delete_current_card,
	  NULL, TRUE } 
    };
    GList *tmp;
    gint  row, col;
    
    if (!event)
	return;
    
    if (event->button == 3) 
	if (gtk_clist_get_selection_info(list,event->x,event->y,&row,&col )) {
	    gnomecard_selected_row = row;
	    gtk_clist_select_row(list, row, 0);
	    tmp = g_list_nth(gnomecard_crds, row);
	    
	    if (!tmp) {
		g_message("Somehow selected non-existant card on row %d",row);
		return;
	    }

	    /* see if card if is being editted */
	    items[0].sensitive = !((Card *) tmp->data)->flag;
	    items[1].sensitive = !((Card *) tmp->data)->flag;

	    gnomecard_set_curr(tmp);
	    popup_menu (items, sizeof (items) / sizeof (items[0]), event);
	}
}

static void
gnomecard_list_selected(GtkCList *list, gint row, gint column,
			GdkEventButton *event, gpointer data)
{
    GList *tmp;

    gnomecard_selected_row = row;
    tmp = g_list_nth(gnomecard_crds, row);
    
    if (!tmp) {
	g_message("Somehow selected non-existant card on row %d",row);
	return;
    }

    if (!event)
	return;
    
    switch (event->button) {
      case 1:
	if (event->type == GDK_2BUTTON_PRESS) {
	    /* see if card if is being editted */
	    if (!((Card *) gnomecard_curr_crd->data)->flag)
		gnomecard_edit(tmp);
	} else {
	    gnomecard_set_curr(tmp);
	}
	break;
	
      default:
	break;
    }
}

/* NOT USED 
void gnomecard_list_unselected(GtkCTree *tree, GtkCTreeNode *row, gint column)
{
    g_message("gnomecard_list_unselected not implemented");
}
*/

extern void gnomecard_save(void)
{
	GList *l;
	FILE *fp;

	fp = fopen (gnomecard_fname, "w");
	for (l = gnomecard_crds; l; l= l->next)
	  card_save((Card *) l->data, fp);
	fclose(fp);
	
	gnomecard_set_changed(FALSE);
}

static gboolean
gnomecard_cards_blocked(void)
{
	GList *l;

	for (l = gnomecard_crds; l; l = l->next)
	  if (((Card *) l->data)->flag)
	    return TRUE;
	
	return FALSE;
}

/* Returns TRUE if the cards were destroyed. FALSE if canceled */
extern int gnomecard_destroy_cards(void)
{
	GList *l;

	if (gnomecard_cards_blocked()) {
		GtkWidget *w;

		w = gnome_message_box_new("There are cards which are currently being modified.\nFinish any pending modifications and try again.",
					  GNOME_MESSAGE_BOX_ERROR,
					  GNOME_STOCK_BUTTON_OK, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gtk_widget_show(w);
		
		return FALSE;
	}
	
	if (gnomecard_changed) {
		GtkWidget *w;
		char *msg;
		char *tmp = N_(" changed. Save?");
		int len;
		
		len = strlen(tmp) + strlen(gnomecard_fname) + 1;
		msg = g_malloc(len);
		snprintf(msg, len, "%s%s", gnomecard_fname, tmp);
		 
		w = gnome_message_box_new(msg,
					  GNOME_MESSAGE_BOX_QUESTION,
					  GNOME_STOCK_BUTTON_YES,
					  GNOME_STOCK_BUTTON_NO,
					  GNOME_STOCK_BUTTON_CANCEL, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gtk_widget_show(w);
		
		switch(gnome_dialog_run_modal(GNOME_DIALOG(w))) {
		 case -1:
		 case 2:
			return FALSE;
		 case 1:
			break;
		 case 0:
			gnomecard_save();
		}
		
		g_free (msg);
	}
	
	for (l = gnomecard_crds; l; l = l->next)
	    card_free (l->data);

	gtk_clist_clear(gnomecard_list);
	g_list_free(gnomecard_crds);
	gnomecard_crds = NULL;
	
	gnomecard_set_curr(NULL);
	gnomecard_set_changed(FALSE);
	
	return TRUE;
}

static void
gnomecard_new_card(GtkWidget *widget, gpointer data)
{
	Card *crd;
	GList *last;
	
	crd = card_new();
	gnomecard_add_card_to_list(crd);
	gnomecard_crds = g_list_append(gnomecard_crds, crd);
	
	last = g_list_last(gnomecard_crds);
	gnomecard_edit(last);
	gnomecard_scroll_list(last);
	gnomecard_set_changed(TRUE);
}

void gnomecard_first_card(GtkWidget *widget, gpointer data)
{
	gnomecard_scroll_list(g_list_first(gnomecard_crds));
}

void gnomecard_prev_card(GtkWidget *widget, gpointer data)
{
	if (gnomecard_curr_crd->prev)
	  gnomecard_scroll_list(gnomecard_curr_crd->prev);
}

void gnomecard_next_card(GtkWidget *widget, gpointer data)
{
	if (gnomecard_curr_crd->next)
	  gnomecard_scroll_list(gnomecard_curr_crd->next);
}

void gnomecard_last_card(GtkWidget *widget, gpointer data)
{
	gnomecard_scroll_list(g_list_last(gnomecard_crds));
}

void gnomecard_quit(GtkWidget *widget, gpointer data)
{
	if (gnomecard_destroy_cards()) {
		gtk_widget_destroy(gnomecard_window);
		gtk_main_quit();
	}
}

gint gnomecard_delete(GtkWidget *w, GdkEvent *e, gpointer data)
{
	if (gnomecard_destroy_cards()) {
		gtk_widget_destroy(gnomecard_window);
		gtk_main_quit();
	}
	  
	return TRUE;
}

void gnomecard_spawn_new(GtkWidget *widget, gpointer data)
{
	GtkWidget *w;
	int pid;
	char *text[] = { "GnomeCard", NULL };
	
	pid = fork();
	if (pid == 0) { /* child */
/* gnomecard: error in loading shared libraries
 * /gnome/lib/libgnome.so.0: undefined symbol: stat */
		if (execvp("GnomeCard", text) == -1) { 
			w = gnome_message_box_new(_("A new Gnomecard could not be spawned. Maybe it is not in your path."),
																GNOME_MESSAGE_BOX_ERROR,
																GNOME_STOCK_BUTTON_OK, NULL);
			GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
			gtk_widget_show(w);
		}
		exit (1);
	}
}

GnomeUIInfo filemenu[] = {
	{GNOME_APP_UI_ITEM, N_("New Gnomecard"), N_("Spawn a new Card Manager."), 
		gnomecard_spawn_new, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_OPEN, 0, 0, NULL},

	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("New"), N_("Create new Card File."), 
		gnomecard_destroy_cards, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Open..."), N_("Open a Card File."), 
		gnomecard_open, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Append..."), N_("Add the contents of another Card File."),
		gnomecard_append, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Save"), N_("Save current changes."), 
		gnomecard_save, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Save as..."), N_("Save current changes, choosing file name."), 
		gnomecard_save_as, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS, 0, 0, NULL},

	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Properties..."), N_("Appereance, customizations..."), gnomecard_setup, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Exit"), N_("Close the program."), 
		gnomecard_quit, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 0, 0, NULL},

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo gomenu[] = {
	{GNOME_APP_UI_ITEM, N_("First"), N_("First card"),
	 gnomecard_first_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardFirstMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Prev"), N_("Previous card"),
	 gnomecard_prev_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Next"), N_("Next card"),
	 gnomecard_next_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FORWARD, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Last"), N_("Last card"),
	 gnomecard_last_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardLastMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};
	
GnomeUIInfo sortradios[] = {
	
	{GNOME_APP_UI_ITEM, N_("By Card Name"), "",
	 gnomecard_sort_cards, (gpointer) gnomecard_cmp_fnames, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("By Name"), "",
	 gnomecard_sort_cards, (gpointer) gnomecard_cmp_names, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("By E-mail"), "",
	 gnomecard_sort_cards, (gpointer) gnomecard_cmp_emails, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("By Organization"), "",
	 gnomecard_sort_cards, (gpointer) gnomecard_cmp_org, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo sortmenu[] = {
	
	GNOMEUIINFO_RADIOLIST(sortradios),

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo editmenu[] = {
	{GNOME_APP_UI_ITEM, N_("Card"), N_("Edit card"),
	 gnomecard_edit_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardEditMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Delete"), N_("Delete element"),
	 gnomecard_delete_current_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE, 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_SUBTREE, N_("Go"), N_("Change current card"),
	 gomenu, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Find"), N_("Search card"),
	 gnomecard_find_card_call, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardFindMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_SUBTREE, N_("Sort"), N_("Set sort criteria"),
	 sortmenu, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};

/* NOT USED
GnomeUIInfo addmenu[] = {
	{GNOME_APP_UI_ITEM, N_("Card"), N_("Create new card"),
	 gnomecard_new_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardNewMenu", 0, 0, NULL},

	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("E-mail"), N_("Add Electronic Address"),
	 gnomecard_add_email_call, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardEMailMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Phone"), N_("Add Telephone Number"),
	 gnomecard_add_phone_call, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardPhoneMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Address"), N_("Add Delivery Address"),
	 gnomecard_add_deladdr_call, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardAddrMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Address Label"), N_("Add Delivery Address Label"),
	 gnomecard_add_dellabel_call, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardAddrMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};
*/
GnomeUIInfo viewmenu[] = {
	{GNOME_APP_UI_TOGGLEITEM, N_("Card"), N_("Toggle Card View"),
	 gnomecard_toggle_card_view, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

#ifdef B4MSF	
	{GNOME_APP_UI_TOGGLEITEM, N_("Tree"), N_("Toggle Tree View"),
	 gnomecard_toggle_tree_view, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
#endif
	
	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo helpmenu[] = {
	{GNOME_APP_UI_ITEM, N_("About..."), N_("Version, credits, etc."), 
		gnomecard_about, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo mainmenu[] = {
	{GNOME_APP_UI_SUBTREE, N_("File"), NULL, filemenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_SUBTREE, N_("Edit"), NULL, editmenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

/*	{GNOME_APP_UI_SUBTREE, N_("Add"), NULL, addmenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
*/
/*	{GNOME_APP_UI_SUBTREE, N_("View"), NULL, viewmenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},*/

	{GNOME_APP_UI_JUSTIFY_RIGHT},
		
	{GNOME_APP_UI_SUBTREE, N_("Help"), NULL, helpmenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo toolbar[] = {
	{GNOME_APP_UI_ITEM, N_("New"), N_("New file"), 
		gnomecard_destroy_cards, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Open"), N_("Open file"), 
		gnomecard_open, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Save"), N_("Save changes"), 
		gnomecard_save, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL},

	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Add"), N_("Create new card"),
	 gnomecard_new_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardNew", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Edit"), N_("Edit card"),
	 gnomecard_edit_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardEdit", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Del"), N_("Delete card"),
	 gnomecard_delete_current_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE, 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("First"), N_("First card"),
	 gnomecard_first_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardFirst", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Prev"), N_("Previous card"),
	 gnomecard_prev_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BACK, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Next"), N_("Next card"),
	 gnomecard_next_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_FORWARD, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Last"), N_("Last card"),
	 gnomecard_last_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardLast", 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Find"), N_("Search card"),
	 gnomecard_find_card_call, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardFind", 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};

void gnomecard_sort_by_fname(GtkWidget *w, gpointer data)
{
	gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(sortradios[0].widget), 
				      TRUE);
}

void gnomecard_init(void)
{
	char *titles[] = { "Name", "Organization", "Phone #", "Email"};
	GtkWidget *canvas, *align, *hbox, *hbox1, *hbox2;
	GtkWidget *scrollwin;
	GList *cols;
	ColumnType hdrs[] = {COLTYPE_CARDNAME, COLTYPE_EMAIL, COLTYPE_ORG, 
			     COLTYPE_END};

	/*, *omenu, *menu, *item;
	int i;*/

	gnomecard_init_stock();
	gnomecard_init_pixes();
	
	gnomecard_window = gnome_app_new("GnomeCard", "GnomeCard");
	gtk_window_set_wmclass(GTK_WINDOW(gnomecard_window), "GnomeCard",
			       "GnomeCard");

	gtk_widget_show(gnomecard_window);
	gtk_signal_connect(GTK_OBJECT(gnomecard_window), "delete_event",
			   GTK_SIGNAL_FUNC(gnomecard_delete), NULL);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gnome_app_set_contents(GNOME_APP(gnomecard_window), hbox);

	gnome_app_create_menus(GNOME_APP(gnomecard_window), mainmenu);
	gnome_app_create_toolbar(GNOME_APP(gnomecard_window), toolbar);

	scrollwin = gtk_scrolled_window_new(NULL, NULL);
/*	gnomecard_list = GTK_CLIST(gtk_clist_new_with_titles(4, titles)); */
	gnomecard_list = GTK_CLIST(gtk_clist_new(4));

	/* Hard coding column types for now */
	cols = buildColumnHeaders(hdrs);
	gtk_object_set_data(GTK_OBJECT(gnomecard_list), "ColumnHeaders",
			    cols);

WORK HERE ON ADDING CODE TO BUILD COLUMN HEADHINGS FROM cols 


	gtk_box_pack_start(GTK_BOX(hbox), scrollwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(gnomecard_list));
/* 	gtk_widget_realize(GTK_WIDGET(gnomecard_list)); */
/*
	gdk_gc_get_values(gnomecard_tree->lines_gc, &crd_tree_values);
*/
	gtk_clist_set_column_width(GTK_CLIST(gnomecard_list), 0, NAME_COL_WIDTH);
	gtk_clist_set_column_width(GTK_CLIST(gnomecard_list), 1, ORG_COL_WIDTH);
	gtk_clist_set_column_width(GTK_CLIST(gnomecard_list), 2, PHONE_COL_WIDTH);
	gtk_clist_set_column_width(GTK_CLIST(gnomecard_list), 3, EMAIL_COL_WIDTH);

	gtk_signal_connect(GTK_OBJECT(gnomecard_list), 
			   "select_row",
			   GTK_SIGNAL_FUNC(gnomecard_list_selected), NULL);

/* NOT USED (yet)
	gtk_signal_connect(GTK_OBJECT(gnomecard_list), 
			   "unselect_row",
			   GTK_SIGNAL_FUNC(gnomecard_list_unselected), NULL);
*/
	gtk_widget_set_usize (GTK_WIDGET(gnomecard_list), LIST_WIDTH, LIST_HEIGHT);
	gtk_clist_column_titles_active(gnomecard_list);

/* NOT USED - will add when I finallize the column header code
	gtk_signal_connect(GTK_OBJECT(GTK_CLIST(gnomecard_tree)->column->button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_sort_by_fname),
			   (gpointer) NULL);
*/
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
			     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_clist_set_selection_mode(gnomecard_list, GTK_SELECTION_BROWSE);

	gtk_signal_connect(GTK_OBJECT(gnomecard_list), "button_press_event",
			   GTK_SIGNAL_FUNC(gnomecard_list_button_press), NULL);

/*	omenu = gtk_option_menu_new();
	gtk_clist_set_column_widget(GTK_CLIST(gnomecard_tree), 1, omenu);
	menu = gtk_menu_new();
	for (i = 0; sort_type_name[i]; i++) {
		item = gtk_menu_item_new_with_label(sort_type_name[i]);
		gtk_object_set_user_data(GTK_OBJECT(item), (gpointer) i);
		gtk_menu_append(GTK_MENU(menu), item);
		gtk_widget_show(item);
	}
	
	gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);
	gtk_widget_show(omenu);*/

/*	
	crd_tree_sel_col = g_new (GdkColor, 1);
	crd_tree_sel_col->red = crd_tree_sel_col->green = 
	  crd_tree_sel_col->blue = 50000;
	gdk_color_alloc(gtk_widget_get_colormap (GTK_WIDGET (gnomecard_tree)),
			crd_tree_sel_col);

	crd_tree_usel_col = g_new (GdkColor, 1);
	crd_tree_usel_col->red = crd_tree_usel_col->green = 
		crd_tree_usel_col->blue = 60000;
	gdk_color_alloc(gtk_widget_get_colormap (GTK_WIDGET (gnomecard_tree)),
			crd_tree_usel_col);
*/
/* & (GTK_WIDGET(crd_tree)->style->fg[GTK_STATE_NORMAL]);*/
	
	gtk_widget_show(GTK_WIDGET(gnomecard_list));
	gtk_widget_show(scrollwin);

	align = gtk_alignment_new(0.5, 0.5, 0.5, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, 0);
	gtk_widget_show(align);

	gtk_widget_push_visual(gdk_imlib_get_visual());
	gtk_widget_push_colormap(gdk_imlib_get_colormap());
	
	canvas = gnomecard_canvas_new();
	gtk_container_add (GTK_CONTAINER (align), canvas);
	gtk_widget_show(canvas);
	
	gnomecard_crds = NULL;
	
	tb_save = toolbar[2].widget;
	tb_edit = toolbar[5].widget;
	tb_del  = toolbar[6].widget;
	tb_first = toolbar[8].widget;
	tb_prev = toolbar[9].widget;
	tb_next = toolbar[10].widget;
	tb_last = toolbar[11].widget;
	tb_find = toolbar[13].widget;
	
	menu_save = filemenu[5].widget;
	menu_edit = editmenu[0].widget;
	menu_del  = editmenu[1].widget;
	menu_find = editmenu[5].widget;
	menu_first = gomenu[0].widget;
	menu_prev = gomenu[1].widget;
	menu_next = gomenu[2].widget;
	menu_last = gomenu[3].widget;

/* NOT USED	add_menu = addmenu; */
	gnomecard_def_data = PHONE;
	gnomecard_sort_criteria = gnomecard_cmp_fnames;
	
	gnomecard_init_defaults();
	gnomecard_set_changed(FALSE);
	
	if (*gnomecard_fname)
		gnomecard_open_file(gnomecard_fname);
	else
		gnomecard_set_curr(NULL);
}

int main (int argc, char *argv[])
{
	textdomain(PACKAGE);
	gnome_init("GnomeCard", VERSION, argc, argv);
	gnomecard_init();

	gtk_main();
	return 0;
}
