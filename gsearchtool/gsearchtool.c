/* Gnome Search Tool 
 * (C) 1998,2000 the Free Software Foundation
 *
 * Author:   George Lebl
 */

#define GTK_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
#define GNOME_DISABLE_DEPRECATED
#define G_DISABLE_DEPRECATED

#include <config.h>
#include <gnome.h>

#include <bonobo-activation/bonobo-activation.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>

#include "gsearchtool.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

typedef enum {
	NOT_RUNNING,
	RUNNING,
	MAKE_IT_STOP
} RunLevel;

const FindOptionTemplate templates[] = {
#define OPTION_FILENAME 0
	{ FIND_OPTION_TEXT, "-iname '%s'", N_("File name is") },
	{ FIND_OPTION_TEXT, "'!' -iname '%s'", N_("File name is not") }, 
#define OPTION_NOMOUNTED 5
	{ FIND_OPTION_BOOL, "-mount", N_("Don't search mounted filesystems") },
#define OPTION_NOSUBDIRS 1
	{ FIND_OPTION_BOOL, "-follow", N_("Follow symbolic links") },	
	{ FIND_OPTION_BOOL, "-depth", N_("Process directory contents depth first") },
	{ FIND_OPTION_BOOL, "-maxdepth 1", N_("Don't search subdirectories") },
	{ FIND_OPTION_TEXT, "-user '%s'", N_("Owned by user") },
	{ FIND_OPTION_TEXT, "-group '%s'", N_("Owned by group") },
	{ FIND_OPTION_BOOL, "-nouser -o -nogroup", N_("Invalid user or group") },
	{ FIND_OPTION_TIME, "-mtime '%s'", N_("Last time modified") },
	{ FIND_OPTION_BOOL, "-empty", N_("File is empty") },
	{ FIND_OPTION_TEXT, "-regex '%s'", N_("Matches regular expression") },
	{ FIND_OPTION_END, NULL,NULL}
};

/*this will not include the directories in search*/
const static char defoptions[] = "'!' -type d";
/*this should be done if the above is made optional*/
/*char defoptions[] = "-mindepth 0";*/

GList *criteria_find = NULL;

static GtkWidget *find_box = NULL;

static GtkWidget *start_dir_e = NULL;

static GtkWidget *locate_entry = NULL;

static GtkWidget *nbook = NULL;

static GtkWidget *app = NULL; 

static GtkWidget *file_selector = NULL;

static GtkListStore *locate_store = NULL;

static GtkListStore *find_store = NULL;

static GtkTreeSelection *locate_selection = NULL;

static GtkTreeSelection *find_selection = NULL;

static GtkTreeIter locate_iter;

static GtkTreeIter find_iter;

static RunLevel find_running = NOT_RUNNING;
static RunLevel locate_running = NOT_RUNNING;

static int current_template = 0;

static char *filename = NULL;


static int
count_char (const char *s, char p)
{
	int cnt = 0;
	for(;*s;s++) {
		if(*s == p)
			cnt++;
	}
	return cnt;
}

static char *
quote_quote_string (const char *s)
{
	GString *gs;

	if(count_char(s, '\'') == 0)
		return g_strdup(s);
	gs = g_string_new("");
	for(;*s;s++) {
		if(*s == '\'')
			g_string_append(gs,"'\\''");
		else
			g_string_append_c(gs,*s);
	}

	return g_string_free (gs, FALSE);
}

