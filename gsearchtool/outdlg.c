/* Gnome Search Tool 
 * (C) 1998 the Free Software Foundation
 *
 * Author:   George Lebl
 *
 */

#include <gtk/gtk.h>
#include <string.h>

#include "gsearchtool.h"
#include "outdlg.h"

static GtkWidget *outdlg=NULL;
static GtkWidget *outlist;
static int list_width=0;

static void
outdlg_clicked(GtkWidget * widget, int button, gpointer data)
{
	if(button == 0)
		gtk_clist_clear(GTK_CLIST(outlist));
	else
		gtk_widget_destroy(outdlg);
	list_width=0;
}

gint 
outdlg_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data) {
	GtkCList *clist=(GtkCList *)widget;
	gint row, col, argsize;
	gchar *fileName;
	const gchar *program;
	const gchar *mimeType;
	gchar *shellcommand;
	gchar *tempstring;
	gchar **args;

	if (event->type==GDK_2BUTTON_PRESS) {
		if (!gtk_clist_get_selection_info(clist, event->x, event->y, &row, &col))
			return FALSE;
		
		gtk_clist_get_text(clist, row, col, &fileName);
		mimeType=gnome_mime_type_of_file(fileName);
		program=gnome_mime_program(mimeType);
		
		if (program) {
			args=g_strsplit(program, " ", 255);
			for (argsize=0; args[argsize] !=NULL; argsize++) {
				if (!strcmp(args[argsize], "%f")) {
						g_free(args[argsize]);
						args[argsize]=g_strdup(fileName);
				}
			}

			if (gnome_mime_needsterminal(mimeType, NULL)) {
				tempstring=g_strjoinv(" ", args);
				shellcommand=g_strconcat("gnome-terminal#--command=", tempstring, "", NULL);
				g_print("Test: %s\n",shellcommand);
				g_strfreev(args);
				args=g_strsplit(shellcommand, "#", 2);
				argsize=2;
				g_free(tempstring);
				g_free(shellcommand);
			} 

			gnome_execute_async(NULL, argsize, args);
			g_strfreev(args);
		}
		return TRUE;
	}
	 return FALSE;
}

static int
outdlg_closedlg(GtkWidget * widget, gpointer data)
{
	list_width=0;
	outdlg=NULL;
	return FALSE;
}

int
outdlg_makedlg(char name[], int clear)
{
	GtkWidget *w;

	/*we already have a dialog!!!*/
	if(outdlg!=NULL) {
		if(clear) {
			list_width=0;
			gtk_clist_clear(GTK_CLIST(outlist));
		}
		return FALSE;
	}
	list_width=0;

	outdlg=gnome_dialog_new(name,
				"Clear",
				GNOME_STOCK_BUTTON_CLOSE,
				NULL);
	gtk_signal_connect(GTK_OBJECT(outdlg), "destroy",
			   GTK_SIGNAL_FUNC(outdlg_closedlg), NULL);

	gtk_signal_connect(GTK_OBJECT(outdlg), "clicked",
			   GTK_SIGNAL_FUNC(outdlg_clicked), NULL);

	w = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_usize(w,200,350);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(outdlg)->vbox),w,TRUE,TRUE,0);
	outlist = gtk_clist_new(1);
	gtk_signal_connect(GTK_OBJECT(outlist), "button_press_event",
				GTK_SIGNAL_FUNC(outdlg_double_click),NULL);
	gtk_container_add(GTK_CONTAINER(w),outlist);

	return TRUE;
}

void
outdlg_additem(char item[])
{
	gtk_clist_append(GTK_CLIST(outlist),&item);
	if(list_width < 
	   gdk_string_width(GTK_WIDGET(outlist)->style->font,item))
		list_width = gdk_string_width(GTK_WIDGET(outlist)->style->font,
					      item);
	gtk_clist_set_column_width(GTK_CLIST(outlist),0,list_width);
}

void
outdlg_freeze(void)
{
	gtk_clist_freeze(GTK_CLIST(outlist));
}

void
outdlg_thaw(void)
{
	gtk_clist_thaw(GTK_CLIST(outlist));
}

void
outdlg_showdlg(void)
{
	gtk_widget_show_all(outdlg);
}

