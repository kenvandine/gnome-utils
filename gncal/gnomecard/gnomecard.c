#include <config.h>
#include <gnome.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <stdio.h>

#include "../versit/vcc.h"
#include "gnomecard.h"
#include "card.h"

#include "world.xpm"
#include "cardnew.xpm"
#include "cardedit.xpm"
#include "ident.xpm"
#include "geo.xpm"
#include "org.xpm"
#include "expl.xpm"
#include "sec.xpm"
#include "tel.xpm"
#include "email.xpm"
#include "addr.xpm"
#include "first.xpm"
#include "last.xpm"

#define CANVAS_FONT "-adobe-helvetica-medium-r-*-*-20-240-*-*-p-*-iso8859-1"
#define CANVAS_WIDTH 250
#define CANVAS_HEIGHT 150
#define COL_WIDTH 192
#define TREE_SPACING 16
#define _FREE(a) if (a) g_free (a)

#define IDENT 0
#define GEO 1
#define ORG 2
#define EXP 3
#define SEC 4

GtkWidget *gnomecard_window;
GtkWidget *crd_canvas;
GnomeCanvasItem *test;
GtkCTree  *crd_tree;

GtkWidget *tb_next, *tb_prev, *tb_first, *tb_last;
GtkWidget *tb_edit, *tb_save, *tb_del;
GtkWidget *menu_next, *menu_prev, *menu_first, *menu_last;
GtkWidget *menu_edit, *menu_save, *menu_del;

GList *crds;
GList *curr_crd;

pix *crd_pix, *ident_pix, *geo_pix, *org_pix, *exp_pix, *sec_pix;
pix *tel_pix, *email_pix, *addr_pix, *expl_pix, *org_pix;

char *gnomecard_fname;
gboolean gnomecard_changed;
gboolean found;                 /* yeah... pretty messy. (fixme) */

void canvas_text_item_set(GnomeCanvasItem *item, char *text) {
	gnome_canvas_item_set (item,
			       "text", text,
			       NULL);
}

GnomeStockPixmapEntry *gnomecard_pentry_new(gchar **xpm_data, gint size)
{
	GnomeStockPixmapEntry *pentry;
	
	pentry = g_malloc(sizeof(GnomeStockPixmapEntry));
	pentry->data.type = GNOME_STOCK_PIXMAP_TYPE_DATA;
	pentry->data.width = size;
	pentry->data.height = size;
	pentry->data.label = NULL;
	pentry->data.xpm_data = xpm_data;
	
	return pentry;
}