static char *
make_find_cmd (const char *start_dir)
{
	GString *cmdbuf;
	GList *list;
	
	if (criteria_find==NULL) return NULL;

	cmdbuf = g_string_new ("");

	if(start_dir)
		g_string_append_printf(cmdbuf, "find %s %s ", start_dir, defoptions);
	else
		g_string_append_printf(cmdbuf, "find . %s ", defoptions);

	for(list=criteria_find;list!=NULL;list=g_list_next(list)) {
		FindOption *opt = list->data;
		if(opt->enabled) {
			char *s;
			switch(templates[opt->templ].type) {
			case FIND_OPTION_BOOL:
				g_string_append_printf(cmdbuf,"%s ",
						  templates[opt->templ].option);
				break;
			case FIND_OPTION_TEXT:
				s = quote_quote_string(opt->data.text);
				g_string_append_printf(cmdbuf,
						  templates[opt->templ].option,
						  s);
				g_free(s);
				g_string_append_c(cmdbuf, ' ');
				break;
			case FIND_OPTION_NUMBER:
				g_string_append_printf(cmdbuf,
						  templates[opt->templ].option,
						  opt->data.number);
				g_string_append_c(cmdbuf, ' ');
				break;
			case FIND_OPTION_TIME:
				s = quote_quote_string(opt->data.time);
				g_string_append_printf(cmdbuf,
						  templates[opt->templ].option,
						  s);
				g_free(s);
				g_string_append_c(cmdbuf, ' ');
				break;
			default:
			        break;
			}
		}
	}
	g_string_append (cmdbuf, "-print0 ");

	return g_string_free(cmdbuf, FALSE);
}

static char *
make_locate_cmd(void)
{
	GString *cmdbuf;
	GList *list;
	gchar *locate_string;
	
	locate_string = 
		(gchar *)gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(locate_entry))));
	locate_string = quote_quote_string(locate_string);

	if(!locate_string || !*locate_string) {
		return NULL;
	}

	cmdbuf = g_string_new ("");
	g_string_append_printf(cmdbuf, "locate '%s'", locate_string);
	
	return g_string_free (cmdbuf, FALSE);
}

