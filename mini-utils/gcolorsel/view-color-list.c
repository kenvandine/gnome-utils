#include <stdio.h>
#include <gnome.h>
#include <glade/glade.h>

#include "view-color-generic.h"
#include "view-color-list.h"
#include "mdi-color-generic.h"
#include "menus.h"
#include "utils.h"

static ViewColorGenericClass *parent_class = NULL;

/* From nautilus */
static char * down_xpm[] = {
"6 5 2 1",
" 	c None",
".	c #000000",
"......",
"      ",
" .... ",
"      ",
"  ..  "};

/* From nautilus */
static char * up_xpm[] = {
"6 5 2 1",
" 	c None",
".	c #000000",
"  ..  ",
"      ",
" .... ",
"      ",
"......"};

#define UP_ARROW_KEY    "up_arrow"
#define DOWN_ARROW_KEY  "down_arrow"

#define COLUMN_PIXMAP 0
#define COLUMN_VALUE  1
#define COLUMN_NAME   2

static const GtkTargetEntry drag_targets[] = {
  { "application/x-color", 0 }
};

static void view_color_list_class_init     (ViewColorListClass *class);
static void view_color_list_init           (ViewColorList *vcl);

static GtkWidget *create_column_widget     (GtkCList *clist,
					    int num, char *text);

static char *view_color_list_render_value  (ViewColorList *vcl, 
					    MDIColorGenericCol *col);
static void  view_color_list_render_pixmap (ViewColorList *cl, 
					    GdkPixmap *pixmap,
					    MDIColorGenericCol *col);
static gint  view_color_list_append        (ViewColorList *vcl, 
					    MDIColorGenericCol *col);

void
view_color_list_set_sort_column (ViewColorList *vcl, 
				 gint column, GtkSortType type);
static int
view_color_list_compare_rows    (GtkCList *clist, 
				 gconstpointer ptr1, gconstpointer ptr2);

static void
view_color_list_data_changed    (ViewColorGeneric *vcg, gpointer data);
static void
view_color_list_remove_selected (ViewColorGeneric *vcg);
static gpointer 
view_color_list_get_control     (ViewColorGeneric *vcg, GtkVBox *box,
				 void (*changed_cb)(gpointer data), 
				 gpointer change_data);
static void     view_color_list_apply (ViewColorGeneric *vcg,
				       gpointer data);
static void     view_color_list_close (ViewColorGeneric *vcg,
				       gpointer data);
static void     view_color_list_sync  (ViewColorGeneric *vcg,
				       gpointer data);

static void 
view_color_list_click_column (GtkCList *clist, gint column,ViewColorList *vcl);
static gint 
view_color_list_button_press (GtkCList *clist, GdkEventButton *event,
			      ViewColorList *vcl);
static void 
view_color_list_drag_begin   (GtkCList *clist, GdkDragContext *context);
static void 
view_color_list_drag_data_get(GtkCList *clist, GdkDragContext *context, 
			      GtkSelectionData *selection_data, guint info,
			      guint time);

GtkType 
view_color_list_get_type (void)
{
  static guint cg_type = 0;

  if (!cg_type) {
    GtkTypeInfo cg_info = {
      "ViewColorList",
      sizeof (ViewColorList),
      sizeof (ViewColorListClass),
      (GtkClassInitFunc) view_color_list_class_init,
      (GtkObjectInitFunc) view_color_list_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };

    cg_type = gtk_type_unique (view_color_generic_get_type (), &cg_info);
  }

  return cg_type;
}

static void
view_color_list_class_init (ViewColorListClass *class)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;
  ViewColorGenericClass *vcg_class;

  object_class = GTK_OBJECT_CLASS (class);
  parent_class = gtk_type_class (TYPE_VIEW_COLOR_GENERIC);
  widget_class = (GtkWidgetClass *)class;
  vcg_class    = (ViewColorGenericClass *)class;
  
  vcg_class->data_changed    = view_color_list_data_changed;
  vcg_class->remove_selected = view_color_list_remove_selected;
  vcg_class->get_control     = view_color_list_get_control;
  vcg_class->close           = view_color_list_close;
  vcg_class->apply           = view_color_list_apply;
  vcg_class->sync            = view_color_list_sync;
}

