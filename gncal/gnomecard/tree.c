#include <config.h>
#include <string.h>
#include <gnome.h>

#include "card.h"
#include "gnomecard.h"
#include "my.h"
#include "pix.h"
#include "sort.h"
#include "tree.h"
#include "type_name.h"

/* I'm using collapse and expand to avoid ctree_remove SIGSEGV */
extern void gnomecard_update_tree(Card *crd)
{
	GtkCTreeNode *node, *tmp;
	
	gnomecard_tree_set_node_info(crd);
	gtk_ctree_collapse_recursive(gnomecard_tree, crd->user_data);
	
	/* FIXME: the gtkctree API is broken. This is a workaround.
	 * GTK_CTREE_NODE_CHILDREN should exist. */
	node = GTK_CTREE_ROW(&GTK_CTREE_NODE(crd->user_data)->list)->children;
	while (node) {
		tmp = GTK_CTREE_NODE_NEXT (node);
		gtk_ctree_remove_node(gnomecard_tree, node);
		node = tmp;
	}
	
	gnomecard_add_card_sections_to_tree(crd);
	gnomecard_tree_set_sorted_pos(crd);
}

extern void gnomecard_scroll_tree(GList *node)
{
	GtkCTreeNode *tree_node;
	
	tree_node = ((Card *) node->data)->user_data;

	gtk_ctree_node_moveto(gnomecard_tree, tree_node, 0, 0, 0);
	gtk_ctree_select(gnomecard_tree, tree_node);
}

static char *gnomecard_first_phone_str(GList *phone)
{
	char *ret;
	
	if (! phone)
	  return NULL;
	
	ret = ((CardPhone *) phone->data)->data;
	
	for ( ; phone; phone = phone->next)
	  if (((CardPhone *) phone->data)->type & PHONE_PREF)	
	    ret = ((CardPhone *) phone->data)->data;
	
	return ret;
}

extern void gnomecard_tree_set_node_info(Card *crd)
{
	char *text, *tmp;
	gint len;
	
	if ((text = crd->fname.str) == NULL)
	  text = _("No Card Name for this card.");
	
	gtk_ctree_set_node_info(gnomecard_tree, crd->user_data, text,
				TREE_SPACING, crd_pix->pixmap,
				crd_pix->mask, crd_pix->pixmap,
				crd_pix->mask, FALSE, FALSE);

	text = malloc(1);
	*text = 0;
	len = 1;
	if (crd->phone && (gnomecard_def_data & PHONE)) {
	  tmp = gnomecard_first_phone_str(crd->phone);
		len += strlen(tmp) + 1;
		text = realloc(text, len);
		snprintf(text, len, "%s %s", text, tmp);
	}
	if (crd->email && (gnomecard_def_data & EMAIL)) {
		tmp = ((CardEMail *) (crd->email->data))->data;
		len += strlen(tmp) + 1;
		text = realloc(text, len);
		snprintf(text, len, "%s %s", text, tmp);
	}

	gtk_ctree_node_set_text(gnomecard_tree, crd->user_data, 1, text);
	
	g_free(text);
}

static int gnomecard_next_addr_type(int type, int start)
{
	int j;
	
	for (j = start; j < 6; j++)
		if (type & (1 << j))
			return j;
	
	return j;
}

