#include <stdio.h>

#include <gnome.h>

#include "widget-color-list.h"

#include "utils.h"

#define COLOR_LIST_FORMAT        COLOR_LIST_FORMAT_DEC_8;
#define COLOR_LIST_PIXMAP_WIDTH  48
#define COLOR_LIST_PIXMAP_HEIGHT 15

#define COLOR_LIST_UP_ARROW_KEY    "up_arrow"
#define COLOR_LIST_DOWN_ARROW_KEY  "down_arrow"

#define COLOR_LIST_COLUMN_PIXMAP 0
#define COLOR_LIST_COLUMN_VALUE  1
#define COLOR_LIST_COLUMN_NAME   2

#define GIMP_PALETTE_HEADER      "GIMP Palette"

/* From nautilus, fm-directory-view-list.c */
static char * down_xpm[] = {
"6 5 2 1",
" 	c None",
".	c #000000",
"......",
"      ",
" .... ",
"      ",
"  ..  "};

/* From nautilus, fm-directory-view-list.c */
static char * up_xpm[] = {
"6 5 2 1",
" 	c None",
".	c #000000",
"  ..  ",
"      ",
" .... ",
"      ",
"......"};

static const GtkTargetEntry color_list_drag_targets[] = {
  { "application/x-color", 0 }
};

static void color_list_class_init (ColorListClass *class);
static void color_list_init (ColorList *cl);

GtkWidget *color_list_create_column_widget (ColorList *cl, int num, char *text);

char *color_list_render_value (ColorList *cl, int r, int g, int b);
gulong color_list_render_pixmap (ColorList *cl, GdkPixmap *pixmap,
				int r, int g, int b, int num);

static void color_list_drag_begin (GtkWidget *widget, GdkDragContext *context);
static void color_list_drag_data_get (GtkWidget *widget, 
				      GdkDragContext *context, 
				      GtkSelectionData *selection_data, 
				      guint info, guint time);
static void color_list_click_column (GtkCList *clist, gint column);

static GtkCListClass *parent_class = NULL;
static GtkContainerClass *container_class = NULL;

GtkType 
color_list_get_type (void)
{
  static guint cl_type = 0;

  if (!cl_type) {
    GtkTypeInfo cl_info = {
      "ColorList",
      sizeof (ColorList),
      sizeof (ColorListClass),
      (GtkClassInitFunc) color_list_class_init,
      (GtkObjectInitFunc) color_list_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };

    cl_type = gtk_type_unique (gtk_clist_get_type (), &cl_info);
  }

  return cl_type;
}

static void
color_list_class_init (ColorListClass *class)
{
  GtkWidgetClass *widget_class;
  GtkCListClass *clist_class;

  parent_class = gtk_type_class (GTK_TYPE_CLIST);
  container_class = gtk_type_class (GTK_TYPE_CONTAINER);
  widget_class = (GtkWidgetClass *)class;
  clist_class = (GtkCListClass *)class;

  widget_class->drag_begin = color_list_drag_begin;
  widget_class->drag_data_get = color_list_drag_data_get;

  clist_class->click_column = color_list_click_column;

  gdk_color_parse ("black", &class->color_black);
  gdk_color_alloc (gtk_widget_get_default_colormap (), &class->color_black);

  gdk_color_parse ("white", &class->color_white);
  gdk_color_alloc (gtk_widget_get_default_colormap (), &class->color_white);

  class->pixmap_font = gdk_font_load ("-bitstream-courier-medium-r-normal-*-12-*-*-*-*-*-*-*");
}

static void
color_list_init (ColorList *cl)
{
  cl->format = COLOR_LIST_FORMAT;

  cl->modified = FALSE;
}

GtkWidget *
color_list_create_column_widget (ColorList *cl, int num, char *text)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *up;
  GtkWidget *down;

  g_assert (cl != NULL);
  g_assert (text != NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  
  label = gtk_label_new (text);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, GNOME_PAD_SMALL);

  gtk_widget_show_all (hbox);

  up = gnome_pixmap_new_from_xpm_d (up_xpm);
  down = gnome_pixmap_new_from_xpm_d (down_xpm);

  gtk_box_pack_end (GTK_BOX (hbox), up, FALSE, FALSE, GNOME_PAD_SMALL); 
  gtk_box_pack_end (GTK_BOX (hbox), down, FALSE, FALSE, GNOME_PAD_SMALL); 

  gtk_object_set_data (GTK_OBJECT (hbox), COLOR_LIST_UP_ARROW_KEY, up);
  gtk_object_set_data (GTK_OBJECT (hbox), COLOR_LIST_DOWN_ARROW_KEY, down);

  gtk_clist_set_column_widget (GTK_CLIST (cl), num, hbox);

  return hbox;
}

