#include <stdio.h>
#include <gnome.h>
#include <glade/glade.h>

#include "view-color-generic.h"
#include "view-color-grid.h"
#include "mdi-color-generic.h"
#include "widget-color-grid.h"
#include "menus.h"
#include "utils.h"

static ViewColorGenericClass *parent_class = NULL;

static void view_color_grid_class_init (ViewColorGridClass *class);
static void view_color_grid_init       (ViewColorGrid *vcl);
static void view_color_grid_move_item  (ColorGrid *cg, int old_pos,
					int new_pos, ViewColorGrid *vcg,
					ViewColorGeneric *mcg);

static void view_color_grid_data_changed    (ViewColorGeneric *vcg, 
					     gpointer data);
static void view_color_grid_remove_selected (ViewColorGeneric *vcg);
static gpointer view_color_grid_get_control (ViewColorGeneric *vcg,
					     GtkVBox *box,
					     void (*changed_cb)(gpointer data), gpointer change_data);
static void     view_color_grid_apply       (ViewColorGeneric *vcg,
					     gpointer data);
static void     view_color_grid_close       (ViewColorGeneric *vcg,
					     gpointer data);
static void     view_color_grid_sync        (ViewColorGeneric *vcg,
					     gpointer data);

GtkType 
view_color_grid_get_type (void)
{
  static guint cg_type = 0;

  if (!cg_type) {
    GtkTypeInfo cg_info = {
      "ViewColorGrid",
      sizeof (ViewColorGrid),
      sizeof (ViewColorGridClass),
      (GtkClassInitFunc) view_color_grid_class_init,
      (GtkObjectInitFunc) view_color_grid_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };

    cg_type = gtk_type_unique (view_color_generic_get_type (), &cg_info);
  }

  return cg_type;
}

static void
view_color_grid_class_init (ViewColorGridClass *class)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;
  ViewColorGenericClass *vcg_class;

  object_class = GTK_OBJECT_CLASS (class);
  parent_class = gtk_type_class (TYPE_VIEW_COLOR_GENERIC);
  widget_class = (GtkWidgetClass *)class;
  vcg_class    = (ViewColorGenericClass *)class;
  
  vcg_class->data_changed = view_color_grid_data_changed;
  vcg_class->remove_selected = view_color_grid_remove_selected;
  vcg_class->get_control = view_color_grid_get_control;
  vcg_class->apply       = view_color_grid_apply;
  vcg_class->close       = view_color_grid_close;
  vcg_class->sync        = view_color_grid_sync;
}

static void
view_color_grid_init (ViewColorGrid *vcg)
{

}

static gint
view_color_grid_compare_func (gconstpointer ptr1, gconstpointer ptr2)
{
  ColorGridCol *col1 = (ColorGridCol *)ptr1, *col2 = (ColorGridCol *)ptr2;
  MDIColor *c1 = col1->data, *c2 = col2->data;

  if (c1->pos < c2->pos) return -1;
  if (c1->pos > c2->pos) return 1;

  return 0;
}


GtkObject *
view_color_grid_new (MDIColorGeneric *mcg)
{
  GtkObject *object;
  ColorGrid *cg;

  object = gtk_type_new (TYPE_VIEW_COLOR_GRID);

  VIEW_COLOR_GENERIC (object)->mcg = mcg;

  cg = COLOR_GRID (color_grid_new (view_color_grid_compare_func));
  VIEW_COLOR_GENERIC (object)->widget = GTK_WIDGET (cg);

  color_grid_can_move (cg, mdi_color_generic_can_do (mcg, CHANGE_POS));

  gtk_signal_connect (GTK_OBJECT (cg), "move_item", 
		      GTK_SIGNAL_FUNC (view_color_grid_move_item), object);
  
  return object;
}

static void
item_destroy_notify (gpointer data)
{
  gtk_object_unref (GTK_OBJECT (data));
}

static void
view_color_grid_data_changed (ViewColorGeneric *vcg, gpointer data)
{
  GList *list = data;
  MDIColor *col;
  ColorGrid *cg = COLOR_GRID (vcg->widget);

  color_grid_freeze (cg);

  while (list) {
    col = list->data;

    if (col->change & CHANGE_APPEND) {
      gtk_object_ref (GTK_OBJECT (col));
      color_grid_append (cg, col->r, col->g, col->b, col, item_destroy_notify);    
    } else

      if (col->change & CHANGE_CLEAR) color_grid_clear (cg);
    
      else {

	if (col->change & CHANGE_REMOVE) 
	  color_grid_remove (cg, col);
	else {

	  if (col->change & CHANGE_RGB) 
	    color_grid_change_rgb (cg, col, col->r, col->g, col->b);
	
	/* CHANGE_NAME, don't care */
	
	  if (col->change & CHANGE_POS) 
	    color_grid_change_pos (cg, col);
	}
      }
    
    list = g_list_next (list);
  }

  color_grid_thaw (cg);
}