void gnomecard_init_stock(void)
{
	GnomeStockPixmapEntry *pentry;
	
	pentry = gnomecard_pentry_new(cardnew_xpm, 24);
	gnome_stock_pixmap_register("GnomeCardNew", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(cardnew_xpm, 16);
	gnome_stock_pixmap_register("GnomeCardNewMenu", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(cardedit_xpm, 24);
	gnome_stock_pixmap_register("GnomeCardEdit", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(cardedit_xpm, 16);
	gnome_stock_pixmap_register("GnomeCardEditMenu", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(first_xpm, 24);
	gnome_stock_pixmap_register("GnomeCardFirst", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(first_xpm, 16);
	gnome_stock_pixmap_register("GnomeCardFirstMenu", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(last_xpm, 24);
	gnome_stock_pixmap_register("GnomeCardLast", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
	pentry = gnomecard_pentry_new(last_xpm, 16);
	gnome_stock_pixmap_register("GnomeCardLastMenu", 
				    GNOME_STOCK_PIXMAP_REGULAR, pentry);
}

pix *pix_new(char **xpm)
{
	GdkImlibImage *image;
	pix *new_pix;
	
	new_pix = g_malloc(sizeof(pix));
	
        image = gdk_imlib_create_image_from_xpm_data(xpm);
        gdk_imlib_render (image, 16, 16);
        new_pix->pixmap = gdk_imlib_move_image (image);
        new_pix->mask = gdk_imlib_move_mask (image);
	gdk_imlib_destroy_image (image);
        gdk_window_get_size(new_pix->pixmap, 
			    &(new_pix->width), &(new_pix->height));
	
	return new_pix;
}

#define MY_STRLEN(x) (x?strlen(x):0)

char *join_name (char *pre, char *given, char *add, char *fam, char *suf)
{
	char *name;
	
	name = g_malloc(MY_STRLEN(given) + MY_STRLEN(add) + MY_STRLEN(fam) +
			MY_STRLEN(pre) + MY_STRLEN(suf) + 5);

	*name = 0;
	if (pre && *pre) { strcpy(name, pre);   strcat(name, " "); }
	if (given && *given) { strcat(name, given); strcat(name, " "); }
	if (add && *add) { strcat(name, add);   strcat(name, " "); }
	if (fam && *fam) { strcat(name, fam);   strcat(name, " "); }
	if (suf && *suf)     
	  strcat(name, suf);
	else
	  if (*name)
	    name[strlen(name) - 1] = 0;
	
	return name;
}

void gnomecard_init_pixes(void)
{
	crd_pix = pix_new(cardnew_xpm);
	ident_pix = pix_new(ident_xpm);
	geo_pix = pix_new(geo_xpm);
	sec_pix = pix_new(sec_xpm);
	tel_pix = pix_new(tel_xpm);
	email_pix = pix_new(email_xpm);
	addr_pix = pix_new(addr_xpm);
	expl_pix = pix_new(expl_xpm);
	org_pix = pix_new(org_xpm);
}
	
#define TAKE(x,y) if((x = y) == NULL) x = "";

void gnomecard_add_card_sections_to_tree(Card *crd)
{
	char *text[2];
	GList *parent, *child;
	
	parent = crd->user_data;
	child = parent;
	
	if (crd->name.prop.used || crd->photo.prop.used ||
	    crd->bday.prop.used) {
		text[0] = _("Identity");
		text[1] = "";
		child = gtk_ctree_insert(crd_tree, parent, NULL, text,
					 TREE_SPACING, ident_pix->pixmap,
					 ident_pix->mask, ident_pix->pixmap,
					 ident_pix->mask, FALSE, FALSE);
		
		
		
		text[0] = _("Name");
		text[1] = join_name(crd->name.prefix, crd->name.given, 
				    crd->name.additional, crd->name.family, 
				    crd->name.suffix);
		if (*text[1])
		  gtk_ctree_insert(crd_tree, child, NULL, text,
				   TREE_SPACING, NULL, NULL, NULL, NULL,
				   FALSE, FALSE);
		g_free(text[1]);
		
		if (crd->bday.prop.used) {
			text[0] = _("Birth Date");
			text[1] = card_bday_str(crd->bday);
			gtk_ctree_insert(crd_tree, child, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
			free(text[1]);
		}
	}
	
	if (crd->timezn.prop.used || crd->geopos.prop.used) {
		text[0] = _("Geographical");
		text[1] = "";
		child = gtk_ctree_insert(crd_tree, parent, NULL, text,
					 TREE_SPACING, geo_pix->pixmap,
					 geo_pix->mask, geo_pix->pixmap,
					 geo_pix->mask, FALSE, FALSE);
		if (crd->timezn.prop.used) {
			text[0] = _("Time Zone");
			text[1] = card_timezn_str(crd->timezn);
			gtk_ctree_insert(crd_tree, child, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
			free(text[1]);
		}
		if (crd->geopos.prop.used) {
			text[0] = _("Geo. Position");
			text[1] = card_geopos_str(crd->geopos);
			gtk_ctree_insert(crd_tree, child, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
			free(text[1]);
		}
	}
	
	if (crd->title.prop.used || crd->role.prop.used ||
	    crd->logo.prop.used || crd->agent || crd->org.prop.used) {
		text[0] = _("Organizational");
		text[1] = "";
		child = gtk_ctree_insert(crd_tree, parent, NULL, text,
				 TREE_SPACING, org_pix->pixmap,
				 org_pix->mask, org_pix->pixmap,
				 org_pix->mask, FALSE, FALSE);
		if (crd->title.prop.used) {
			text[0] = _("Title");
			text[1] = crd->title.str;
			gtk_ctree_insert(crd_tree, child, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
		}
		if (crd->role.prop.used) {
			text[0] = _("Role");
			text[1] = crd->role.str;
			gtk_ctree_insert(crd_tree, child, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
		}
		if (crd->org.prop.used) {
			GList *org;
			
			text[0] = _("Organization");
			text[1] = "";
			org = gtk_ctree_insert(crd_tree, child, NULL, text,
					       TREE_SPACING, org_pix->pixmap,
					       org_pix->mask, org_pix->pixmap,
					       org_pix->mask, FALSE, FALSE);
			if (crd->org.name) {
				text[0] = _("Name");
				text[1] = crd->org.name;
				gtk_ctree_insert(crd_tree, org, NULL, text,
						 TREE_SPACING, NULL, NULL, NULL, NULL,
						 FALSE, FALSE);
			}
			if (crd->org.unit1) {
				text[0] = _("Unit 1");
				text[1] = crd->org.unit1;
				gtk_ctree_insert(crd_tree, org, NULL, text,
						 TREE_SPACING, NULL, NULL, NULL, NULL,
						 FALSE, FALSE);
			}
			if (crd->org.unit2) {
				text[0] = _("Unit 2");
				text[1] = crd->org.unit2;
				gtk_ctree_insert(crd_tree, org, NULL, text,
						 TREE_SPACING, NULL, NULL, NULL, NULL,
						 FALSE, FALSE);
			}
			if (crd->org.unit3) {
				text[0] = _("Unit 3");
				text[1] = crd->org.unit3;
				gtk_ctree_insert(crd_tree, org, NULL, text,
						 TREE_SPACING, NULL, NULL, NULL, NULL,
						 FALSE, FALSE);
			}
			if (crd->org.unit4) {
				text[0] = _("Unit 4");
				text[1] = crd->org.unit4;
				gtk_ctree_insert(crd_tree, org, NULL, text,
						 TREE_SPACING, NULL, NULL, NULL, NULL,
						 FALSE, FALSE);
			}
				
		}
	}
	
	if (crd->comment.prop.used || crd->sound.prop.used ||
	    crd->url.prop.used) {
		GList *expl;
		
		text[0] = _("Explanatory");
		text[1] = "";
		expl = gtk_ctree_insert(crd_tree, parent, NULL, text,
					TREE_SPACING, expl_pix->pixmap,
					expl_pix->mask, expl_pix->pixmap,
					expl_pix->mask, FALSE, FALSE);
		if (crd->comment.prop.used) {
			text[0] = _("Comment");
			text[1] = "";
			gtk_ctree_insert(crd_tree, expl, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
		}
		if (crd->url.prop.used) {
			text[0] = _("URL");
			text[1] = crd->url.str;
			gtk_ctree_insert(crd_tree, expl, NULL, text,
					 TREE_SPACING, NULL, NULL, NULL, NULL,
					 FALSE, FALSE);
		}
			
	}
	
	if (crd->key.prop.used) {
		text[0] = _("Security");
		text[1] = "";
		gtk_ctree_insert(crd_tree, parent, NULL, text,
				 TREE_SPACING, sec_pix->pixmap,
				 sec_pix->mask, sec_pix->pixmap,
				 sec_pix->mask, FALSE, FALSE);
	}
	
	if (crd->phone) {
		text[0] = _("Telephone Numbers");
		text[1] = "";
		gtk_ctree_insert(crd_tree, parent, NULL, text,
				 TREE_SPACING, tel_pix->pixmap,
				 tel_pix->mask, tel_pix->pixmap,
				 tel_pix->mask, FALSE, FALSE);
	}
	
	if (crd->email) {
		text[0] = _("E-mail Addresses");
		text[1] = "";
		gtk_ctree_insert(crd_tree, parent, NULL, text,
				 TREE_SPACING, email_pix->pixmap,
				 email_pix->mask, email_pix->pixmap,
				 email_pix->mask, FALSE, FALSE);
	}
	
	if (crd->deladdr || crd->dellabel) {
		text[0] = _("Delivery Addresses");
		text[1] = "";
		gtk_ctree_insert(crd_tree, parent, NULL, text,
				 TREE_SPACING, addr_pix->pixmap,
				 addr_pix->mask, addr_pix->pixmap,
				 addr_pix->mask, FALSE, FALSE);
	}
	/*	TAKE(text[0], crd->*/
}

void gnomecard_add_card_to_tree(Card *crd)
{
	char *text[2];
	
	if ((text[1] = crd->fname.str) == NULL)
	  text[1] = _("No Formatted Name for this card.");
	text[0] = "";
	
	crd->user_data = gtk_ctree_insert(crd_tree, NULL, NULL, text,
					TREE_SPACING, crd_pix->pixmap,
					crd_pix->mask, crd_pix->pixmap,
					crd_pix->mask, FALSE, FALSE);
	
	gnomecard_add_card_sections_to_tree(crd);
}
	

GtkWidget *my_gtk_entry_new(gint len, char *init)
{
	GtkWidget *entry;
	
	entry = gtk_entry_new();
	if (len)
	  gtk_widget_set_usize (entry, 
				gdk_char_width (entry->style->font, 'M') * len, 0);
	if (init)
	  gtk_entry_set_text(GTK_ENTRY(entry), init);

	return entry;
}

GtkWidget *my_gtk_spin_button_new(GtkAdjustment *adj, gint width)
{
	GtkWidget *spin;
	
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(spin), TRUE);			      
	gtk_widget_set_usize (spin, /*22 + 8 * width, 0);*/
		    gdk_char_width (spin->style->font, '-') * width + 22, 0);
			      
	return spin;
}


GtkWidget *my_gtk_vbox_new(void)
{
	GtkWidget *vbox;
	
	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD_SMALL);

	return vbox;
}

GtkWidget *my_gtk_table_new(int x, int y)
{
	GtkWidget *table;
	
	table = gtk_table_new(x, y, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(table), GNOME_PAD_SMALL);
	
	return table;
}

void gnomecard_set_next(gboolean state)
{
	gtk_widget_set_sensitive(tb_next, state);
	gtk_widget_set_sensitive(menu_next, state);
	gtk_widget_set_sensitive(tb_last, state);
	gtk_widget_set_sensitive(menu_last, state);
}

void gnomecard_set_prev(gboolean state)
{
	gtk_widget_set_sensitive(tb_prev, state);
	gtk_widget_set_sensitive(menu_prev, state);
	gtk_widget_set_sensitive(tb_first, state);
	gtk_widget_set_sensitive(menu_first, state);
}

void gnomecard_set_edit_del(gboolean state)
{
	gtk_widget_set_sensitive(tb_edit, state);
	gtk_widget_set_sensitive(menu_edit, state);
	gtk_widget_set_sensitive(tb_del, state);
	gtk_widget_set_sensitive(menu_del, state);
}

void gnomecard_set_changed(gboolean val)
{
	gnomecard_changed = val;
	gtk_widget_set_sensitive(tb_save, val);
	gtk_widget_set_sensitive(menu_save, val);
}

void gnomecard_scroll_tree(GList *node)
{
	GList *tree_node;
	
	tree_node = ((Card *) node->data)->user_data;

	gtk_ctree_moveto(crd_tree, tree_node, 0, 0, 0);
	gtk_ctree_select(crd_tree, tree_node);
}

void gnomecard_set_curr(GList *node)
{
	curr_crd = node;
	
	if (curr_crd) {
		if (((Card *) curr_crd->data)->fname.str)
		  canvas_text_item_set(test, 
				       ((Card *) curr_crd->data)->fname.str);
		else
		  canvas_text_item_set(test, "No fname for this card.");
		
		if (!((Card *) curr_crd->data)->flag)
		  gnomecard_set_edit_del(TRUE);
		else
		  gnomecard_set_edit_del(FALSE);
		  

		if (curr_crd->next)
		  gnomecard_set_next(TRUE);
		else
		  gnomecard_set_next(FALSE);
		
		if (curr_crd->prev)
		  gnomecard_set_prev(TRUE);
		else
		  gnomecard_set_prev(FALSE);
		
	} else {
		canvas_text_item_set(test, "No cards, yet.");
		
		gnomecard_set_edit_del(FALSE);
		
		gnomecard_set_next(FALSE);
		gnomecard_set_prev(FALSE);
	}
}
	  
void gnomecard_cmp_nodes(GtkCTree *tree, GList *node, gpointer data)
{
	if ((GList *) data == node)
	  found = TRUE;
}

void gnomecard_tree_selected(GtkCTree *tree, GList *row, gint column)
{
	GList *i;
	
	found = FALSE;
	for (i = crds; i; i = i->next) {
		gtk_ctree_post_recursive(tree, ((Card *) i->data)->user_data,
					 GTK_CTREE_FUNC(gnomecard_cmp_nodes), 
					 row);
		if (found)
		  break;
	}
	
	if (i && i != curr_crd)
	  gnomecard_set_curr(i);
}

void gnomecard_property_used(GtkWidget *w, gpointer data)
{
	CardProperty *prop;
	
	prop = (CardProperty *) data;
	prop->used = TRUE;
}

void my_connect(gpointer widget, char *sig, gpointer box, CardProperty *prop)
{
 	gtk_signal_connect_object(GTK_OBJECT(widget), sig,
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_signal_connect(GTK_OBJECT(widget), sig,
			   GTK_SIGNAL_FUNC(gnomecard_property_used),
			   prop);
}

void gnomecard_update_tree(Card *crd)
{
	GList *node, *tmp;
	
	gtk_ctree_set_node_info(crd_tree, 
				crd->user_data,	crd->fname.str, TREE_SPACING, 
				crd_pix->pixmap, crd_pix->mask, 
				crd_pix->pixmap, crd_pix->mask, FALSE, FALSE);

	node = GTK_CTREE_ROW((GList *) crd->user_data)->children;
	while (node) {
		tmp = node->next;
		gtk_ctree_remove(crd_tree, node);
		node = tmp;
	}
}

void gnomecard_prop_apply(GtkWidget *widget, int page)
{
	GnomeCardEditor *ce;
	Card *crd;
	struct tm *tm;
	time_t tt;

	if (page != -1)
	  return;             /* ignore partial applies */
	
	ce = (GnomeCardEditor *) gtk_object_get_user_data(GTK_OBJECT(widget));
	crd = (Card *) ce->l->data;
	
	_FREE(crd->fname.str);
	_FREE(crd->name.family);
        _FREE(crd->name.given);
        _FREE(crd->name.additional);
        _FREE(crd->name.prefix);
        _FREE(crd->name.suffix);
	
	crd->fname.str       = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->fn)));
	crd->name.family     = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->fam)));
	crd->name.given      = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->given)));
	crd->name.additional = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->add)));
	crd->name.prefix     = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->pre)));
	crd->name.suffix     = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->suf)));
	
	tt = gnome_date_edit_get_date(GNOME_DATE_EDIT(ce->bday));
	tm = localtime(&tt);
	
	crd->bday.year       = tm->tm_year + 1900;
	crd->bday.month      = tm->tm_mon + 1;
	crd->bday.day        = tm->tm_mday;
	
	crd->timezn.sign  = (ce->tzh < 0)? -1: 1;
	crd->timezn.hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ce->tzh));
	crd->timezn.mins  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ce->tzm));
	
	crd->geopos.lon = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ce->gplon));
	crd->geopos.lat = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ce->gplat));
	
        _FREE(crd->title.str);
        _FREE(crd->role.str);
        _FREE(crd->org.name);
        _FREE(crd->org.unit1);
        _FREE(crd->org.unit2);
        _FREE(crd->org.unit3);
        _FREE(crd->org.unit4);

	crd->title.str = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->title)));
	crd->role.str  = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->role)));
	crd->org.name  = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->orgn)));
	crd->org.unit1 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->org1)));
	crd->org.unit2 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->org2)));
	crd->org.unit3 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->org3)));
	crd->org.unit4 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->org4)));
	
        _FREE(crd->comment.str);
        _FREE(crd->url.str);
	
	crd->comment.str = gtk_editable_get_chars(GTK_EDITABLE(ce->comment), 
						      0, gtk_text_get_length(GTK_TEXT(ce->comment)));
	crd->url.str     = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->url)));
	
        _FREE(crd->key.data);
	
	crd->key.data = gtk_editable_get_chars(GTK_EDITABLE(ce->key), 
						   0, gtk_text_get_length(GTK_TEXT(ce->key)));
	if (GTK_TOGGLE_BUTTON(ce->keypgp)->active)
	  crd->key.type = KEY_PGP;
	else
	  crd->key.type = KEY_X509;


	gnomecard_update_tree(crd);
	gnomecard_scroll_tree(ce->l);
	gnomecard_set_changed(TRUE);
}

