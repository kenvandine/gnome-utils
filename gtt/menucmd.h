#ifndef __MENUCMD_H__
#define __MENUCMD_H__

void not_implemented(GtkWidget *w, gpointer data);

void quit_app(GtkWidget *, gpointer);
void about_box(GtkWidget *, gpointer);

void new_project(GtkWidget *, gpointer);
void init_project_list(GtkWidget *, gpointer);
void save_project_list(GtkWidget *, gpointer);
void cut_project(GtkWidget *w, gpointer data);
void paste_project(GtkWidget *w, gpointer data);
void copy_project(GtkWidget *w, gpointer data);

void menu_start_timer(GtkWidget *w, gpointer data);
void menu_stop_timer(GtkWidget *w, gpointer data);
void menu_toggle_timer(GtkWidget *w, gpointer data);

void menu_set_states(void);

void menu_options(GtkWidget *w, gpointer data);

void menu_properties(GtkWidget *w, gpointer data);


#endif
