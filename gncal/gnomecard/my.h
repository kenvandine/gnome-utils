#ifndef __GNOMECARD_MY
#define __GNOMECARD_MY

#include <gnome.h>

#include "card.h"
#include "pix.h"

#define MY_FREE(a) if (a) g_free (a)
#define MY_STRLEN(x) (x?strlen(x):0)
#define MY_STRDUP(x) (*x?g_strdup(x):NULL)

extern GtkCTreeNode *
my_gtk_ctree_insert(GtkCTree *ctree, GtkCTreeNode *parent,
		    GtkCTreeNode *sibling, char **text, pix *p);
extern GtkWidget *my_gtk_entry_new(gint len, char *init);
extern GtkWidget *my_hbox_entry(GtkWidget *parent, char *label, char *init);
extern GtkWidget *my_gtk_spin_button_new(GtkAdjustment *adj, gint width);
extern GtkWidget *my_gtk_vbox_new(void);
extern GtkWidget *my_gtk_table_new(int x, int y);
extern void my_connect(gpointer widget, char *sig, gpointer box, 
		       CardProperty *prop, enum PropertyType type);

#endif