static gboolean
kill_after_nth_nl (GString *str, int n)
{
	int i;
	int count = 0;
	for (i = 0; str->str[i] != '\0'; i++) {
		if (str->str[i] == '\n') {
			count++;
			if (count == n) {
				g_string_truncate (str, i);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void
really_run_command(char *cmd, char sepchar, RunLevel *running, GtkListStore *store, GtkTreeIter *iter)
{
	static gboolean lock = FALSE;
	int idle;

	GString *string;
	char ret[PIPE_READ_BUFFER];
	int fd[2], fderr[2];
	int i,n;
	int pid;
	GString *errors = NULL;
	gboolean add_to_errors = TRUE;
		
	if( ! lock) {
		lock = TRUE;
	} else {
		gnome_app_error(GNOME_APP(app),
				_("A search is already running.  Please wait for that search "
				  "to complete or cancel it."));
		return;
	}

	gtk_list_store_clear(GTK_LIST_STORE(store));

	/* running = NOT_RUNNING, RUNNING, MAKE_IT_STOP */
	*running = RUNNING;
	
	pipe(fd);
	pipe(fderr);
	
	if ( (pid = fork()) == 0) {
		close(fd[0]);
		close(fderr[0]);
		
		close(STDOUT); 
		close(STDIN);
		close(STDERR);

		dup2(fd[1],STDOUT);
		dup2(fderr[1],STDERR);

		close(fd[1]);
		close(fderr[1]);

		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		_exit(0); /* in case it fails */
	}
	close(fd[1]);
	close(fderr[1]);

	idle = gtk_idle_add((GtkFunction)gtk_true,NULL);

	fcntl(fd[0], F_SETFL, O_NONBLOCK);
	fcntl(fderr[0], F_SETFL, O_NONBLOCK);

	string = g_string_new (NULL);

	while (*running == RUNNING) {
		n = read (fd[0], ret, PIPE_READ_BUFFER);
		for (i = 0; i < n; i++) {
			if(ret[i] == sepchar) {
				gchar *utf8 = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
				gtk_list_store_append (GTK_LIST_STORE(store), iter); 
				gtk_list_store_set (GTK_LIST_STORE(store), iter, 0, utf8, -1); 
				g_string_assign (string, "");
				g_free (utf8);
			} else {
				g_string_append_c (string, ret[i]);
			}
		}

		n = read (fderr[0], ret, PIPE_READ_BUFFER-1);
		if (n > 0) {
			ret[n] = '\0';
			if (add_to_errors) {
				if (errors == NULL)
					errors = g_string_new (ret);
				else
					errors = g_string_append (errors, ret);
				add_to_errors =
					! kill_after_nth_nl (errors, 20);
			}
			fprintf (stderr, "%s", ret);
		}
		
		if (waitpid (-1, NULL, WNOHANG) != 0)
			break;
		/*this for some reason doesn't work, I need to add an
		  idle handler and do iteration with TRUE*/
		/*if(gtk_events_pending())
			gtk_main_iteration_do(FALSE);*/
		gtk_main_iteration_do (TRUE);
		if (*running == MAKE_IT_STOP) {
			kill(pid, SIGKILL);
			wait(NULL);
		}
	}
	/* now we got it all ... so finish reading from the pipe */
	while ((n = read (fd[0], ret, PIPE_READ_BUFFER)) > 0) {
		for (i = 0; i < n; i++) {
			if (ret[i] == sepchar) {
				gchar *utf8 = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
				gtk_list_store_append (GTK_LIST_STORE(store), iter); 
				gtk_list_store_set (GTK_LIST_STORE(store), iter, 0, utf8, -1); 
				g_string_assign (string, "");
				g_free (utf8);
			} else {
				g_string_append_c (string, ret[i]);
			}
		}
	}
	while((n = read(fderr[0], ret, PIPE_READ_BUFFER-1)) > 0) {
		ret[n]='\0';
		if (add_to_errors) {
			if (errors == NULL)
				errors = g_string_new (ret);
			else
				errors = g_string_append (errors, ret);
			add_to_errors =
				! kill_after_nth_nl (errors, 20);
		}
		fprintf (stderr, "%s", ret);
	}

	close(fd[0]);
	close(fderr[0]);

	gtk_idle_remove(idle);
	
	/* if any errors occured */
	if(errors) {
		if ( ! add_to_errors) {
			errors = g_string_append
				(errors,
				 _("\n... Too many errors to display ..."));
		}
		/* make an error message */
		gnome_app_error (GNOME_APP (app), errors->str);
		/* freeing allocated memory */
		g_string_free (errors, TRUE);
	}

	g_string_free (string, TRUE);

	*running = NOT_RUNNING;

	lock = FALSE;
}

static void
run_command(GtkWidget *w, gpointer data)
{
	char *cmd;
	GtkWidget **buttons = data;

	char *start_dir;

	if (buttons[0] == w) { /*we are in the stop button*/
		if(find_running == RUNNING)
			find_running = MAKE_IT_STOP;
		return;
	}

	if(start_dir_e) {
		start_dir = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(start_dir_e), TRUE);
		if(!start_dir) {
			gnome_app_error (GNOME_APP(app),
					 _("Start directory does not exist."));
			return;
		}
	} else
		start_dir = NULL;

	cmd = make_find_cmd(start_dir);
	g_free(start_dir);

	if (cmd == NULL) {
		gnome_app_error (GNOME_APP(app),
				 _("Cannot perform the search. Please specify a search criteria."));
		return;
	}

	gtk_widget_set_sensitive(buttons[0], TRUE);
	gtk_widget_set_sensitive(buttons[1], FALSE);

	really_run_command(cmd, '\0', &find_running, find_store, &find_iter);
	g_free(cmd);

	gtk_widget_set_sensitive(buttons[0], FALSE);
	gtk_widget_set_sensitive(buttons[1], TRUE);
}

static void
run_cmd_dialog(GtkWidget *wid, gpointer data)
{
	char *cmd;
	char *start_dir;
	GtkWidget *dlg;
	GtkWidget *w;

	if(start_dir_e) {
		start_dir = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(start_dir_e), TRUE);
		if(!start_dir) {
			gnome_app_error (GNOME_APP(app),
					 _("Start directory does not exist"));
			return;
		}
	} else
		start_dir = NULL;

	if (!gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook))) 
		cmd = make_locate_cmd();
	else
		cmd = make_find_cmd(start_dir);
		
	g_free(start_dir);
	
	if (cmd == NULL) {
		gnome_app_error (GNOME_APP(app),
				 _("Cannot show the search command line. Please specify a search criteria."));
		return;
	}
	
	dlg = gtk_dialog_new_with_buttons(_("Search Command Line"), GTK_WINDOW(app),
			       GTK_DIALOG_DESTROY_WITH_PARENT,
			       GTK_STOCK_OK, GTK_RESPONSE_OK,
			       NULL);
	
	gtk_window_set_modal (GTK_WINDOW(dlg), TRUE);
				      
	g_object_set (G_OBJECT (dlg),
		      "allow_grow", FALSE,
		      "allow_shrink", FALSE,
		      "resizable", FALSE,
		      NULL);
		      
	gtk_dialog_set_default_response (GTK_DIALOG(dlg), GTK_RESPONSE_CLOSE); 

	w = gtk_label_new(_("This is the command line that can be used to "
			    "execute this search from the console.\n"
			    "(Note: To print one file per line rather then "
			    "null separated, append \"| tr '\\000' '\\n'\" "
			    "to the line below.)\n\nCommand:"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), w, TRUE, TRUE, 0);
	
	w = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(w), FALSE);
	gtk_entry_set_text(GTK_ENTRY(w), cmd);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), w, FALSE, FALSE, 0);

	g_signal_connect_swapped (GTK_OBJECT (dlg), 
                             	  "response", 
                             	  G_CALLBACK (gtk_widget_destroy),
                                  GTK_OBJECT (dlg));

	gtk_widget_show_all(dlg); 
	
	g_free(cmd);
}

