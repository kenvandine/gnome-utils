#include <stdio.h>

#include <gnome.h>

#include "widget-color-search.h"
#include "widget-color-list.h"

#include "utils.h"

/* Signals we emit */
enum {
  SEARCH_UPDATE,
  LAST_SIGNAL
};

static guint cs_signals [LAST_SIGNAL] = { 0 };

static void color_search_class_init (ColorSearchClass *class);
static void color_search_init (ColorSearch *cl);

static void preview_drag_begin_cb (GtkWidget *widget, 
				   GdkDragContext *context, GtkWidget *cs);

static void preview_drag_data_get_cb (GtkWidget *widget, 
				      GdkDragContext *context, 
				      GtkSelectionData *selection_data, 
				      guint info,
				      guint time, GtkWidget *cs);

static GtkHBoxClass *parent_class = NULL;

static const GtkTargetEntry preview_drag_targets[] = {
  { "application/x-color", 0 }
};

GtkType 
color_search_get_type (void)
{
  static guint cs_type = 0;

  if (!cs_type) {
    GtkTypeInfo cs_info = {
      "ColorSearch",
      sizeof (ColorSearch),
      sizeof (ColorSearchClass),
      (GtkClassInitFunc) color_search_class_init,
      (GtkObjectInitFunc) color_search_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };

    cs_type = gtk_type_unique (gtk_vbox_get_type (), &cs_info);
  }

  return cs_type;
}

static void
color_search_class_init (ColorSearchClass *class)
{
  GtkObjectClass *object_class;

  parent_class = gtk_type_class (GTK_TYPE_VBOX);
  object_class = (GtkObjectClass *) class;

  /* The signals */
  cs_signals [SEARCH_UPDATE] =
    gtk_signal_new ("search_update",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (ColorSearchClass, search_update),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE, 0);
	
  gtk_object_class_add_signals (object_class, cs_signals, LAST_SIGNAL);
}

static GtkWidget *
range_create (gchar *title, gfloat max, gfloat val,
	      GtkWidget **prange, GtkSignalFunc cb, gpointer data)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *range;
  GtkObject *adj;

  adj = gtk_adjustment_new (val, 0.0, max, 1.0, 1.0, 1.0);

  if (cb)
    gtk_signal_connect (GTK_OBJECT (adj), "value_changed", cb, data);

  hbox = gtk_hbox_new (TRUE, 2);

  label = gtk_label_new (title);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

  range = gtk_hscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_value_pos (GTK_SCALE (range), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (range), 0);

  gtk_box_pack_start_defaults(GTK_BOX(hbox), range);

  if (prange) *prange = range;

  return hbox;
}

static void 
range_value_changed_cb (GtkWidget *widget, gpointer data)
{
  color_search_update_preview (COLOR_SEARCH (data));

  gtk_signal_emit (GTK_OBJECT (data), cs_signals [SEARCH_UPDATE]);
}

static void
color_search_init (ColorSearch *cs)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *frame;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (cs), hbox, FALSE, FALSE, 2);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 2);

  /* Create entries */
  
  gtk_box_pack_start_defaults (GTK_BOX(vbox),
	      range_create (_("Red :"), 256.0, 0.0, 
			    &cs->range_red, range_value_changed_cb, cs));

  gtk_box_pack_start_defaults (GTK_BOX(vbox),
	      range_create (_("Green : "), 256.0, 0.0, 
			    &cs->range_green, range_value_changed_cb, cs));

  gtk_box_pack_start_defaults (GTK_BOX(vbox),
              range_create (_("Blue : "), 256.0, 0.0, 
			    &cs->range_blue, range_value_changed_cb, cs));

  gtk_box_pack_start_defaults (GTK_BOX(vbox),
              range_create (_("Tolerance (%) : "), 101.0, 10.0, 
			    &cs->range_tolerance, range_value_changed_cb, cs));
 
  /* Create preview */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(hbox), frame, TRUE, TRUE, 2);

  cs->preview = gtk_preview_new (GTK_PREVIEW_COLOR);    
  gtk_preview_set_expand (GTK_PREVIEW (cs->preview), TRUE);  
  gtk_container_add (GTK_CONTAINER (frame), cs->preview);

  gtk_drag_source_set (cs->preview, GDK_BUTTON1_MASK,
		       preview_drag_targets, 1, 
		       GDK_ACTION_COPY);

  /*  gtk_signal_connect (GTK_OBJECT (preview), "button_press_event",
      GTK_SIGNAL_FUNC (preview_button_press_cb), 
      gnome_popup_menu_new (popup_menu_add)); */
  
  gtk_signal_connect (GTK_OBJECT (cs->preview), "drag_data_get",
		      GTK_SIGNAL_FUNC (preview_drag_data_get_cb), cs);
  
  gtk_signal_connect (GTK_OBJECT (cs->preview), "drag_begin",
		      GTK_SIGNAL_FUNC (preview_drag_begin_cb), cs);

  /* Create ColorList */

  cs->sw = gtk_scrolled_window_new (NULL, NULL);  
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cs->sw),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);

  cs->cl = color_list_new ();

  gtk_container_add (GTK_CONTAINER (cs->sw), cs->cl);

  gtk_box_pack_start (GTK_BOX (cs), cs->sw, TRUE, TRUE, 0);
}

