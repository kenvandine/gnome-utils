/* Gnome Search Tool 
 * (C) 1998 the Free Software Foundation
 *
 * Author:   George Lebl
 */

#include <config.h>
#include <gnome.h>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>

#include "gsearchtool.h"
#include "outdlg.h"

FindOptionTemplate templates[] = {
	{ FIND_OPTION_TEXT, "-name '%s'", N_("File name") },
	{ FIND_OPTION_CHECKBOX_TRUE, "-maxdepth 1",
				N_("Don't search subdirectories") },
	{ FIND_OPTION_TEXT, "-user '%s'", N_("File owner") },
	{ FIND_OPTION_TEXT, "-group '%s'", N_("File owner group") },
	{ FIND_OPTION_TIME, "-mtime '%s'", N_("Last modification time") },
	{ FIND_OPTION_CHECKBOX_TRUE, "-mount",
				N_("Don't search mounted filesystems") },
	{ FIND_OPTION_CHECKBOX_TRUE, "-empty",
				N_("Empty file") },
	{ FIND_OPTION_CHECKBOX_TRUE, "-nouser -o -nogroup",
				N_("Invalid user or group") },
	{ FIND_OPTION_TEXT, "\\! -name '%s'", N_("Filenames except") },
	{ FIND_OPTION_GREP, "fgrep -l '%s'", N_("Simple substring search") },
	{ FIND_OPTION_GREP, "grep -l '%s'", N_("Regular expression search") },
	{ FIND_OPTION_GREP, "egrep -l '%s'",
				N_("Extended regular expression search") },
	{ FIND_OPTION_END, NULL,NULL}
};

/*this will not include the directories in search*/
char defoptions[]="\\! -type d";
/*this should be done if the above is made optional*/
/*char defoptions[]="-mindepth 0";*/

GList *criteria_find=NULL;
GList *criteria_grep=NULL;

static GtkWidget *find_box=NULL;
static GtkWidget *grep_box=NULL;

static GtkWidget *start_dir_e=NULL;

static gint current_template=0;

static char *
makecmd(void)
{
	char *cmdbuf;
	char *p;
	GList *list;
	int len=1; /*length of the cmdbuf needed*/
	char *start_dir;
	
	if(start_dir_e &&
	   gtk_entry_get_text(GTK_ENTRY(start_dir_e)) &&
	   *gtk_entry_get_text(GTK_ENTRY(start_dir_e)))
		start_dir = gtk_entry_get_text(GTK_ENTRY(start_dir_e));
	else
		start_dir = NULL;

	len+=strlen("find . -print "); /*the neccessary base of the command*/
	len+=strlen(defoptions)+1;
	if(start_dir)
		len+=strlen(start_dir)+1;
	for(list=criteria_find;list!=NULL;list=g_list_next(list)) {
		FindOption *opt = list->data;
		if(opt->enabled) {
			len+=strlen(templates[opt->templ].option)+1;
			switch(templates[opt->templ].type) {
			case FIND_OPTION_CHECKBOX_TRUE:
			case FIND_OPTION_CHECKBOX_FALSE:
				break;
			case FIND_OPTION_TEXT:
				len+=strlen(opt->data.text)+1;
				break;
			case FIND_OPTION_NUMBER:
				len+=log10(opt->data.number)+2;
				break;
			case FIND_OPTION_TIME:
				len+=strlen(opt->data.time)+1;
				break;
			case FIND_OPTION_GREP:
				g_warning(_("grep options found in find list bad bad!"));
				break;
			default:
			        break;
			}
		}
	}

	for(list=criteria_grep;list!=NULL;list=g_list_next(list)) {
		FindOption *opt = list->data;
		if(opt->enabled) {
			len+=strlen(" | xargs ");
			len+=strlen(templates[opt->templ].option)+1;
			if(templates[opt->templ].type!=FIND_OPTION_GREP)
				g_warning("non-grep option found in grep list, bad bad!");
			else
				len+=strlen(opt->data.text)+1;
		}
	}
	
	cmdbuf = g_new(char,len);
	p = cmdbuf;
	
	if(start_dir)
		p+=sprintf(p,"find %s %s ",start_dir,defoptions);
	else
		p+=sprintf(p,"find . %s ",defoptions);

	for(list=criteria_find;list!=NULL;list=g_list_next(list)) {
		FindOption *opt = list->data;
		if(opt->enabled) {
			switch(templates[opt->templ].type) {
			case FIND_OPTION_CHECKBOX_TRUE:
				if(opt->data.bool)
					p+=sprintf(p,"%s ",templates[opt->templ].option);
				break;
			case FIND_OPTION_CHECKBOX_FALSE:
				if(!opt->data.bool)
					p+=sprintf(p,"%s ",templates[opt->templ].option);
				break;
			case FIND_OPTION_TEXT:
				p+=sprintf(p,templates[opt->templ].option,
					   opt->data.text);
				strcat(p++," ");
				break;
			case FIND_OPTION_NUMBER:
				p+=sprintf(p,templates[opt->templ].option,
					   opt->data.number);
				strcat(p++," ");
				break;
			case FIND_OPTION_TIME:
				p+=sprintf(p,templates[opt->templ].option,
					   opt->data.time);
				strcat(p++," ");
				break;
			case FIND_OPTION_GREP:
				g_warning(_("grep options found in find list bad bad!"));
				break;
			default:
			        break;
			}
		}
	}

	p+=sprintf(p,"-print ");

	for(list=criteria_grep;list!=NULL;list=g_list_next(list)) {
		FindOption *opt = list->data;
		if(opt->enabled) {
			p+=sprintf(p,"| xargs ");
			if(templates[opt->templ].type!=FIND_OPTION_GREP)
				g_warning(_("non-grep option found in grep list, bad bad!"));
			else
				p+=sprintf(p,templates[opt->templ].option,
					   opt->data.text);
			strcat(p++," ");
		}
	}

	return cmdbuf;
}

