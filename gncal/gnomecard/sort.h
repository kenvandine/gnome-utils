#ifndef __GNOMECARD_SORT
#define __GNOMECARD_SORT

#include <gnome.h>

#include "card.h"
#include "columnhdrs.h"

typedef int (*sort_func) (const void *, const void *);

gint gnomecard_sort_col;

void gnomecard_sort_card_list(gint sort_col);
void gnomecard_sort_card_list_by_default(void);
void gnomecard_sort_cards(GtkWidget *w, gpointer data);

#endif
