#include "mdi-color-generic.h"
#include "view-color-generic.h"
#include "view-color-list.h"
#include "view-color-grid.h"
#include "view-color-edit.h"
#include "menus.h"

#include <gnome.h>
#include <glade/glade.h>

static void mdi_color_generic_class_init       (MDIColorGenericClass *class);
static void mdi_color_generic_init             (MDIColorGeneric *mcg);
static void mdi_color_generic_destroy          (GtkObject *);
static void mdi_color_generic_document_changed (MDIColorGeneric *mcg,
						gpointer data);
static GtkWidget *
mdi_color_generic_create_view                  (MDIColorGeneric *child);

static GList *mdi_color_generic_get_append_pos   (MDIColorGeneric *mcg, 
						MDIColorGenericCol *col);
static void mdi_color_generic_free_col         (MDIColorGenericCol *col);
static GList *mdi_color_generic_find_col       (MDIColorGeneric *mcg, int pos);

static gpointer
mdi_color_generic_get_control       (MDIColorGeneric *vcg, GtkVBox *box,
				     void (*changed_cb)(gpointer data), 
				     gpointer change_data);
static void mdi_color_generic_apply (MDIColorGeneric *mcg, gpointer data);
static void mdi_color_generic_close (MDIColorGeneric *mcg, gpointer data);
static void mdi_color_generic_sync  (MDIColorGeneric *mcg, gpointer data);

enum {
  DOCUMENT_CHANGED,
  LAST_SIGNAL
};

static guint mcg_signals [LAST_SIGNAL] = { 0 };

static GnomeMDIChildClass *parent_class = NULL;

