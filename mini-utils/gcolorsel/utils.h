#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>

void pixel_to_rgb (GdkColormap *cmap, guint32 pixel, 
		   gint *red, gint *green, gint *blue);

GtkWidget *drag_window_create (gint red, gint green, gint blue);

/* Set properties without emiting a signal */
void spin_set_value (GtkSpinButton *spin, float val, gpointer data);
void entry_set_text (GtkEntry *entry, char *str, gpointer data);

void spin_connect_value_changed (GtkSpinButton *spin, GtkSignalFunc cb, 
				 gpointer data);

void preview_fill (GtkWidget *preview, int r, int g, int b);


void set_config_key_pos (int pos);
int  get_config_key_pos (void);
int  get_config_key (void);

#endif