void gnomecard_prop_close(GtkWidget *widget, gpointer node)
{
	((Card *) ((GList *) node)->data)->flag = FALSE;
	
	if ((GList *) node == curr_crd)
	  gnomecard_set_edit_del(TRUE);
}

void gnomecard_take_from_name(GtkWidget *widget, gpointer data)
{
        GnomeCardEditor *ce;
	char *name;
	
	ce = (GnomeCardEditor *) data;
	
	name = join_name(gtk_entry_get_text(GTK_ENTRY(ce->pre)),
			 gtk_entry_get_text(GTK_ENTRY(ce->given)),
			 gtk_entry_get_text(GTK_ENTRY(ce->add)),
			 gtk_entry_get_text(GTK_ENTRY(ce->fam)),
			 gtk_entry_get_text(GTK_ENTRY(ce->suf)));
	
	gtk_entry_set_text(GTK_ENTRY(ce->fn), name);
	
	g_free(name);
}
	
void gnomecard_edit(GList *node, int section)
{
	GnomePropertyBox *box;
	GnomeCardEditor *ce;
	GtkWidget *hbox, *hbox2, *vbox, *frame, *table;
	GtkWidget *label, *entry, *align, *align2, *pix;
	GtkWidget *radio1, *radio2, *button;
	GtkObject *adj;
	Card *crd;
	time_t tmp_time;
	
	ce = g_malloc(sizeof(GnomeCardEditor));
	ce->l = node;
	crd = (Card *) node->data;
	
	/* Set flag and disable Delete and Edit. */
	
	crd->flag = TRUE;
	gnomecard_set_edit_del(FALSE);
	
	box = GNOME_PROPERTY_BOX(gnome_property_box_new());
	gtk_object_set_user_data(GTK_OBJECT(box), (gpointer) ce);
	gtk_window_set_wmclass(GTK_WINDOW(box), "gnomecard",
			       "GnomeCard");
	gtk_signal_connect(GTK_OBJECT(box), "apply",
			   (GtkSignalFunc)gnomecard_prop_apply, NULL);
	gtk_signal_connect(GTK_OBJECT(box), "destroy",
			   (GtkSignalFunc)gnomecard_prop_close, node);
	
	/* Identity */
	
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Identity"));
	gnome_property_box_append_page(box, vbox, label);
	
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("Formatted Name:"));
	ce->fn = entry = my_gtk_entry_new(0, crd->fname.str);
	my_connect(entry, "changed", box, &crd->fname.prop);
	button = gtk_button_new_with_label(_("Take from Name"));
 	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(gnomecard_take_from_name),
			   ce);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	frame = gtk_frame_new(_("Name"));	
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	table = my_gtk_table_new(6, 2);
	gtk_container_add(GTK_CONTAINER(frame), table);
	label = gtk_label_new(_("Given:"));
	ce->given = entry = my_gtk_entry_new(12, crd->name.given);
	my_connect(entry, "changed", box, &crd->name.prop);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, 0, 0, 0, 0);
	label = gtk_label_new(_("Additional:"));
	ce->add = entry = my_gtk_entry_new(12, crd->name.additional);
	my_connect(entry, "changed", box, &crd->name.prop);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 0, 1, 0, 0, 0, 0);
	label = gtk_label_new(_("Family:"));
	ce->fam = entry = my_gtk_entry_new(15, crd->name.family);
	my_connect(entry, "changed", box, &crd->name.prop);
	gtk_table_attach(GTK_TABLE(table), label, 4, 5, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 5, 6, 0, 1, 0, 0, 0, 0);
	label = gtk_label_new(_("Prefix:"));
	ce->pre = entry = my_gtk_entry_new(5, crd->name.prefix);
	align = gtk_alignment_new(0.0, 0.0, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), entry);
	my_connect(entry, "changed", box, &crd->name.prop);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), align, 1, 2, 1, 2, 
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
			 0, 0);
	label = gtk_label_new(_("Suffix:"));
	ce->suf = entry = my_gtk_entry_new(5, crd->name.suffix);
	align = gtk_alignment_new(0.0, 0.0, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), entry);
	my_connect(entry, "changed", box, &crd->name.prop);
        gtk_table_attach(GTK_TABLE(table), label, 4, 5, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), align, 5, 6, 1, 2, 
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
			 0, 0);
	
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("Birthdate:"));
	
	if (crd->bday.prop.used) {
		struct tm tm = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		
		tm.tm_year = crd->bday.year - 1900;
		tm.tm_mon  = crd->bday.month - 1;
		tm.tm_mday = crd->bday.day;
		
		tmp_time = mktime(&tm);
	} else
	  tmp_time = time(NULL);
	
	ce->bday = entry = gnome_date_edit_new(tmp_time, FALSE, FALSE);
	my_connect(GNOME_DATE_EDIT(entry)->calendar, "day_selected",
		   box, &crd->bday.prop);
	my_connect(GNOME_DATE_EDIT(entry)->date_entry, "changed",
		   box, &crd->bday.prop);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Geographical */
	
	align2 = gtk_alignment_new(0.5, 0.5, 0, 0);
	hbox2 = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (align2), hbox2);
	label = gtk_label_new(_("Geographical"));
	gnome_property_box_append_page(box, align2, label);
	
	pix = gnome_pixmap_new_from_xpm_d (world_xpm);
	gtk_box_pack_start(GTK_BOX(hbox2), pix, FALSE, FALSE, 0);
	
	vbox = my_gtk_vbox_new();
	gtk_box_pack_start(GTK_BOX(hbox2), vbox, FALSE, FALSE, 0);
	frame = gtk_frame_new(_("Time Zone"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(hbox), GNOME_PAD_SMALL);
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_container_add(GTK_CONTAINER(frame), align);
	label = gtk_label_new(_("hrs."));
	adj = gtk_adjustment_new(crd->timezn.prop.used? 
				 crd->timezn.sign * crd->timezn.hours : 0,
				 -12, 12, 1, 1, 3);
	my_connect(adj, "value_changed", box, &crd->timezn.prop);
	ce->tzh = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 3);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(_("mins."));
	adj = gtk_adjustment_new(crd->timezn.prop.used? crd->timezn.mins : 0,
				 0, 59, 1, 1, 10);
	my_connect(adj, "value_changed", box, &crd->timezn.prop);
	ce->tzm = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 2);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		
	frame = gtk_frame_new(_("Geographic Position"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(hbox), GNOME_PAD_SMALL);
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_container_add(GTK_CONTAINER(frame), align);
	label = gtk_label_new(_("lat, "));
	adj = gtk_adjustment_new(crd->geopos.prop.used? crd->geopos.lat : 0,
				 -90, 90, .01, 1, 1);
	ce->gplat = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 5);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(entry), 2);
	my_connect(adj, "value_changed", box, &crd->geopos.prop);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(_("lon."));
	adj = gtk_adjustment_new(crd->geopos.prop.used? crd->geopos.lon : 0,
				 -180, 180, .01, 1, 1);
	ce->gplon = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 6);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(entry), 2);
	my_connect(adj, "value_changed", box, &crd->geopos.prop);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* Organization */
	
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Organization"));
	gnome_property_box_append_page(box, vbox, label);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("Title:"));
	ce->title = entry = my_gtk_entry_new(0, crd->title.str);
	my_connect(entry, "changed", box, &crd->title.prop);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	label = gtk_label_new(_("Role:"));
	ce->role = entry = my_gtk_entry_new(0, crd->role.str);
	my_connect(entry, "changed", box, &crd->role.prop);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	frame = gtk_frame_new(_("Organization"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	table = my_gtk_table_new(4, 5);
	gtk_container_add(GTK_CONTAINER(frame), table);
	label = gtk_label_new(_("Name:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	ce->orgn = entry = my_gtk_entry_new(0, crd->org.name);
	my_connect(entry, "changed", box, &crd->org.prop);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, 
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new(_("Units:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	ce->org1 = entry = my_gtk_entry_new(0, crd->org.unit1);
	my_connect(entry, "changed", box, &crd->org.prop);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 0, 1, 
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	ce->org2 = entry = my_gtk_entry_new(0, crd->org.unit2);
	my_connect(entry, "changed", box, &crd->org.prop);
	gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 1, 2,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	ce->org3 = entry = my_gtk_entry_new(0, crd->org.unit3);
	my_connect(entry, "changed", box, &crd->org.prop);
	gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 2, 3,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	ce->org4 = entry = my_gtk_entry_new(0, crd->org.unit4);
	my_connect(entry, "changed", box, &crd->org.prop);
	gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 4, 5,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);

	/* Explanatory */
	
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Explanatory"));
	gnome_property_box_append_page(box, vbox, label);
	
	label = gtk_label_new(_("Comment:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	ce->comment = entry = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_realize(entry);
	if (crd->comment.prop.used) {
		int pos = 0;
		gtk_editable_insert_text(GTK_EDITABLE(entry), crd->comment.str,
					 strlen(crd->comment.str), &pos);
	}
	gtk_widget_set_usize (entry, 0, (entry->style->font->ascent +
					 entry->style->font->descent) * 4);
	my_connect(entry, "changed", box, &crd->comment.prop);
	
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("URL:"));
	ce->url = entry = my_gtk_entry_new(0, crd->url.str);
	my_connect(entry, "changed", box, &crd->url.prop);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, GNOME_PAD);
	label = gtk_label_new(_("Last Revision:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(_("The last revision goes here."));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	/* Security */
	
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Security"));
	gnome_property_box_append_page(box, vbox, label);
	
	label = gtk_label_new(_("Public Key:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	ce->key = entry = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_realize(entry);
	if (crd->key.prop.used) {
		int pos = 0;
		gtk_editable_insert_text(GTK_EDITABLE(entry), crd->key.data,
					 strlen(crd->key.data), &pos);
	}
	gtk_widget_set_usize (entry, 0, (entry->style->font->ascent +
					 entry->style->font->descent) * 6);
	my_connect(entry, "changed", box, &crd->key.prop);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	ce->keypgp = radio1 = gtk_radio_button_new_with_label(NULL, _("PGP"));
	if (crd->key.prop.used && crd->key.type != KEY_PGP)
	  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(radio1), 0);
	my_connect(radio1, "toggled", box, &crd->key.prop);
	gtk_box_pack_start(GTK_BOX(hbox), radio1, FALSE, FALSE, 0);
	radio2 = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(radio1)),
		_("X509"));
	if (crd->key.prop.used && crd->key.type == KEY_X509)
	  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(radio2), 1);
	my_connect(radio2, "toggled", box, &crd->key.prop);
	gtk_box_pack_start(GTK_BOX(hbox), radio2, FALSE, FALSE, 0);

	gtk_widget_show_all(GTK_WIDGET(box));
}

void gnomecard_cancel(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(widget);
}

void gnomecard_save(void)
{
	GList *l;
	FILE *fp;

	fp = fopen (gnomecard_fname, "w");
	for (l = crds; l; l= l->next)
	  card_save((Card *) l->data, fp);
	fclose(fp);
	
	gnomecard_set_changed(FALSE);
}

void gnomecard_save_call(GtkWidget *widget, gpointer data)
{
	g_free(gnomecard_fname);
	gnomecard_fname = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget)));
	gtk_widget_destroy(widget);
	
	gnomecard_save();
}

void gnomecard_save_as(GtkWidget *widget, gpointer data)
{
	GtkWidget *fsel;
	
	fsel = gtk_file_selection_new(_("Save GnomeCard File As..."));
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->ok_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_save_call),
			   GTK_OBJECT(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_cancel),
			   GTK_OBJECT(fsel));
	gtk_widget_show(fsel);
}

