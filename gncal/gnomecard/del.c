#include <gnome.h>

#include "card.h"
#include "cardtypes.h"
#include "del.h"
#include "gnomecard.h"
#include "tree.h"

static void del_email(GtkCTreeNode *node, gpointer data)
{
	CardList *email_list;
	GList *email;
	
	email_list = & ((Card *) gnomecard_curr_crd->data)->email;
	for (email = email_list->l; email; email = email->next)
	  if (email->data == data)
	    break;
	
	if (email) {
		GList *curr = NULL;
		
		if (email->next)
		  curr = email->next;
		else if (email->prev)
		  curr = email->prev;
		
		if (curr)
		  gtk_ctree_select(gnomecard_tree, 
				   ((CardEMail *) curr->data)->prop.user_data);
		else
		  gtk_ctree_select(gnomecard_tree, 
				   ((Card *) gnomecard_curr_crd->data)->prop.user_data);
		
		email_list->l = g_list_remove_link(email_list->l, email);
		g_free(((CardEMail *) email->data)->data);
		g_free(email->data);
		g_list_free(email);
		
		gtk_ctree_remove_node(gnomecard_tree, node);
		
		if (email_list->l && !email_list->l->next) {
			gnomecard_update_tree(gnomecard_curr_crd->data);
/*			gtk_ctree_collapse_recursive(gnomecard_tree,
						     ((Card *) gnomecard_curr_crd->data)->prop.user_data);
			gtk_ctree_move(gnomecard_tree, ((CardEMail *) email->data)->prop.user_data,
				       ((Card *) gnomecard_curr_crd->data)->prop.user_data,
				       email_list->prop.user_data);
			gtk_ctree_remove_node(gnomecard_tree, email_list->prop.user_data);*/
		}
	}
}

static void del_card(GtkCTreeNode *node, gpointer data)
{
	GList *tmp;
	
	node = 0; /* avoid warnings */
	data = 0;
	
	if (gnomecard_curr_crd->next)
	  tmp = gnomecard_curr_crd->next;
	else
	  tmp = gnomecard_curr_crd->prev;
	
	card_free(gnomecard_curr_crd->data);
	gnomecard_crds = g_list_remove_link(gnomecard_crds, gnomecard_curr_crd);
	gtk_ctree_remove_node(gnomecard_tree, ((Card *) gnomecard_curr_crd->data)->prop.user_data);
	g_list_free(gnomecard_curr_crd);
	
	if (tmp)
	  gnomecard_scroll_tree(tmp);
	else
	  gnomecard_set_curr(NULL);
}

typedef void (*DelFunc) (GtkCTreeNode *, gpointer);

extern void gnomecard_del(GtkWidget *widget, gpointer data)
{
	CardProperty *prop;
	DelFunc del_func[] = {
		NULL, del_card, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL, NULL, del_email, NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
		NULL };
	
	prop = gtk_ctree_node_get_row_data(gnomecard_tree, gnomecard_selected_node);
	
	if (del_func[prop->type])
	  (*del_func[prop->type]) (gnomecard_selected_node, prop);
	
	gnomecard_set_changed(TRUE);
}
