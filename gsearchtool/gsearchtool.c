/* GNOME Search Tool
 * Copyright (C) 1997 George Lebl.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gnome.h>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "gsearchtool.h"
#include "unixcmd.h"
#include "outdlg.h"

FindOptionTemplate templates[] = {
	{ FIND_OPTION_TEXT, "-name '%s'", "File name" },
	{ FIND_OPTION_CHECKBOX_TRUE, "-maxdepth 0",
				"Don't search subdirectories" },
	{ FIND_OPTION_TEXT, "-user '%s'", "File owner" },
	{ FIND_OPTION_TEXT, "-group '%s'", "File owner group" },
	{ FIND_OPTION_TIME, "-mtime '%s'", "Last modification time" },
	{ FIND_OPTION_CHECKBOX_TRUE, "-mount",
				"Don't search mounted filesystems" },
	{ FIND_OPTION_GREP, "fgrep -l '%s'", "Simple substring search" },
	{ FIND_OPTION_GREP, "grep -l '%s'", "Regular expression search" },
	{ FIND_OPTION_GREP, "egrep -l '%s'",
				"Extended regular expression search" },
	{ FIND_OPTION_END, NULL,NULL}
};

GList *criteria=NULL;

/* search for a filename */
static void
search(GtkWidget * widget, gpointer data)
{
	char cmd[1024]="";
	char showcmd[1024]="";
	char appcmd[512]="";
	char s[256];
	int spos=0;
	char ret[PIPE_READ_BUFFER];
	int fd[2];
	int afd[2];
	struct finfo *f;

	int pid,pid2;
	static int running=0;
	

	int i,n;

	f=(struct finfo *)data;

	if(running>0) {
		running=2;
		return;
	}
		
	/* running =0 not running */
	/* running =1 running */
	/* running =2 make it stop! */
	running=1;

	gtk_grab_add(widget);
	gtk_label_set(GTK_LABEL(GTK_BUTTON(widget)->child),"STOP!");

	pipe(fd);

	/*make a unix command*/
	makecmd(cmd,showcmd,appcmd,f,fd[1]);

	/*create the results box*/
	outdlg_makedlg("Search Results");
	outdlg_clearlist(NULL,NULL);

	puts("Real unix equivalent:");
	puts(showcmd);
	puts("Commands we will use:");
	puts(cmd);
	puts(appcmd);

	if((pid=fork())==0) {
		close(fd[0]);
		execl("/bin/sh","/bin/sh","-c",cmd,NULL);
		_exit(0); /* in case it fails */
	}
	close(fd[1]);
	if(appcmd[0]!='\0') {
		pipe(afd);
		if((pid2=fork())==0) {
			close(afd[1]);
			close(0); dup(afd[0]); close(afd[0]);
			execl("/bin/sh","/bin/sh","-c",appcmd,NULL);
			_exit(0); /* in case it fails */
		}
		close(afd[0]);
	}
	
	fcntl(fd[0],F_SETFL,O_NONBLOCK);
	while(running==1) {
		n=read(fd[0],ret,PIPE_READ_BUFFER);
		for(i=0;i<n;i++) {
			if((f->methodfind && ret[i]==0) ||
				(f->methodlocate && ret[i]=='\n')) {
				s[spos]=0;
				spos=0;
				outdlg_additem(s);
				if(appcmd[0]!='\0') write(afd[1],s,strlen(s)+1);
			} else
				s[spos++]=ret[i];
		}
		if(waitpid(-1,NULL,WNOHANG)!=0)
			break;
		if(gtk_main_iteration_do(FALSE))
			_exit(0);
		if(running==2) {
			kill(pid,SIGKILL);
			if(appcmd[0]!='\0') kill(pid2,SIGKILL);
			wait(NULL);
		}
	}
	/* now we got it all ... so finish reading from the pipe */
	while((n=read(fd[0],ret,PIPE_READ_BUFFER))>0) {
		for(i=0;i<n;i++) {
			if((f->methodfind && ret[i]==0) ||
				(f->methodlocate && ret[i]=='\n')) {
				s[spos]=0;
				spos=0;
				outdlg_additem(s);
				if(appcmd[0]!='\0') write(afd[1],s,strlen(s)+1);
			} else
				s[spos++]=ret[i];
		}
	}
	close(fd[0]);
	close(afd[1]);

	gtk_label_set(GTK_LABEL(GTK_BUTTON(widget)->child),"GO!");
	gtk_grab_remove(widget);

	running=0;

	outdlg_showdlg();
}

