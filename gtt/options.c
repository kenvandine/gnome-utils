#include <config.h>
#include <gtk/gtk.h>

#include "gtt.h"


typedef struct _OptionsDlg {
	GtkDialog *dlg;
	GtkCheckButton *show_secs;
	GtkWidget *ok;
} OptionsDlg;



static void options_ok(GtkWidget *w, OptionsDlg *odlg)
{
	int state;

	state = GTK_TOGGLE_BUTTON(odlg->show_secs)->active;
	if (state != config_show_secs) {
		config_show_secs = state;
		setup_list();
	}
	if (w == odlg->ok) {
		gtk_widget_hide(GTK_WIDGET(odlg->dlg));
	}
}



void options_dialog(void)
{
	GtkWidget *w;
	GtkBox *vbox, *aa;
	static OptionsDlg *odlg = NULL;
	
	if (!odlg) {
		odlg = g_malloc(sizeof(OptionsDlg));

		odlg->dlg = GTK_DIALOG(gtk_dialog_new());
		gtk_window_set_title(GTK_WINDOW(odlg->dlg), APP_NAME " - Preferences");

		vbox = (GtkBox *)gtk_vbox_new(FALSE, 0);
		gtk_widget_show(GTK_WIDGET(vbox));
		gtk_container_border_width(GTK_CONTAINER(vbox), 10);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(odlg->dlg)->vbox), GTK_WIDGET(vbox),
				   FALSE, FALSE, 0);
		aa = GTK_BOX(GTK_DIALOG(odlg->dlg)->action_area);

		w = gtk_button_new_with_label("OK");
		gtk_widget_show(w);
		gtk_signal_connect(GTK_OBJECT(w), "clicked",
				   GTK_SIGNAL_FUNC(options_ok),
				   (gpointer *)odlg);
		gtk_box_pack_start(aa, w, FALSE, FALSE, 2);
		odlg->ok = w;

		w = gtk_button_new_with_label("Apply");
		gtk_widget_show(w);
		gtk_signal_connect(GTK_OBJECT(w), "clicked",
				   GTK_SIGNAL_FUNC(options_ok),
				   (gpointer *)odlg);
		gtk_box_pack_start(aa, w, FALSE, FALSE, 2);

		w = gtk_button_new_with_label("Cancel");
		gtk_widget_show(w);
		gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
					  GTK_SIGNAL_FUNC(gtk_widget_hide),
					  GTK_OBJECT(odlg->dlg));
		gtk_box_pack_start(aa, w, FALSE, FALSE, 2);

		w = gtk_check_button_new_with_label("Show Seconds");
		gtk_widget_show(w);
		gtk_box_pack_start(vbox, w, FALSE, FALSE, 2);
		odlg->show_secs = GTK_CHECK_BUTTON(w);
	}
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(odlg->show_secs), config_show_secs);

	gtk_widget_show(GTK_WIDGET(odlg->dlg));
}
