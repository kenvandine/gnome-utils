#include "config.h"

#include "gcolorsel.h"
#include "menus.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "mdi-color-virtual.h"
#include "view-color-grid.h"
#include "view-color-list.h"
#include "view-color-edit.h"
#include "widget-control-virtual.h"
#include "session.h"

#include <gnome.h>
#include <glade/glade.h>

GnomeMDI *mdi;

gint mdi_remove_child (GnomeMDI *mdi, MDIColorGeneric *mcg);

gint mdi_remove_child (GnomeMDI *mdi, MDIColorGeneric *mcg)
{
//  mdi_color_generic_clear (mcg);

  while (mcg->parents) 
    mdi_color_generic_disconnect (mcg->parents->data, mcg);

  while (mcg->other_views) 
    mdi_color_generic_disconnect (mcg, mcg->other_views->data);

  return TRUE;
}

int main (int argc, char *argv[])
{
  /* Initialize i18n */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init ("gcolorsel", VERSION, argc, argv);
  glade_gnome_init ();

  /* For gtk_type_from_name in session.c */
  mdi_color_virtual_get_type ();
  mdi_color_file_get_type    ();
  view_color_list_get_type   ();
  view_color_grid_get_type   ();
  view_color_edit_get_type   ();
	
  /* Init GnomeMDI */
  mdi = GNOME_MDI (gnome_mdi_new ("gcolorsel", _("GColorsel")));

  gtk_signal_connect (GTK_OBJECT (mdi), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_signal_connect (GTK_OBJECT (mdi), "remove_child", 
		      GTK_SIGNAL_FUNC (mdi_remove_child), NULL);

  /* Init menu/toolbar */
  gnome_mdi_set_menubar_template (mdi, main_menu);
  gnome_mdi_set_toolbar_template (mdi, toolbar);

  /* Try loading old session ; if fail, construct a new session */
  if (! session_load (mdi)) 
    session_create (mdi);

  gtk_widget_set_usize (GTK_WIDGET (gnome_mdi_get_active_window (mdi)),
			320, 400);

  /* Load all file */
  session_load_data (mdi);
    
  gtk_main ();

  return 0;
}

