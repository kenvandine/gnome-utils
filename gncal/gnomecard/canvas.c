#include <config.h>
#include <gnome.h>

#include "card.h"
#include "dialog.h"
#include "addresslist.h"
#include "phonelist.h"
#include "gnomecard.h"
#include "canvas.h"

#define CANVAS_FONT "-adobe-helvetica-medium-r-*-*-*-120-*-*-p-*-iso8859-1"
#define ADDR_FONT "-adobe-helvetica-medium-r-*-*-*-100-*-*-p-*-iso8859-1"
#define CANVAS_WIDTH 225
#define CANVAS_HEIGHT 350
#define LIST_SPACING 15.0
#define ADDR_SPACING 11.0

#define HEADER_BOX_COLOR  "khaki"
#define SUBHEADER_BOX_COLOR ""
#define LABEL_BOX_COLOR "dark khaki"

#define HEADER_TEXT_COLOR "black"
#define LABEL_TEXT_COLOR   "black"
#define BODY_TEXT_COLOR    "black"

typedef struct {
    GnomeCanvasItem *street1;
    GnomeCanvasItem *street2;
    GnomeCanvasItem *state;
    GnomeCanvasItem *country;
} CanvasAddressItem;

GtkWidget *gnomecard_canvas;
static GnomeCanvasItem *cardname, *fullname, *workphone, *homephone;
static GnomeCanvasItem *email, *url, *org, *title, *memos;
static CanvasAddressItem homeaddr, workaddr;
static GnomeCanvasGroup *root;

static void gnomecard_canvas_text_item_set(GnomeCanvasItem *p, gchar *text);