guint 
mdi_color_generic_get_type()
{
  static guint mdi_gen_child_type = 0;
  
  if (!mdi_gen_child_type) {
    GtkTypeInfo mdi_gen_child_info = {
     "MDIColorGeneric",
      sizeof (MDIColorGeneric),
      sizeof (MDIColorGenericClass),
      (GtkClassInitFunc) mdi_color_generic_class_init,
      (GtkObjectInitFunc) mdi_color_generic_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    mdi_gen_child_type = gtk_type_unique (gnome_mdi_child_get_type (), 
					  &mdi_gen_child_info);
  }
  
  return mdi_gen_child_type;
}

static void 
mdi_color_generic_class_init (MDIColorGenericClass *class)
{
  GtkObjectClass      *object_class;
  GnomeMDIChildClass  *mdi_child_class;

  object_class          = GTK_OBJECT_CLASS (class);
  mdi_child_class       = GNOME_MDI_CHILD_CLASS (class);
  parent_class          = gtk_type_class (gnome_mdi_child_get_type());
  object_class->destroy = mdi_color_generic_destroy;

  mcg_signals [DOCUMENT_CHANGED] = 
    gtk_signal_new ("document_changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (MDIColorGenericClass, document_changed),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, mcg_signals, LAST_SIGNAL);


  mdi_child_class->create_view = (GnomeMDIChildViewCreator)mdi_color_generic_create_view;

  class->document_changed = mdi_color_generic_document_changed;
  class->get_control      = mdi_color_generic_get_control;
  class->apply            = mdi_color_generic_apply;
  class->close            = mdi_color_generic_close;
  class->sync             = mdi_color_generic_sync;
}

static void
mdi_color_generic_init (MDIColorGeneric *mcg)
{
  mcg->freeze_count = 0;
  mcg->last         = -1;
  mcg->col          = NULL;
  mcg->changes      = NULL;
  mcg->other_views  = NULL;
  mcg->parents      = NULL;

  mcg->get_owner        = NULL;
  mcg->get_append_pos   = NULL;
  mcg->get_control_type = NULL;

  mcg->flags = CHANGE_APPEND | CHANGE_REMOVE | CHANGE_NAME | CHANGE_POS
                     | CHANGE_RGB | CHANGE_CLEAR;
  
  mcg->next_view_type = view_color_list_get_type ();

  gnome_mdi_child_set_menu_template (GNOME_MDI_CHILD (mcg), mdi_menu);
}

static void 
mdi_color_generic_destroy (GtkObject *obj)
{
  //MDIColorGeneric *child = MDI_COLOR_GENERIC(obj);
  
  if(GTK_OBJECT_CLASS(parent_class)->destroy)
    (* GTK_OBJECT_CLASS(parent_class)->destroy)(obj);
}

static void
mdi_color_generic_set_all_color_change (MDIColorGeneric *mcg, int val)
{
  GList *list = mcg->col;
  MDIColorGenericCol *col;

  while (list) {
    col = list->data;
    col->change = val;
    list = g_list_next (list);
  }
}

static void
mdi_color_generic_view_realize (GtkWidget *widget, ViewColorGeneric *view)
{
  /* Quick hack. Send all the color to the newly created view.
     Humm ... col->change should be null, so no problem with that */

  mdi_color_generic_set_all_color_change (view->mcg, CHANGE_APPEND);
  view_color_generic_data_changed (view, view->mcg->col);
  mdi_color_generic_set_all_color_change (view->mcg, 0);

  gtk_signal_disconnect_by_func (GTK_OBJECT (widget), 
				 mdi_color_generic_view_realize, view);
}

static GtkWidget * 
mdi_color_generic_create_view (MDIColorGeneric *child)
{
  GtkWidget *vbox;
  GtkWidget *sw = NULL;
  ControlGeneric *control = NULL;
  ViewColorGeneric *view = NULL;
  int type;

  vbox = gtk_vbox_new (FALSE, 0);

  /* Create control */

  type = mdi_color_generic_get_control_type (child);
  if (type) {
    control = CONTROL_GENERIC (gtk_type_new (mdi_color_generic_get_control_type (child)));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (control), FALSE, FALSE, 0);
  
    control_generic_assign (control, child);
  }

  /* Create scrolled window, if needed and view*/

  if (child->next_view_type == TYPE_VIEW_COLOR_LIST) { /* List */
    sw = gtk_scrolled_window_new (NULL, NULL);  
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_ALWAYS);

    view = VIEW_COLOR_GENERIC (view_color_list_new (child)); 
  } else 
    if (child->next_view_type == TYPE_VIEW_COLOR_GRID) { /* Grid */
      sw = gtk_scrolled_window_new (NULL, NULL);  
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_AUTOMATIC);
      view = VIEW_COLOR_GENERIC (view_color_grid_new (child)); 
    } else
      if (child->next_view_type == TYPE_VIEW_COLOR_EDIT) {
	view = VIEW_COLOR_GENERIC (view_color_edit_new (child));
      } else
	g_assert_not_reached ();

  /* Pack view */

  if (sw) {
    gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (sw), view->widget);
  } else
    gtk_box_pack_start (GTK_BOX (vbox), view->widget, TRUE, TRUE, 0);

  /* Set view */

  gtk_object_set_data (GTK_OBJECT (vbox), "view_object", view);

  view->control = control ? CONTROL_GENERIC (control) : NULL;
  view->show_control = control ? TRUE : FALSE;

  gtk_signal_connect_after (GTK_OBJECT (view->widget), "realize",
		      GTK_SIGNAL_FUNC (mdi_color_generic_view_realize), view);

  /* Sync control */

  
  if (control) 
    control_generic_sync (CONTROL_GENERIC (control));

  gtk_widget_show_all (vbox);

  return vbox;
}

gboolean 
mdi_color_generic_can_do (MDIColorGeneric *mcg, 
			  MDIColorGenericChangeType what)
{
  return mcg->flags & what;
}

void
mdi_color_generic_freeze (MDIColorGeneric *mcg)
{
  GList *list;

  mcg->freeze_count++;

  list = mcg->other_views;
  while (list) {
    mdi_color_generic_freeze (list->data);
    list = g_list_next (list);
  }
}

void
mdi_color_generic_thaw (MDIColorGeneric *mcg)
{
  GList *list;

  if (!mcg->freeze_count) return;

  mcg->freeze_count--;

  if (!mcg->freeze_count) 
    mdi_color_generic_dispatch_changes (mcg);  

  list = mcg->other_views;
  while (list) {
    mdi_color_generic_thaw (list->data);
    list = g_list_next (list);
  }
}

static void 
mdi_color_generic_document_changed (MDIColorGeneric *mcg, gpointer data)
{
//  g_assert_not_reached ();
}

static GList*
mdi_color_generic_find_col (MDIColorGeneric *mcg, int pos)
{
  MDIColorGenericCol *col;
  GList *list = mcg->col;

  while (list) {
    col = list->data;
    if (col->pos == pos) return list;
    list = g_list_next (list);
  }

  return NULL;
}

static void 
append_change (MDIColorGeneric *mcg, MDIColorGenericCol *col)
{
  if (mcg->changes) { /* 1 or more in list */
    g_list_append (mcg->last_changes, col);
    mcg->last_changes = mcg->last_changes->next;
  } else  /* Empty */
    mcg->changes = mcg->last_changes = g_list_append (mcg->changes, col);  
}

