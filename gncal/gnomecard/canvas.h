#ifndef __GNOMECARD_CANVAS
#define __GNOMECARD_CANVAS

#include "card.h"

extern GtkWidget *gnomecard_canvas;

GtkWidget *gnomecard_canvas_new(void);
void gnomecard_canvas_text_item_set(gchar *text);
void gnomecard_update_canvas(Card *crd);

#endif
