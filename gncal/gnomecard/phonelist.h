#ifndef __GNOMECARD_PHONELIST_H
#define __GNOMECARD_PHONELIST_H

#include "dialog.h"

GtkWidget *gnomecard_create_phone_page(Card *crd, GnomeCardEditor *ce,
				       GnomePropertyBox *box);
void deletePhoneList(CardList src);
void copyPhoneList(CardList src, CardList *dest);
void copyGUIToCurPhone(GnomeCardEditor *ce);
void copyCurPhoneToGUI(GnomeCardEditor *ce);
GList *findmatchPhoneType(GList *l, gint type);



#endif