void
mdi_color_generic_post_change (MDIColorGeneric *mcg, MDIColorGenericCol *col,
			       MDIColorGenericChangeType type)
{
  /* 
     Optimisation : Ca ne sert à rien d'ajouter ET de supprimer une couleur
                    Ca ne sert à rien d'ajouter ET de modifier une couleur 
                    Ca ne sert à rien de modifier ET de supprimer une couleur 
  */

  if (type == CHANGE_CLEAR) {
    g_list_free (mcg->changes);
    col = g_new0 (MDIColorGenericCol, 1);
    col->change = CHANGE_CLEAR;
    mcg->changes = mcg->last_changes = NULL;
    append_change (mcg, col);
  } else {
    if (! col->change) {
      append_change (mcg, col);
      col->change = type;
    } else {
      col->change = col->change | type;  
      
      if (col->change & CHANGE_APPEND) {
	if (col->change & CHANGE_REMOVE) { /* 1 */
	  if (mcg->last_changes->data == col) {
	    GList *prev = mcg->last_changes->prev;
	    g_list_remove (mcg->last_changes, col);
	    mcg->last_changes = prev;

	    if (! mcg->last_changes) mcg->changes = NULL;
	  } else
	    mcg->changes = g_list_remove (mcg->changes, col);
	  return;
	}
	
	col->change = CHANGE_APPEND; /* 2 */
      } else 
	if (col->change & CHANGE_REMOVE) col->change = CHANGE_REMOVE; /* 3 */
    }    
  }

  if (!mcg->freeze_count) 
    mdi_color_generic_dispatch_changes (mcg);
}

void
mdi_color_generic_dispatch_changes (MDIColorGeneric *mcg)
{
  MDIColorGenericCol *col;
  ViewColorGeneric *view;
  GList *list;

  if (!mcg->changes) return;

  list = GNOME_MDI_CHILD (mcg)->views;
  while (list) {
    view = gtk_object_get_data (GTK_OBJECT (list->data), "view_object");
    if (view) 
      view_color_generic_data_changed (view, mcg->changes);

    list = g_list_next (list);
  }
  
  list = mcg->other_views;
  while (list) {
    gtk_signal_emit_by_name (GTK_OBJECT (list->data), "document_changed", 
			     mcg->changes);

    list = g_list_next (list);
  }

  list = mcg->changes;
  while (list) {
    col = list->data;

    if ((col->change & CHANGE_REMOVE) || (col->change & CHANGE_CLEAR))
      mdi_color_generic_free_col (col);
    else
      col->change = 0;

    list = g_list_next (list);
  }

  g_list_free (mcg->changes);
  mcg->changes = mcg->last_changes = NULL;
}

static void
mdi_color_generic_free_col (MDIColorGenericCol *col)
{
  g_free (col->name);
  g_free (col);
}

MDIColorGenericCol *
mdi_color_generic_append_new (MDIColorGeneric *mcg,
			      int r, int g, int b, char *name, 
			      gpointer data)
{
  MDIColorGenericCol *col;

  col = g_new0 (MDIColorGenericCol, 1);

  col->r      = r;
  col->g      = g;
  col->b      = b;
  col->name   = g_strdup (name);
  col->change = 0;
  col->data   = data;
  col->owner  = mcg;

  mdi_color_generic_append (mcg, col);

  return col;
}

void
mdi_color_generic_append (MDIColorGeneric *mcg, MDIColorGenericCol *col)
{
  MDIColorGenericCol *col2;
  GList *pos;

  mcg->last++;

  if (! mcg->col) { /* Empty */
    mcg->col = mcg->last_col = g_list_append (NULL, col);
    col->pos = 0;
  } else {
    pos = mdi_color_generic_get_append_pos (mcg, col);
    if (! pos) /* append last */ {
      g_list_append (mcg->last_col, col);
      mcg->last_col = mcg->last_col->next;
      col->pos = mcg->last;
    } else {
      if (pos == mcg->col) /* append first */ {
	mcg->col = g_list_prepend (pos, col);
      } else {
	g_list_prepend (pos, col);
      }

      col->pos = ((MDIColorGenericCol *)(pos->data))->pos;
	
      /* We need to update position of other colors ! */

      pos = pos;
      while (pos) {
	col2 = pos->data;
	col2->pos++;
	mdi_color_generic_post_change (mcg, col2, CHANGE_POS);
	
	pos = g_list_next (pos);
      }
      
    }
  }

  mdi_color_generic_post_change (mcg, col, CHANGE_APPEND);
}

