#include <config.h>
#include <gtk/gtk.h>

#include "gtt.h"



#if 0 /* not needed */
void not_implemented(GtkWidget *w, gpointer data)
{
	msgbox_ok("Notice", "Not implemented yet", "Close", NULL);
}
#endif


void quit_app(GtkWidget *w, gpointer data)
{
	unlock_gtt();
	if (!project_list_save(NULL)) {
#if 1
		msgbox_ok("Warning", "Could not write init file!", "Oops",
			  GTK_SIGNAL_FUNC(gtk_main_quit));
#else
		GtkWidget *dlg, *t;
		GtkBox *vbox;
		
		new_dialog_ok("Warning", &dlg, &vbox,
			      "Oops", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
		t = gtk_label_new("Could not write init file!");
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);
		gtk_widget_show(dlg);
#endif 
		return;
	}
	gtk_main_quit();
}



void about_box(GtkWidget *w, gpointer data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;

	new_dialog_ok(APP_NAME " - About", &dlg, &vbox,
		      "Close", NULL, NULL);

	t = gtk_label_new(APP_NAME);
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 8);

#ifdef DEBUG
	t = gtk_label_new("Version " VERSION "\n" __DATE__ " " __TIME__);
#else
	t = gtk_label_new("Version " VERSION);
#endif
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

	t = gtk_label_new("Eckehard Berns <eb@berns.prima.de>");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

	gtk_widget_show(dlg);
}




static void project_name(GtkWidget *w, GtkEntry *gentry)
{
	char *s;
	project *proj;

	if (!(s = gtk_entry_get_text(gentry))) return;
	if (!s[0]) return;
	project_list_add(proj = project_new_title(s));
#if 1
	add_item(glist, proj);
#else 
	/* TODO: optimize */
	setup_list();
#endif 
}

void new_project(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg, *t, *text;
	GtkBox *vbox;

	text = gtk_entry_new();
	new_dialog_ok_cancel(APP_NAME " - MessageBox", &dlg, &vbox,
			     "OK", GTK_SIGNAL_FUNC(project_name), (gpointer *)text,
			     "Cancel", NULL, NULL);
	t = gtk_label_new("Please enter the name of the new Project:");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);

	gtk_widget_show(text);
	gtk_box_pack_end(vbox, text, TRUE, TRUE, 2);
	gtk_widget_grab_focus(text);
	/*
	gtk_signal_connect(GTK_OBJECT(text), "activate",
			   GTK_SIGNAL_FUNC(project_name), (gpointer *)text);
	gtk_signal_connect_object(GTK_OBJECT(text), "activate",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(dlg));
	 */
	
	gtk_widget_show(dlg);
}



static void init_project_list_2(GtkWidget *widget, gpointer *data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	if (!project_list_load(NULL)) {
		new_dialog_ok("Warning", &dlg, &vbox,
			      "Oops", NULL, NULL);
		t = gtk_label_new("I could not read the init file");
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
		gtk_widget_show(dlg);
	} else {
		setup_list();
	}
}



void init_project_list(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg, *t;
	GtkBox *vbox;
	
	new_dialog_ok_cancel("Configmation Request", &dlg, &vbox,
			     "OK", GTK_SIGNAL_FUNC(init_project_list_2), NULL,
			     "Cancel", NULL, NULL);
	t = gtk_label_new("This will overwrite your current set of projects.");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
	t = gtk_label_new("Do you really want to reload the init file?");
	gtk_widget_show(t);
	gtk_box_pack_start(vbox, t, TRUE, FALSE, 2);
	gtk_widget_show(dlg);
}



void save_project_list(GtkWidget *widget, gpointer data)
{
	if (!project_list_save(NULL)) {
		GtkWidget *dlg, *t;
		GtkBox *vbox;
		
		new_dialog_ok("Warning", &dlg, &vbox,
			      "Oops", NULL, NULL);
		t = gtk_label_new("Could not write init file!");
		gtk_widget_show(t);
		gtk_box_pack_start(vbox, t, FALSE, FALSE, 2);
		gtk_widget_show(dlg);
		return;
	}
}



static project *cutted_project = NULL;

void cut_project(GtkWidget *w, gpointer data)
{
	if (!cur_proj) return;
	if (cutted_project)
		project_destroy(cutted_project);
	cutted_project = cur_proj;
	prop_dialog_set_project(NULL);
	project_list_remove(cur_proj);
	cur_proj = NULL;
	gtk_container_remove(GTK_CONTAINER(glist), GTK_LIST(glist)->selection->data);
}



void paste_project(GtkWidget *w, gpointer data)
{
	int pos;
	project *p;
	
	if (!cutted_project) return;
	p = project_dup(cutted_project);
	if (!cur_proj) {
		add_item(glist, p);
		project_list_add(p);
		return;
	}
	pos = gtk_list_child_position(GTK_LIST(glist), GTK_LIST(glist)->selection->data);
	project_list_insert(p, pos);
	add_item_at(glist, p, pos);
}



void copy_project(GtkWidget *w, gpointer data)
{
	if (!cur_proj) return;
	if (cutted_project)
		project_destroy(cutted_project);
	cutted_project = project_dup(cur_proj);
}




/*
 * timer related menu functions
 */

void menu_start_timer(GtkWidget *w, gpointer data)
{
	start_timer();
	menu_set_states();
}



void menu_stop_timer(GtkWidget *w, gpointer data)
{
	stop_timer();
	menu_set_states();
}


void menu_toggle_timer(GtkWidget *w, gpointer data)
{
	if (GTK_IS_CHECK_MENU_ITEM(w)) {
		if (GTK_CHECK_MENU_ITEM(w)->active) {
			start_timer();
		} else {
			stop_timer();
		}
	}
	menu_set_states();
}


void menu_set_states(void)
{
	menus_set_state("<Main>/Timer/Timer running", (main_timer == 0) ? 0 : 1);
	menus_set_sensitive("<Main>/Timer/Start", (main_timer == 0) ? 1 : 0);
	menus_set_sensitive("<Main>/Timer/Stop", (main_timer == 0) ? 0 : 1);
	toolbar_set_states();
}



void menu_options(GtkWidget *w, gpointer data)
{
	options_dialog();
}



void menu_properties(GtkWidget *w, gpointer data)
{
	if (cur_proj) {
		prop_dialog(cur_proj);
	} else {
		msgbox_ok("Info", "I cannot edit properties without\n"
			  "a selected project.", "Oops", NULL);
	}
}

