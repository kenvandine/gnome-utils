#include <config.h>
#include <gnome.h>

#include "gnomecard.h"
#include "canvas.h"
#include "card.h"

#define CANVAS_FONT "-adobe-helvetica-medium-r-*-*-12-140-*-*-p-*-iso8859-1"
#define CANVAS_WIDTH 225
#define CANVAS_HEIGHT 350

GtkWidget *gnomecard_canvas;
static GnomeCanvasItem *cardname, *fullname, *workphone, *homephone;
static GnomeCanvasItem *email, *url, *org, *title;

static void gnomecard_canvas_text_item_set(GnomeCanvasItem *p, gchar *text);


GtkWidget *
gnomecard_canvas_new(void)
{
	GnomeCanvasGroup *root;
	GtkWidget *canvas;
	gdouble ypos;

	GnomeCanvasItem *item;

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

	/* label at top of canvas */
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", 5.0,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) 35,
			       "fill_color", "NavyBlue",
			       "outline_color", "NavyBlue",
			       "width_pixels", 0,
			       NULL);

	ypos = 20.0;
	cardname = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", CANVAS_WIDTH / 2.0,
				      "y", ypos,
				      "font", "-adobe-helvetica-medium-r-*-*-20-240-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_CENTER,
				      "fill_color", "white",
				      NULL);
	ypos += 20.0;
	fullname = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 0.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);
	ypos += 20.0;
	org = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 100.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_CENTER,
				      "fill_color", "black",
				      NULL);

	return canvas;
}


static void
gnomecard_canvas_text_item_set(GnomeCanvasItem *p, gchar *text)
{
	gnome_canvas_item_set (p, "text", text, NULL);
}

void
gnomecard_clear_canvas(void)
{
    g_message("gnomecard_clear_canvas not implemented");
}

void
gnomecard_update_canvas(Card *crd) 
{

    gchar *name;

    if (!crd) {
	g_message("NULL card in gnomecard_update_canvas!");
	return;
    }

    if (crd->fname.str)
	gnomecard_canvas_text_item_set(cardname, crd->fname.str);
    else
	gnomecard_canvas_text_item_set(cardname, _("No fname for this card."));

    name = gnomecard_join_name(crd->name.prefix,
			       crd->name.given, 
			       crd->name.additional,
			       crd->name.family, 
			       crd->name.suffix);

    gnomecard_canvas_text_item_set(fullname, name);
}