/* Returns TRUE if the cards were destroyed. FALSE if canceled */
int gnomecard_destroy_cards(void)
{
	GList *l;
	
	if (gnomecard_changed) {
		GtkWidget *w;
		char *msg;
		char *tmp = N_(" changed. Save?");
		int len;
		
		len = strlen(tmp) + strlen(gnomecard_fname) + 1;
		msg = g_malloc(len);
		snprintf(msg, len, "%s%s", gnomecard_fname, tmp);
		 
		w = gnome_message_box_new(msg,
					  GNOME_MESSAGE_BOX_QUESTION,
					  GNOME_STOCK_BUTTON_YES,
					  GNOME_STOCK_BUTTON_NO,
					  GNOME_STOCK_BUTTON_CANCEL, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gtk_widget_show(w);
		
		switch(gnome_dialog_run_modal(GNOME_DIALOG(w))) {
		 case -1:
		 case 2:
			return FALSE;
		 case 1:
			break;
		 case 0:
			gnomecard_save();
		}
		
		g_free (msg);
	}
	
	for (l = crds; l; l = l->next)
	  card_free (l->data);
	
	gtk_ctree_remove(crd_tree,NULL);
	g_list_free(crds);
	crds = NULL;
	
	gnomecard_set_curr(NULL);
	gnomecard_set_changed(FALSE);
	
	return TRUE;
}

void gnomecard_new_card(GtkWidget *widget, gpointer data)
{
	Card *crd;
	GList *last;
	
	crd = card_new();
	gnomecard_add_card_to_tree(crd);
	crds = g_list_append(crds, crd);
	
	last = g_list_last(crds);
	gnomecard_edit(last, IDENT);
	gnomecard_scroll_tree(last);
	
	gnomecard_set_changed(TRUE);
	
}

void gnomecard_del_card(GtkWidget *widget, gpointer data)
{
	GList *tmp;
	
	if (curr_crd->next)
	  tmp = curr_crd->next;
	else
	  tmp = curr_crd->prev;
	
	card_free(curr_crd->data);
	crds = g_list_remove_link(crds, curr_crd);
	gtk_ctree_remove(crd_tree, ((Card *) curr_crd->data)->user_data);
	g_list_free(curr_crd);
	
	if (tmp)
	  gnomecard_scroll_tree(tmp);
	else
	  gnomecard_set_curr(NULL);
	
	gnomecard_set_changed(TRUE);
}

void gnomecard_edit_card(GtkWidget *widget, gpointer data)
{
	if (curr_crd)
	  gnomecard_edit(curr_crd, IDENT);
}

void gnomecard_first_card(GtkWidget *widget, gpointer data)
{
	gnomecard_scroll_tree(g_list_first(crds));
}

void gnomecard_prev_card(GtkWidget *widget, gpointer data)
{
	if (curr_crd->prev)
	  gnomecard_scroll_tree(curr_crd->prev);
}

void gnomecard_next_card(GtkWidget *widget, gpointer data)
{
	if (curr_crd->next)
	  gnomecard_scroll_tree(curr_crd->next);
}

void gnomecard_last_card(GtkWidget *widget, gpointer data)
{
	gnomecard_scroll_tree(g_list_last(crds));
}

void gnomecard_open_call(GtkWidget *widget, gpointer data)
{
	char *fname;
	GtkWidget *w;
	GList *c;
	
	fname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget));
	
	if (!(c = card_load(NULL, fname))) {
		w = gnome_message_box_new(_("Wrong file format."),
					  GNOME_MESSAGE_BOX_ERROR,
					  GNOME_STOCK_BUTTON_OK, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gtk_widget_show(w);
	} else {
		if (gnomecard_destroy_cards()) {
			crds = c;
			
			gtk_clist_freeze(GTK_CLIST(crd_tree));
			for (c = crds; c; c = c->next)
			  gnomecard_add_card_to_tree((Card *) c->data);
			gtk_clist_thaw(GTK_CLIST(crd_tree));
			
			g_free(gnomecard_fname);
			gnomecard_fname = g_strdup(fname);
			gnomecard_scroll_tree(g_list_first(crds));
			gtk_widget_destroy(widget);
		}
	}
}

