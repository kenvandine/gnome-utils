#ifndef __GNOMECARD_PHONELIST_H
#define __GNOMECARD_PHONELIST_H

GtkWidget *gnomecard_create_phone_page(Card *crd, GnomeCardEditor *ce,
				       GnomePropertyBox *box);
void deletePhoneList(CardList src);
void copyPhoneList(CardList src, CardList *dest);
void copyGUIToCurPhone(GnomeCardEditor *ce);
void copyCurPhoneToGUI(GnomeCardEditor *ce);



#endif

