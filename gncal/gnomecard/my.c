#include <config.h>
#include <gnome.h>

#include "gnomecard.h"
#include "my.h"

#ifndef B4MSF
extern GtkCTreeNode *
my_gtk_ctree_insert(GtkCTreeNode *parent, GtkCTreeNode *sibling, 
		    char **text, pix *p, gpointer data)
{
    g_message("in my_gtk_ctree_insert - shouldnt be!");
}
#else
extern GtkCTreeNode *
my_gtk_ctree_insert(GtkCTreeNode *parent, GtkCTreeNode *sibling, 
		    char **text, pix *p, gpointer data)
{
	GtkCTreeNode *node;
	
	node =  gtk_ctree_insert_node(gnomecard_tree, parent, sibling, text, TREE_SPACING,
				      p->pixmap, p->mask, p->pixmap, p->mask, 
				      FALSE, FALSE);
	gtk_ctree_node_set_row_data(gnomecard_tree, node, data);
	
	return node;
}
#endif

extern GtkWidget *my_gtk_entry_new(gint len, char *init)
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

extern GtkWidget *my_hbox_entry(GtkWidget *parent, char *label, char *init)
{
	GtkWidget *hbox, *w;

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);
	w = gtk_label_new(label);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
	w = my_gtk_entry_new(0, init);
	gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 0);
	
	return w;
}

extern GtkWidget *my_gtk_spin_button_new(GtkAdjustment *adj, gint width)
{
	GtkWidget *spin;
	
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(spin), TRUE);			      
	gtk_widget_set_usize (spin, /*22 + 8 * width, 0);*/
		    gdk_char_width (spin->style->font, '-') * width + 22, 0);
			      
	return spin;
}

extern GtkWidget *my_gtk_vbox_new(void)
{
	GtkWidget *vbox;
	
	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD_SMALL);

	return vbox;
}

extern GtkWidget *my_gtk_table_new(int x, int y)
{
	GtkWidget *table;
	
	table = gtk_table_new(x, y, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(table), GNOME_PAD_SMALL);
	
	return table;
}

void gnomecard_property_used(GtkWidget *w, gpointer data)
{
	CardProperty *prop;
	
	prop = (CardProperty *) data;
	prop->type = (int) gtk_object_get_user_data(GTK_OBJECT(w));
	prop->used = TRUE;
}

extern void my_connect(gpointer widget, char *sig, gpointer box, 
		       CardProperty *prop, enum PropertyType type)
{
 	gtk_signal_connect_object(GTK_OBJECT(widget), sig,
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_signal_connect(GTK_OBJECT(widget), sig,
			   GTK_SIGNAL_FUNC(gnomecard_property_used),
			   prop);
	gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer) type);
}