static void
run_command(GtkWidget *w, gpointer data)
{
	char *cmd;
	static int running = 0;
	GtkWidget *stopbutton = data;
	int idle;

	char s[PATH_MAX+1];
	int spos=0;
	char ret[PIPE_READ_BUFFER];
	int fd[2];
	int pid;
	int i,n;

	if(data==w) {/*we are in the stop button*/
		if(running>0)
			running = 2;
		return;
	}

	/* running =0 not running */
	/* running =1 running */
	/* running =2 make it stop! */
	running=1;

	/*create the results box*/
	/*FIXME: add an option to autoclear result box and pass TRUE in that
	  case*/
	outdlg_makedlg(_("Search Results"),FALSE);

	gtk_widget_set_sensitive(stopbutton,TRUE);
	gtk_grab_add(stopbutton);
	
	cmd = makecmd();
	pipe(fd);
	
	if((pid=fork())==0) {
		close(fd[0]);
		close(1); dup(fd[1]); close(fd[1]);
		execl("/bin/sh","/bin/sh","-c",cmd,NULL);
		_exit(0); /* in case it fails */
	}
	close(fd[1]);

	outdlg_freeze();
	idle = gtk_idle_add((GtkFunction)gtk_true,NULL);

	fcntl(fd[0],F_SETFL,O_NONBLOCK);
	while(running==1) {
		n=read(fd[0],ret,PIPE_READ_BUFFER);
		for(i=0;i<n;i++) {
			if(ret[i]=='\n') {
				s[spos]=0;
				spos=0;
				outdlg_additem(s);
			} else
				s[spos++]=ret[i];
		}
		if(waitpid(-1,NULL,WNOHANG)!=0)
			break;
		/*this for some reason doesn't work, I need to add an
		  idle handler and do iteration with TRUE*/
		/*if(gtk_events_pending())
			gtk_main_iteration_do(FALSE);*/
		gtk_main_iteration_do(TRUE);
		if(running==2) {
			kill(pid,SIGKILL);
			wait(NULL);
		}
	}
	/* now we got it all ... so finish reading from the pipe */
	while((n=read(fd[0],ret,PIPE_READ_BUFFER))>0) {
		for(i=0;i<n;i++) {
			if(ret[i]=='\n') {
				s[spos]=0;
				spos=0;
				outdlg_additem(s);
			} else
				s[spos++]=ret[i];
		}
	}
	close(fd[0]);

	gtk_idle_remove(idle);
	outdlg_thaw();
	
	g_free(cmd);

	gtk_grab_remove(stopbutton);
	gtk_widget_set_sensitive(stopbutton,FALSE);
	
	outdlg_showdlg();
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
	gint i;

	menu = gtk_menu_new ();

	for(i=0;templates[i].type!=FIND_OPTION_END;i++) {
		menuitem=gtk_radio_menu_item_new_with_label(group,
							    _(templates[i].desc));
		gtk_signal_connect(GTK_OBJECT(menuitem),"toggled",
				   GTK_SIGNAL_FUNC(menu_toggled),
				   (gpointer)(long)i);
		group=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menuitem));
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_widget_show (menuitem);
	}
	return menu;
}

