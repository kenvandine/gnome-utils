#include "gcolorsel.h"
#include "menus.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "mdi-color-virtual.h"
#include "view-color-grid.h"
#include "widget-control-virtual.h"

#include "config.h"
#include <gnome.h>
#include <glade/glade.h>

GnomeMDI *mdi;

gint mdi_remove_child (GnomeMDI *mdi, MDIColorGeneric *mcg);

gint mdi_remove_child (GnomeMDI *mdi, MDIColorGeneric *mcg)
{
  mdi_color_generic_clear (mcg);

  while (mcg->parents) 
    mdi_color_generic_disconnect (mcg->parents->data, mcg);

  while (mcg->other_views) 
    mdi_color_generic_disconnect (mcg, mcg->other_views->data);

  return TRUE;
}

int main (int argc, char *argv[])
{
  MDIColorFile *file;
  MDIColorVirtual *virtual;
  
  gnome_init ("gcolorsel", VERSION, argc, argv);
  glade_gnome_init ();
	
  mdi = GNOME_MDI (gnome_mdi_new ("gcolorsel", "GColorsel"));
  mdi->tab_pos = GTK_POS_BOTTOM;
  gnome_mdi_set_mode (mdi, GNOME_MDI_NOTEBOOK);
  gnome_mdi_set_menubar_template (mdi, main_menu);
  gnome_mdi_set_child_menu_path (mdi, GNOME_MENU_FILE_STRING);
  gtk_signal_connect (GTK_OBJECT (mdi), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_signal_connect (GTK_OBJECT (mdi), "remove_child", 
		      GTK_SIGNAL_FUNC (mdi_remove_child), NULL);

  file = mdi_color_file_new ("/usr/X11R6/lib/X11/rgb.txt");
  gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (file));
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (file));

  mdi_color_generic_next_view_type (MDI_COLOR_GENERIC (file), 
				    TYPE_VIEW_COLOR_GRID);
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (file));
  
  virtual = mdi_color_virtual_new ();
  gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (virtual));
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (virtual)); 
  mdi_color_generic_next_view_type (MDI_COLOR_GENERIC (virtual), 
				    TYPE_VIEW_COLOR_GRID);
  gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (virtual)); 
  mdi_color_generic_connect (MDI_COLOR_GENERIC (file),
			     MDI_COLOR_GENERIC (virtual));
  

  gtk_widget_set_usize (GTK_WIDGET (gnome_mdi_get_active_window (mdi)),
			320, 380);  
  mdi_color_file_load (file);

  gtk_main ();

  return 0;
}