GtkWidget *
color_search_new (void)
{
  GtkWidget *widget;

  widget = gtk_type_new (TYPE_COLOR_SEARCH);

  return widget;
}

static void 
preview_drag_begin_cb (GtkWidget *widget, 
		       GdkDragContext *context, GtkWidget *cs)
{
  gint red, green, blue;

  color_search_get_rgb (COLOR_SEARCH (cs), &red, &green, &blue);
    
  gtk_drag_set_icon_widget (context, 
			    drag_window_create (red, green, blue), -2, -2);
}

static void 
preview_drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, 
			  GtkSelectionData *selection_data, guint info,
			  guint time, GtkWidget *cs)
{
  gint red, green, blue;
  guint16 vals[4];

  color_search_get_rgb (COLOR_SEARCH (cs), &red, &green, &blue);

  vals[0] = (red / 255.0) * 0xffff;
  vals[1] = (green / 255.0) * 0xffff;
  vals[2] = (blue / 255.0) * 0xffff;
  vals[3] = 0xffff;
    
  gtk_selection_data_set (selection_data, 
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}	         

void
color_search_get_rgb (ColorSearch *cs, int *r, int *g, int *b)
{
  g_assert (cs != NULL);

  if (r) *r = gtk_range_get_adjustment (GTK_RANGE (cs->range_red))->value; 
  if (g) *g = gtk_range_get_adjustment (GTK_RANGE (cs->range_green))->value;
  if (b) *b = gtk_range_get_adjustment (GTK_RANGE (cs->range_blue))->value;
}

gint
color_search_get_tolerance (ColorSearch *cs)
{
  g_assert (cs != NULL);

  return gtk_range_get_adjustment (GTK_RANGE (cs->range_tolerance))->value;
}

void
color_search_update_preview (ColorSearch *cs)
{
  guchar *buf;    
  int x, y;
  int r, g, b;

  color_search_get_rgb (cs, &r, &g, &b);

  buf = g_new (guchar, 3 * cs->preview->allocation.width);
  
  for (x = 0; x < cs->preview->allocation.width; x++) {
    buf [x * 3] = r;
    buf [x * 3 + 1] = g;
    buf [x * 3 + 2] = b;
  }

  for (y=0; y < cs->preview->allocation.height; y++)
     gtk_preview_draw_row (GTK_PREVIEW (cs->preview), 
			   buf, 0, y, cs->preview->allocation.width);
     
  gtk_widget_draw (cs->preview, NULL);

  g_free (buf);    
}

void 
color_search_update_result (ColorSearch *cs)
{

}

void 
color_search_update (ColorSearch *cs)
{

}

void
color_search_search (ColorSearch *cs, gint r, gint g, gint b)
{
  gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (cs->range_red)), r);
  gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (cs->range_green)), g);
  gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (cs->range_blue)), b);

  color_search_update_preview (cs);
}

static int 
color_get_diff (int red1, int green1, int blue1, 
	       int red2, int green2, int blue2)
{
  return abs (red1 - red2) + abs (green1 - green2) + abs (blue1 - blue2); 
}                 

void 
color_search_search_in (ColorSearch *cs, GtkCList *clist)
{
  GList *row_list;
  GtkCListRow *row;
  int red, green, blue, tolerance;
  ColorListData *data;
  int diff;

  g_assert (cs != NULL);
  g_assert (clist != NULL);

  color_search_get_rgb (cs, &red, &green, &blue);
  tolerance = color_search_get_tolerance (cs);

  row_list = clist->row_list;

  while (row_list) {
    row = row_list->data;
    g_assert (row != NULL);

    data = row->data; 
    g_assert (data != NULL);

    diff = color_get_diff (data->r, data->g, data->b, 
			   red, green, blue);

    if (((float)diff / (255.0 * 3.0)) * 100.0  <= (float)tolerance) {
      color_list_append (COLOR_LIST (cs->cl), data->r, data->g, data->b, 
			 diff, data->name);
    }    

    row_list = row_list->next;
  }
}