static void
browse(GtkWidget * widget, gpointer data)
{
	g_print("Browse\n");
}

/*
 * change data boolean to the value of the checkbox to keep the structure
 * allways up to date
 */
static void
changetb(GtkWidget * widget, gpointer data)
{
	if(GTK_TOGGLE_BUTTON(widget)->active)
		*(gboolean *)data=TRUE;
	else
		*(gboolean *)data=FALSE;
}

/*
 * change the text to what it is in the entry, so that the structure is
 * allways up to date
 */
static void
changeentry(GtkWidget * widget, gpointer data)
{
	strcpy((gchar *)data,gtk_entry_get_text(GTK_ENTRY(widget)));
}

/* quit */
static gint
destroy(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

/*
 * Kind of a long main but all the gui stuff is here and that I guess
 * should be kept in one place
 */
int
main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *searchpb,*quitpb,*browsepb;
	GtkWidget *subdircb,*mountcb;
	GtkWidget *usefindrb,*uselocaterb;
	GtkWidget *filee,*dire,*ownere,*groupe,*lastmode,*extraoptse;
	GtkWidget *searchtexte,*applycmde;
	GtkWidget *searchgreprb,*searchfgreprb,*searchegreprb;
	GtkWidget *boxv,*boxh,*boxm;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *nbook;
	GtkWidget *nbookfind;
	GtkWidget *nbookcontent;

	GtkTooltips *tips;


	/* FIXME: UUUUUUUUUUUUUUUUUGLY */
	struct finfo findinfo= {"","",TRUE,"","","","",TRUE,FALSE,"",TRUE,
				FALSE,FALSE,"",FALSE};

	gtk_init(&argc, &argv);

	/*set up the top level window*/
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),"GNOME Search Tool");
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
			   GTK_SIGNAL_FUNC(destroy), NULL);
	gtk_container_border_width(GTK_CONTAINER(window), 10);


	/*set up the tooltips*/
	tips=gtk_tooltips_new();


	/*the main vertical box to put everything in*/
	boxm=gtk_vbox_new(FALSE,10);

	/*set up the notebooks*/
	nbook = gtk_notebook_new();
	nbookfind = gtk_notebook_new();
	nbookcontent = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nbookfind),GTK_POS_RIGHT);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nbookcontent),GTK_POS_RIGHT);


	/*files and directories info page*/
	boxv=gtk_vbox_new(FALSE,0);

	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Filename pattern:");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	filee = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(filee), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.file));
	gtk_box_pack_start(GTK_BOX(boxh),filee,TRUE,TRUE,5);
	gtk_widget_show(filee);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Search in:");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	dire = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(dire), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.dir));
	gtk_box_pack_start(GTK_BOX(boxh),dire,TRUE,TRUE,5);
	gtk_widget_show(dire);
	browsepb = gtk_button_new_with_label("Browse...");
	gtk_signal_connect_object(GTK_OBJECT(browsepb), "clicked",
			   GTK_SIGNAL_FUNC(browse),GTK_OBJECT(dire));
	gtk_box_pack_start(GTK_BOX(boxh),browsepb,FALSE,FALSE,5);
	gtk_widget_show(browsepb);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	boxh=gtk_hbox_new(FALSE,0);
	subdircb = gtk_check_button_new_with_label("Search subdirectories");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(subdircb),1);
	gtk_signal_connect(GTK_OBJECT(subdircb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.subdirs));
	gtk_box_pack_start(GTK_BOX(boxh),subdircb,FALSE,FALSE,5);
	gtk_widget_show(subdircb);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	/*add this stuff to a page for files and directories*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookfind),boxv,
		gtk_label_new("File &\nDir"));
	gtk_widget_show(boxv);


	/*time and date page*/
	boxv=gtk_vbox_new(FALSE,0);

	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Last Modified (days ago):");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	lastmode = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(lastmode), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.lastmod));
	gtk_box_pack_start(GTK_BOX(boxh),lastmode,TRUE,TRUE,5);
	gtk_widget_show(lastmode);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	/*add this stuff to a page for time and date*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookfind),boxv,
		gtk_label_new("Time &\nDate"));
	gtk_widget_show(boxv);


	/*Ownership and permissions page*/
	boxv=gtk_vbox_new(FALSE,0);

	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Owner:");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	ownere = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(ownere), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.owner));
	gtk_box_pack_start(GTK_BOX(boxh),ownere,TRUE,TRUE,5);
	gtk_widget_show(ownere);

	label = gtk_label_new("Group:");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	groupe = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(groupe), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.group));
	gtk_box_pack_start(GTK_BOX(boxh),groupe,TRUE,TRUE,5);
	gtk_widget_show(groupe);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	/*add this stuff to a page for ownership and persmissions*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookfind),boxv,
		gtk_label_new("Ownership"));
	gtk_widget_show(boxv);

	/*the advanced option page*/
	boxv=gtk_vbox_new(FALSE,0);
	frame=gtk_frame_new("Method");
	boxh=gtk_hbox_new(FALSE,0);
	usefindrb=gtk_radio_button_new_with_label(NULL,
		"Slower but more powerful search\n(find command)");
	gtk_signal_connect(GTK_OBJECT(usefindrb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.methodfind));
	gtk_box_pack_start(GTK_BOX(boxh),usefindrb,TRUE,TRUE,5);
	gtk_widget_show(usefindrb);
	uselocaterb=gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(usefindrb)),
		"Faster but inaccurate search\n(locate command)");
	gtk_signal_connect(GTK_OBJECT(uselocaterb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.methodlocate));
	gtk_box_pack_start(GTK_BOX(boxh),uselocaterb,TRUE,TRUE,5);
	gtk_widget_show(uselocaterb);
	gtk_container_add(GTK_CONTAINER(frame), boxh);
	gtk_widget_show(boxh);
	boxh=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(boxh),frame,TRUE,TRUE,5);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	boxh=gtk_hbox_new(FALSE,0);
	mountcb=gtk_check_button_new_with_label(
		"Don't descend to mounted filesystems");
	gtk_signal_connect(GTK_OBJECT(mountcb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.mount));
	gtk_box_pack_start(GTK_BOX(boxh),mountcb,TRUE,TRUE,5);
	gtk_widget_show(mountcb);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);


	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Extra \"find\" options:");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	extraoptse = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(extraoptse), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.extraopts));
	gtk_box_pack_start(GTK_BOX(boxh),extraoptse,TRUE,TRUE,5);
	gtk_widget_show(extraoptse);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	/*add this stuff to a page advanced find stuff*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookfind),boxv,
		gtk_label_new("Advanced"));
	gtk_widget_show(boxv);

	
	/* add the subnotebook to the main notebook*/
	boxv=gtk_vbox_new(FALSE,0);
	gtk_container_border_width(GTK_CONTAINER(boxv), 5);
	gtk_box_pack_start(GTK_BOX(boxv),nbookfind,FALSE,FALSE,0);
	gtk_widget_show(nbookfind);
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),boxv,
		gtk_label_new("Files to Find"));
	gtk_widget_show(boxv);

	/*page to search for text in files*/
	boxv=gtk_vbox_new(FALSE,0);

	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Search pattern:");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	searchtexte = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(searchtexte), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.searchtext));
	gtk_box_pack_start(GTK_BOX(boxh),searchtexte,TRUE,TRUE,5);
	gtk_widget_show(searchtexte);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	boxh=gtk_hbox_new(FALSE,0);
	searchgreprb=gtk_radio_button_new_with_label(NULL,
		"Regular expression\n(grep command)");
	gtk_signal_connect(GTK_OBJECT(searchgreprb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.searchgrep));
	gtk_box_pack_start(GTK_BOX(boxh),searchgreprb,TRUE,TRUE,5);
	gtk_widget_show(searchgreprb);
	searchfgreprb=gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(searchgreprb)),
		"Fixed string\n(fgrep command)");
	gtk_signal_connect(GTK_OBJECT(searchfgreprb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.searchfgrep));
	gtk_box_pack_start(GTK_BOX(boxh),searchfgreprb,TRUE,TRUE,5);
	gtk_widget_show(searchfgreprb);
	searchegreprb=gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(searchgreprb)),
		"Extended regular expression\n(egrep command)");
	gtk_signal_connect(GTK_OBJECT(searchegreprb), "toggled",
		   GTK_SIGNAL_FUNC(changetb),(void *)(&findinfo.searchegrep));
	gtk_box_pack_start(GTK_BOX(boxh),searchegreprb,TRUE,TRUE,5);
	gtk_widget_show(searchegreprb);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);

	/*add this stuff to a page for text searches*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookcontent),boxv,
		gtk_label_new("Text\nSearches"));
	gtk_widget_show(boxv);

	/*content (type) page*/
	boxv=gtk_vbox_new(FALSE,0);
	/*add this stuff to a page for content*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookcontent),boxv,
		gtk_label_new("Content\n(File Type)"));
	gtk_widget_show(boxv);

	/*size page*/
	boxv=gtk_vbox_new(FALSE,0);
	/*add this stuff to a page for content*/
	gtk_notebook_append_page(GTK_NOTEBOOK(nbookcontent),boxv,
		gtk_label_new("Size"));
	gtk_widget_show(boxv);

	/* add the subnotebook to the main notebook*/
	boxv=gtk_vbox_new(FALSE,0);
	gtk_container_border_width(GTK_CONTAINER(boxv), 5);
	gtk_box_pack_start(GTK_BOX(boxv),nbookcontent,FALSE,FALSE,0);
	gtk_widget_show(nbookcontent);
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),boxv,
		gtk_label_new("Search For Content"));
	gtk_widget_show(boxv);

	/*page for applying commands to found files*/
	boxv=gtk_vbox_new(FALSE,0);

	boxh=gtk_hbox_new(FALSE,0);
	label = gtk_label_new("Command to apply (be very careful here):");
	gtk_box_pack_start(GTK_BOX(boxh),label,FALSE,FALSE,5);
	gtk_widget_show(label);
	applycmde = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(applycmde), "changed",
		   GTK_SIGNAL_FUNC(changeentry),(void *)(findinfo.applycmd));
	gtk_box_pack_start(GTK_BOX(boxh),applycmde,TRUE,TRUE,5);
	gtk_widget_show(applycmde);
	gtk_box_pack_start(GTK_BOX(boxv),boxh,FALSE,FALSE,5);
	gtk_widget_show(boxh);


	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),boxv,
		gtk_label_new("Apply Command"));
	gtk_widget_show(boxv);

	/*page for setting options*/
	boxv=gtk_vbox_new(FALSE,0);
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),boxv,
		gtk_label_new("Options"));
	gtk_widget_show(boxv);


	/*put the main notebook on the main window*/
	gtk_box_pack_start(GTK_BOX(boxm),nbook,TRUE,TRUE,0);
	gtk_widget_show(nbook);


	/*action area*/
	boxh=gtk_hbox_new(FALSE,10);

	searchpb = gtk_button_new_with_label("GO!");
	gtk_signal_connect(GTK_OBJECT(searchpb), "clicked",
			   GTK_SIGNAL_FUNC(search), (void *)(&findinfo));
	gtk_box_pack_start(GTK_BOX(boxh),searchpb,FALSE,FALSE,0);
	gtk_widget_show(searchpb);

	quitpb = gtk_button_new_with_label("Quit");
	gtk_signal_connect_object(GTK_OBJECT(quitpb), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
	gtk_box_pack_end(GTK_BOX(boxh),quitpb,FALSE,FALSE,0);
	gtk_widget_show(quitpb);
	gtk_box_pack_start(GTK_BOX(boxm),boxh,TRUE,TRUE,0);
	gtk_widget_show(boxh);
	
	gtk_container_add(GTK_CONTAINER(window), boxm);
	gtk_widget_show(boxm);

	gtk_widget_show(window);

	gtk_main();

	return 0;
}

GtkWidget *
make_list_of_templates(void)
{
	
}

/*
int
main(int argc, char *argv[])
{
	GtkWidget *toplevel;
	GtkWidget *w;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),"GNOME Search Tool");
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
int

*/
