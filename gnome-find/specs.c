#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gnome.h>
#include "gnome-find.h"

typedef struct s_spec_type spec_type;
typedef struct s_spec_item spec_item;
typedef struct s_spec_type_and_item spec_type_and_item;

struct s_spec_type {
  char *name;
  void (*constructor)(spec_item *);
  void (*destructor)(spec_item *);
  void (*set_advanced)(spec_item *, gboolean);
};

struct s_spec_item {
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *hbox;
  int initialized:1;
  int advanced:1;
  spec_type *type;
  spec_item *next;
};

struct s_spec_type_and_item {
  spec_type *type;
  spec_item *item;
};

typedef enum {
  SPEC_TYPE_NAMED = 0,
  SPEC_TYPE_CONTAINING = 1,
  SPEC_TYPE_LAST_MODIFIED = 2,
  SPEC_TYPE_OF_TYPE = 3,
  SPEC_TYPE_CREATED = 4,
  SPEC_TYPE_WITH_SIZE = 5,
  SPEC_OWNED_BY = 6,
  SPEC_WITH_GROUP = 7
} SpecType;

static void spec_type_pattern_constructor (spec_item *si);
static void spec_type_date_constructor (spec_item *si);
static void spec_type_of_type_constructor (spec_item *si);
static void spec_type_with_size_constructor (spec_item *si);
static void spec_type_noop_constructor (spec_item *si);

static void spec_type_pattern_destructor (spec_item *si);
static void spec_type_date_destructor (spec_item *si);
static void spec_type_of_type_destructor (spec_item *si);
static void spec_type_with_size_destructor (spec_item *si);
static void spec_type_noop_destructor (spec_item *si);

static void spec_type_pattern_set_advanced (spec_item *si, int advanced);
static void spec_type_of_type_set_advanced (spec_item *si, int advanced);
static void spec_type_with_size_set_advanced (spec_item *si, int advanced);
static void spec_type_noop_set_advanced (spec_item *si, int advanced);

static GtkWidget *add_option_to_menu_setting_data(GtkWidget *menu, char *text);

static spec_item *new_spec (int advanced);
void spec_set_advanced (spec_item *s, int advanced);
static void spec_option_select_cb (GtkWidget *widget, void *data);
static GtkWidget *add_spec_option_menu_item(GtkWidget *menu, 
					    spec_item *spec, SpecType st);


static spec_type spec_types[] = {
  {"Named:", 
   spec_type_pattern_constructor, 
   spec_type_pattern_destructor, 
   spec_type_pattern_set_advanced
  },
  {"Containing:", 
   spec_type_pattern_constructor, 
   spec_type_pattern_destructor, 
   spec_type_pattern_set_advanced
  },
  {"Last modified:", 
   spec_type_date_constructor, 
   spec_type_date_destructor, 
   spec_type_noop_set_advanced
  },
  {"Of type:", 
   spec_type_of_type_constructor, 
   spec_type_of_type_destructor, 
   spec_type_of_type_set_advanced
  },
  {"Created:", 
   spec_type_date_constructor, 
   spec_type_date_destructor, 
   spec_type_noop_set_advanced},
  {"With size:", 
   spec_type_with_size_constructor, 
   spec_type_with_size_destructor, 
   spec_type_noop_set_advanced
  },
  {"Owned by:", 
   spec_type_noop_constructor, 
   spec_type_noop_destructor, 
   spec_type_noop_set_advanced
  },
  {"With group:", 
   spec_type_noop_constructor, 
   spec_type_noop_destructor, 
   spec_type_noop_set_advanced
  }
};




spec_item *spec_list = NULL;
int num_specs = 0;

GtkWidget *spec_vbox;




GtkWidget *
init_spec_vbox (int advanced)
{
  spec_vbox = gtk_vbox_new(FALSE,5);
  spec_vbox_add_spec(advanced);
  gtk_widget_show (spec_vbox);

  return spec_vbox;
}

void 
spec_vbox_add_spec (int advanced)
{
  spec_item *si;
  
  si = new_spec(advanced);

  si->next = spec_list;
  spec_list=si;
  gtk_box_pack_start(GTK_BOX(spec_vbox), si->table, FALSE, FALSE, 0);
  gtk_widget_show(si->table);
    
  num_specs++;
  printf ("specs: %d\n", num_specs);
}

