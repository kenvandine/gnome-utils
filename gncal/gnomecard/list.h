#ifndef __GNOMECARD_LIST
#define __GNOMECARD_LIST

#include <gnome.h>

void gnomecard_update_list(Card *crd);
void gnomecard_rebuild_list(Card *curcrd);
void gnomecard_scroll_list(GList *node);
void gnomecard_list_set_node_info(Card *crd);
void gnomecard_add_card_to_list(Card *crd);
void gnomecard_add_card_sections_to_list(Card *crd);
void gnomecard_list_set_sorted_pos(Card *crd);

GList *gnomecardCreateColTitles(GList *col);
void gnomecardFreeColTitles(GList *titles);
#endif
