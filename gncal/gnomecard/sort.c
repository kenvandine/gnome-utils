#include <config.h>
#include <gnome.h>
#include <string.h>

#include "gnomecard.h"
#include "sort.h"
#include "list.h"

/* sort_func gnomecard_sort_criteria; */

gint gnomecard_sort_col=COLTYPE_CARDNAME;

static gint gnomecard_cmp_fnames(const void *crd1, const void *crd2);
static gint gnomecard_cmp_names(const void *crd1, const void *crd2);
static gint gnomecard_cmp_firstnames(const void *crd1, const void *crd2);
static gint gnomecard_cmp_middlenames(const void *crd1, const void *crd2);
static gint gnomecard_cmp_lastnames(const void *crd1, const void *crd2);
static gint gnomecard_cmp_titles(const void *crd1, const void *crd2);
static gint gnomecard_cmp_urls(const void *crd1, const void *crd2);
static gint gnomecard_cmp_emails(const void *crd1, const void *crd2);
static gint gnomecard_cmp_orgs(const void *crd1, const void *crd2);
static sort_func getSortFuncFromColType(gint sort_col);




static sort_func
getSortFuncFromColType(gint sort_col)
{
    sort_func func=NULL;

    switch (sort_col) {
      case COLTYPE_FULLNAME:
	func = gnomecard_cmp_names;
	break;

      case COLTYPE_CARDNAME:
	func = gnomecard_cmp_fnames;
	break;

      case COLTYPE_FIRSTNAME:
	func = gnomecard_cmp_firstnames;
	break;

      case COLTYPE_MIDDLENAME:
	func = gnomecard_cmp_middlenames;
	break;

      case COLTYPE_LASTNAME:
	func = gnomecard_cmp_lastnames;
	break;

      case COLTYPE_PREFIX:
	func = NULL;
	break;

      case COLTYPE_SUFFIX:
	func = NULL;
	break;

      case COLTYPE_ORG:
	func = gnomecard_cmp_orgs;
	break;

      case COLTYPE_TITLE:
	func = gnomecard_cmp_titles;
	break;

      case COLTYPE_EMAIL:
	func = gnomecard_cmp_emails;
	break;

      case COLTYPE_WEBPAGE:
	func = gnomecard_cmp_urls;
	break;

      case COLTYPE_HOMEPHONE:
	func = NULL;
	break;

      case COLTYPE_WORKPHONE:
	func = NULL;
	break;
	
      default:
	break;
    }

    return func;
}


void
gnomecard_sort_card_list(gint sort_col)
{
	GList *l;
	Card **array;
	guint i, len;
	sort_func compar;

	compar = getSortFuncFromColType(sort_col);
	if (!compar)
	    return;
	
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


void
gnomecard_sort_card_list_by_default(void)
{
    gnomecard_sort_card_list(gnomecard_sort_col);
}

static void
gnomecard_do_sort_cards(gint sort_col)
{
    GList *l;
    Card *curr;
    
    curr = gnomecard_curr_crd->data;
    gnomecard_sort_card_list(sort_col);
    
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
	gint  sort_col;
	
	sort_col = GPOINTER_TO_INT(data);
	gnomecard_sort_col = sort_col;
	gnomecard_do_sort_cards(sort_col);

}

static gint
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

static gint
gnomecard_cmp_lastnames(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	
	name1 = (* (Card **) crd1)->name.family;
	name2 = (* (Card **) crd2)->name.family;
	
	if (name1 == name2)
	  return 0;
	if (!name1)
	  return 1;
	if (!name2)
	  return -1;
	
	return strcmp(name1, name2);
}

static gint
gnomecard_cmp_firstnames(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	
	name1 = (* (Card **) crd1)->name.given;
	name2 = (* (Card **) crd2)->name.given;
	
	if (name1 == name2)
	  return 0;
	if (!name1)
	  return 1;
	if (!name2)
	  return -1;
	
	return strcmp(name1, name2);
}

static gint
gnomecard_cmp_middlenames(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	
	name1 = (* (Card **) crd1)->name.additional;
	name2 = (* (Card **) crd2)->name.additional;
	
	if (name1 == name2)
	  return 0;
	if (!name1)
	  return 1;
	if (!name2)
	  return -1;
	
	return strcmp(name1, name2);
}

static gint
gnomecard_cmp_titles(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	
	name1 = (* (Card **) crd1)->title.str;
	name2 = (* (Card **) crd2)->title.str;
	
	if (name1 == name2)
	  return 0;
	if (!name1)
	  return 1;
	if (!name2)
	  return -1;
	
	return strcmp(name1, name2);
}

static gint
gnomecard_cmp_urls(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	
	name1 = (* (Card **) crd1)->url.str;
	name2 = (* (Card **) crd2)->url.str;
	
	if (name1 == name2)
	  return 0;
	if (!name1)
	  return 1;
	if (!name2)
	  return -1;
	
	return strcmp(name1, name2);
}


static int
gnomecard_cmp_names(const void *crd1, const void *crd2)
{
	char *name1, *name2;
	Card *card1, *card2;
	int ret;
	
	card1 = (* (Card **) crd1);
	card2 = (* (Card **) crd2);
	
	name1 = gnomecard_join_name(card1->name.prefix, card1->name.given, 
				    card1->name.additional, 
				    card1->name.family, 
				    card1->name.suffix);
	name2 = gnomecard_join_name(card2->name.prefix, card2->name.given, 
				    card2->name.additional,
				    card2->name.family, 
				    card2->name.suffix);
	
	ret =  strcmp(name1, name2);
	g_free(name1);
	g_free(name2);
	
	return ret;
}

static int
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

static gint
gnomecard_cmp_orgs(const void *crd1, const void *crd2)
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
