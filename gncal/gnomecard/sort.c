#include <config.h>
#include <gnome.h>
#include <string.h>

#include "gnomecard.h"
#include "sort.h"
#include "list.h"

sort_func gnomecard_sort_criteria;

/*static char *sort_type_name[] =
                 { N_("Name"), N_("Address"), N_("E-mail"), 
		   N_("Title"), N_("Role"), N_("Company"), NULL };*/

void
gnomecard_sort_card_list(sort_func compar)
{
	GList *l;
	Card **array;
	guint i, len;
	
	len = g_list_length(gnomecard_crds);
	array = g_malloc(sizeof(Card *) * len);
	
	i = 0;
	for (l = gnomecard_crds; l; l = l->next)
		array[i++] = l->data;
	
	qsort(array, len, sizeof(Card *), compar);
	
	i = 0;
	for (l = gnomecard_crds; l; l = l->next, i++)
	  l->data = array[i];
	
	g_free(array);
}

static void
gnomecard_do_sort_cards(sort_func criteria)
{
    GList *l;
    Card *curr;
    
    curr = gnomecard_curr_crd->data;
    gnomecard_sort_card_list(criteria);
    
    for (l = gnomecard_crds; l; l = l->next) {
	if (curr == l->data)
	    gnomecard_curr_crd = l;
    }
    gnomecard_rebuild_list();
/*	gnomecard_set_changed(TRUE); WHY? We didnt change data any */
}

void
gnomecard_sort_cards(GtkWidget *w, gpointer data)
{
	sort_func criteria;
	
	criteria = (sort_func) data;
	if (gnomecard_sort_criteria != criteria) {
		gnomecard_sort_criteria = criteria;
		gnomecard_do_sort_cards(criteria);
	}
}

/* NOT USED
GtkCTreeNode
*gnomecard_search_sorted_pos(Card *crd)
{
	GList *l;
	
	for (l = gnomecard_crds; l; l = l->next)
	  if ((*gnomecard_sort_criteria) (& (l->data), &crd) > 0)
	    return ((Card *) l->data)->prop.user_data;
	
	return NULL;
}
*/
	
gint
gnomecard_cmp_fnames(const void *crd1, const void *crd2)
{
	char *fname1, *fname2;
	
	fname1 = (* (Card **) crd1)->fname.str;
	fname2 = (* (Card **) crd2)->fname.str;
	
	if (fname1 == fname2)
	  return 0;
	if (!fname1)
	  return 1;
	if (!fname2)
	  return -1;
	
	return strcmp(fname1, fname2);
}

int
gnomecard_cmp_names(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	Card *card1, *card2;
	int ret;
	
	card1 = (* (Card **) crd1);
	card2 = (* (Card **) crd2);
	
	name1 = gnomecard_join_name(card1->name.prefix, card1->name.given, 
				    card1->name.additional, card1->name.family, 
				    card1->name.suffix);
	name2 = gnomecard_join_name(card2->name.prefix, card2->name.given, 
				    card2->name.additional, card2->name.family, 
				    card2->name.suffix);
	
	ret =  strcmp(name1, name2);
	g_free(name1);
	g_free(name2);
	
	return ret;
}

int
gnomecard_cmp_emails(const void *crd1, const void *crd2)
{
	char *email1, *email2;
	char *host1, *host2;
	Card *card1, *card2;
	int ret;

	card1 = (* (Card **) crd1);
	card2 = (* (Card **) crd2);

	if (! card1->email.address && !card2->email.address)
	  return gnomecard_cmp_fnames(crd1, crd2);
	if (! card1->email.address)
	  return 1;
	if (! card2->email.address)
	  return -1;
	
	email1 = g_strdup(card1->email.address);
	email2 = g_strdup(card2->email.address);
	
	if ((host1 = strchr(email1, '@'))) {
		*host1++ = 0;
		if ((host2 = strchr(email2, '@'))) {
			*host2++ = 0;
			if ((ret = strcmp(host1, host2))) {
			    g_free(email1);
			    g_free(email2);
			    return ret;
			}
		}
	}
	
	ret = strcmp(email1, email2);
	g_free(email1);
	g_free(email2);
	return ret;
}

gint
gnomecard_cmp_org(const void *crd1, const void *crd2)
{
	char *fname1, *fname2;
	
	fname1 = (* (Card **) crd1)->org.name;
	fname2 = (* (Card **) crd2)->org.name;
	
	if (fname1 == fname2)
	  return 0;
	if (!fname1)
	  return 1;
	if (!fname2)
	  return -1;
	
	return strcmp(fname1, fname2);
}