void 
spec_vbox_remove_spec ()
{
  if (num_specs > 1) {
    spec_item *tmp;

    gtk_widget_hide(spec_list->table);
    gtk_container_remove(GTK_CONTAINER(spec_vbox),spec_list->table);
    tmp=spec_list->next;
    free(spec_list);
    spec_list=tmp;

    num_specs--;
  }

  printf ("specs: %d\n", num_specs);
}

void 
spec_vbox_set_advanced (int advanced)
{
  spec_item *p;

  for (p = spec_list; NULL != p; p = p->next) {
    spec_set_advanced(p, advanced);
  }
}


GtkWidget *
add_spec_option_menu_item(GtkWidget *menu, spec_item *spec, SpecType st)
{
  GtkWidget *tmp;
  spec_type_and_item *sti;

  tmp = gtk_menu_item_new_with_label(spec_types[st].name);
  gtk_widget_show(tmp);

  sti = (spec_type_and_item *) malloc(sizeof(spec_type_and_item));

  sti->type=&spec_types[st];
  sti->item=spec;

  gtk_signal_connect_full(GTK_OBJECT(tmp), "activate", 
			  spec_option_select_cb, NULL,
			  sti, free, FALSE, FALSE);

  gtk_menu_append(GTK_MENU(menu), tmp);

  return tmp;
}

void
spec_set_advanced (spec_item *si, int advanced)
{
  (si->type->set_advanced)(si,advanced);
}

static spec_item *
new_spec (int advanced)
{
  spec_item *retval = (spec_item *) malloc (sizeof (spec_item));
  GtkWidget *menu;
  GtkWidget *option_menu;
  GtkWidget *entry, *entry2;
  GtkWidget *first_item;

  retval->initialized = 0;
  retval->advanced = advanced;

  menu = gtk_menu_new();
 
  first_item = add_spec_option_menu_item(menu, retval, SPEC_TYPE_NAMED);
  add_spec_option_menu_item(menu, retval, SPEC_TYPE_CONTAINING);
  add_spec_option_menu_item(menu, retval, SPEC_TYPE_LAST_MODIFIED);
  add_spec_option_menu_item(menu, retval, SPEC_TYPE_OF_TYPE);
  add_spec_option_menu_item(menu, retval, SPEC_TYPE_CREATED);
  add_spec_option_menu_item(menu, retval, SPEC_TYPE_WITH_SIZE);
  add_spec_option_menu_item(menu, retval, SPEC_OWNED_BY);
  add_spec_option_menu_item(menu, retval, SPEC_WITH_GROUP);

  entry = gtk_entry_new();

  gtk_widget_show (entry);

  option_menu=gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_show(option_menu);

  retval->hbox = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(retval->hbox);
  gtk_box_pack_end(GTK_BOX(retval->hbox), entry, TRUE, TRUE, 0);

  retval->vbox = gtk_vbox_new(FALSE, 5);
  gtk_widget_show(retval->vbox);
  gtk_box_pack_start(GTK_BOX(retval->vbox), retval->hbox,
		     FALSE, FALSE, 0);

  retval->table = gtk_table_new(1, 2, 0);
  gtk_table_set_col_spacings(GTK_TABLE(retval->table), 5);
  gtk_table_attach(GTK_TABLE(retval->table), option_menu, 0, 1, 0, 1,
		   0, 0, 0, 0);
  gtk_table_attach(GTK_TABLE(retval->table), retval->vbox, 1, 2, 0, 1,
		   GTK_FILL | GTK_EXPAND, 0, 0, 0);


  gtk_menu_item_activate(GTK_MENU_ITEM(first_item));

  return retval;
}


static void
spec_option_select_cb (GtkWidget *widget, void *data)
{
  spec_type_and_item *sti = (spec_type_and_item *) data;
  GList *children;

  if (sti->item->initialized) {
    if (sti->item->type->constructor != sti->type->constructor) {
      (sti->item->type->destructor)(sti->item);
      sti->item->type = sti->type;
      (sti->type->constructor)(sti->item);
    }
  } else {
    sti->item->initialized = 1;
    sti->item->type = sti->type;
    (sti->type->constructor)(sti->item);
  }

  printf("Spec type: %s.\n", sti->type->name);
}


static void
generic_menuitem_cb (GtkWidget *widget, gpointer data)
{
  GtkWidget *other_widget = (GtkWidget *) data;
  
  gtk_object_set_user_data 
    (GTK_OBJECT(other_widget),
     gtk_object_get_user_data(GTK_OBJECT(widget)));
}