static gboolean
view_file_with_application(char *file)
{
	const char *mimeType = gnome_vfs_mime_type_from_name(file);	
	GnomeVFSMimeApplication *mimeApp = gnome_vfs_mime_get_default_application(mimeType);
		
	if (mimeApp) {
		char **argv;
		int argc = 2;

		argv = g_new0 (char *, argc);
		argv[0] = mimeApp->command;
		argv[1] = file;	
		
		if (mimeApp->requires_terminal) 
			gnome_prepend_terminal_to_vector(&argc, &argv);	
		
		gnome_execute_async(NULL, argc, argv);	
		gnome_vfs_mime_application_free(mimeApp);	
		g_free(argv);
		return TRUE;
	}
	return FALSE;
}

static gboolean
nautilus_is_running()
{
	CORBA_Environment ev;
	CORBA_Object obj;
	gboolean ret;
	
	CORBA_exception_init (&ev); 
	obj = bonobo_activation_activate_from_id ("OAFIID:nautilus_factory:bd1e1862-92d7-4391-963e-37583f0daef3",
		Bonobo_ACTIVATION_FLAG_EXISTING_ONLY, NULL, &ev);
		
	ret = !CORBA_Object_is_nil (obj, &ev);
	
	CORBA_Object_release (obj, &ev);	
	CORBA_exception_free (&ev);
	
	return ret;
}

static void
launch_nautilus (char *file)
{
	int argc = 2;
	char **argv = g_new0 (char *, argc);
	
	argv[0] = "nautilus";
	argv[1] = file;
	
	gnome_execute_async(NULL, argc, argv);
	g_free(argv);
}

static gboolean
launch_file (GtkWidget *w, GdkEventButton *event, gpointer data)
{
	gchar *file = NULL;
	gchar *utf8 = NULL;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	if (event->type==GDK_2BUTTON_PRESS) {
		
		if (!gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook))) {
			store = locate_store;
			selection = locate_selection;
		}
		else {
			store = find_store;
			selection = find_selection;
		}
		
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    			return TRUE;
		
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,0,&utf8,-1);
		file = g_locale_from_utf8 (utf8, -1, NULL, NULL, NULL);
		
		if (nautilus_is_running ()) {
			launch_nautilus (file);
		}
		else {
			if (!view_file_with_application(file))	
				gnome_error_dialog_parented(_("No viewer available for this mime type."),
						    	    GTK_WINDOW(app));
		}
		g_free(utf8);
		g_free(file);
		return TRUE;
	}
	return FALSE;
}

