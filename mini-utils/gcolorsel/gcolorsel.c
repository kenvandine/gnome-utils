#include "config.h"

#include "gcolorsel.h"
#include "menus.h"
#include "mdi-color-generic.h"
#include "mdi-color-file.h"
#include "mdi-color-virtual-rgb.h"
#include "mdi-color-virtual-monitor.h"
#include "view-color-grid.h"
#include "view-color-list.h"
#include "view-color-edit.h"
#include "session.h"
#include "utils.h"

#include <gnome.h>
#include <glade/glade.h>

GnomeMDI *mdi;

/* First view is considered as the default view ... */
views_t views_tab[] = { {N_("List"), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS, 
			 (views_new)view_color_list_new, 
			 view_color_list_get_type       },
			{N_("Grid"), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,
			 (views_new)view_color_grid_new, 
			 view_color_grid_get_type       },
			{N_("Edit"), GTK_POLICY_NEVER, GTK_POLICY_NEVER,
			 (views_new)view_color_edit_new, 
			 view_color_edit_get_type       },
			
			{ NULL, 0, 0, NULL, 0 } };

docs_t docs_tab[] = { 
  { N_("File"),       mdi_color_file_get_type,            TRUE,  FALSE },
  { N_("Search RGB"), mdi_color_virtual_rgb_get_type,     TRUE,  TRUE  },
  { N_("Monitor"),    mdi_color_virtual_monitor_get_type, FALSE, TRUE  },
  { N_("Concact"),    mdi_color_virtual_get_type,         TRUE,  TRUE  },
  { NULL, NULL, FALSE },
};

views_t *
get_views_from_type (GtkType type)
{
  int i = 0;

  while (views_tab[i].name) {
    if (views_tab[i].type () == type) return &views_tab[i];
    i++;
  }

  return NULL;
}

static void
call_get_type (void)
{
  int i = 0;

  while (views_tab[i].name) {
    views_tab[i].type ();
    i++;
  }

  i = 0;
  while (docs_tab[i].name) {
    docs_tab[i].type ();
    i++;
  }
}

static gint 
mdi_remove_child (GnomeMDI *mdi, MDIColorGeneric *mcg)
{
  GtkWidget *dia;
  int ret;
  char *str;

  if ((IS_MDI_COLOR_FILE (mcg)) && (mcg->modified)) {

    str = g_strdup_printf (_("'%s' document has been modified; do you wish to save it ?"), mcg->name);
    
    dia = gnome_message_box_new (str, GNOME_MESSAGE_BOX_QUESTION, 
				 GNOME_STOCK_BUTTON_YES, 
				 GNOME_STOCK_BUTTON_NO,
				 GNOME_STOCK_BUTTON_CANCEL, NULL);    
    g_free (str);
      
    ret = gnome_dialog_run_and_close (GNOME_DIALOG (dia));
    if (ret == 2) return FALSE;

    if (!ret)     
      while (1) {
	ret = save_file (MDI_COLOR_FILE (mcg));
	if (ret == -1) return FALSE;
	if (ret != 1) break;
      }
  }

  return TRUE;
}

static void
app_created (GnomeMDI *mdi, GnomeApp *app)
{
  GtkWidget *statusbar;

  gtk_widget_set_usize (GTK_WIDGET (app), 320, 400);

  statusbar = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_USER);
  gnome_app_set_statusbar (GNOME_APP (app), statusbar);
  gnome_app_install_menu_hints (app, gnome_mdi_get_menubar_info (app));

  gtk_widget_set_usize (GTK_WIDGET (gnome_appbar_get_progress (GNOME_APPBAR (statusbar))), 60, -1);
}

int main (int argc, char *argv[])
{
  /* Initialize i18n */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init ("gcolorsel", VERSION, argc, argv);
  glade_gnome_init ();

  /* For gtk_type_from_name in session.c */
  call_get_type ();
	
  /* Init GnomeMDI */
  mdi = GNOME_MDI (gnome_mdi_new ("gcolorsel", _("GColorsel")));

  gtk_signal_connect (GTK_OBJECT (mdi), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_signal_connect (GTK_OBJECT (mdi), "remove_child", 
		      GTK_SIGNAL_FUNC (mdi_remove_child), NULL);
  gtk_signal_connect (GTK_OBJECT (mdi), "app_created",
		      GTK_SIGNAL_FUNC (app_created), NULL);

  /* Init menu/toolbar */
  gnome_mdi_set_menubar_template (mdi, main_menu);
  gnome_mdi_set_toolbar_template (mdi, toolbar);

  /* Try loading old session ; if fail, construct a new session */
  if (! session_load (mdi)) 
    session_create (mdi);

  /* Load all file */
  session_load_data (mdi);

  gtk_main ();

  return 0;
}