GtkWidget *
gnomecard_canvas_new(void)
{
	GtkWidget *canvas;
	gdouble ypos, x1, x2, y1, y2;
	GtkStyle *style;
	GnomeCanvasItem *item;
	gint i;

	canvas = gnomecard_canvas = gnome_canvas_new();
	gtk_widget_pop_visual();
	gtk_widget_pop_colormap();

	style = gtk_style_copy(gtk_widget_get_style(canvas));
	for (i=0; i<5; i++)
	    style->bg[i].red = style->bg[i].green = style->bg[i].blue = 0xffff;

	gtk_widget_set_style(GTK_WIDGET(canvas), style);

	gnome_canvas_set_size(GNOME_CANVAS(canvas), 
			      CANVAS_WIDTH, CANVAS_HEIGHT);

	root = GNOME_CANVAS_GROUP (gnome_canvas_root(GNOME_CANVAS(canvas)));

	/* label at top of canvas, contains current cardname */
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", 5.0,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) 35,
			       "fill_color", HEADER_BOX_COLOR,
			       "outline_color", HEADER_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);

	cardname = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", CANVAS_WIDTH / 2.0,
				      "y", 20.0,
				      "font", "-adobe-helvetica-medium-r-normal-*-*-180-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_CENTER,
				      "fill_color", HEADER_TEXT_COLOR,
				      NULL);

	/* Full name for card */
	ypos = 45.0;
	fullname = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);

	/* Organization and title */
	ypos += LIST_SPACING;
	title = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);

	ypos += LIST_SPACING;
	org = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);

	/* web site and email */
	ypos += LIST_SPACING;
	email = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);
	ypos += LIST_SPACING;
	url = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);

	/* phone #'s */
	ypos += LIST_SPACING;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) ypos+20,
			       "fill_color", HEADER_BOX_COLOR,
			       "outline_color", HEADER_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Phone Numbers",
				      "x", 10.0,
				      "y", ypos+10,
				      "font",  "-adobe-helvetica-medium-r-*-*-14-140-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", HEADER_TEXT_COLOR,
				      NULL);

	ypos += 30;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos-6.0,
			       "x2", (double) 45.0,
			       "y2", (double) ypos+6,
			       "fill_color", LABEL_BOX_COLOR,
			       "outline_color", LABEL_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);
	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Home:",
				      "x", 40.0,
				      "y", ypos,
				      "font",  CANVAS_FONT,
				      "anchor", GTK_ANCHOR_EAST,
				      "fill_color", LABEL_TEXT_COLOR,
				      NULL);

	homephone = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 15.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);


	ypos += 15;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos-6.0,
			       "x2", (double) 45.0,
			       "y2", (double) ypos+6,
			       "fill_color", LABEL_BOX_COLOR,
			       "outline_color", LABEL_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);
	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Work:",
				      "x", 40.0,
				      "y", ypos,
				      "font",  CANVAS_FONT,
				      "anchor", GTK_ANCHOR_EAST,
				      "fill_color", LABEL_TEXT_COLOR,
				      NULL);

	workphone = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 50.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);

	/* addresses */
	ypos += LIST_SPACING;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) ypos+20,
			       "fill_color", HEADER_BOX_COLOR,
			       "outline_color", HEADER_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Addresses",
				      "x", 10.0,
				      "y", ypos+10,
				      "font",  "-adobe-helvetica-medium-r-*-*-14-140-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", HEADER_TEXT_COLOR,
				      NULL);

	/* home address */
	ypos += 30;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 10.0,
			       "y1", ypos,
			       "x2", (double) 150.0,
			       "y2", (double) ypos+10,
			       "fill_color", LABEL_BOX_COLOR,
			       "outline_color", LABEL_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Home",
				      "x", 15.0,
				      "y", ypos+5,
				      "font",  "-adobe-helvetica-medium-r-*-*-10-100-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", LABEL_TEXT_COLOR,
				      NULL);
	
	ypos += 20;
	homeaddr.street1 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);

	ypos += ADDR_SPACING;
	homeaddr.street2 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);

	ypos += ADDR_SPACING;
	homeaddr.state = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);

	ypos += ADDR_SPACING;
	homeaddr.country = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);



	/* work address */
	ypos += 10;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 10.0,
			       "y1", ypos,
			       "x2", (double) 150.0,
			       "y2", (double) ypos+10,
			       "fill_color", LABEL_BOX_COLOR,
			       "outline_color", LABEL_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Work",
				      "x", 15.0,
				      "y", ypos+5,
				      "font",  "-adobe-helvetica-medium-r-*-*-10-100-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", LABEL_TEXT_COLOR,
				      NULL);
	
	ypos += 20;
	workaddr.street1 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);

	ypos += ADDR_SPACING;
	workaddr.street2 = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);

	ypos += ADDR_SPACING;
	workaddr.state = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);

	ypos += ADDR_SPACING;
	workaddr.country = gnome_canvas_item_new (root, 
						  gnome_canvas_text_get_type(),
						  "text", "",
						  "x", 10.0,
						  "y", ypos,
						  "font", ADDR_FONT,
						  "anchor", GTK_ANCHOR_WEST,
						  "fill_color",BODY_TEXT_COLOR,
						  NULL);


	/* memo(s) */
	ypos += LIST_SPACING;
	gnome_canvas_item_new (root, gnome_canvas_rect_get_type (),
			       "x1", 5.0,
			       "y1", ypos,
			       "x2", (double) (CANVAS_WIDTH - 6.0),
			       "y2", (double) ypos+20,
			       "fill_color", HEADER_BOX_COLOR,
			       "outline_color", HEADER_BOX_COLOR,
			       "width_pixels", 0,
			       NULL);

	gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "Memo(s)",
				      "x", 10.0,
				      "y", ypos+10,
				      "font",  "-adobe-helvetica-medium-r-*-*-14-140-*-*-p-*-iso8859-1",
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", HEADER_TEXT_COLOR,
				      NULL);


	ypos += 30;
	memos = gnome_canvas_item_new (root, gnome_canvas_text_get_type (),
				      "text", "",
				      "x", 5.0,
				      "y", ypos,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_WEST,
				      "fill_color", BODY_TEXT_COLOR,
				      NULL);

	/* set scrolling region */
	gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(root), &x1,&y1,&x2,&y2);
	gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas), x1, y1, x2, y2);

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
    gdouble x1, x2, y1, y2;

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

    /* memos */
    if (crd->comment.str)
	gnomecard_canvas_text_item_set(memos, crd->comment.str);
    else
	gnomecard_canvas_text_item_set(memos, "");

    /* set scrolling region */
    gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(root), &x1,&y1,&x2,&y2);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(gnomecard_canvas),
				   x1, y1, x2, y2);
    
}