static void
quit_cb(GtkWidget *w, gpointer data)
{
	if(find_running == RUNNING)
		find_running = MAKE_IT_STOP;
	if(locate_running == RUNNING)
		locate_running = MAKE_IT_STOP;
	gtk_main_quit();
}

static void
menu_toggled(GtkWidget *w, gpointer data)
{
	if(GTK_CHECK_MENU_ITEM(w)->active)
		current_template = (long)data;
}

static GtkWidget *
make_list_of_templates(void)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList *group=NULL;
	int i;

	menu = gtk_menu_new ();

	for(i=0;templates[i].type!=FIND_OPTION_END;i++) {
		menuitem=gtk_radio_menu_item_new_with_label(group,
							    _(templates[i].desc));
		g_signal_connect(G_OBJECT(menuitem),"toggled",
				 G_CALLBACK(menu_toggled),
			         (gpointer)(long)i);
		group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show (menuitem);
	}
	return menu;
}

static void
remove_option(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	
	gtk_container_remove(GTK_CONTAINER(find_box),w->parent);
	criteria_find = g_list_remove(criteria_find,opt);
}

static void
enable_option(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	GtkWidget *frame = GTK_WIDGET (g_object_get_data (G_OBJECT(w), "frame"));
	gtk_widget_set_sensitive(GTK_WIDGET(frame),
				 GTK_TOGGLE_BUTTON(w)->active);
	opt->enabled = GTK_TOGGLE_BUTTON(w)->active;
}

static void
entry_changed(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	switch(templates[opt->templ].type) {
	case FIND_OPTION_TEXT:
		opt->data.text = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
		break;
	case FIND_OPTION_NUMBER:
		sscanf(gtk_entry_get_text(GTK_ENTRY(w)),"%d",
		       &opt->data.number);
		break;
	case FIND_OPTION_TIME:
		opt->data.time = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
		break;
	default:
		g_warning(_("Entry changed called for a non entry option!"));
		break;
	}
}

char empty_str[]="";

static void
set_option_defaults(FindOption *opt)
{
	switch(templates[opt->templ].type) {
	case FIND_OPTION_BOOL:
		break;
	case FIND_OPTION_TEXT:
		opt->data.text = empty_str;
		break;
	case FIND_OPTION_NUMBER:
		opt->data.number = 0;
		break;
	case FIND_OPTION_TIME:
		opt->data.time = empty_str;
		break;
	default:
	        break;
	}
}

static GtkWidget *
create_option_box(FindOption *opt, gboolean enabled)
{
	GtkWidget *hbox;
	GtkWidget *option;
	GtkWidget *frame;
	GtkWidget *w;

	hbox = gtk_hbox_new(FALSE,5);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(hbox),frame,TRUE,TRUE,0);

	switch(templates[opt->templ].type) {
	case FIND_OPTION_BOOL:
		option = gtk_label_new(_(templates[opt->templ].desc));
		gtk_misc_set_alignment(GTK_MISC(option), 0.0, 0.5);
		break;
	case FIND_OPTION_TEXT:
	case FIND_OPTION_NUMBER:
	case FIND_OPTION_TIME:
		option = gtk_hbox_new(FALSE,5);
		w = gtk_label_new(_(templates[opt->templ].desc));
		gtk_box_pack_start(GTK_BOX(option),w,FALSE,FALSE,0);
		w = gtk_entry_new();
		g_signal_connect(G_OBJECT(w),"changed",
				 G_CALLBACK(entry_changed),opt);
		gtk_box_pack_start(GTK_BOX(option),w,TRUE,TRUE,0);
		break;
	default:
		/* This should never happen, if it does, there is a bug */
		option = gtk_label_new("???");
	        break;
	}
	gtk_container_add(GTK_CONTAINER(frame), option);

	w = gtk_check_button_new_with_label(_("Enabled"));
	g_object_set_data (G_OBJECT (w), "frame", frame);
	g_signal_connect(G_OBJECT(w), "toggled",
			 G_CALLBACK(enable_option), opt);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), enabled);
	enable_option(w, opt);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE,FALSE,0);

	w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(w), "clicked",
			 G_CALLBACK(remove_option), opt);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE,FALSE, 0);

	return hbox;
}

