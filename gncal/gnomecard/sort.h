#ifndef __GNOMECARD_SORT
#define __GNOMECARD_SORT

#include <gnome.h>

#include "card.h"

typedef int (*sort_func) (const void *, const void *);

extern sort_func gnomecard_sort_criteria;

extern void gnomecard_sort_card_list(sort_func compar);
extern void gnomecard_sort_cards(GtkWidget *w, gpointer data);
extern GtkCTreeNode *gnomecard_search_sorted_pos(Card *crd);

extern int gnomecard_cmp_fnames(const void *crd1, const void *crd2);
extern int gnomecard_cmp_names(const void *crd1, const void *crd2);
extern int gnomecard_cmp_emails(const void *crd1, const void *crd2);

#endif