void gnomecard_open(GtkWidget *widget, gpointer data)
{
	GtkWidget *fsel;
	
	fsel = gtk_file_selection_new(_("Open GnomeCard File"));
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->ok_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_open_call),
			   GTK_OBJECT(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_cancel),
			   GTK_OBJECT(fsel));
	gtk_widget_show(fsel);
}

void gnomecard_setup(GtkWidget *widget, gpointer data)
{
	g_warning ("Setup card.");
}

void gnomecard_quit(GtkWidget *widget, gpointer data)
{
	if (gnomecard_destroy_cards()) {
		gtk_widget_destroy(gnomecard_window);
		gtk_main_quit();
	}
}

gint gnomecard_delete(GtkWidget *w, GdkEvent *e, gpointer data)
{
	if (gnomecard_destroy_cards()) {
		gtk_widget_destroy(gnomecard_window);
		gtk_main_quit();
	}
	  
	return TRUE;
}

void
gnomecard_about(GtkWidget *widget, gpointer data)
{
        GtkWidget *about;
        gchar *authors[] = {
		"arturo@nuclecu.unam.mx",
		NULL
	};

        about = gnome_about_new (_("GnomeCard"), 0,
				 "(C) 1997-1998 the Free Software Fundation",
				 authors,
				 _("Electronic Business Card Manager"),
				 NULL);
        gtk_widget_show (about);
}

