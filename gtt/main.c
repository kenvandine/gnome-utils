#include <config.h>
#if HAS_GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include "gtt.h"



project *cur_proj = NULL;
GtkWidget *glist, *window;
#ifdef DEBUG
int config_show_secs = 1;
#else
int config_show_secs = 0;
#endif


static void select_item(GtkList *glist, GtkWidget *w)
{
	GtkWidget *li;
	
	if (!GTK_LIST(glist)->selection) {
		cur_proj = NULL;
		prop_dialog_set_project(NULL);
		return;
	}
	li = GTK_LIST(glist)->selection->data;
	if (!li) {
		cur_proj = NULL;
		prop_dialog_set_project(NULL);
		return;
	}
	cur_proj = (project *)gtk_object_get_data(GTK_OBJECT(li), "list_item_data");
	prop_dialog_set_project(cur_proj);
}



void update_title_label(project *p)
{
	if (!p) return;
	if (!p->title_label) return;
	gtk_label_set(p->title_label, p->title);
}



void update_label(project *p)
{
	char buf[20];

	if (!p) return;
	if (!p->label) return;
	if (config_show_secs) {
		sprintf(buf, "%02d:%02d:%02d",
			p->day_secs / 3600,
			(p->day_secs / 60) % 60,
			p->day_secs % 60);
	} else {
		sprintf(buf, "%02d:%02d", p->day_secs / 3600, (p->day_secs / 60) % 60);
	}
	gtk_label_set(p->label, buf);
}



static GtkWidget *build_item(project *p)
{
	GtkWidget *item;
	GtkWidget *label;
	GtkWidget *hbox;
	char buf[20];
	
	item = gtk_list_item_new();
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(item), hbox);
	if (config_show_secs) {
		sprintf(buf, "%02d:%02d:%02d",
			p->day_secs / 3600,
			(p->day_secs / 60) % 60,
			p->day_secs % 60);
	} else {
		sprintf(buf, "%02d:%02d", p->day_secs / 3600, (p->day_secs / 60) % 60);
	}
	label = gtk_label_new(buf);
	p->label = GTK_LABEL(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	label = gtk_label_new(p->title);
	p->title_label = GTK_LABEL(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(hbox);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "list_item_data", p);
	return item;
}

void add_item(GtkWidget *glist, project *p)
{
	GtkWidget *item = build_item(p);

	gtk_container_add(GTK_CONTAINER(glist), item);
	if (cur_proj == p) {
		gtk_list_item_select(GTK_LIST_ITEM(item));
	}
}



void add_item_at(GtkWidget *glist, project *p, int pos)
{
	GList *gl;
	GtkWidget *item = build_item(p);

	gl = g_malloc(sizeof(GList));
	gl->data = item;
	gl->next = gl->prev = NULL;
	gtk_list_insert_items(GTK_LIST(glist), gl, pos);
}



void setup_list(void)
{
	project_list *pl;
	GList *gl;
	int timer_running, cp_found = 0;

	/* cur_proj = NULL; */
	timer_running = (main_timer != 0);
	stop_timer();
	for (;;) {
		gl = GTK_LIST(glist)->children;
		if (!gl) break;
		gtk_container_remove(GTK_CONTAINER(glist), gl->data);
	}
	if (!plist) {
		project_list_add(project_new_title("empty"));
	}
	
	for (pl = plist; pl; pl = pl->next) {
		add_item(glist, pl->proj);
		if (pl->proj == cur_proj) cp_found = 1;
	}
	if (!cp_found) cur_proj = NULL;
	prop_dialog_set_project(cur_proj);
	err_init();
	gtk_widget_show(window);
	if (timer_running) start_timer();
	menu_set_states();
}



static void unlock_quit(void)
{
	unlock_gtt();
	gtk_main_quit();
}

static void init_list_2(GtkWidget *w, gint butnum)
{
	if (butnum == 2) unlock_quit();
	else setup_list();
}

static void init_list(void)
{
	if (!project_list_load(NULL)) {
		msgbox_ok_cancel("Confirmation request", "I could not read the init file.\n"
				 "Shall I install a new init file?", "OK", "No, just quit!",
				 GTK_SIGNAL_FUNC(init_list_2));
	} else {
		setup_list();
	}
}



static char *build_lock_fname()
{
	static char fname[1024] = "";
	void strcat(char *, const char *);
	
	if (fname[0]) return fname;
	if (getenv("HOME")) {
		strcpy(fname, getenv("HOME"));
	} else {
		fname[0] = 0;
	}
	strcat(fname, "/.gtimetracker");
#ifdef DEBUG
	strcat(fname, "-" VERSION);
#endif
	strcat(fname, ".pid");
	return fname;
}


static void lock_gtt()
{
	FILE *f;
	char *fname;
	int getpid(void);
	
	fname = build_lock_fname();
	if (NULL != (f = fopen(fname, "rt"))) {
		fclose(f);
		msgbox_ok("Error", "There seems to be another " APP_NAME " running.\n"
			  "Please remove the pid file, if that is not correct.",
			  "Oops",
			  GTK_SIGNAL_FUNC(gtk_main_quit));
		gtk_main();
		exit(0);
	}
	if (NULL == (f = fopen(fname, "wt"))) {
		g_error("Cannot create pid-file!");
	}
	fprintf(f, "%d\n", getpid());
	fclose(f);
}



void unlock_gtt(void)
{
	int unlink(const char *);

	unlink(build_lock_fname());
}



int main(int argc, char *argv[])
{
	GtkWidget *vbox;
	GtkWidget *swin;
	GtkWidget *w;
	GtkAcceleratorTable *accel;
	int i;

#if HAS_GNOME
	gnome_init(&argc, &argv);
#else 
	gtk_init(&argc, &argv);
#endif 

	for (i = 1; i < argc; i++) {
		/* TODO: parsing arguments */
		g_print("unknown arg: %s\n", argv[i]);
	}
	
	lock_gtt();

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), APP_NAME " " VERSION);
	/* TODO: get this from "-geometry" arg */
#ifdef EXTENDED_TOOLBAR
	/*
	 * TODO:
	 * because I still use hard coded pixel sized here,
	 * I have to make the window bigger, if more toolbar
	 * buttons are shown.
	 */
	gtk_widget_set_usize(window, 380, 240);
#else /* EXTENDED_TOOLBAR */ 
	gtk_widget_set_usize(window, 320, 240);
#endif /* EXTENDED_TOOLBAR */ 
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(quit_app), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
			   GTK_SIGNAL_FUNC(quit_app), NULL);
	gtk_container_border_width(GTK_CONTAINER(window), 1);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	get_menubar(&w, &accel, MENU_MAIN);
	gtk_widget_show(w);
	gtk_window_add_accelerator_table(GTK_WINDOW(window), accel);
	gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
	
	w = build_toolbar(window, NULL);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), swin, TRUE, TRUE, 2);
	gtk_widget_show(swin);
	glist = gtk_list_new();
	gtk_list_set_selection_mode(GTK_LIST(glist), GTK_SELECTION_SINGLE);
	gtk_container_add(GTK_CONTAINER(swin), glist);
	gtk_signal_connect(GTK_OBJECT(glist), "select_child",
			   GTK_SIGNAL_FUNC(select_item), NULL);
	/* start timer before the state of the menu items is set */
	start_timer();
	init_list();
	gtk_widget_show(glist);
	
	gtk_widget_show(vbox);
	
	gtk_main();

	unlock_gtt();
	return 0;
}