void 
mdi_color_generic_clear (MDIColorGeneric *mcg)
{ 
  GList *list;

  if (mcg->col) {
    mcg->last = -1;

    list = mcg->col;
    while (list) {
      mdi_color_generic_free_col (list->data);
      
      list = g_list_next (list);
    }
    
    g_list_free (mcg->col);

    mcg->last_col = mcg->col = NULL;
    mdi_color_generic_post_change (mcg, NULL, CHANGE_CLEAR);  
  }
}

void 
mdi_color_generic_remove (MDIColorGeneric *mcg, MDIColorGenericCol *col)
{
  GList *list;
  GList *next, *prev;  
  mdi_color_generic_post_change (mcg, col, CHANGE_REMOVE);

  if (mcg->last_col->data == col) { /* remove last */
    if (mcg->col == mcg->last_col) {/* 1 color in list */
      mcg->last_col = mcg->col = NULL;
      g_list_free (mcg->col);
    } else {
      prev = mcg->last_col->prev;
      g_list_remove (mcg->last_col, col);
      mcg->last_col = prev;
    }

  } else {

    if (mcg->col->data == col) { /* remove first */
      next = mcg->col->next;
      mcg->col = g_list_remove (mcg->col, col);
    } else { /* Not first, not last */

      list = g_list_find (mcg->col, col);
      next = list->next;

      g_list_remove (list, col);   
    }

    /* We need to update position of other colors ! */
    
    while (next) {
      col = next->data;
      
      col->pos--;;
      mdi_color_generic_post_change (mcg, col, CHANGE_POS);
      
      next = g_list_next (next);
    }
  }
    
  mcg->last--;
}

void 
mdi_color_generic_change_rgb (MDIColorGeneric *mcg, 
			      MDIColorGenericCol *col,
			      int r, int g, int b)
{
  col->r = r;
  col->g = g;
  col->b = b;

  mdi_color_generic_post_change (mcg, col, CHANGE_RGB);
}

void 
mdi_color_generic_change_name (MDIColorGeneric *mcg, 
			       MDIColorGenericCol *col,
			       char *name)
{
  g_free (col->name);
  col->name = g_strdup (name);

  mdi_color_generic_post_change (mcg, col, CHANGE_NAME);
}

void 
mdi_color_generic_change_pos (MDIColorGeneric *mcg, 
			      MDIColorGenericCol *col, int new_pos)
{
  GList *list;
  MDIColorGenericCol *col_tmp;
  int i;

  if (new_pos == -1) new_pos = mcg->last;

  if (new_pos < col->pos) {
    list = mdi_color_generic_find_col (mcg, new_pos);
    for (i = new_pos; i < col->pos; i++, list = g_list_next (list)) {
      col_tmp = list->data;
      col_tmp->pos++;
      mdi_color_generic_post_change (mcg, col_tmp, CHANGE_POS);
    }

  } else

    if (new_pos > col->pos) {     
      list = mdi_color_generic_find_col (mcg, new_pos);
      for (i = new_pos; i > col->pos; i--, list = g_list_previous (list)) {
	col_tmp = list->data;
	col_tmp->pos--;
	mdi_color_generic_post_change (mcg, col_tmp, CHANGE_POS);
      }

    } else 
      return;

  mcg->col = g_list_remove (mcg->col, col);
  col->pos = new_pos;
  mdi_color_generic_post_change (mcg, col, CHANGE_POS);
  mcg->col = g_list_insert (mcg->col, col, new_pos);
}

MDIColorGenericCol *
mdi_color_generic_search_by_data (MDIColorGeneric *mcg,
				  gpointer data)
{
  GList *list;
  MDIColorGenericCol *col;

  list = mcg->col;
  while (list) {
    col = list->data;

    if (col->data == data) return col;

    list = list->next;
  }

  return NULL;
}

/* 'To' sera avertit par mcg quand celui est modifié. */
void
mdi_color_generic_connect (MDIColorGeneric *mcg,
			   MDIColorGeneric *to)
{
  mcg->other_views = g_list_prepend (mcg->other_views, to);
  to->parents = g_list_prepend (to->parents, mcg);
  
  /* send append request for 'mcg color' to connected document 'to' */

  mdi_color_generic_set_all_color_change (mcg, CHANGE_APPEND);
  gtk_signal_emit_by_name (GTK_OBJECT (to), "document_changed", mcg->col);
  mdi_color_generic_set_all_color_change (mcg, 0);
}

