#include <stdio.h>

#include <gnome.h>

#include "widget-color-grid.h"
#include "mdi-color-generic.h"
#include "utils.h"

enum {
  DATA_CHANGED,
  LAST_SIGNAL
};

static guint cg_signals [LAST_SIGNAL] = { 0 };

static const GtkTargetEntry color_grid_drag_targets[] = {
  { "application/x-color", 0 }
};

static void color_grid_class_init    (ColorGridClass *class);
static void color_grid_init          (ColorGrid *cl);

static void color_grid_data_changed  (ColorGrid *cl, gpointer data);

static void color_grid_allocate      (GtkWidget *widget, 
				      GtkAllocation *allocation);

static void color_grid_item_pos      (ColorGrid *cg, GnomeCanvasItem *item,
				      int column, int line);
static void color_grid_item_border   (GnomeCanvasItem *item,
				      char *color, float size, int to_top);

static void color_grid_set_scroll_region (ColorGrid *cg, int nb);

static ColorGridCol *color_grid_search_col (ColorGrid *cg, 
					    MDIColorGenericCol *col);
static ColorGridCol *color_grid_append_col (ColorGrid *cg, 
					    MDIColorGenericCol *col);

static GnomeCanvasClass *parent_class = NULL;

GtkType 
color_grid_get_type (void)
{
  static guint cg_type = 0;

  if (!cg_type) {
    GtkTypeInfo cg_info = {
      "ColorGrid",
      sizeof (ColorGrid),
      sizeof (ColorGridClass),
      (GtkClassInitFunc) color_grid_class_init,
      (GtkObjectInitFunc) color_grid_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };

    cg_type = gtk_type_unique (gnome_canvas_get_type (), &cg_info);
  }

  return cg_type;
}

static void
color_grid_class_init (ColorGridClass *class)
{
  GtkWidgetClass *widget_class;
  GnomeCanvasClass *canvas_class;
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);
  parent_class = gtk_type_class (GNOME_TYPE_CANVAS);
  widget_class = (GtkWidgetClass *)class;
  canvas_class = (GnomeCanvasClass *)class;

  cg_signals [DATA_CHANGED] = 
    gtk_signal_new ("data_changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (ColorGridClass, data_changed),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, cg_signals, LAST_SIGNAL);

  widget_class->size_allocate = color_grid_allocate;

  class->data_changed = color_grid_data_changed;
}

static void
color_grid_init (ColorGrid *cg)
{
  cg->nb_col         = 10;
  cg->col_height     = 15;
  cg->col_width      = 15;
  cg->in_drag        = FALSE;
  cg->button_pressed = FALSE;
  cg->selected       = NULL;
  cg->col            = NULL;
}

GtkWidget *
color_grid_new (void)
{
  GtkWidget *widget;

  widget = gtk_type_new (TYPE_COLOR_GRID);
		       
  return widget;
}

static void
color_grid_set_scroll_region (ColorGrid *cg, int nb)
{
  gnome_canvas_set_scroll_region (GNOME_CANVAS (cg), 0, 0, 
				  cg->nb_col * cg->col_width, 
				  ((nb / (cg->nb_col)) + 1) * cg->col_height);
}

static void
color_grid_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  ColorGrid *cg= COLOR_GRID (widget);
  GList *list;
  ColorGridCol *c;
  int line, column;
  gboolean change = FALSE;
  int nb = 0;

  if ((allocation->width != widget->allocation.width)
      || (allocation->height != widget->allocation.height))
    change = TRUE;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (!change)
    return;

  cg->nb_col = allocation->width / cg->col_width;

  list = cg->col;  
  while (list) {
    nb++;
    c = list->data;    
    
    line = c->col->pos / cg->nb_col;
    column = c->col->pos - (line * cg->nb_col);

    color_grid_item_pos (COLOR_GRID (widget), c->item, column, line);

    list = g_list_next (list);
  }

  color_grid_set_scroll_region (COLOR_GRID (widget), nb);
}

static ColorGridCol *
color_grid_search_col (ColorGrid *cg, MDIColorGenericCol *col)
{
  ColorGridCol *c;
  GList *list = cg->col;

  while (list) {
    c = list->data;

    if (c->col == col) return c;

    list = g_list_next (list);
  }

  return NULL;
}

