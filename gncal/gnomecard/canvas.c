#include <config.h>
#include <gnome.h>

#include "card.h"
#include "dialog.h"
#include "addresslist.h"
#include "phonelist.h"
#include "gnomecard.h"
#include "canvas.h"

#define CANVAS_FONT "-adobe-helvetica-medium-r-*-*-12-140-*-*-p-*-iso8859-1"
#define ADDR_FONT "-adobe-helvetica-medium-r-*-*-10-100-*-*-p-*-iso8859-1"
#define CANVAS_WIDTH 225
#define CANVAS_HEIGHT 350
#define LIST_SPACING 15.0
#define ADDR_SPACING 11.0

typedef struct {
    GnomeCanvasItem *street1;
    GnomeCanvasItem *street2;
    GnomeCanvasItem *state;
    GnomeCanvasItem *country;
} CanvasAddressItem;

GtkWidget *gnomecard_canvas;
static GnomeCanvasItem *cardname, *fullname, *workphone, *homephone;
static GnomeCanvasItem *email, *url, *org, *title;
static CanvasAddressItem homeaddr, workaddr;

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

	/* label at top of canvas, contains current cardname */
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", 5.0,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) 35,
			       "fill_color", "khaki",
			       "outline_color", "khaki",
			       "width_pixels", 0,
			       NULL);

	cardname = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", CANVAS_WIDTH / 2.0,
				      "y", 20.0,
				      "font", "-adobe-helvetica-medium-r-*-*-20-240-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_CENTER,
				      "fill_color", "black",
				      NULL);

	/* Full name for card */
	ypos = 45.0;
	fullname = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	/* Organization and title */
	ypos += LIST_SPACING;
	title = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	ypos += LIST_SPACING;
	org = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	/* web site and email */
	ypos += LIST_SPACING;
	email = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);
	ypos += LIST_SPACING;
	url = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	/* phone #'s */
	ypos += LIST_SPACING;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) ypos+20,
			       "fill_color", "khaki",
			       "outline_color", "khaki",
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Phone Numbers",
				      "x", 10.0,
				      "y", ypos+10,
				      "font",  "-adobe-helvetica-medium-r-*-*-14-140-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	ypos += 30;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos-6.0,
			       "x2", (double) 45.0,
			       "y2", (double) ypos+6,
			       "fill_color", "dark khaki",
			       "outline_color", "dark khaki",
			       "width_pixels", 0,
			       NULL);
	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Home:",
				      "x", 40.0,
				      "y", ypos,
				      "font",  CANVAS_FONT,
				      "anchor", GTK_ANCHOR_EAST,
				      "fill_color", "black",
				      NULL);

	homephone = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 15.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);


	ypos += 15;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos-6.0,
			       "x2", (double) 45.0,
			       "y2", (double) ypos+6,
			       "fill_color", "dark khaki",
			       "outline_color", "dark khaki",
			       "width_pixels", 0,
			       NULL);
	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Work:",
				      "x", 40.0,
				      "y", ypos,
				      "font",  CANVAS_FONT,
				      "anchor", GTK_ANCHOR_EAST,
				      "fill_color", "black",
				      NULL);

	workphone = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 50.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	/* addresses */
	ypos += LIST_SPACING;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) ypos+20,
			       "fill_color", "khaki",
			       "outline_color", "khaki",
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Addresses",
				      "x", 10.0,
				      "y", ypos+10,
				      "font",  "-adobe-helvetica-medium-r-*-*-14-140-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);

	/* home address */
	ypos += 30;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 10.0,
			       "y1", ypos,
			       "x2", (double) 150.0,
			       "y2", (double) ypos+10,
			       "fill_color", "dark khaki",
			       "outline_color", "dark khaki",
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Home",
				      "x", 15.0,
				      "y", ypos+5,
				      "font",  "-adobe-helvetica-medium-r-*-*-10-100-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);
	
	ypos += 20;
	homeaddr.street1 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);

	ypos += ADDR_SPACING;
	homeaddr.street2 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);

	ypos += ADDR_SPACING;
	homeaddr.state = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);

	ypos += ADDR_SPACING;
	homeaddr.country = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);



	/* work address */
	ypos += 10;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 10.0,
			       "y1", ypos,
			       "x2", (double) 150.0,
			       "y2", (double) ypos+10,
			       "fill_color", "dark khaki",
			       "outline_color", "dark khaki",
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Work",
				      "x", 15.0,
				      "y", ypos+5,
				      "font",  "-adobe-helvetica-medium-r-*-*-10-100-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", "black",
				      NULL);
	
	ypos += 20;
	workaddr.street1 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);

	ypos += ADDR_SPACING;
	workaddr.street2 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);

	ypos += ADDR_SPACING;
	workaddr.state = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);

	ypos += ADDR_SPACING;
	workaddr.country = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color", "black",
						  NULL);



	/* all done */
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
    GList *l;
    gchar *name;

    if (!crd) {
	g_message("NULL card in gnomecard_update_canvas!");
	return;
    }

    /* card name */
    if (crd->fname.str)
	gnomecard_canvas_text_item_set(cardname, crd->fname.str);
    else
	gnomecard_canvas_text_item_set(cardname, _("No fname for this card."));

    name = gnomecard_join_name(crd->name.prefix,
			       crd->name.given, 
			       crd->name.additional,
			       crd->name.family, 
			       crd->name.suffix);
    /* full name */
    gnomecard_canvas_text_item_set(fullname, name);

    /* title and organization */
    if (crd->title.str)
	gnomecard_canvas_text_item_set(title, crd->title.str);
    else
	gnomecard_canvas_text_item_set(title, "");

    gnomecard_canvas_text_item_set(org, crd->org.name);

    /* email and web site */
    gnomecard_canvas_text_item_set(email, crd->email.address);

    if (crd->url.str)
	gnomecard_canvas_text_item_set(url, crd->url.str);
    else
	gnomecard_canvas_text_item_set(url, "");

    /* phone numbers */
    if (crd->phone.l) {
	GList *l;
	
	l = findmatchPhoneType(crd->phone.l, PHONE_HOME);
	if (l) {
	    CardPhone *phone;
	    
	    phone = (CardPhone *)l->data;
	    gnomecard_canvas_text_item_set(homephone, phone->data);
	} else {
	    gnomecard_canvas_text_item_set(homephone, "");
	}	    

	l = findmatchPhoneType(crd->phone.l, PHONE_WORK);
	if (l) {
	    CardPhone *phone;
	    
	    phone = (CardPhone *)l->data;
	    gnomecard_canvas_text_item_set(workphone, phone->data);
	} else {
	    gnomecard_canvas_text_item_set(workphone, "");
	}
    } else {
	gnomecard_canvas_text_item_set(homephone, "");
    }	

    /* address */
    if (crd->postal.l) {
	l = findmatchAddrType(crd->postal.l, ADDR_HOME);
	if (l) {
	    gchar tmp[200];

	    CardPostAddr *p=(CardPostAddr *)l->data;

	    gnomecard_canvas_text_item_set(homeaddr.street1, 
					   (p->street1) ? p->street1 : "");
	    gnomecard_canvas_text_item_set(homeaddr.street2,  
					   (p->street2) ? p->street2 : "");

	    snprintf(tmp, sizeof(tmp), "%s, %s   %s",
		     (p->city) ? p->city : "", (p->state) ? p->state : "",
		     (p->zip) ? p->zip : "");
	    gnomecard_canvas_text_item_set(homeaddr.state, tmp);
	    gnomecard_canvas_text_item_set(homeaddr.country, 
					    (p->country) ? p->country : "");
	} else {
	    gnomecard_canvas_text_item_set(homeaddr.street1, "");
	    gnomecard_canvas_text_item_set(homeaddr.street2, "");
	    gnomecard_canvas_text_item_set(homeaddr.state, "");
	    gnomecard_canvas_text_item_set(homeaddr.country, "");
	}

	l = findmatchAddrType(crd->postal.l, ADDR_WORK);
	if (l) {
	    gchar tmp[200];

	    CardPostAddr *p=(CardPostAddr *)l->data;

	    gnomecard_canvas_text_item_set(workaddr.street1, 
					   (p->street1) ? p->street1 : "");
	    gnomecard_canvas_text_item_set(workaddr.street2,  
					   (p->street2) ? p->street2 : "");

	    snprintf(tmp, sizeof(tmp), "%s, %s   %s",
		     (p->city) ? p->city : "", (p->state) ? p->state : "",
		     (p->zip) ? p->zip : "");
	    gnomecard_canvas_text_item_set(workaddr.state, tmp);
	    gnomecard_canvas_text_item_set(workaddr.country, 
					    (p->country) ? p->country : "");
	} else {
	    gnomecard_canvas_text_item_set(workaddr.street1, "");
	    gnomecard_canvas_text_item_set(workaddr.street2, "");
	    gnomecard_canvas_text_item_set(workaddr.state, "");
	    gnomecard_canvas_text_item_set(workaddr.country, "");
	}
    } else {
	gnomecard_canvas_text_item_set(homeaddr.street1, "");
	gnomecard_canvas_text_item_set(homeaddr.street2, "");
	gnomecard_canvas_text_item_set(homeaddr.state, "");
	gnomecard_canvas_text_item_set(homeaddr.country, "");
	gnomecard_canvas_text_item_set(workaddr.street1, "");
	gnomecard_canvas_text_item_set(workaddr.street2, "");
	gnomecard_canvas_text_item_set(workaddr.state, "");
	gnomecard_canvas_text_item_set(workaddr.country, "");
    }

}
