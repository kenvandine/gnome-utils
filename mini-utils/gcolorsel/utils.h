#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>

void pixel_to_rgb (GdkColormap *cmap, guint32 pixel, 
		   gint *red, gint *green, gint *blue);

GtkWidget *drag_window_create (gint red, gint green, gint blue);

#endif

