#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <gnome.h>

static GtkWidget *penguindow;
static GtkWidget *pixmap;
static char *title;
static char *prog_name;

static int penguin_x, penguin_y;

static void produce_penguin(void)
{
	int px,py;
	gchar *filename;
	gchar *pmapdir;

	title = "gpenguin";
	
	penguindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (penguindow), title);
	gtk_container_border_width (GTK_CONTAINER (penguindow), 0);
	gtk_widget_realize (penguindow);
	gdk_window_set_override_redirect(penguindow->window, 1);

	gtk_signal_connect (GTK_OBJECT (penguindow), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			&penguindow);

#ifdef SOMEONE_DREW_AN_ICON
/*plashbits = gdk_bitmap_create_from_data (penguindow->window,
						splashbits_bits,
						splashbits_width,
						splashbits_height);
	gdk_window_set_icon (penguindow->window, NULL,
				splashbits, splashbits);
*/
#endif
	gdk_window_set_icon_name (penguindow->window, title);
  
	gdk_window_set_decorations (penguindow->window, 0);
	gdk_window_set_functions (penguindow->window, 0);
	
	pmapdir = gnome_unconditional_datadir_file("pixmaps");
	filename = g_strconcat(pmapdir, "/penguin1.png", NULL);

	pixmap = NULL;
	pixmap = gnome_pixmap_new_from_file (filename);
	g_free(filename);
	if(pixmap == NULL)
	{
		fprintf(stderr,"%s: unable to load the penguin.\n", prog_name);
		exit(1);
	}
	
	gtk_container_add(GTK_CONTAINER(penguindow), pixmap);
	gtk_widget_shape_combine_mask(penguindow, GNOME_PIXMAP(pixmap)->mask, 0, 0);
	
	/*
	 *	Placement
	 */
	
	gdk_window_get_size(GNOME_PIXMAP(pixmap)->pixmap, &px, &py);
	
	penguin_x= (gdk_screen_width()-px)/2;
	penguin_y= (gdk_screen_height()-px)/2;
	 
	gtk_widget_set_uposition(penguindow, penguin_x, penguin_y);
	gtk_widget_show(pixmap);
	gtk_widget_show(penguindow);
}	

void penguin_mover(void)
{
	int xp, yp;
	gdk_window_get_pointer(penguindow->window, &xp, &yp, NULL);
	if(xp<-10)
		penguin_x-=4;
	if(xp>10)
		penguin_x+=4;
	if(yp<-10)
		penguin_y-=4;
	if(yp>10)
		penguin_y+=4;
	gtk_widget_set_uposition(penguindow, penguin_x, penguin_y);
	gdk_window_raise(penguindow->window);
//	gtk_timeout_add(200, (void *)penguin_mover, NULL);
}

void set_ticking(void)
{
	gtk_timeout_add(100, (void *)penguin_mover, NULL);
}

int main(int argc, char *argv[])
{
	prog_name=argv[0];	
	gtk_init(&argc, &argv);
	gdk_imlib_init();
	produce_penguin();
	set_ticking();
	gtk_main();

	return 0;
}
	