static void
add_option(int templ, gboolean enabled)
{
	FindOption *opt = g_new(FindOption,1);
	GtkWidget *w;

	opt->templ = templ;
	opt->enabled = TRUE;

	set_option_defaults(opt);
	
	w = create_option_box(opt, enabled);
	gtk_widget_show_all(w);

	criteria_find = g_list_append(criteria_find,opt);
	gtk_box_pack_start(GTK_BOX(find_box),w,FALSE,FALSE,0);
}

static void
add_option_cb(GtkWidget *w, gpointer data)
{
	add_option(current_template, TRUE);
}

static GtkWidget *
create_find_page(void)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *w;
	GtkWidget *treeview;
	char *s;
	static GtkWidget *buttons[2];
	GtkTreeModel *model;
	GtkCellRenderer *renderer;

	vbox = gtk_vbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),GNOME_PAD_SMALL);

	hbox = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
	w = gtk_label_new(_("Start in directory "));
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);
	start_dir_e = gnome_file_entry_new("directory", _("Browse"));
	gnome_file_entry_set_directory_entry(GNOME_FILE_ENTRY(start_dir_e), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox),start_dir_e,TRUE,TRUE,0);
	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(start_dir_e));
	s = g_get_current_dir();
	gtk_entry_set_text(GTK_ENTRY(w), s);
	g_free(s);

	find_box = gtk_vbox_new(FALSE,GNOME_PAD_SMALL);
	w = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(w),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
					
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(w), find_box);
        gtk_box_pack_start (GTK_BOX(vbox),w,TRUE,TRUE,GNOME_PAD_SMALL);
	
	hbox = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	w = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(w), make_list_of_templates());
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	w = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(w),"clicked",
			 G_CALLBACK(add_option_cb),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	w = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(w),
                                        GTK_POLICY_ALWAYS,
                                        GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox),w,TRUE,TRUE,0);
	
	find_store = gtk_list_store_new(1, G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(find_store));
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),1);
	
	find_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	renderer = gtk_cell_renderer_text_new ();

	g_signal_connect(G_OBJECT(treeview), "button_press_event",
		         G_CALLBACK(launch_file),NULL);	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(treeview), -1, 
			"Results", renderer, "text", NULL);
	
	gtk_container_add(GTK_CONTAINER(w),treeview);

	hbox = gtk_hbutton_box_new(); 
	gtk_box_pack_end(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	w = gtk_hseparator_new();
	gtk_box_pack_end(GTK_BOX(vbox),w,FALSE,FALSE,0);

	w = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(w), "clicked",
			 G_CALLBACK(quit_cb), NULL);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 0);

	buttons[1] = gtk_button_new_from_stock(GTK_STOCK_FIND);
	buttons[0] = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(buttons[1]),"clicked",
			 G_CALLBACK(run_command), buttons);
	g_signal_connect(G_OBJECT(buttons[0]),"clicked",
			 G_CALLBACK(run_command), buttons);
	gtk_box_pack_end(GTK_BOX(hbox),buttons[0],FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(hbox),buttons[1],FALSE,FALSE,0);
	gtk_widget_set_sensitive(buttons[0],FALSE);

	add_option(OPTION_FILENAME, TRUE);

	return vbox;
}

