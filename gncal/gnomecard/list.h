#ifndef __GNOMECARD_LIST
#define __GNOMECARD_LIST

#include <gnome.h>

extern void gnomecard_update_list(Card *crd);
extern void gnomecard_scroll_list(GList *node);
extern void gnomecard_list_set_node_info(Card *crd);
extern void gnomecard_add_card_to_list(Card *crd);
extern void gnomecard_add_card_sections_to_list(Card *crd);
extern void gnomecard_list_set_sorted_pos(Card *crd);

#endif
