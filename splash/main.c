#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "splash.h"
/*#include "splashbits.h" - someone draw an icon */

/*
 *	Splash - a simple gnomefriendly installation helper
 */

static int child_pid;
char *prog_name;

static GtkWidget *frame;
static GtkWidget *pixmap;
static GdkPixmap *pixmap_data;
static GdkBitmap *mask;
static GtkWidget *picholder;
static char *title;

static void splash_init(char *file)
{
	int x,y;
	int px,py;
	GtkStyle *style = gtk_widget_get_default_style();
	
	frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (frame), title);
	gtk_container_border_width (GTK_CONTAINER (frame), 0);
	gtk_widget_realize (frame);
	gdk_window_set_override_redirect(frame->window, 1);

	gtk_signal_connect (GTK_OBJECT (frame), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			&frame);

#ifdef SOMEONE_DREW_AN_ICON
	splashbits = gdk_bitmap_create_from_data (frame->window,
						splashbits_bits,
						splashbits_width,
						splashbits_height);
	gdk_window_set_icon (frame->window, NULL,
				splashbits, splashbits);
#endif
	gdk_window_set_icon_name (frame->window, title);
  
	gdk_window_set_decorations (frame->window, 0);
	gdk_window_set_functions (frame->window, 0);
	
	pixmap_data = gdk_pixmap_create_from_xpm (
			frame->window, &mask,
			&style->bg[GTK_STATE_NORMAL],
                        file);
	if(pixmap_data == NULL)
	{
		fprintf(stderr,"%s: unable to load '%s'.\n", prog_name, file);
		exit(1);
	}
	
	pixmap = gtk_pixmap_new(pixmap_data, mask);
	
	gtk_container_add(GTK_CONTAINER(frame), pixmap);
	gtk_widget_shape_combine_mask(frame, mask, 0, 0);
	
	/*
	 *	Placement
	 */
	
	gdk_window_get_size(pixmap_data, &px, &py);
	
	x= (gdk_screen_width()-px)/2;
	y= (gdk_screen_height()-px)/2;
	 
	gtk_widget_set_uposition(frame, x, y);
	gtk_widget_show(pixmap);
	gtk_widget_show(frame);
}	

static void splash_message(gpointer data, gint fd, GdkInputCondition fred)
{
	struct splash_message sm;
	
	if((fred&GDK_INPUT_READ)==0)
		return;

	if(read(fd, &sm, sizeof(sm))!=sizeof(sm))
	{
		gdk_input_remove(fd);
		gtk_main_quit();
		return;
	}

	switch(sm.type)
	{
		case SM_TYPE_QUIT:
			gdk_input_remove(fd);
			gtk_main_quit();
			return;
		case SM_TYPE_TITLE:
			free(title);
			title=strdup(sm.data);
			gtk_window_set_title (GTK_WINDOW (frame), title);
			gdk_window_set_icon_name (frame->window, title);
			return;
		case SM_TYPE_IMAGE:
			gtk_widget_destroy(pixmap);
			gtk_widget_destroy(frame);
			splash_init(sm.data);
			return;
	}
}			
			
void main(int argc, char *argv[])
{
	int pipeline[2];
	
	prog_name=argv[0];
	
	title=strdup(prog_name);
	
	if(argc<3)
	{
		fprintf(stderr, "%s: initialscreen command arguments...\n",
			argv[0]);
		exit(1);
	}
	
	if(pipe(pipeline)==-1)
	{
		perror(argv[0]);
		exit(2);
	}
	
	gtk_init(&argc, &argv);
	
	splash_init(argv[1]);
	
	switch(child_pid=fork())
	{
		default:
			close(pipeline[1]);
			gdk_input_add(pipeline[0],
				GDK_INPUT_READ,
				splash_message,
				NULL);
				
			gtk_main();
			exit(0);
		case -1:
			perror(argv[0]);
			exit(2);
		case 0:
			close(pipeline[0]);
			dup2(pipeline[1], 3);
			execvp(argv[2], argv+2);
			perror(argv[2]);
			exit(1);
	}
}