static void
run_locate_command(GtkWidget *w, gpointer data)
{
	gchar *cmd;
	GtkWidget **buttons = data;

	char *locate_string;

	if (buttons[0] == w) { /*we are in the stop button*/
		if(locate_running == RUNNING)
			locate_running = MAKE_IT_STOP;
		return;
	}

	cmd = make_locate_cmd();
	
	if (cmd == NULL) {
		gnome_app_error (GNOME_APP(app),
				 _("Cannot perform the search. "
				   "Please specify a search criteria."));
		return;
	}
	
	gtk_widget_set_sensitive(buttons[0], TRUE);
	gtk_widget_set_sensitive(buttons[1], FALSE);

	really_run_command(cmd, '\n', &locate_running, locate_store, &locate_iter);
	g_free(cmd);

	gtk_widget_set_sensitive(buttons[0], FALSE);
	gtk_widget_set_sensitive(buttons[1], TRUE);
}

static void
locate_activate (GtkWidget *entry, gpointer data)
{
	GtkWidget **buttons = data;
	run_locate_command (buttons[1], buttons);
}

static GtkWidget *
create_locate_page(void)
{
	GtkWidget *w, *vbox, *hbox, *treeview;
	GtkTreeViewColumn *column;	
	static GtkWidget *buttons[2];
	static gchar *history = NULL;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;

	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), GNOME_PAD_SMALL);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	w = gtk_label_new(_("Search For "));
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);

	locate_entry = gnome_entry_new(history);
	gnome_entry_set_max_saved(GNOME_ENTRY(locate_entry), 10);
	gtk_box_pack_start(GTK_BOX(hbox), locate_entry, TRUE, TRUE, 0);
	
	g_signal_connect (G_OBJECT (locate_entry), "activate",
			  G_CALLBACK (locate_activate),
			  buttons);

	/* add search results window here */
	w = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_usize(w,480,260);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(w),
                                        GTK_POLICY_ALWAYS,
                                        GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox),w,TRUE,TRUE,0);
	
	locate_store = gtk_list_store_new(1, G_TYPE_STRING);
	
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(locate_store));
        
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),1);
	
	locate_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	renderer = gtk_cell_renderer_text_new ();
	
	g_signal_connect(G_OBJECT(treeview), "button_press_event",
		         G_CALLBACK(launch_file),NULL);		   
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(treeview), -1, 
			"Results", renderer, "text", NULL);
	
	gtk_container_add(GTK_CONTAINER(w),treeview);

	hbox = gtk_hbutton_box_new();
	gtk_box_pack_end(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	w = gtk_hseparator_new();
	gtk_box_pack_end(GTK_BOX(vbox),w,FALSE,FALSE,0);

	w = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(w), "clicked",
			 G_CALLBACK(quit_cb), NULL);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 0);

	buttons[1] = gtk_button_new_from_stock(GTK_STOCK_FIND);
	buttons[0] = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(buttons[1]),"clicked",
			 G_CALLBACK(run_locate_command), buttons);
	g_signal_connect(G_OBJECT(buttons[0]),"clicked",
			 G_CALLBACK(run_locate_command), buttons);
	gtk_box_pack_end(GTK_BOX(hbox),buttons[0],FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(hbox),buttons[1],FALSE,FALSE,0);
	gtk_widget_set_sensitive(buttons[0],FALSE);

	return vbox;
}

static GtkWidget *
create_window(void)
{
	nbook = gtk_notebook_new();
	gtk_container_set_border_width(GTK_CONTAINER(nbook),GNOME_PAD_SMALL);

	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),create_locate_page(),
				 gtk_label_new(_("Simple")));  /* Can we connect to this  */

	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),create_find_page(),
				 gtk_label_new(_("Advanced")));
	
	return nbook;
}

static void
about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	static const char *authors[] = {
		"George Lebl <jirka@5z.com>",
		NULL
	};
	gchar *documenters[] = {
		NULL
	};
	/* Translator credits */
	gchar *translator_credits = _("");

	if (about != NULL)
	{
		gdk_window_show(about->window);
		gdk_window_raise(about->window);
		return;
	}

	about = gnome_about_new(_("The Gnome Search Tool"), VERSION,
				_("(C) 1998,2000 the Free Software Foundation"),
				_("Frontend to the unix find/locate "
				  "commands"),
				authors,
				(const char **)documenters,
				(const char *)translator_credits,
				NULL);
	g_signal_connect (G_OBJECT (about), "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &about);
	gtk_widget_show (about);
}