GnomeUIInfo filemenu[] = {
	{GNOME_APP_UI_ITEM, N_("New"), N_("Create new Card File."), 
		gnomecard_destroy_cards, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Open..."), N_("Open a Card File."), 
		gnomecard_open, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Save"), N_("Save current changes."), 
		gnomecard_save, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Save as..."), N_("Save current changes, choosing file name."), 
		gnomecard_save_as, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS, 0, 0, NULL},

	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Properties..."), N_("Appereance, customizations..."), gnomecard_setup, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Exit"), N_("Close the program."), 
		gnomecard_quit, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 0, 0, NULL},

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo cardmenu[] = {
	{GNOME_APP_UI_ITEM, N_("Add..."), N_("Create new card"),
	 gnomecard_new_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardNewMenu", 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Edit"), N_("Edit card"),
	 gnomecard_edit_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardEditMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("First"), N_("First card"),
	 gnomecard_first_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardFirstMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Prev"), N_("Previous card"),
	 gnomecard_prev_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Next"), N_("Next card"),
	 gnomecard_next_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FORWARD, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Last"), N_("Last card"),
	 gnomecard_last_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardLastMenu", 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Delete"), N_("Delete card"),
	 gnomecard_del_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo helpmenu[] = {
	{GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("About..."), N_("Version, credits, etc."), 
		gnomecard_about, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo mainmenu[] = {
	{GNOME_APP_UI_SUBTREE, N_("File"), NULL, filemenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_SUBTREE, N_("Card"), NULL, cardmenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_JUSTIFY_RIGHT},
		
	{GNOME_APP_UI_SUBTREE, N_("Help"), NULL, helpmenu, NULL, NULL,
	GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},

	{GNOME_APP_UI_ENDOFINFO}
};

GnomeUIInfo toolbar[] = {
	{GNOME_APP_UI_ITEM, N_("New"), N_("New file"), 
		gnomecard_destroy_cards, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Open"), N_("Open file"), 
		gnomecard_open, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 0, 0, NULL},

	{GNOME_APP_UI_ITEM, N_("Save"), N_("Save changes"), 
		gnomecard_save, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL},

	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Add"), N_("Create new card"),
	 gnomecard_new_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardNew", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Edit"), N_("Edit card"),
	 gnomecard_edit_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardEdit", 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("First"), N_("First card"),
	 gnomecard_first_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardFirst", 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Prev"), N_("Previous card"),
	 gnomecard_prev_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BACK, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Next"), N_("Next card"),
	 gnomecard_next_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_FORWARD, 0, 0, NULL},
	
	{GNOME_APP_UI_ITEM, N_("Last"), N_("Last card"),
	 gnomecard_last_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, "GnomeCardLast", 0, 0, NULL},
	
	{GNOME_APP_UI_SEPARATOR},
	
	{GNOME_APP_UI_ITEM, N_("Delete"), N_("Delete card"),
	 gnomecard_del_card, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
};