extern void gnomecard_add_card_sections_to_tree(Card *crd)
{
	char *text[2];
	GtkCTreeNode *parent, *child;
	
	parent = crd->user_data;
	child = parent;
	
	text[0] = _("Name");
	text[1] = gnomecard_join_name(crd->name.prefix, crd->name.given, 
				      crd->name.additional, crd->name.family, 
				      crd->name.suffix);
	if (*text[1])
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, ident_pix);
	g_free(text[1]);
	
	if (crd->bday.prop.used) {
		text[0] = _("Birth Date");
		text[1] = card_bday_str(crd->bday);
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, ident_pix);
		free(text[1]);
	}
	
	if (crd->timezn.prop.used) {
		text[0] = _("Time Zone");
		text[1] = card_timezn_str(crd->timezn);
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, geo_pix);
		free(text[1]);
		}

	if (crd->geopos.prop.used) {
		text[0] = _("Geo. Position");
		text[1] = card_geopos_str(crd->geopos);
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, geo_pix);
		free(text[1]);
	}
	
	if (crd->title.prop.used) {
		text[0] = _("Title");
		text[1] = crd->title.str;
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, org_pix);
	}
	if (crd->role.prop.used) {
		text[0] = _("Role");
		text[1] = crd->role.str;
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, org_pix);
	}
	
	if (crd->org.prop.used) {
		GtkCTreeNode *org;
		
		text[0] = _("Organization");
		if (crd->org.name)
			text[1] = crd->org.name;
		else
			text[1] = "";
		
		org = my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, org_pix);
		
		if (crd->org.unit1) {
			text[0] = _("Unit 1");
			text[1] = crd->org.unit1;
			my_gtk_ctree_insert(gnomecard_tree, org, NULL, text, null_pix);
		}
		
		if (crd->org.unit2) {
			text[0] = _("Unit 2");
			text[1] = crd->org.unit2;
			my_gtk_ctree_insert(gnomecard_tree, org, NULL, text, null_pix);
	}
		
		if (crd->org.unit3) {
			text[0] = _("Unit 3");
			text[1] = crd->org.unit3;
			my_gtk_ctree_insert(gnomecard_tree, org, NULL, text, null_pix);
		}
		
		if (crd->org.unit4) {
			text[0] = _("Unit 4");
			text[1] = crd->org.unit4;
			my_gtk_ctree_insert(gnomecard_tree, org, NULL, text, null_pix);
		}
	}
	
	if (crd->comment.prop.used) {
		char *rem, *c;
		GtkCTreeNode *comment;
		
		text[0] = _("Comment");
		text[1] = "";
		comment = my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, expl_pix);
		
		text[0] = "";
		c = rem = text[1] = g_strdup(crd->comment.str);
		while ((c = strchr(text[1], '\n')) != NULL) {
			*c = 0;
			my_gtk_ctree_insert(gnomecard_tree, comment, NULL, text, null_pix);
			text[1] = c + 1;
		}
		
		if (*text[1])
			my_gtk_ctree_insert(gnomecard_tree, comment, NULL, text, null_pix);
	}
	if (crd->url.prop.used) {
		text[0] = _("URL");
		text[1] = crd->url.str;
		my_gtk_ctree_insert(gnomecard_tree, child, NULL, text, null_pix);
	}

	if (crd->key.prop.used) {
		text[0] = _("Security");
		text[1] = "";
		my_gtk_ctree_insert(gnomecard_tree, parent, NULL, text, sec_pix);
	}
	
	if (crd->phone) {
		GtkCTreeNode *phone, *phone2;
		GList *l;
		int i, len;
		
		text[0] = _("Telephone Numbers");
		text[1] = gnomecard_first_phone_str(crd->phone);
		if (! crd->phone->next)
			phone = parent;
		else
			phone = my_gtk_ctree_insert(gnomecard_tree, parent, NULL, text, phone_pix);
		
		for (l = crd->phone; l; l = l->next) {
			text[0] = ((CardPhone *)l->data)->data;
			text[1] = malloc(1);
			*text[1] = 0;
			len = 1;
			for (i = 0; i < 13; i++)
			  if (((CardPhone *)l->data)->type & (1 << i)) {
				  len += strlen(phone_type_name[i]) + 1;
				  text[1] = realloc(text[1], len);
				  snprintf(text[1], len, "%s %s", text[1], 
					   phone_type_name[i]);
			  }
			
			phone2 = my_gtk_ctree_insert(gnomecard_tree, phone, NULL, text, phone_pix);
			free(text[1]);
		}
	}
	
	if (crd->email) {
		GtkCTreeNode *email;
		GList *l;
		
		text[0] = _("E-mail Addresses");
		text[1] = "";
		
		if (! crd->email->next)
			email = parent;
		else
			email = my_gtk_ctree_insert(gnomecard_tree, parent, NULL, text, email_pix);
		
		for (l = crd->email; l; l = l->next) {
			text[0] = email_type_name[((CardEMail *)l->data)->type];
			text[1] = ((CardEMail *)l->data)->data;
			my_gtk_ctree_insert(gnomecard_tree, email, NULL, text, email_pix);
		}
	}
	
	if (crd->deladdr || crd->dellabel) {
		GtkCTreeNode *addr, *addr2;
		GList *l;
		int i, j, k;
		
		text[0] = _("Delivery Addresses");
		text[1] = "";
		
		/* fixme: not handling dellabel case. */
		
		if (! crd->deladdr->next)
			addr = parent;
		else
			addr = my_gtk_ctree_insert(gnomecard_tree, parent, NULL, text, addr_pix);
		
		for (l = crd->deladdr; l; l = l->next) {
			CardDelAddr *deladdr;
			
			text[0] = "";
			text[1] = NULL;
			deladdr = ((CardDelAddr *)l->data);
			
			for (i = 0; i < DELADDR_MAX; i++)
				if (deladdr->data[i]) {
					text[1] = deladdr->data[i];
					break;
				}
			
			if (text[1]) {
				if ((j = gnomecard_next_addr_type(deladdr->type, 0)) < 6) {
					text[0] = addr_type_name[j];
					k = j + 1;
				} else
					text[0] = "";
				
				addr2 = my_gtk_ctree_insert(gnomecard_tree, addr, NULL, text, addr_pix);
				
				for (i = 0; i < DELADDR_MAX; i++)
					if (deladdr->data[i] && text[1] != deladdr->data[i]) {
						
						if ((j = gnomecard_next_addr_type(deladdr->type, k)) < 6) {
							text[0] = addr_type_name[j];
							k = j + 1;
						} else
							text[0] = "";
						
						text[1] = deladdr->data[i];
						my_gtk_ctree_insert(gnomecard_tree, addr2, NULL, text, null_pix);
					}
			}
		}
	}
}

extern void gnomecard_add_card_to_tree(Card *crd)
{
	char *text[2];
	
	text[0] = text[1] = "";
	
	crd->user_data = gtk_ctree_insert_node(gnomecard_tree, NULL, NULL, text,
																		0, NULL, NULL, NULL, NULL, FALSE, FALSE);
	
	gnomecard_tree_set_node_info(crd);
	gnomecard_add_card_sections_to_tree(crd);
}

extern void gnomecard_tree_set_sorted_pos(Card *crd)
{
	gtk_ctree_move(gnomecard_tree, crd->user_data,
		       NULL, gnomecard_search_sorted_pos(crd));
}