static void
view_color_grid_remove_selected (ViewColorGeneric *vcg)
{
  ColorGrid *cg = COLOR_GRID (vcg->widget);
  GList *list = cg->selected;
  MDIColor *col;
  GList *freezed = NULL;

  while (list) {  
    col = ((ColorGridCol *)list->data)->data;    
    list = g_list_next (list);

    col = mdi_color_generic_get_owner (col);

    freezed = g_list_prepend (freezed, col->owner);
    
    mdi_color_generic_freeze (col->owner);
    mdi_color_generic_remove (col->owner, col);
  }   

  list = freezed;
  while (list) {
    g_assert (IS_MDI_COLOR_GENERIC (freezed->data));
    mdi_color_generic_thaw (freezed->data);
    list = g_list_next (list);
  }    
  
  g_list_free (freezed);
}

static void 
view_color_grid_move_item (ColorGrid *cg, int old_pos,
			   int new_pos, ViewColorGrid *vcg,
			   ViewColorGeneric *mvg)
{
  ColorGridCol *col = g_list_nth (cg->col, old_pos)->data;
  MDIColor *c = col->data;

  mdi_color_generic_freeze (mvg->mcg);
  mdi_color_generic_change_pos (mvg->mcg, c, new_pos);
  mdi_color_generic_thaw (mvg->mcg);
}

/*********************** PROPERTIES ***************************/

typedef struct prop_t {
  GladeXML *gui;

  gpointer parent_data;

  void (*changed_cb)(gpointer data);
  gpointer change_data;

  GtkWidget *spin_width;
  GtkWidget *spin_height;
} prop_t;

static void
spin_changed_cb (GtkWidget *widget, prop_t *prop)
{
  prop->changed_cb (prop->change_data);
}

static gpointer 
view_color_grid_get_control (ViewColorGeneric *vcg, GtkVBox *box,
			     void (*changed_cb)(gpointer data), 
			     gpointer change_data)
{
  prop_t *prop = g_new0 (prop_t, 1);
  GtkWidget *frame;
  GtkAdjustment *adj;

  prop->changed_cb  = changed_cb;
  prop->change_data = change_data;

  prop->parent_data = parent_class->get_control (vcg, box, changed_cb, change_data);

  prop->gui = glade_xml_new (GCOLORSEL_GLADEDIR "view-color-grid-properties.glade", "frame");
  g_assert (prop->gui != NULL);

  frame = glade_xml_get_widget (prop->gui, "frame");
  gtk_box_pack_start_defaults (GTK_BOX (box), frame);

  prop->spin_width = glade_xml_get_widget (prop->gui, "spin-width");  
  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (prop->spin_width));
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (spin_changed_cb), prop);

  prop->spin_height = glade_xml_get_widget (prop->gui, "spin-height");  
  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (prop->spin_height));
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed", 
		      GTK_SIGNAL_FUNC (spin_changed_cb), prop);

  return prop;
}

static void     
view_color_grid_apply (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;
  ColorGrid *cg = COLOR_GRID (vcg->widget);

  printf ("Grid    :: apply\n");

  color_grid_set_col_width_height (cg,
        gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prop->spin_width)),
	gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prop->spin_height)));

  parent_class->apply (vcg, prop->parent_data);
}

static void 
view_color_grid_close (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;

  printf ("Grid    :: close\n");

  parent_class->close (vcg, prop->parent_data);

  gtk_object_unref (GTK_OBJECT (prop->gui));
  g_free (prop);
}

static void 
view_color_grid_sync (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;
  ColorGrid *cg = COLOR_GRID (vcg->widget);

  printf ("Grid    :: sync \n");

  spin_set_value (GTK_SPIN_BUTTON (prop->spin_width), cg->col_width, prop);
  spin_set_value (GTK_SPIN_BUTTON (prop->spin_height), cg->col_height, prop);

  parent_class->sync (vcg, prop->parent_data);
}