static gint
remove_option(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	if(templates[opt->templ].type == FIND_OPTION_GREP) {
		gtk_container_remove(GTK_CONTAINER(grep_box),w->parent);
		criteria_grep = g_list_remove(criteria_grep,opt);
	} else {
		gtk_container_remove(GTK_CONTAINER(find_box),w->parent);
		criteria_find = g_list_remove(criteria_find,opt);
	}
	return FALSE;
}

static gint
enable_option(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	GtkWidget *frame = gtk_object_get_user_data(GTK_OBJECT(w));
	gtk_widget_set_sensitive(GTK_WIDGET(frame),
				 GTK_TOGGLE_BUTTON(w)->active);
	opt->enabled = GTK_TOGGLE_BUTTON(w)->active;
	return FALSE;
}

static gint
entry_changed(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	switch(templates[opt->templ].type) {
	case FIND_OPTION_TEXT:
	case FIND_OPTION_GREP:
		opt->data.text=gtk_entry_get_text(GTK_ENTRY(w));
		break;
	case FIND_OPTION_NUMBER:
		sscanf(gtk_entry_get_text(GTK_ENTRY(w)),"%d",
		       &opt->data.number);
		break;
	case FIND_OPTION_TIME:
		opt->data.time=gtk_entry_get_text(GTK_ENTRY(w));
		break;
	default:
		g_warning("Entry changed called for a non entry option!");
		break;
	}
	return FALSE;
}

static gint
bool_changed(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	opt->data.bool = (GTK_TOGGLE_BUTTON(w)->active!=FALSE);
	return FALSE;
}

char empty_str[]="";

static void
set_option_defaults(FindOption *opt)
{
	switch(templates[opt->templ].type) {
	case FIND_OPTION_CHECKBOX_TRUE:
		opt->data.bool = TRUE;
		break;
	case FIND_OPTION_CHECKBOX_FALSE:
		opt->data.bool = FALSE;
		break;
	case FIND_OPTION_TEXT:
	case FIND_OPTION_GREP:
		opt->data.text=empty_str;
		break;
	case FIND_OPTION_NUMBER:
		opt->data.number = 0;
		break;
	case FIND_OPTION_TIME:
		opt->data.time=empty_str;
		break;
	default:
	        break;
	}
}

static GtkWidget *
create_option_box(FindOption *opt)
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
	case FIND_OPTION_CHECKBOX_TRUE:
		option = gtk_check_button_new_with_label(
						_(templates[opt->templ].desc));
		gtk_signal_connect(GTK_OBJECT(option),"toggled",
				   GTK_SIGNAL_FUNC(bool_changed),opt);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(option),TRUE);
		break;
	case FIND_OPTION_CHECKBOX_FALSE:
		option = gtk_check_button_new_with_label(
						_(templates[opt->templ].desc));
		gtk_signal_connect(GTK_OBJECT(option),"toggled",
				   GTK_SIGNAL_FUNC(bool_changed),opt);
		break;
	case FIND_OPTION_TEXT:
	case FIND_OPTION_NUMBER:
	case FIND_OPTION_TIME:
	case FIND_OPTION_GREP:
		option = gtk_hbox_new(FALSE,5);
		w = gtk_label_new(_(templates[opt->templ].desc));
		gtk_box_pack_start(GTK_BOX(option),w,FALSE,FALSE,0);
		w = gtk_entry_new();
		gtk_signal_connect(GTK_OBJECT(w),"changed",
				   GTK_SIGNAL_FUNC(entry_changed),opt);
		gtk_box_pack_start(GTK_BOX(option),w,TRUE,TRUE,0);
		break;
	default:
	        break;
	}
	gtk_container_add(GTK_CONTAINER(frame),option);

	w = gtk_check_button_new_with_label(_("Enable"));
	gtk_object_set_user_data(GTK_OBJECT(w),frame);
	gtk_signal_connect(GTK_OBJECT(w),"toggled",
			   GTK_SIGNAL_FUNC(enable_option),opt);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),TRUE);
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	w = gtk_button_new_with_label(_("Remove"));
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(remove_option),opt);
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	return hbox;
}

