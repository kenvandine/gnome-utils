#include <config.h>
#include <gnome.h>

#include "canvas.h"
#include "card.h"

#define CANVAS_FONT "-adobe-helvetica-medium-r-*-*-20-240-*-*-p-*-iso8859-1"
#define CANVAS_WIDTH 250
#define CANVAS_HEIGHT 150

GtkWidget *gnomecard_canvas;
GnomeCanvasItem *test;

GtkWidget *
gnomecard_canvas_new(void)
{
	GnomeCanvasGroup *root;
	GtkWidget *canvas;
	
	canvas = gnomecard_canvas = gnome_canvas_new();
	gtk_widget_pop_visual();
	gtk_widget_pop_colormap();
	gnome_canvas_set_size(GNOME_CANVAS(canvas), 
			      CANVAS_WIDTH, CANVAS_HEIGHT);
	gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas), 0, 0, 
				       CANVAS_WIDTH, CANVAS_HEIGHT);

	root = GNOME_CANVAS_GROUP (gnome_canvas_root(GNOME_CANVAS(canvas)));
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (double) (CANVAS_WIDTH - 1),
			       "y2", (double) (CANVAS_HEIGHT - 1),
			       "fill_color", "white",
			       "outline_color", "black",
			       "width_pixels", 0,
			       NULL);
	test = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", CANVAS_WIDTH / 2.0,
				      "y", CANVAS_HEIGHT / 2.0,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_CENTER,
				      "fill_color", "black",
				      NULL);
	return canvas;
}

extern void gnomecard_canvas_text_item_set(char *text)
{
	gnome_canvas_item_set (test, "text", text, NULL);
}

extern void gnomecard_update_canvas(Card *crd) 
{
    if (crd && crd->fname)
	if (crd->fname.str)
	    gnomecard_canvas_text_item_set(crd->fname.str);
	else
	    gnomecard_canvas_text_item_set(_("No fname for this card."));
}