void mdi_color_generic_disconnect (MDIColorGeneric *mcg,
				   MDIColorGeneric *to)
{
  mcg->other_views = g_list_remove (mcg->other_views, to);
  to->parents = g_list_remove (to->parents, mcg);

  /* send remove request for 'mcg color' to disconnected document 'to' */

  mdi_color_generic_set_all_color_change (mcg, CHANGE_REMOVE);
  gtk_signal_emit_by_name (GTK_OBJECT (to), "document_changed", mcg->col);
  mdi_color_generic_set_all_color_change (mcg, 0);
}

MDIColorGenericCol *
mdi_color_generic_get_owner (MDIColorGenericCol *col)
{
  if (col->owner->get_owner)
    return col->owner->get_owner (col);

  return col;
}

GList *
mdi_color_generic_get_append_pos (MDIColorGeneric *mcg, 
				  MDIColorGenericCol *col)
{
  if (mcg->get_append_pos)
    return mcg->get_append_pos (mcg, col);

  return NULL;
}

GtkType 
mdi_color_generic_get_control_type (MDIColorGeneric *mcg)
{
  if (mcg->get_control_type)
    return mcg->get_control_type (mcg);

  return 0;
}

void
mdi_color_generic_sync_control (MDIColorGeneric *mcg)
{
  GList *list;
  ViewColorGeneric *view;

  list = GNOME_MDI_CHILD (mcg)->views;
  while (list) {
    view = gtk_object_get_data (GTK_OBJECT (list->data), "view_object");
    if (view->control)					  
      control_generic_sync (view->control);

    list = g_list_next (list);
  }
}

void
mdi_color_generic_next_view_type (MDIColorGeneric *mcg, GtkType type)
{
  mcg->next_view_type = type;
}

/******************************* PROPERTIES ********************************/

typedef struct prop_t {
  GladeXML *gui;

  GtkWidget *entry_name;

  void (*changed_cb)(gpointer data);
  gpointer change_data;
} prop_t;

static void
entry_changed_cb (GtkWidget *widget, prop_t *prop)
{
  prop->changed_cb (prop->change_data);
}

static gpointer
mdi_color_generic_get_control (MDIColorGeneric *vcg, GtkVBox *box,
				void (*changed_cb)(gpointer data), 
			       gpointer change_data)
{
  GtkWidget *frame;
  prop_t *prop = g_new0 (prop_t, 1);

  prop->changed_cb   = changed_cb;
  prop->change_data = change_data;

  prop->gui = glade_xml_new (GCOLORSEL_GLADEDIR "mdi-color-generic-properties.glade", "frame");
  if (!prop->gui) {
    printf ("Could not find mdi-color-generic-properties.glade\n");
    return NULL;
  }

  frame = glade_xml_get_widget (prop->gui, "frame");
  if (!frame) {
    printf ("Corrupt file mdi-color-generic-properties.glade");
    return NULL;
  }

  gtk_box_pack_start_defaults (GTK_BOX (box), frame);

  prop->entry_name = glade_xml_get_widget (prop->gui, "entry-name");  
  gtk_signal_connect (GTK_OBJECT (prop->entry_name), "changed", 
		      GTK_SIGNAL_FUNC (entry_changed_cb), prop);

  return prop;
}

static void
mdi_color_generic_sync (MDIColorGeneric *mcg, gpointer data)
{
  prop_t *prop = data;

  printf ("MDI :: sync\n");

  gtk_signal_handler_block_by_data (GTK_OBJECT (prop->entry_name), prop);
  gtk_entry_set_text (GTK_ENTRY (prop->entry_name), 
		      GNOME_MDI_CHILD (mcg)->name);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (prop->entry_name), prop);
}

static void
mdi_color_generic_apply (MDIColorGeneric *mcg, gpointer data)
{
  prop_t *prop = data;  

  printf ("MDI :: apply\n");

  gnome_mdi_child_set_name (GNOME_MDI_CHILD (mcg), 
			    gtk_entry_get_text (GTK_ENTRY (prop->entry_name)));
}

static void
mdi_color_generic_close (MDIColorGeneric *mcg, gpointer data)
{
  prop_t *prop = data;

  printf ("MDI :: close\n");

  gtk_object_unref (GTK_OBJECT (prop->gui));
  g_free (prop);
}