static void
view_color_list_init (ViewColorList *vcg)
{
  vcg->col_width = 48;
  vcg->col_height = 15;
  vcg->draw_numbers = TRUE;

  gdk_color_parse ("black", &vcg->color_black);
  gdk_color_alloc (gtk_widget_get_default_colormap (), &vcg->color_black);

  gdk_color_parse ("white", &vcg->color_white);
  gdk_color_alloc (gtk_widget_get_default_colormap (), &vcg->color_white);

  vcg->pixmap_font = gdk_font_load ("-bitstream-courier-medium-r-normal-*-12-*-*-*-*-*-*-*");
}

GtkObject *
view_color_list_new (MDIColorGeneric *mcg)
{
  GtkObject *object;
  GtkCList *cl;

  object = gtk_type_new (TYPE_VIEW_COLOR_LIST);
  
  VIEW_COLOR_GENERIC (object)->mcg = mcg;

  cl = GTK_CLIST (gtk_clist_new (3));
  VIEW_COLOR_GENERIC (object)->widget = GTK_WIDGET (cl);

  gtk_signal_connect (GTK_OBJECT (cl), "click_column", 
		      GTK_SIGNAL_FUNC (view_color_list_click_column), object);
  gtk_signal_connect (GTK_OBJECT (cl), "button_press_event", 
		      GTK_SIGNAL_FUNC (view_color_list_button_press), object);
  gtk_signal_connect (GTK_OBJECT (cl), "drag_begin",
		      GTK_SIGNAL_FUNC (view_color_list_drag_begin), object);
  gtk_signal_connect (GTK_OBJECT (cl), "drag_data_get",
		      GTK_SIGNAL_FUNC (view_color_list_drag_data_get), object);

  gtk_drag_source_set (GTK_WIDGET (cl), GDK_BUTTON1_MASK, 
		       drag_targets, 1, GDK_ACTION_COPY);

  gtk_clist_set_selection_mode (cl, GTK_SELECTION_EXTENDED);
  gtk_clist_column_titles_show (cl);

  create_column_widget (cl, COLUMN_PIXMAP, _("Color"));
  create_column_widget (cl, COLUMN_VALUE, _("Value"));
  create_column_widget (cl, COLUMN_NAME, _("Name"));

  gtk_clist_set_column_auto_resize (cl, COLUMN_PIXMAP, TRUE);
  gtk_clist_set_row_height (cl, VIEW_COLOR_LIST (object)->col_height);

  gtk_clist_set_column_width (cl, COLUMN_VALUE, 70);
  gtk_clist_set_column_resizeable (cl, COLUMN_PIXMAP, FALSE);

  gtk_clist_set_auto_sort (cl, TRUE);
  gtk_clist_set_compare_func (cl, view_color_list_compare_rows);
  view_color_list_set_sort_column (VIEW_COLOR_LIST (object), COLUMN_PIXMAP,
				   GTK_SORT_ASCENDING);

  return object;
}

static GtkWidget *
create_column_widget (GtkCList *cl, int num, char *text)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *up;
  GtkWidget *down;

  hbox = gtk_hbox_new (FALSE, 0);
  
  label = gtk_label_new (text);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, GNOME_PAD_SMALL);

  gtk_widget_show_all (hbox);

  up = gnome_pixmap_new_from_xpm_d (up_xpm);
  down = gnome_pixmap_new_from_xpm_d (down_xpm);

  gtk_box_pack_end (GTK_BOX (hbox), up, FALSE, FALSE, GNOME_PAD_SMALL); 
  gtk_box_pack_end (GTK_BOX (hbox), down, FALSE, FALSE, GNOME_PAD_SMALL); 

  gtk_object_set_data (GTK_OBJECT (hbox), UP_ARROW_KEY, up);
  gtk_object_set_data (GTK_OBJECT (hbox), DOWN_ARROW_KEY, down);

  gtk_clist_set_column_widget (cl, num, hbox);

  return hbox;
}

