#ifndef _UTILS_H_
#define _UTILS_H_

#include <gtk/gtk.h>

void pixel_to_rgb (GdkColormap *cmap, guint32 pixel, 
		   gint *red, gint *green, gint *blue);

GtkWidget *scrolled_window_get_child (GtkScrolledWindow *sw);

GtkWidget *drag_window_create (gint red, gint green, gint blue);

#endif

