#include "mdi-color-virtual.h"
#include "mdi-color-generic.h"
#include "widget-control-virtual.h"
#include "menus.h"
#include "utils.h"

#include <gnome.h>

static void         mdi_color_virtual_class_init (MDIColorVirtualClass *class);
static void         mdi_color_virtual_init       (MDIColorVirtual *mcv);
static void         mdi_color_virtual_changed    (MDIColorGeneric *mcg,
						  gpointer data);

static int          color_diff                   (int r1, int g1, int b1, 
						  int r2, int g2, int b2);
static int          mdi_color_virtual_get_diff   (MDIColorVirtual *mcv, 
						  MDIColor *col);

MDIColor           *mdi_color_virtual_get_owner  (MDIColor *col);
GtkType             mdi_color_virtual_get_control_type (MDIColorGeneric *mcg);
static GList *      mdi_color_virtual_get_append_pos (MDIColorGeneric *mcg,
						      MDIColor *col);

static void         mdi_color_virtual_save       (MDIColorGeneric *mcg);
static void         mdi_color_virtual_load       (MDIColorGeneric *mcg);

static MDIColorGenericClass *parent_class = NULL;

guint 
mdi_color_virtual_get_type()
{
  static guint mdi_gen_child_type = 0;
  
  if (!mdi_gen_child_type) {
    GtkTypeInfo mdi_gen_child_info = {
      "MDIColorVirtual",
      sizeof (MDIColorVirtual),
      sizeof (MDIColorVirtualClass),
      (GtkClassInitFunc) mdi_color_virtual_class_init,
      (GtkObjectInitFunc) mdi_color_virtual_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    mdi_gen_child_type = gtk_type_unique (mdi_color_generic_get_type (),
					  &mdi_gen_child_info);
  }
  
  return mdi_gen_child_type;
}

static void 
mdi_color_virtual_class_init (MDIColorVirtualClass *class)
{
  MDIColorGenericClass *mcg_class;
  GnomeMDIChildClass   *mdi_child_class;
  GtkObjectClass       *object_class;
  
  object_class     = GTK_OBJECT_CLASS (class);
  mdi_child_class  = GNOME_MDI_CHILD_CLASS (class);
  mcg_class        = (MDIColorGenericClass *)class;
  parent_class     = gtk_type_class (mdi_color_generic_get_type());

  mcg_class->document_changed = mdi_color_virtual_changed;
  mcg_class->save             = mdi_color_virtual_save;
  mcg_class->load             = mdi_color_virtual_load;
}

static void
mdi_color_virtual_init (MDIColorVirtual *mcv)
{
  MDIColorGeneric *mcg = MDI_COLOR_GENERIC (mcv);
  
  mcv->r = 0;
  mcv->g = 0;
  mcv->b = 0;
  mcv->t = 100;

  mcg->get_owner        = mdi_color_virtual_get_owner;
  mcg->get_append_pos   = mdi_color_virtual_get_append_pos;
  mcg->get_control_type = mdi_color_virtual_get_control_type;

  mcg->flags = CHANGE_APPEND | CHANGE_REMOVE | CHANGE_NAME | CHANGE_RGB
    | CHANGE_CLEAR;
}

MDIColorVirtual *
mdi_color_virtual_new ()
{
  MDIColorVirtual *mcv; 

  mcv = gtk_type_new (mdi_color_virtual_get_type ()); 

  return mcv;
}

MDIColor *
mdi_color_virtual_get_owner (MDIColor *col)
{
  MDIColor *parent_col;

//  parent_col = col->data;
  parent_col = gtk_object_get_data (GTK_OBJECT (col), "parent");

  return mdi_color_generic_get_owner (parent_col);
}

GtkType
mdi_color_virtual_get_control_type (MDIColorGeneric *mcg)
{
  return control_virtual_get_type ();
}

static GList *
mdi_color_virtual_get_append_pos (MDIColorGeneric *mcg,
				  MDIColor *col)
{
  MDIColorVirtual *mcv = MDI_COLOR_VIRTUAL (mcg);
  GList *list;
  int diff, mid;
  
  diff = mdi_color_virtual_get_diff (mcv, col);
  
  mid = (mdi_color_virtual_get_diff (mcv, mcg->col->data) +
             mdi_color_virtual_get_diff (mcv, mcg->last_col->data)) / 2;  
  
  if (diff > mid) {
    list = mcg->last_col;
    while (list) {
      if (col != list->data)
        if (diff >= mdi_color_virtual_get_diff (mcv, list->data)) 
          return list->next;
        
      list = g_list_previous (list);
    }
  }
  
  else
  
  {                  
    list = mcg->col;
    while (list) {
      if (col != list->data) /* see CHANGE_RGB case */
        if (diff <= mdi_color_virtual_get_diff (mcv, list->data)) return list;
        
      list = g_list_next (list);
    }
  }

  return NULL;
}

static int 
color_diff (int r1, int g1, int b1, int r2, int g2, int b2)
{
  return abs (r1 - r2) + abs (g1 - g2) + abs (b1 - b2); 
}           

static int
mdi_color_virtual_get_diff (MDIColorVirtual *mcv, MDIColor *col)
{
  return color_diff (mcv->r, mcv->g, mcv->b, col->r, col->g, col->b);
}      

static MDIColor *
append_col (MDIColorVirtual *mcv, MDIColor *col, gboolean ref)
{
  if (ref) gtk_object_ref (GTK_OBJECT (col));

  return mdi_color_generic_append_new_set_data (MDI_COLOR_GENERIC (mcv), 
						col->r, col->g, col->b, 
						col->name, "parent", col);
}

static void 
remove_col (MDIColorVirtual *mcv, MDIColor *col, gboolean unref)
{  
  MDIColor *col_parent = gtk_object_get_data (GTK_OBJECT (col), "parent");

  mdi_color_generic_remove (MDI_COLOR_GENERIC (mcv), col);

  if (unref)
    gtk_object_unref (GTK_OBJECT (col_parent));
}

static void
clear_col (MDIColorVirtual *mcv, MDIColorGeneric *from, gboolean unref)
{
  GList *list = MDI_COLOR_GENERIC (mcv)->col;
  GtkObject *col;

  if (unref)
    while (list) {
      col = list->data;
      
      if (unref)
	gtk_object_unref (GTK_OBJECT (gtk_object_get_data (col, "parent")));
      
      list = g_list_next (list);
    }

  mdi_color_generic_clear (MDI_COLOR_GENERIC (mcv));
/* TODO : clear if only 1 parent
   if > 1 parent, remove some item */
}

static void       
mdi_color_virtual_changed (MDIColorGeneric *mcg, gpointer data)
{
  MDIColorVirtual *mcv;
  MDIColor *col_parent;
  MDIColor *col;
  GList *list = data;

  mcv = MDI_COLOR_VIRTUAL (mcg);

  mdi_color_generic_freeze (mcg);
  
  while (list) {
    col_parent = list->data;

  if (col_parent->change & CHANGE_CLEAR) {
  clear_col (mcv, col_parent->owner, TRUE); }
    else {
      
      if (mdi_color_virtual_get_diff (mcv, col_parent) < mcv->t) {   
    	
        if (col_parent->change & CHANGE_APPEND) 
	  append_col (mcv, col_parent, TRUE);
	
	else {
	  col = mdi_color_generic_search_by_data (mcg, "parent", col_parent);
	  
	  if (col_parent->change & CHANGE_REMOVE) 
	    remove_col (mcv, col, TRUE);
	  else {
	    if (!col)  /* Yes, it's possible */ 
	      append_col (mcv, col_parent, FALSE);
	    else {
	      if (col_parent->change & CHANGE_RGB) {
		GList *new_pos;
		
		mdi_color_generic_change_rgb (mcg, col, col_parent->r, 
					      col_parent->g, col_parent->b);
		
		new_pos = mdi_color_virtual_get_append_pos (mcg, col);
		
		mdi_color_generic_change_pos (mcg, col, 
			     ((MDIColor *)new_pos->data)->pos);
		
	      }
	      if (col_parent->change & CHANGE_NAME)
		mdi_color_generic_change_name (mcg, col, col_parent->name);
	      
	    }
	  }	
	}
      } else {
	/* Maybe we should remove the color */

	if (col_parent->change & CHANGE_APPEND)
	  gtk_object_ref (GTK_OBJECT (col_parent));
	else
	  if (col_parent->change & CHANGE_REMOVE)
	    gtk_object_unref (GTK_OBJECT (col_parent));
	  else
	    if (col_parent->change & CHANGE_RGB) {
	      col = mdi_color_generic_search_by_data (mcg, "parent", col_parent);
	  
	      if (col) remove_col (mcv, col, FALSE);
	    }
      }
    }

    list = g_list_next (list);
  }

  mdi_color_generic_thaw (mcg);
}

void 
mdi_color_virtual_set (MDIColorVirtual *mcv, 
		       float r, float g, float b, float t)
{
  GList *list, *list2;
  MDIColor *col;
  int action;
  
  /*
    Si R OU/ET G OU/ET B changent, alors on reconstruit entièrement la liste.
    Si t diminue, on supprime les éléments en fin de liste.
    Si t augemente, on garde les éléments et on ajoute les nouveaux éléments.
  */

  if ((mcv->r == r) && (mcv->g == g) && (mcv->b == b)) {
    if (mcv->t < t) 
      action = 1;
    else     
      action = 2;
    
  } else { 
    action = 0;
    
    mcv->r = r;
    mcv->g = g;
    mcv->b = b;
  }

  mdi_color_generic_freeze (MDI_COLOR_GENERIC (mcv));
    
  if (action == 2) {
    list = MDI_COLOR_GENERIC (mcv)->col;

    while (list) {
      col = list->data;
      list = g_list_next (list);
      if (mdi_color_virtual_get_diff (mcv, col) >= t) remove_col (mcv, col, FALSE);
    }
  } else {
    if (action == 0) clear_col (mcv, NULL, FALSE);
        
    list = MDI_COLOR_GENERIC (mcv)->parents;
    while (list) {
      list2 = MDI_COLOR_GENERIC (list->data)->col;
      while (list2) {
        col = list2->data;
       
        /* Si action = 0 -> on ajoute toujours
           Si action = 1 -> on ajoute EVENTUELLEMENT (si pas deja dedans) */
             
        if (action == 0) {
          if (mdi_color_virtual_get_diff (mcv, col) < t) 
	    append_col (mcv, col, FALSE);
        } 
        
        else
      
        if (action == 1) {            
          int diff = mdi_color_virtual_get_diff (mcv, col);
          if ((diff >= mcv->t) && (diff < t)) append_col (mcv, col, FALSE);  
        }
    
        list2 = g_list_next (list2);
      }
  
      list = g_list_next (list);
    }
  }

  mdi_color_generic_thaw (MDI_COLOR_GENERIC (mcv));
  
  mcv->t = t;

  mdi_color_generic_sync_control (MDI_COLOR_GENERIC (mcv)); 
}

void     
mdi_color_virtual_get (MDIColorVirtual *mcv, 
		       float *r, float *g, float *b, float *t)
{
  if (r) *r = mcv->r;
  if (g) *g = mcv->g;
  if (b) *b = mcv->b;
  if (t) *t = mcv->t;
}

/******************************** Config *********************************/

static void         
mdi_color_virtual_save (MDIColorGeneric *mcg)
{
  MDIColorVirtual *mcv = MDI_COLOR_VIRTUAL (mcg);

  gnome_config_set_int ("Red", mcv->r);
  gnome_config_set_int ("Green", mcv->g);
  gnome_config_set_int ("Blue", mcv->b);
  gnome_config_set_int ("Tolerance", mcv->t);
    
  parent_class->save (mcg);
}

static void         
mdi_color_virtual_load (MDIColorGeneric *mcg)
{
  MDIColorVirtual *mcv = MDI_COLOR_VIRTUAL (mcg);
  
  mcv->r = gnome_config_get_int ("Red");
  mcv->g = gnome_config_get_int ("Green");
  mcv->b = gnome_config_get_int ("Blue");
  mcv->t = gnome_config_get_int ("Tolerance");

  mdi_color_generic_sync_control (mcg); 

  parent_class->load (mcg);
}