int main (int argc, char *argv[])
{
	GnomeCanvasGroup *root;
	GtkWidget *vbox, *canvas, *align;
	char *titles[] = { N_("Field"), N_("Value")};

	gnome_init("gnomecard", NULL, argc, argv, 0, NULL);
        gdk_imlib_init ();
	gnomecard_init_stock();
	gnomecard_init_pixes();
	textdomain(PACKAGE);
	
	gnomecard_window = gnome_app_new("GnomeCard", "GnomeCard");
	gtk_window_set_wmclass(GTK_WINDOW(gnomecard_window), "gnomecard",
			       "GnomeCard");

	gtk_widget_show(gnomecard_window);
	gtk_signal_connect(GTK_OBJECT(gnomecard_window), "delete_event",
			   GTK_SIGNAL_FUNC(gnomecard_delete), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	gnome_app_set_contents(GNOME_APP(gnomecard_window), vbox);
	gnome_app_create_menus(GNOME_APP(gnomecard_window), mainmenu);
	gnome_app_create_toolbar(GNOME_APP(gnomecard_window), toolbar);

	canvas =
	  crd_canvas = gnome_canvas_new(gtk_widget_get_default_visual(),
					gtk_widget_get_default_colormap());
	gnome_canvas_set_size(GNOME_CANVAS(canvas), 
			      CANVAS_WIDTH, CANVAS_HEIGHT);

	root = GNOME_CANVAS_GROUP (gnome_canvas_root(GNOME_CANVAS(canvas)));
	gnome_canvas_item_new (GNOME_CANVAS (canvas),
			       root,
			       gnome_canvas_rect_get_type (),
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (double) (CANVAS_WIDTH - 1),
			       "y2", (double) (CANVAS_HEIGHT - 1),
			       "fill_color", "white",
			       "outline_color", "black",
			       "width_pixels", 0,
			       NULL);
	test = gnome_canvas_item_new (GNOME_CANVAS(canvas), root,
				      gnome_canvas_text_get_type (),
				      "text", "",
				      "x", CANVAS_WIDTH / 2.0,
				      "y", CANVAS_HEIGHT / 2.0,
				      "font", CANVAS_FONT,
				      "anchor", GTK_ANCHOR_CENTER,
				      "fill_color", "black",
				      NULL);
	
	gtk_widget_show(canvas);

	align = gtk_alignment_new(0.5, 0.5, 0, GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (align), canvas);
	gtk_widget_show(align);
	gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, GNOME_PAD_SMALL);

	crd_tree = GTK_CTREE(gtk_ctree_new_with_titles(2, 0, titles));
	gtk_clist_set_column_width(GTK_CLIST(crd_tree), 0, COL_WIDTH);
	gtk_signal_connect(GTK_OBJECT(crd_tree), 
			   "tree_select_row",
			   GTK_SIGNAL_FUNC(gnomecard_tree_selected), NULL);
	gtk_ctree_set_line_style (crd_tree, GTK_CTREE_LINES_SOLID);
	gtk_ctree_set_reorderable (crd_tree, FALSE);
	gtk_clist_column_titles_passive(GTK_CLIST(crd_tree));
	gtk_clist_set_policy(GTK_CLIST(crd_tree),
			     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize (GTK_WIDGET(crd_tree), 0, 200);
	
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(crd_tree), TRUE, TRUE, 0);
	gtk_widget_show(GTK_WIDGET(crd_tree));

	crds = NULL;
	
	tb_save = toolbar[2].widget;
	tb_edit = toolbar[5].widget;
	tb_first = toolbar[7].widget;
	tb_prev = toolbar[8].widget;
	tb_next = toolbar[9].widget;
	tb_last = toolbar[10].widget;
	tb_del  = toolbar[12].widget;
	
	menu_save = filemenu[2].widget;
	menu_edit = cardmenu[1].widget;
	menu_first = cardmenu[3].widget;
	menu_prev = cardmenu[4].widget;
	menu_next = cardmenu[5].widget;
	menu_last = cardmenu[6].widget;
	menu_del  = cardmenu[8].widget;

	gnomecard_fname = g_strdup("untitled.gcrd");
	gnomecard_set_changed(FALSE);
	gnomecard_set_curr(NULL);
	
	gtk_main();
	return 0;
}