static void
color_grid_item_pos (ColorGrid *cg, GnomeCanvasItem *item,
		     int column, int line)
{
  gnome_canvas_item_set (item, 
			 "x1", (float) column * cg->col_width,
			 "y1", (float) line * cg->col_height,
			 "x2", (float) column * cg->col_width + cg->col_width,
			 "y2", (float) line * cg->col_height + cg->col_height,
			 NULL);
}

static void
color_grid_item_border (GnomeCanvasItem *item,
			char *color, float size, int to_top) 
{
  if (item) {
    gnome_canvas_item_set (item,
			   "outline_color", color,
			   "width_units", size,
			   NULL); 

    if (to_top) 
      gnome_canvas_item_raise_to_top (item);
  }
}

static void
color_grid_select (ColorGridCol *col, gboolean selected)
{
  if (col->selected != selected) {
    col->selected = selected;
    color_grid_item_border (col->item, selected ? "white" : "black",
			    selected ? 2 : 1, selected);
    
    if (selected) 
      col->cg->selected = g_list_append (col->cg->selected, col);
    else
      col->cg->selected = g_list_remove (col->cg->selected, col);
  }
}

static void
color_grid_deselect_all (ColorGrid *cg)
{
  while (cg->selected) 
    color_grid_select (cg->selected->data, FALSE);
}

static gint
color_grid_item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
  GnomeCanvasItem *drop;
  ColorGridCol *col, *col_drop;
  MDIColorGeneric *mcg;
  ColorGrid *cg;  

  col = gtk_object_get_data (GTK_OBJECT (item), "col");
  cg = col->cg;

  if (cg->in_drag) {
    switch (event->type) {
    case GDK_BUTTON_RELEASE:
      cg->button_pressed = FALSE;
      cg->in_drag = FALSE;
    
      gnome_canvas_item_ungrab (item, event->button.time);

      if (cg->drop) {
	mcg = gtk_object_get_data (GTK_OBJECT (cg), "color_generic");
	col_drop = gtk_object_get_data (GTK_OBJECT (cg->drop), "col");
	
	mdi_color_generic_freeze (mcg);
	mdi_color_generic_change_pos (mcg, col->col, col_drop->col->pos);
	mdi_color_generic_thaw (mcg);

	color_grid_item_border (cg->drop, "black", 1, FALSE);
	gnome_canvas_item_raise_to_top (item);
      }
      
      break;
    case GDK_MOTION_NOTIFY:
      drop = gnome_canvas_get_item_at (GNOME_CANVAS (cg), 
				       event->motion.x,
				       event->motion.y);

      if (drop != cg->drop) {
	color_grid_item_border (cg->drop, "black", 1, FALSE);
	gnome_canvas_item_raise_to_top (item);	  	  

	if (item != drop) {
	  color_grid_item_border (drop, "red", 2, TRUE);	  

	  cg->drop = drop;
	} else 
	  cg->drop = NULL;
      }

      break;
    default:
      return FALSE;
    }  

    return TRUE;
  }
  
  switch (event->type) {
  case GDK_2BUTTON_PRESS:
    break;
  case GDK_MOTION_NOTIFY:

    if (mdi_color_generic_can_do (gtk_object_get_data (GTK_OBJECT (cg), 
					  "color_generic"), CHANGE_POS)) 
      if (cg->button_pressed) {
	if (!cg->in_drag) {	
	  color_grid_deselect_all (cg);
	  color_grid_select (col, TRUE);
	  cg->last_clicked = col;

	  cg->in_drag = TRUE;
	  cg->drop = NULL;
	  gnome_canvas_item_grab (item, 
			  GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				  NULL,
				  event->button.time); 
	}
      }
    break;
  case GDK_BUTTON_RELEASE:
    cg->button_pressed = FALSE;
    break;
  case GDK_BUTTON_PRESS:
    cg->button_pressed = TRUE;

    if (event->button.state & GDK_SHIFT_MASK) {
      GList *list;

      if (! (event->button.state & GDK_CONTROL_MASK))
	  color_grid_deselect_all (cg);

      list = g_list_find (cg->col, cg->last_clicked);

      if (cg->last_clicked->col->pos < col->col->pos) 
	while (list) {
	  color_grid_select (list->data, TRUE);
	  if (list->data == col) break;
	  list = g_list_next (list);
	}
      else 
	while (list) {
	  color_grid_select (list->data, TRUE);
	  if (list->data == col) break;
	  list = g_list_previous (list);
	}            
    }
    
    else
      
      if (event->button.state & GDK_CONTROL_MASK) {
	color_grid_select (col, ! col->selected);
	cg->last_clicked = col;
      }
        
      else {
	color_grid_deselect_all (cg);
	color_grid_select (col, TRUE);
	cg->last_clicked = col;
      }

    break;
  default:
    return FALSE;
  }

    
  return TRUE;
}