static int
color_list_compare_rows (GtkCList *clist, 
			 gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCListRow *row1;
  GtkCListRow *row2;
  ColorListData *c1;
  ColorListData *c2;
  int t1;
  int t2;
  
  row1 = (GtkCListRow *) ptr1;
  row2 = (GtkCListRow *) ptr2;

  c1 = (ColorListData *) row1->data;
  c2 = (ColorListData *) row2->data;

  if (!c2) return (c1 != NULL);
  if (!c1) return -1;

  switch (clist->sort_column) {
  case COLOR_LIST_COLUMN_PIXMAP:
    if (c1->num < c2->num) return -1;
    if (c1->num > c2->num) return 1;
    return 0;

  case COLOR_LIST_COLUMN_NAME:
    return g_strcasecmp (c1->name, c2->name);

  case COLOR_LIST_COLUMN_VALUE:
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

GtkWidget *
color_list_new (void)
{
  GtkWidget *widget;

  widget = gtk_type_new (TYPE_COLOR_LIST);

  gtk_clist_construct (GTK_CLIST (widget), 3, NULL);

  gtk_clist_set_selection_mode (GTK_CLIST (widget), GTK_SELECTION_EXTENDED);

  gtk_clist_column_titles_show (GTK_CLIST (widget));
/*  gtk_clist_set_column_auto_resize (GTK_CLIST (widget), 
				    COLOR_LIST_COLUMN_PIXMAP, TRUE);  */

  color_list_create_column_widget (COLOR_LIST (widget), 
				   COLOR_LIST_COLUMN_PIXMAP, _("Color"));
  color_list_create_column_widget (COLOR_LIST (widget), 
				   COLOR_LIST_COLUMN_VALUE, _("Value"));
  color_list_create_column_widget (COLOR_LIST (widget), 
				   COLOR_LIST_COLUMN_NAME, _("Name"));

  gtk_clist_set_column_width (GTK_CLIST (widget), 0, COLOR_LIST_PIXMAP_WIDTH);
  gtk_clist_set_column_resizeable (GTK_CLIST (widget), 0, FALSE);
  gtk_clist_set_column_width (GTK_CLIST (widget), 1, 70);

  gtk_clist_set_auto_sort (GTK_CLIST (widget), TRUE);
  gtk_clist_set_compare_func (GTK_CLIST (widget), color_list_compare_rows);

  color_list_set_sort_column (COLOR_LIST (widget), COLOR_LIST_COLUMN_PIXMAP,
			      GTK_SORT_ASCENDING);

  gtk_drag_source_set (widget, GDK_BUTTON1_MASK, 
		       color_list_drag_targets, 1, GDK_ACTION_COPY);
		       
  return widget;
}

static void
color_list_show_arrow (ColorList *cl)
{
  GtkWidget *hbox;
  
  g_assert (cl != NULL);
  
  hbox = gtk_clist_get_column_widget (GTK_CLIST (cl), 
				      GTK_CLIST (cl)->sort_column);

  gtk_widget_show (gtk_object_get_data (GTK_OBJECT (hbox), 
                   GTK_CLIST (cl)->sort_type == GTK_SORT_ASCENDING ? 
   		   COLOR_LIST_UP_ARROW_KEY : COLOR_LIST_DOWN_ARROW_KEY));
}

static void
color_list_hide_arrow (ColorList *cl)
{
  GtkWidget *hbox;
  
  g_assert (cl != NULL);
  
  hbox = gtk_clist_get_column_widget (GTK_CLIST (cl), 
				      GTK_CLIST (cl)->sort_column);

  gtk_widget_hide (gtk_object_get_data (GTK_OBJECT (hbox), 
                   GTK_CLIST (cl)->sort_type == GTK_SORT_ASCENDING ? 
   		   COLOR_LIST_UP_ARROW_KEY : COLOR_LIST_DOWN_ARROW_KEY));
}

void
color_list_set_sort_column (ColorList *cl, gint column, GtkSortType type)
{
  color_list_hide_arrow (cl);

  gtk_clist_set_sort_column (GTK_CLIST (cl), column);
  gtk_clist_set_sort_type (GTK_CLIST (cl), type);

  color_list_show_arrow (cl);

  gtk_clist_sort (GTK_CLIST (cl));
}

static void
color_list_drag_begin (GtkWidget *widget, GdkDragContext *context)
{
  ColorListData *col;
  int row;

  if (GTK_CLIST (widget)->focus_row < 0) 
    row = 0;
  else 
    row = GTK_CLIST (widget)->focus_row;

  gtk_clist_select_row (GTK_CLIST (widget), row, 0);
  col = gtk_clist_get_row_data (GTK_CLIST(widget), row);

  if (!col) return;

  gtk_drag_set_icon_widget (context, 
	       drag_window_create (col->r, col->g, col->b), -2, -2);
}

static void 
color_list_drag_data_get (GtkWidget *widget, GdkDragContext *context, 
			  GtkSelectionData *selection_data, guint info,
			  guint time)
{
  ColorListData *col;
  guint16 vals[4];

  col = gtk_clist_get_row_data (GTK_CLIST (widget), 
				GTK_CLIST (widget)->focus_row);

  if (!col) return;
  
  vals[0] = ((gdouble)col->r / 255.0) * 0xffff;
  vals[1] = ((gdouble)col->g / 255.0) * 0xffff;
  vals[2] = ((gdouble)col->b / 255.0) * 0xffff;
  vals[3] = 0xffff;
  
  gtk_selection_data_set (selection_data, 
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}	         

static void
color_list_click_column (GtkCList *clist, gint column)
{
  GtkSortType type;

  if (column == clist->sort_column) 
    type = clist->sort_type == GTK_SORT_ASCENDING ? 
                                   GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
  else
    type = GTK_SORT_ASCENDING;

  color_list_set_sort_column (COLOR_LIST (clist), column, type);
}

void
color_list_set_format (ColorList *cl, ColorListFormat format)
{
  cl->format = format;
}

char *
color_list_render_value (ColorList *cl, int r, int g, int b)
{
  g_assert (cl != NULL);

  switch (cl->format) {
  case COLOR_LIST_FORMAT_DEC_8:
    return g_strdup_printf ("%d %d %d", r, g, b);
  case COLOR_LIST_FORMAT_DEC_16:
    return g_strdup_printf ("%d %d %d", r * 256, g * 256, b * 256);
  case COLOR_LIST_FORMAT_HEX_8:
    return g_strdup_printf ("#%02x%02x%02x", r, g, b);
  case COLOR_LIST_FORMAT_HEX_16:
    return g_strdup_printf ("#%04x%04x%04x", r * 256, g * 256, b * 256);
  case COLOR_LIST_FORMAT_FLOAT:
    return g_strdup_printf ("%1.4g %1.4g %1.4g", (float) r / 256,
                    			         (float) g / 256,
                        			 (float) b / 256);
  default:
    g_assert_not_reached ();
  }

  return NULL;   
}

gulong
color_list_render_pixmap (ColorList *cl, GdkPixmap *pixmap,
			  int r, int g, int b, int num)
{
  GdkColor color;
  int h, w;
  char *str;

  color.red = r * 255; 
  color.green = g * 255;
  color.blue = b * 255;
  
  gdk_color_alloc (gtk_widget_get_default_colormap (), &color);

  if (cl->gc == NULL) 
    cl->gc = gdk_gc_new (GTK_WIDGET (cl)->window);

  /* Border */
  gdk_gc_set_foreground (cl->gc, &COLOR_LIST_GET_CLASS (cl)->color_black);
  gdk_draw_rectangle (pixmap, cl->gc, FALSE, 0, 0, 
		      COLOR_LIST_PIXMAP_WIDTH - 1,
		      COLOR_LIST_PIXMAP_HEIGHT - 1);
  /* Color */
  gdk_gc_set_foreground (cl->gc, &color);
  gdk_draw_rectangle (pixmap, cl->gc, TRUE, 1, 1, 
		      COLOR_LIST_PIXMAP_WIDTH - 2,
		      COLOR_LIST_PIXMAP_HEIGHT - 2);


  str = g_strdup_printf ("%d", num);
  
  if ((r + g + b) < ((255 * 3) / 2)) 
    gdk_gc_set_foreground (cl->gc, &COLOR_LIST_GET_CLASS (cl)->color_white);
  else
    gdk_gc_set_foreground (cl->gc, &COLOR_LIST_GET_CLASS (cl)->color_black);
  
  h = gdk_string_height (COLOR_LIST_GET_CLASS(cl)->pixmap_font, str);  
  w = gdk_string_width  (COLOR_LIST_GET_CLASS(cl)->pixmap_font, str);
  
  gdk_draw_string (pixmap, COLOR_LIST_GET_CLASS (cl)->pixmap_font, cl->gc, 
		   (COLOR_LIST_PIXMAP_WIDTH - w) / 2,
		   ((COLOR_LIST_PIXMAP_HEIGHT - h) / 2) + h, str);
  
  g_free (str);

  return color.pixel;
}

static void
row_destroy_notify (gpointer data)
{
//  GdkColor c;
  ColorListData *col = data;

  g_assert (col != NULL);

//  c.pixel = col->pixel;
//  gdk_colormap_free_colors (gtk_widget_get_default_colormap (), &c, 1);

  g_free (col->name);
  g_free (col);
}

gint
color_list_append (ColorList *cl, int r, int g, int b, int num, char *name)
{
  ColorListData *data;
  gchar *string[3];
  gint row;
  GdkPixmap *pixmap;

  g_assert (cl != NULL);
  g_assert (name != NULL);
  g_assert ((r >= 0) && (r <= 255));
  g_assert ((g >= 0) && (g <= 255));
  g_assert ((b >= 0) && (b <= 255));
  
  data = g_new (ColorListData, 1);

  data->r = r;
  data->g = g;
  data->b = b;
  data->num = num;
  data->name = g_strdup (name);

  pixmap = gdk_pixmap_new (GTK_WIDGET (cl)->window, 
			   COLOR_LIST_PIXMAP_WIDTH,
			   COLOR_LIST_PIXMAP_HEIGHT, 
			   -1);

  data->pixel = color_list_render_pixmap (COLOR_LIST (cl), pixmap, 
					 r, g, b, num);

  string[COLOR_LIST_COLUMN_PIXMAP] = NULL;
  string[COLOR_LIST_COLUMN_VALUE] = color_list_render_value (cl, r, g, b);
  string[COLOR_LIST_COLUMN_NAME] = data->name;
  
  row = gtk_clist_append (GTK_CLIST (cl), string);
  gtk_clist_set_pixmap (GTK_CLIST (cl), row, 
			COLOR_LIST_COLUMN_PIXMAP, pixmap, NULL);

  if (string[COLOR_LIST_COLUMN_VALUE]) 
    g_free (string[COLOR_LIST_COLUMN_VALUE]);

  gtk_clist_set_row_data_full (GTK_CLIST (cl), row, data, row_destroy_notify);

  gdk_pixmap_unref (pixmap);
    
  return row;
}

gint 
color_list_load (ColorList *cl, gchar *filename, GnomeApp *app)
{
  FILE *fp;
  int r, g, b;
  char tmp[256], name[256];
  int i = 0;
  long size;
  char *str;

  g_assert (cl != NULL);
  g_assert (filename != NULL);

  fp = fopen(filename, "r");
  if (!fp) return -1;

  gtk_clist_freeze (GTK_CLIST (cl));

  fseek (fp, 0, SEEK_END);
  size = ftell (fp);
  fseek (fp, 0, SEEK_SET);

  str = g_strdup_printf (_("Loading %s ..."), filename);
  gnome_appbar_push (GNOME_APPBAR (app->statusbar), str);
  g_free (str);

  while (1) {
    fgets(tmp, 255, fp);    

    if (feof (fp)) break;

    if ((tmp[0] == '!') || (tmp[0] == '#')) continue;
    if (!strncmp (tmp, GIMP_PALETTE_HEADER, 
		  strlen (GIMP_PALETTE_HEADER)))  continue;

    name[0] = 0;
    if (sscanf(tmp, "%d %d %d\t\t%[a-zA-Z0-9 ]\n", &r, &g, &b, name) >= 3) 
      color_list_append (cl, r, g, b, i, name);
    else
      return -1;

    gnome_appbar_set_progress (GNOME_APPBAR (app->statusbar), 
			       (float)ftell (fp) / (float)size);

    while (gtk_events_pending ()) gtk_main_iteration ();
      
    i++;
  }

  gnome_appbar_pop (GNOME_APPBAR (app->statusbar));
  gnome_appbar_set_progress (GNOME_APPBAR (app->statusbar), 0); 

  gtk_clist_thaw (GTK_CLIST (cl));
  gtk_clist_sort (GTK_CLIST (cl));

  return 0;
}

gint 
color_list_save (ColorList *cl, gchar *filename, GnomeApp *app)
{
  FILE *file;
  GList *row_list;
  GtkCListRow *row;
  ColorListData *data;
  char *tmp;
  int size, pos = 0;

  g_assert (cl != NULL);
  g_assert (filename != NULL);

  file = fopen (filename, "w");
  if (!file) return -1;
  
  row_list = GTK_CLIST (cl)->row_list;

  size = g_list_length (row_list);

  tmp = g_strdup_printf (_("Saving %s ..."), filename);
  gnome_appbar_push (GNOME_APPBAR (app->statusbar), tmp);
  g_free (tmp);

  while (row_list) {
    row = row_list->data;
    g_assert (row != NULL);

    data = row->data; 
    g_assert (data != NULL);

    tmp = g_strdup_printf ("%d %d %d\t\t%s\n", 
			   data->r, data->g, data->b, data->name);

    fputs (tmp, file);   
    g_free (tmp);
    
    gnome_appbar_set_progress (GNOME_APPBAR (app->statusbar), 
			       (float)pos / (float)size);

    while (gtk_events_pending ()) gtk_main_iteration ();

    row_list = row_list->next; pos++;
  }

  fclose (file);

  gnome_appbar_pop (GNOME_APPBAR (app->statusbar));
  gnome_appbar_set_progress (GNOME_APPBAR (app->statusbar), 0); 

  return 0;
}

void 
color_list_set_modified (ColorList *cl, gboolean val)
{
  cl->modified = val;
}