static void
save_results(GtkFileSelection *selector, gpointer user_data)
{
	FILE *fp;
	GtkListStore *store;
	GtkTreeIter iter;
	gint n_children, i;
	
	if (!gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook)))
		store = locate_store;
	else
		store = find_store;
	
	filename = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));
	
	if (fp = fopen(filename, "w")) {
	
		if (gtk_tree_model_get_iter_root(GTK_TREE_MODEL(store), &iter)) {
			
			n_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL);
			
			for (i = 0; i < n_children; i++)
			{
				gchar *text, *utf8; 
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &utf8, -1);
				text = g_locale_from_utf8 (utf8, -1, NULL, NULL, NULL); 
				gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);						    
				fprintf(fp, "%s\n", text);
				g_free(text);
			}
		}		 
		fclose(fp);
	} 
	else {
		gnome_app_error(GNOME_APP(app), _("Cannot save the results file."));
	}
}

static void
show_file_selector(GtkWidget *widget)
{
	file_selector = gtk_file_selection_new(_("Save Results"));
		
	if (filename) gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), filename);

	g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button), "clicked",
				G_CALLBACK (save_results), NULL);
	
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                             	"clicked",
                             	G_CALLBACK (gtk_widget_destroy), 
                             	(gpointer) file_selector); 

   	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
                             	"clicked",
                             	G_CALLBACK (gtk_widget_destroy),
                             	(gpointer) file_selector); 

	gtk_window_set_modal (GTK_WINDOW(file_selector), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW(file_selector), GTK_WINDOW(app));
	gtk_window_set_position (GTK_WINDOW (file_selector), GTK_WIN_POS_MOUSE);

	gtk_widget_show (GTK_WIDGET(file_selector));
}

/* thy evil easter egg */
static gboolean
window_click(GtkWidget *w, GdkEventButton *be)
{
	static int foo = 0;
	if(be->button == 3 && (++foo)%3 == 0)
		gnome_ok_dialog("9\\_/_-\n  /\\ /\\\n\nGEGL!");
	return TRUE;
}

static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_ITEM_STOCK(N_("S_how Command"), "", run_cmd_dialog,GTK_STOCK_NEW),
	GNOMEUIINFO_ITEM_STOCK(N_("Save Results _As..."), "",show_file_selector,GTK_STOCK_SAVE_AS),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM(quit_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {  
	GNOMEUIINFO_HELP("gsearchtool"),
	GNOMEUIINFO_MENU_ABOUT_ITEM(about_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo gsearch_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE(file_menu),
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
        GNOMEUIINFO_END
};

int
main(int argc, char *argv[])
{
	GnomeProgram *gsearchtool;
	GtkWidget *search;

	/* Initialize the i18n stuff */
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gsearchtool = gnome_program_init ("gsearchtool", VERSION, LIBGNOMEUI_MODULE, argc, argv, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-searchtool.png");

	if (!bonobo_init (bonobo_activation_orb_get (), NULL, NULL))
		g_error (_("Cannot initialize bonobo."));

	bonobo_activate();

        app = gnome_app_new("gsearchtool", _("Search Tool"));
	gtk_window_set_wmclass (GTK_WINDOW (app), "gsearchtool", "gsearchtool");
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, TRUE);

        g_signal_connect(G_OBJECT(app), "delete_event",
			 G_CALLBACK(quit_cb), NULL);

	g_signal_connect(G_OBJECT(app), "button_press_event",
			 G_CALLBACK(window_click), NULL);

	/*set up the menu*/
        gnome_app_create_menus(GNOME_APP(app), gsearch_menu);

	search = create_window();
	gtk_widget_show_all(search);

	gnome_app_set_contents(GNOME_APP(app), search);

	gtk_widget_show(app);
	
	gtk_main ();

	return 0;
}
