/* Handle list view of avialable cards */
/* Michael Fulbright  <msf@redhat.com> */
/* Copyright (c) 1998 Red Hat Software, Inc */

#include <config.h>
#include <string.h>
#include <gnome.h>

#include "card.h"
#include "gnomecard.h"
#include "my.h"
#include "pix.h"
#include "sort.h"
#include "list.h"
#include "type_name.h"

static void 
gnomecard_create_list_row(Card *crd, gchar **text)
{
	gchar  *cardname=NULL;
	gchar  *name=NULL;
	GList  *l;

	/* for now we just display either cardname or real name and email */
	cardname = crd->fname.str;
	name = gnomecard_join_name(crd->name.prefix, crd->name.given, 
				      crd->name.additional, crd->name.family, 
				      crd->name.suffix);
	if (cardname)
	    text[0] = g_strdup(cardname);
	else if (name)
	    text[0] = name;
	else
	    text[0] = g_strdup("No name");

	if (crd->org.name)
	    text[1] = g_strdup(crd->org.name);
	else
	    text[1] = g_strdup("");

	/* show first phone # for now */
	text[2] = g_strdup("");
	for (l=crd->phone.l; l; l=l->next) {
	    CardPhone *phone;

	    phone = (CardPhone *)l->data;
	    if (l && phone && phone->data) {
		MY_FREE(text[2]);
		text[2] = g_strdup(phone->data);
	    }
	}

	if (crd->email.address) {
	    text[3] = g_strdup(crd->email.address);
	} else {
	    text[3] = g_strdup("");
	}

}

static void
gnomecard_destroy_list_row(gchar **text)
{
    MY_FREE(text[0]);
    MY_FREE(text[1]);
    MY_FREE(text[2]);
    MY_FREE(text[3]);
}


void
gnomecard_update_list(Card *crd)
{
    gchar  *text[4];
    gint   row;

    row = gtk_clist_find_row_from_data( gnomecard_list, crd);
    
    gnomecard_create_list_row(crd, text);
    gtk_clist_remove(gnomecard_list, row);
    gtk_clist_insert(gnomecard_list, row, text);
    gtk_clist_set_row_data(gnomecard_list, row, crd);
    gnomecard_destroy_list_row(text);
}


/* redraw list based on gnomecard_crd structure               */
/* used after a sort, for example, has changed order of cards */
void
gnomecard_rebuild_list(Card *curcard)
{
    GList *c;

    gtk_clist_freeze(gnomecard_list);
    gtk_clist_clear(gnomecard_list);
    for (c = gnomecard_crds; c; c = c->next)
	gnomecard_add_card_to_list((Card *) c->data);
    gtk_clist_thaw(gnomecard_list);
    if (curcard)
	gnomecard_scroll_list(curcard);
    else
	gnomecard_scroll_list(gnomecard_crds);
}

void
gnomecard_scroll_list(GList *node)
{
    gint row;
    GList *tmp;

    row = gtk_clist_find_row_from_data(gnomecard_list,(Card *) node->data);

    if (gtk_clist_row_is_visible(gnomecard_list, row) != GTK_VISIBILITY_FULL)
	gtk_clist_moveto(gnomecard_list, row, 0, 0.5, 0.0);

    gtk_clist_select_row(gnomecard_list, row, 0);
    tmp = g_list_nth(gnomecard_crds, row);

    if (!tmp) {
	g_message("Somehow selected non-existant card");
	return;
    }

    gnomecard_set_curr(tmp);
}

/* NOT USED 
static gchar *
gnomecard_first_phone_str(GList *phone)
{
	gchar *ret;
	
	if (! phone)
	  return NULL;
	
	ret = ((CardPhone *) phone->data)->data;
	
	for ( ; phone; phone = phone->next)
	  if (((CardPhone *) phone->data)->type & PHONE_PREF)	
	    ret = ((CardPhone *) phone->data)->data;
	
	return ret;
}
*/

/* NOT USED
void
gnomecard_list_set_node_info(Card *crd)
{
    g_message("in gnomecard_list_set_node_info - not implemented");
}
*/

static gint
gnomecard_next_addr_type(gint type, gint start)
{
    gint j;
    
    for (j = start; j < 6; j++)
	if (type & (1 << j))
	    return j;
    
    return j;
}

/* NOT USED
void
gnomecard_add_card_sections_to_list(Card *crd)
{
    g_message("gnomecard_add_card_sections_to_list not implemented");
}
*/
void
gnomecard_list_remove_card(Card *crd)
{
    gint row;

    row = gtk_clist_find_row_from_data( gnomecard_list, crd);
    if (row < 0)
	g_message("list_remove_card: couldnt find data %p",crd);
    gtk_clist_freeze(gnomecard_list);
    gtk_clist_remove(gnomecard_list, row);
    gtk_clist_thaw(gnomecard_list);
}

void
gnomecard_add_card_to_list(Card *crd)
{
	gchar  *text[4];
	gint   row;

	gnomecard_create_list_row(crd, text);
	g_message("gnomcard_add_card_to_list - adding name %s to list",text[0]);
	row = gtk_clist_append(gnomecard_list, text); 
/*	crd->prop.user_data = GINT_TO_POINTER(row); */
	gtk_clist_set_row_data(gnomecard_list, row, crd);

	gnomecard_destroy_list_row(text);
}

void
gnomecard_list_set_sorted_pos(Card *crd)
{
    g_message("gnomecard_tree_set_sorted_pos not implemented");
}