static int
view_color_list_compare_rows (GtkCList *clist, 
			 gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCListRow *row1;
  GtkCListRow *row2;
  MDIColorGenericCol *c1;
  MDIColorGenericCol *c2;
  int t1;
  int t2;
  
  row1 = (GtkCListRow *) ptr1;
  row2 = (GtkCListRow *) ptr2;

  c1 = (MDIColorGenericCol *) row1->data;
  c2 = (MDIColorGenericCol *) row2->data;

  if (!c2) return (c1 != NULL);
  if (!c1) return -1;

  switch (clist->sort_column) {
  case COLUMN_PIXMAP:
    if (c1->pos < c2->pos) return -1;
    if (c1->pos > c2->pos) return 1;
    return 0;

  case COLUMN_NAME:
    return g_strcasecmp (c1->name, c2->name);

  case COLUMN_VALUE:
    t1 = c1->r + c1->g + c1->b;
    t2 = c2->r + c2->g + c2->b;

    if (t1 < t2) return -1;
    if (t1 > t2) return 1;

    break;
  default:
    g_assert_not_reached ();
  }

  return 0;
}

static void
color_list_show_arrow (ViewColorList *vcl)
{
  GtkWidget *hbox;
  GtkCList *clist = GTK_CLIST (VIEW_COLOR_GENERIC (vcl)->widget);
  
  hbox = gtk_clist_get_column_widget (clist, 
				      GTK_CLIST (clist)->sort_column);

  gtk_widget_show (gtk_object_get_data (GTK_OBJECT (hbox), 
                   GTK_CLIST (clist)->sort_type == GTK_SORT_ASCENDING ? 
   		   UP_ARROW_KEY : DOWN_ARROW_KEY));
}

static void
color_list_hide_arrow (ViewColorList *vcl)
{
  GtkWidget *hbox;
  GtkCList *clist = GTK_CLIST (VIEW_COLOR_GENERIC (vcl)->widget);
    
  hbox = gtk_clist_get_column_widget (GTK_CLIST (clist), 
				      GTK_CLIST (clist)->sort_column);

  gtk_widget_hide (gtk_object_get_data (GTK_OBJECT (hbox), 
                   GTK_CLIST (clist)->sort_type == GTK_SORT_ASCENDING ? 
   		   UP_ARROW_KEY : DOWN_ARROW_KEY));
}

void
view_color_list_set_sort_column (ViewColorList *vcl, 
				 gint column, GtkSortType type)
{
  GtkCList *clist = GTK_CLIST (VIEW_COLOR_GENERIC (vcl)->widget);

  color_list_hide_arrow (vcl);

  gtk_clist_set_sort_column (clist, column);
  gtk_clist_set_sort_type (clist, type);

  color_list_show_arrow (vcl);

  gtk_clist_sort (clist);
}

static void
view_color_list_click_column (GtkCList *clist, gint column, ViewColorList *vcl)
{
  GtkSortType type;

  if (column == clist->sort_column) 
    type = clist->sort_type == GTK_SORT_ASCENDING ? 
                                   GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
  else
    type = GTK_SORT_ASCENDING;

  view_color_list_set_sort_column (vcl, column, type);
}

static gint
view_color_list_button_press (GtkCList *clist, GdkEventButton *event,
			      ViewColorList *vcl)
{
  int col, row;
  GtkCListRow *r;

  if (event->button == 3) {
    if (gtk_clist_get_selection_info (clist, event->x, event->y, &row, &col));
    
    /* 1. Si la ligne est deja selectionnee, on continue 
       2. Sinon, on deselectionne tout et on selectionne la ligne */
    
    r = g_list_nth (clist->row_list, row)->data;
    if (r->state != GTK_STATE_SELECTED) {
      gtk_clist_freeze (clist);
      gtk_clist_unselect_all (clist);
      clist->focus_row = row;
      gtk_clist_select_row (clist, row, col);
      gtk_clist_thaw (clist);
    }      
  
    menu_view_do_popup (event);
    return FALSE;
  }
  
  return TRUE;
}