static void
add_option(gint templ)
{
	FindOption *opt = g_new(FindOption,1);
	GtkWidget *w;

	opt->templ = templ;
	opt->enabled = TRUE;

	set_option_defaults(opt);

	w = create_option_box(opt);
	gtk_widget_show_all(w);

	/*if it's a grep type option (criterium)*/
	if(templates[templ].type == FIND_OPTION_GREP) {
		criteria_grep = g_list_append(criteria_grep,opt);
		gtk_box_pack_start(GTK_BOX(grep_box),w,FALSE,FALSE,0);
	} else {
		criteria_find = g_list_append(criteria_find,opt);
		gtk_box_pack_start(GTK_BOX(find_box),w,FALSE,FALSE,0);
	}
}

static gint
add_option_cb(GtkWidget *w, gpointer data)
{
	add_option(current_template);
	return FALSE;
}

static GtkWidget *
create_find_page(void)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *w;
	GtkWidget *sb;

	vbox = gtk_vbox_new(FALSE,5);
	gtk_container_border_width(GTK_CONTAINER(vbox),5);

	hbox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
	w = gtk_label_new(_("Start in directory:"));
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);
	start_dir_e = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),start_dir_e,TRUE,TRUE,0);
	gtk_entry_set_text(GTK_ENTRY(start_dir_e),".");

	find_box = gtk_vbox_new(TRUE,5);
	gtk_box_pack_start(GTK_BOX(vbox),find_box,TRUE,TRUE,0);
	grep_box = gtk_vbox_new(TRUE,5);
	gtk_box_pack_start(GTK_BOX(vbox),grep_box,TRUE,TRUE,0);

	hbox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	w = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(w), make_list_of_templates());
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	w = gtk_button_new_with_label(_("Add"));
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_option_cb),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	w = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox),w,FALSE,FALSE,0);

	hbox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	w = gtk_button_new_with_label(_("Start"));
	sb = gtk_button_new_with_label(_("Stop"));
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(run_command),sb);
	gtk_signal_connect(GTK_OBJECT(sb),"clicked",
			   GTK_SIGNAL_FUNC(run_command),sb);
	gtk_box_pack_end(GTK_BOX(hbox),w,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(hbox),sb,FALSE,FALSE,0);
	gtk_widget_set_sensitive(sb,FALSE);

	add_option(0);

	return vbox;
}

#ifdef NEED_UNUSED_FUNCTION
static GtkWidget *
create_locate_page(void)
{
	return gtk_label_new(_("This is not yet implemented"));
}
#endif

static GtkWidget *
create_window(void)
{
	GtkWidget *nbook;

	nbook = gtk_notebook_new();
	gtk_container_border_width(GTK_CONTAINER(nbook),5);

	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),create_find_page(),
				 gtk_label_new(_("Full find (find)")));
#ifdef QUICKFIND_IMPLEMENTED
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),create_locate_page(),
				 gtk_label_new(_("Quick find (locate)")));
#endif

	return nbook;
}

static void
about_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *about;
	static const char *authors[] = {
		"George Lebl <jirka@5z.com>",
		NULL
	};

	about = gnome_about_new(_("The Gnome Search Tool"), VERSION,
				_("(C) 1998 the Free Software Foundation"),
				authors,
				_("Frontend to the unix find/grep/locate "
				  "commands"),
				NULL);
	gtk_widget_show (about);
}

static void
quit_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}


static GnomeUIInfo file_menu[] = {
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
	GtkWidget *app;
	GtkWidget *search;

	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init ("gsearchtool", VERSION, argc, argv);
	
        app=gnome_app_new("gsearchtool", _("Gnome Search Tool"));
	gtk_window_set_wmclass (GTK_WINDOW (app), "gsearchtool", "gsearchtool");
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, TRUE);

        gtk_signal_connect(GTK_OBJECT(app), "delete_event",
			   GTK_SIGNAL_FUNC(quit_cb), NULL);
        gtk_window_set_policy(GTK_WINDOW(app),1,1,0);

	/*set up the menu*/
        gnome_app_create_menus(GNOME_APP(app), gsearch_menu);

	search = create_window();
	gtk_widget_show_all(search);

	gnome_app_set_contents(GNOME_APP(app), search);

	gtk_widget_show(app);
	
	gtk_main ();

	return 0;
}
