#ifndef __GNOMECARD_TREE
#define __GNOMECARD_TREE

#include <gnome.h>

extern void gnomecard_update_tree(Card *crd);
extern void gnomecard_scroll_tree(GList *node);
extern void gnomecard_tree_set_node_info(Card *crd);
extern void gnomecard_add_card_to_tree(Card *crd);
extern void gnomecard_add_card_sections_to_tree(Card *crd);
extern void gnomecard_tree_set_sorted_pos(Card *crd);

#endif