static void
view_color_list_drag_begin (GtkCList *clist, GdkDragContext *context)
{
  MDIColorGenericCol *col;
  int row;

  if (clist->focus_row < 0) 
    row = 0;
  else 
    row = clist->focus_row;

  gtk_clist_select_row (clist, row, 0);
  col = gtk_clist_get_row_data (clist, row);  
  if (!col) return;
  gtk_drag_set_icon_widget (context, 
	       drag_window_create (col->r, col->g, col->b), -2, -2);
}

static void 
view_color_list_drag_data_get (GtkCList *clist, GdkDragContext *context, 
			  GtkSelectionData *selection_data, guint info,
			  guint time)
{
  MDIColorGenericCol *col;
  guint16 vals[4];

  col = gtk_clist_get_row_data (clist, clist->focus_row);
  if (!col) return;
  
  vals[0] = ((gdouble)col->r / 255.0) * 0xffff;
  vals[1] = ((gdouble)col->g / 255.0) * 0xffff;
  vals[2] = ((gdouble)col->b / 255.0) * 0xffff;
  vals[3] = 0xffff;
  
  gtk_selection_data_set (selection_data, 
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}	         

static char *
view_color_list_render_value (ViewColorList *vcl, MDIColorGenericCol *col)
{
  switch (VIEW_COLOR_GENERIC (vcl)->format) {
  case FORMAT_DEC_8:
    return g_strdup_printf ("%d %d %d", col->r, col->g, col->b);
  case FORMAT_DEC_16:
    return g_strdup_printf ("%d %d %d", 
			    col->r * 256, col->g * 256, col->b * 256);
  case FORMAT_HEX_8:
    return g_strdup_printf ("#%02x%02x%02x", col->r, col->g, col->b);
  case FORMAT_HEX_16:
    return g_strdup_printf ("#%04x%04x%04x", 
			    col->r * 256, col->g * 256, col->b * 256);
  case FORMAT_FLOAT:
    return g_strdup_printf ("%1.4g %1.4g %1.4g", (float) col->r / 256,
                    			         (float) col->g / 256,
                        			 (float) col->b / 256);
  default:
    g_assert_not_reached ();
  }

  return NULL;   
}

static gint
view_color_list_append (ViewColorList *vcl, MDIColorGenericCol *col)
{
  gchar *string[3];
  gint row;
  GdkPixmap *pixmap;
  GtkCList *clist = GTK_CLIST (VIEW_COLOR_GENERIC (vcl)->widget);

  pixmap = gdk_pixmap_new (VIEW_COLOR_GENERIC (vcl)->widget->window, 
			   vcl->col_width,
			  vcl->col_height, 
			   -1);

  view_color_list_render_pixmap (vcl, pixmap, col);

  string[COLUMN_PIXMAP] = NULL;
  string[COLUMN_VALUE]  = view_color_list_render_value (vcl, col);
  string[COLUMN_NAME]   = col->name;
  
  row = gtk_clist_append (clist, string);
  gtk_clist_set_pixmap (clist, row, 
			COLUMN_PIXMAP, pixmap, NULL);

  if (string[COLUMN_VALUE]) 
    g_free (string[COLUMN_VALUE]);

  gtk_clist_set_row_data (clist, row, col);

  gdk_pixmap_unref (pixmap);
    
  return row;
}

static void
view_color_list_render_pixmap (ViewColorList *vcl, GdkPixmap *pixmap,
			       MDIColorGenericCol *col)
{
  GdkColor color;
  int h, w;
  char *str;

  color.red   = col->r * 255; 
  color.green = col->g * 255;
  color.blue  = col->b * 255;
  
  gdk_color_alloc (gtk_widget_get_default_colormap (), &color);

  if (vcl->gc == NULL) 
    vcl->gc = gdk_gc_new (VIEW_COLOR_GENERIC (vcl)->widget->window);

  /* Border */
  gdk_gc_set_foreground (vcl->gc, &vcl->color_black);
  gdk_draw_rectangle (pixmap, vcl->gc, FALSE, 0, 0, 
		      vcl->col_width - 1,
		      vcl->col_height - 1);
  /* Color */
  gdk_gc_set_foreground (vcl->gc, &color);
  gdk_draw_rectangle (pixmap, vcl->gc, TRUE, 1, 1, 
		      vcl->col_width - 2,
		      vcl->col_height - 2);

  if (vcl->draw_numbers) {
    
    str = g_strdup_printf ("%d", col->pos);
    
    if ((col->r + col->g + col->b) < ((255 * 3) / 2)) 
      gdk_gc_set_foreground (vcl->gc, &vcl->color_white);
    else
      gdk_gc_set_foreground (vcl->gc, &vcl->color_black);
    
    h = gdk_string_height (vcl->pixmap_font, str);  
    w = gdk_string_width  (vcl->pixmap_font, str);
    
    gdk_draw_string (pixmap, vcl->pixmap_font, vcl->gc, 
		     (vcl->col_width - w) / 2,
		     ((vcl->col_height - h) / 2) + h, str);
    
    g_free (str);
  }
}

static void
view_color_list_redraw_all (ViewColorList *vcl)
{  
  GtkCList *clist = GTK_CLIST (VIEW_COLOR_GENERIC (vcl)->widget);
  MDIColorGenericCol *col;
  GdkPixmap *pixmap;
  int i;
  char *str;

  for (i=0; i<clist->rows; i++) {
    col = gtk_clist_get_row_data (clist, i);

    pixmap = gdk_pixmap_new (VIEW_COLOR_GENERIC (vcl)->widget->window, 
			     vcl->col_width, vcl->col_height, -1);

    view_color_list_render_pixmap (vcl, pixmap, col);
    
    gtk_clist_set_pixmap (clist, i, COLUMN_PIXMAP, pixmap, NULL);

    str = view_color_list_render_value (vcl, col);
    gtk_clist_set_text (clist, i, COLUMN_VALUE, str);
    g_free (str);

    gdk_pixmap_unref (pixmap);
  }

  gtk_clist_set_row_height (clist, vcl->col_height);
}

/********************************* MDI **************************************/

static void
view_color_list_data_changed (ViewColorGeneric *vcg, gpointer data)
{
  ViewColorList *vcl = VIEW_COLOR_LIST (vcg);
  GList *list = data;
  MDIColorGenericCol *col;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  gint row;
  GtkCList *clist = GTK_CLIST (vcg->widget);

  gtk_clist_freeze (clist);

  while (list) {
    col = list->data;

    if (col->change & CHANGE_CLEAR) {
      gtk_clist_clear (clist);
    }

    else

      if (col->change & CHANGE_APPEND) {
	view_color_list_append (vcl, col);
      } 
    
      else
	
	if (col->change & CHANGE_REMOVE) {
	  row = gtk_clist_find_row_from_data (clist, col);
	  gtk_clist_remove (clist, row);
	}
    
	else
	  
	  if (col->change & CHANGE_POS) { 
	    if (vcl->draw_numbers) {
	      row = gtk_clist_find_row_from_data (clist, col);
	      gtk_clist_get_pixmap (clist, row, 0, &pixmap, &mask);
	      view_color_list_render_pixmap (vcl, pixmap, col);
	    }
	  }
    
    list = g_list_next (list);
  }

  gtk_clist_thaw (clist);
  gtk_clist_sort (clist); 
}

static void
view_color_list_remove_selected (ViewColorGeneric *vcg)
{
  GtkCList *clist = GTK_CLIST (vcg->widget);
  GList *list = clist->selection;
  MDIColorGenericCol *col;
  GList *freezed = NULL;

  while (list) {  
    col = gtk_clist_get_row_data (clist, GPOINTER_TO_INT (list->data));
    
    col = mdi_color_generic_get_owner (col);
    
    mdi_color_generic_freeze (col->owner);
    mdi_color_generic_remove (col->owner, col);
    
    freezed = g_list_append (freezed, col->owner);
    
    list = g_list_next (list);
  }   

  list = freezed;
  while (list) {
    mdi_color_generic_thaw (freezed->data);
    list = g_list_next (list);
  }    

  g_list_free (freezed);
}

/*************************** PROPERTIES **********************************/

typedef struct prop_t {
  GladeXML *gui;

  gpointer parent_data;

  void (*changed_cb)(gpointer data);
  gpointer change_data;

  GtkWidget *spin_width;
  GtkWidget *spin_height;
  GtkWidget *check_draw_numbers;
} prop_t;

static void
spin_changed_cb (GtkWidget *widget, prop_t *prop)
{
  prop->changed_cb (prop->change_data);
}

static void
check_toggled_cb (GtkWidget *widget, prop_t *prop)
{
  prop->changed_cb (prop->change_data);
}

static gpointer 
view_color_list_get_control (ViewColorGeneric *vcg, GtkVBox *box,
			     void (*changed_cb)(gpointer data), 
			     gpointer change_data)
{
  GtkWidget *frame;
  prop_t *prop = g_new0 (prop_t, 1);
  GtkAdjustment *adj;

  prop->changed_cb  = changed_cb;
  prop->change_data = change_data;

  prop->parent_data = parent_class->get_control (vcg, box, 
						 changed_cb, change_data);

  prop->gui = glade_xml_new (GCOLORSEL_GLADEDIR "view-color-list-properties.glade", "frame");
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

  prop->check_draw_numbers = glade_xml_get_widget (prop->gui, 
						   "check-draw-numbers");
  gtk_signal_connect (GTK_OBJECT (prop->check_draw_numbers), "toggled",
		      GTK_SIGNAL_FUNC (check_toggled_cb), prop);

  return prop;
}

static void     
view_color_list_apply (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;
  GtkCList *clist = GTK_CLIST (vcg->widget);

  printf ("List    :: apply\n");

  VIEW_COLOR_LIST (vcg)->col_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prop->spin_width));
  
  VIEW_COLOR_LIST (vcg)->col_height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prop->spin_height));

  VIEW_COLOR_LIST (vcg)->draw_numbers = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop->check_draw_numbers));

  parent_class->apply (vcg, prop->parent_data);

  gtk_clist_freeze (clist);
  view_color_list_redraw_all (VIEW_COLOR_LIST (vcg));
  gtk_clist_thaw (clist);
}

static void 
view_color_list_close (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;

  printf ("List    :: close\n");

  parent_class->close (vcg, prop->parent_data);

  gtk_object_unref (GTK_OBJECT (prop->gui));
  g_free (prop);
}

static void 
view_color_list_sync (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;
  GtkAdjustment *adj;

  printf ("List    :: sync \n");

  /* spin-width */
  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (prop->spin_width));
  gtk_signal_handler_block_by_data (GTK_OBJECT (adj), prop);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON (prop->spin_width), 
			    VIEW_COLOR_LIST (vcg)->col_width);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (adj), prop);

  /* spin-height */
  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (prop->spin_height));
  gtk_signal_handler_block_by_data (GTK_OBJECT (adj), prop);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON (prop->spin_height),
			    VIEW_COLOR_LIST (vcg)->col_height);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (adj), prop);

  /* check-draw-numbers */
  gtk_signal_handler_block_by_data (GTK_OBJECT (prop->check_draw_numbers), prop);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop->check_draw_numbers),
				VIEW_COLOR_LIST (vcg)->draw_numbers);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (prop->check_draw_numbers), prop);

  parent_class->sync (vcg, prop->parent_data);
}
