#ifndef __GNOMECARD_CANVAS
#define __GNOMECARD_CANVAS

#include "card.h"

extern GtkWidget *gnomecard_canvas;

extern GtkWidget *gnomecard_canvas_new(void);
extern void gnomecard_canvas_text_item_set(char *text);
extern void gnomecard_update_canvas(Card *crd);

#endif
