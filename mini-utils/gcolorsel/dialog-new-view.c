#include "gcolorsel.h"
#include "dialogs.h"
#include "mdi-color-generic.h"

#include "gnome.h"
#include "glade/glade.h"

GtkCList *clist_doc;
GtkCList *clist_view;
GtkWidget *dia;

static void
dialog_fill_clist_doc (GtkCList *clist)
{
  GList *list = mdi->children;
  MDIColorGeneric *mcg;
  char *str[1];
  int pos;

  gtk_clist_freeze (clist);

  while (list) {
    mcg = list->data;

    if (! mcg->temp) {
      
      str[0] = g_strdup_printf ("%s (%s)", mcg->name,
			      gtk_type_name (GTK_OBJECT_TYPE (mcg)));
      
      pos = gtk_clist_append (clist, str);
      gtk_clist_set_row_data (clist, pos, mcg);
      
      g_free (str[0]);
    }
     
    list = g_list_next (list);
  }

  gtk_clist_sort (clist);
  gtk_clist_thaw (clist);
}

static void 
dialog_fill_clist_view (GtkCList *clist)
{
  int i = 0;
  int pos;
  char *str[1];

  gtk_clist_freeze (clist);

  while (views_tab[i].name) {
    str[0] = views_tab[i].name;

    pos = gtk_clist_append (clist, str);

    gtk_clist_set_row_data (clist, pos, &views_tab[i]);
    
    if (!i) gtk_clist_select_row (clist, pos, 0); /* Select default */

    i++;
  }

  gtk_clist_sort (clist);
  gtk_clist_thaw (clist);
}

static void
clist_select (GtkWidget *widget, gint row, gint column, gpointer data)
{
  gnome_dialog_set_sensitive (GNOME_DIALOG (dia), 0, 
		     ((clist_doc->selection) && (clist_view->selection)));
}

static void
create_view (void)
{
  views_t *views;
  MDIColorGeneric *mcg;

  g_assert (clist_view->selection != NULL);
  g_assert (clist_doc->selection != NULL);

  views = gtk_clist_get_row_data (clist_view,
				GPOINTER_TO_INT (clist_view->selection->data));
  g_assert (views != NULL);

  mcg = gtk_clist_get_row_data (clist_doc,
				GPOINTER_TO_INT (clist_doc->selection->data));
  g_assert (mcg != NULL);

  mdi_color_generic_append_view_type (mcg, views->type ());
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (mcg));
}

void
dialog_new_view (void)
{
  GladeXML *gui;
  int result;

  gui = glade_xml_new (GCOLORSEL_GLADEDIR "dialog-new-view.glade", NULL);
  g_assert (gui != NULL);

  dia = glade_xml_get_widget (gui, "dialog-new-view");
  g_assert (dia != NULL);

  clist_doc = GTK_CLIST (glade_xml_get_widget (gui, "clist-document"));
  clist_view = GTK_CLIST (glade_xml_get_widget (gui, "clist-view"));

  gtk_object_unref (GTK_OBJECT (gui));

  dialog_fill_clist_doc (clist_doc);
  dialog_fill_clist_view (clist_view);

  gtk_signal_connect (GTK_OBJECT (clist_doc), "select_row",
		      GTK_SIGNAL_FUNC (clist_select), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_doc), "unselect_row",
		      GTK_SIGNAL_FUNC (clist_select), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_view), "select_row",
		      GTK_SIGNAL_FUNC (clist_select), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_view), "unselect_row",
		      GTK_SIGNAL_FUNC (clist_select), NULL);

  gnome_dialog_set_sensitive (GNOME_DIALOG (dia), 0, FALSE);

  while (1) {
    result = gnome_dialog_run (GNOME_DIALOG (dia));

    if (result == 2) break;
    
    if (result == 1) { /* FIXME : Help */
    }

    if (result == 0) { /* Ok */
      create_view ();
      break;
    }
  } 

  gnome_dialog_close (GNOME_DIALOG (dia));
}
