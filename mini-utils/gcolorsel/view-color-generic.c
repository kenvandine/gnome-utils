#include <stdio.h>
#include <gnome.h>
#include <glade/glade.h>

#include "view-color-generic.h"

enum {
  DATA_CHANGED,
  REMOVE_SELECTED,
  LAST_SIGNAL
};

static guint cg_signals [LAST_SIGNAL] = { 0 };

char *ColFormatStr[] = { N_("Decimal 8 bits"),
			 N_("Decimal 16 bits"),
			 N_("Hex 8 bits"),
			 N_("Hex 16 bits"),
			 N_("Float"),
			 NULL };
				 
static void view_color_generic_class_init      (ViewColorGenericClass *class);
static void view_color_generic_init            (ViewColorGeneric *vcl);

static gpointer view_color_generic_get_control (ViewColorGeneric *vcg,
						GtkVBox *box,
						void (*changed_cb) (gpointer data), gpointer change_data);
static void     view_color_generic_apply       (ViewColorGeneric *vcg,
						gpointer data);
static void     view_color_generic_close       (ViewColorGeneric *vcg,
						gpointer data);
static void     view_color_generic_sync        (ViewColorGeneric *vcg,
						gpointer data);

static GList   *str_tab_2_str_list             (char *tab[]);

static GtkObjectClass *parent_class = NULL;

GtkType 
view_color_generic_get_type (void)
{
  static guint cg_type = 0;

  if (!cg_type) {
    GtkTypeInfo cg_info = {
      "ViewColorGeneric",
      sizeof (ViewColorGeneric),
      sizeof (ViewColorGenericClass),
      (GtkClassInitFunc) view_color_generic_class_init,
      (GtkObjectInitFunc) view_color_generic_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };

    cg_type = gtk_type_unique (gtk_object_get_type (), &cg_info);
  }

  return cg_type;
}

static void
view_color_generic_class_init (ViewColorGenericClass *class)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);
  parent_class = gtk_type_class (GTK_TYPE_OBJECT);
  widget_class = (GtkWidgetClass *)class;

  cg_signals [DATA_CHANGED] = 
    gtk_signal_new ("data_changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (ViewColorGenericClass, data_changed),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

  cg_signals [REMOVE_SELECTED] = 
    gtk_signal_new ("remove_selected",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (ViewColorGenericClass, remove_selected),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, cg_signals, LAST_SIGNAL);

  class->get_control = view_color_generic_get_control;
  class->apply       = view_color_generic_apply;
  class->close       = view_color_generic_close;
  class->sync        = view_color_generic_sync;
}

static void
view_color_generic_init (ViewColorGeneric *vcg)
{
  vcg->format = FORMAT_DEC_8;
}

void 
view_color_generic_data_changed (ViewColorGeneric *vcg, GList *changes)
{
  printf ("data_changed ...\n");
  gtk_signal_emit (GTK_OBJECT (vcg), cg_signals[DATA_CHANGED], changes);
}

void 
view_color_generic_remove_selected (ViewColorGeneric *vcg)
{
  gtk_signal_emit (GTK_OBJECT (vcg), cg_signals[REMOVE_SELECTED]);
}

/************************* PROPERTIES ********************************/

typedef struct prop_t {
  GladeXML *gui;

  void (*changed_cb) (gpointer data);
  gpointer change_data;

  GtkWidget *combo_format;
  GtkWidget *check_show_control; 
} prop_t;

static GList *
str_tab_2_str_list (char *tab[])
{
  int i = 0;
  GList *list = NULL;

  while (tab[i]) 
    list = g_list_append (list, tab[i++]);
  
  return list;
}

static void
combo_changed (GtkWidget *widget, prop_t *prop)
{
  prop->changed_cb (prop->change_data);
}

static void
check_toggled_cb (GtkWidget *widget, prop_t *prop)
{
  prop->changed_cb (prop->change_data);
}

static gpointer
view_color_generic_get_control (ViewColorGeneric *vcg, GtkVBox *box,
				void (*changed_cb)(gpointer data), 
				gpointer change_data)
{
  GtkWidget *frame;
  GList *list;
  prop_t *prop = g_new0 (prop_t, 1);

  prop->changed_cb  = changed_cb;
  prop->change_data = change_data;

  prop->gui = glade_xml_new (GCOLORSEL_GLADEDIR "view-color-generic-properties.glade", "frame");
  g_assert (prop->gui != NULL);

  frame = glade_xml_get_widget (prop->gui, "frame");
  g_assert (frame != NULL);

  gtk_box_pack_start_defaults (GTK_BOX (box), frame);

  /* Format */

  prop->combo_format = glade_xml_get_widget (prop->gui, "combo-format");

  list = str_tab_2_str_list (ColFormatStr);
  gtk_combo_set_popdown_strings (GTK_COMBO (prop->combo_format), list);
  g_list_free (list);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (prop->combo_format)->entry), "changed", GTK_SIGNAL_FUNC (combo_changed), prop);

  /* Show control */

  prop->check_show_control = glade_xml_get_widget (prop->gui, 
						   "check-show-control");
  gtk_signal_connect (GTK_OBJECT (prop->check_show_control), "toggled",
		      GTK_SIGNAL_FUNC (check_toggled_cb), prop);

  return prop;
}

static void     
view_color_generic_apply (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;
  int i = 0;
  GtkWidget *entry;

  printf ("Generic :: apply\n");

  /* Format */

  entry = GTK_COMBO (prop->combo_format)->entry;
  while (strcmp (ColFormatStr[i], gtk_entry_get_text (GTK_ENTRY (entry)))) i++;
  vcg->format = i;

  /* Show control */

  vcg->show_control = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop->check_show_control));
  if (vcg->show_control) {
    if (vcg->control) gtk_widget_show (GTK_WIDGET (vcg->control));
  } else 
    if (vcg->control) gtk_widget_hide (GTK_WIDGET (vcg->control));
}

static void 
view_color_generic_close (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;

  printf ("Generic :: close\n");

  gtk_object_unref (GTK_OBJECT (prop->gui));
  g_free (prop);
}

static void 
view_color_generic_sync (ViewColorGeneric *vcg, gpointer data)
{
  prop_t *prop = data;
  GtkWidget *entry;

  printf ("Generic :: sync\n");

  /* Format */

  entry = GTK_COMBO (prop->combo_format)->entry;

  gtk_signal_handler_block_by_data (GTK_OBJECT (entry), prop);
  gtk_entry_set_text (GTK_ENTRY (entry), ColFormatStr[vcg->format]);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (entry), prop);

  /* Show control */

  gtk_signal_handler_block_by_data (GTK_OBJECT (prop->check_show_control), prop);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop->check_show_control),
				vcg->show_control);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (prop->check_show_control), prop);
}