static gint
color_grid_compare_func (gconstpointer a, gconstpointer b)
{
  ColorGridCol *col1 = (ColorGridCol *)a; 
  ColorGridCol *col2 = (ColorGridCol *)b;

  if (col1->col->pos < col2->col->pos) return -1;
  if (col1->col->pos > col2->col->pos) return 1;

  return 0;
}

static ColorGridCol *
color_grid_append_col (ColorGrid *cg, MDIColorGenericCol *col)
{
  ColorGridCol *c;
  int line, column;
  GdkColor color;

  c = g_new0 (ColorGridCol, 1);
  c->selected = FALSE;
  c->cg  = cg;
  c->col = col;

  cg->col = g_list_insert_sorted (cg->col, c, color_grid_compare_func);
  
  line = col->pos / cg->nb_col;
  column = col->pos - (line * cg->nb_col);
  
  color.red   = col->r * 255; 
  color.green = col->g * 255;
  color.blue  = col->b * 255; 
  gdk_color_alloc (gtk_widget_get_default_colormap (), &color);
      
  c->item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (cg)),
		     GNOME_TYPE_CANVAS_RECT,
		     "x1", (float) column * cg->col_width,
		     "y1", (float) line * cg->col_height,
		     "x2", (float) column * cg->col_width + cg->col_width,
		     "y2", (float) line * cg->col_height + cg->col_height,
		     "outline_color", "black",
		     "width_units", 1.0,
		     "fill_color_gdk", &color,
		     NULL);

  gtk_object_set_data (GTK_OBJECT (c->item), "col", c);

  gtk_signal_connect (GTK_OBJECT (c->item), "event",
		      GTK_SIGNAL_FUNC (color_grid_item_event), NULL);
  
  return c;
}

static void
color_grid_clear (ColorGrid *cg)
{
  ColorGridCol *col;

  while (cg->col) {
    col = cg->col->data;

    gtk_object_destroy (GTK_OBJECT (col->item));
    g_free (col);

    cg->col = g_list_remove (cg->col, col);
  }  

  g_list_free (cg->selected);
  cg->selected = NULL;
}

static void
color_grid_data_changed (ColorGrid *cg, gpointer data)
{
  GList *list = data;
  MDIColorGenericCol *col;
  int line, column;
  ColorGridCol *c;

  while (list) {
    col = list->data;

    if (col->change & CHANGE_APPEND) {
      color_grid_append_col (cg, col);
    }

    else
      
      if (col->change & CHANGE_POS) {
	line = col->pos / cg->nb_col;
	column = col->pos - (line * cg->nb_col);

	c = color_grid_search_col (cg, col);
	color_grid_item_pos (cg, c->item, column, line);

	cg->col = g_list_remove (cg->col, c);
	cg->col = g_list_insert_sorted (cg->col, c, color_grid_compare_func);
      }

      else

	if (col->change & CHANGE_REMOVE) {
	  c = color_grid_search_col (cg, col);
	  cg->col = g_list_remove (cg->col, c);
	  gtk_object_destroy (GTK_OBJECT (c->item));

	  if (c->selected) 
	    cg->selected = g_list_remove (cg->selected, c);

	  g_free (c);
	}      

	else

	  if (col->change & CHANGE_CLEAR) {
	    color_grid_clear (cg);
	  }   
    
    list = g_list_next (list);
  }

  color_grid_set_scroll_region (cg, 
				g_list_length (COLOR_GRID (cg)->col) - 1);
}