static GtkWidget *
add_option_to_menu_setting_data(GtkWidget *menu, char *text)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label(text);
  gtk_object_set_user_data(GTK_OBJECT(menuitem), text);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
		     generic_menuitem_cb, menu);
  gtk_widget_show(menuitem);
  gtk_menu_append(GTK_MENU(menu), menuitem);

  return (menuitem);
}


/* For `named' and `containing' searches. */

static void
spec_type_pattern_constructor(spec_item *si)
{
  GtkWidget *extra_option_hbox;
  GtkWidget *pattern_type_option;
  GtkWidget *pattern_type_menu;
  GtkWidget *first_item;
  GtkWidget *case_sensitive_check;

  pattern_type_menu = gtk_menu_new();

  first_item = add_option_to_menu_setting_data (pattern_type_menu, 
						"Pattern");
  add_option_to_menu_setting_data (pattern_type_menu, "Regexp");
  add_option_to_menu_setting_data (pattern_type_menu, "Exact");

  pattern_type_option = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(pattern_type_option),
			   pattern_type_menu);
  gtk_menu_item_activate(GTK_MENU_ITEM(first_item));
  gtk_widget_show(pattern_type_option);

  case_sensitive_check = gtk_check_button_new_with_label ("Case sensitive");
  gtk_widget_show(case_sensitive_check);

  extra_option_hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(extra_option_hbox), case_sensitive_check,
		     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(extra_option_hbox), pattern_type_option, 
		     FALSE, FALSE, 0);

  gtk_box_pack_end(GTK_BOX(si->vbox), extra_option_hbox,
		   FALSE, FALSE, 0);

  if (si->advanced) {
    gtk_widget_show (extra_option_hbox);
  }

  gtk_object_set_user_data(GTK_OBJECT(si->table), extra_option_hbox);
}


static void
spec_type_pattern_destructor(spec_item *si)
{
  GtkWidget *extra_option_hbox;

  extra_option_hbox = gtk_object_get_user_data(GTK_OBJECT(si->table));

  gtk_widget_hide (extra_option_hbox);
  gtk_container_remove (GTK_CONTAINER(si->vbox), extra_option_hbox);
}

static void 
spec_type_pattern_set_advanced (spec_item *si, int advanced)
{
  GtkWidget *extra_option_hbox;
  extra_option_hbox = gtk_object_get_user_data(GTK_OBJECT(si->table));

  if (advanced) {
    gtk_widget_show (extra_option_hbox);
  } else {
    gtk_widget_hide (extra_option_hbox);
  }
}


/* For 'last modified' and 'created' searches. */

static void
spec_type_date_constructor(spec_item *si)
{
  GtkWidget *comparison_option;
  GtkWidget *comparison_menu;
  GtkWidget *first_item;

  comparison_menu = gtk_menu_new();

  first_item = add_option_to_menu_setting_data (comparison_menu, 
						"Before");
  add_option_to_menu_setting_data (comparison_menu, "After");
  add_option_to_menu_setting_data (comparison_menu, "Exactly");

  comparison_option = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(comparison_option),
			   comparison_menu);
  gtk_menu_item_activate(GTK_MENU_ITEM(first_item));

  gtk_widget_show(comparison_option);

  gtk_box_pack_start(GTK_BOX(si->hbox), comparison_option,
		     FALSE, FALSE, 0);

  gtk_object_set_user_data(GTK_OBJECT(si->table), comparison_option);
}

static void
spec_type_date_destructor(spec_item *si)
{
  GtkWidget *extra_option;

  extra_option = gtk_object_get_user_data(GTK_OBJECT(si->table));

  gtk_widget_hide (extra_option);
  gtk_container_remove (GTK_CONTAINER(si->hbox), extra_option);
}

static void
spec_type_of_type_constructor(spec_item *si)
{
}

static void
spec_type_of_type_destructor(spec_item *si)
{
}

static void spec_type_of_type_set_advanced (spec_item *si, int advanced)
{
}


static void
spec_type_with_size_constructor(spec_item *si)
{
  GtkWidget *comparison_option;
  GtkWidget *comparison_menu;
  GtkWidget *less_menuitem;
  GtkWidget *greater_menuitem;
  GtkWidget *exactly_menuitem;  

}

static void
spec_type_with_size_destructor(spec_item *si)
{  
}

static void spec_type_with_size_set_advanced (spec_item *si, int advanced)
{
}

static void
spec_type_noop_constructor(spec_item *si)
{
}

static void
spec_type_noop_destructor(spec_item *si)
{
}

static void 
spec_type_noop_set_advanced (spec_item *si, int advanced)
{
}



