#ifndef __GNOMECARD_ADDRESSLIST_H
#define __GNOMECARD_ADDRESSLIST_H

GtkWidget *gnomecard_create_address_page(Card *crd, GnomeCardEditor *ce,
					 GnomePropertyBox *box);
void deleteAddrList(CardList src);
void copyAddrList(CardList src, CardList *dest);
void copyGUIToCurAddr(GnomeCardEditor *ce);
void copyCurAddrToGUI(GnomeCardEditor *ce);

#endif

