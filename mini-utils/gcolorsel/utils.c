#include "utils.h"

#include "gtk/gtk.h"

/* From gdk-pixbuf, gdk-pixbuf-drawable.c */       
void pixel_to_rgb (GdkColormap *cmap, guint32 pixel, 
		   gint *red, gint *green, gint *blue)
{
  GdkVisual *v;

  g_assert (cmap != NULL);
  g_assert (red != NULL);
  g_assert (green != NULL);
  g_assert (blue != NULL);

  v = gdk_colormap_get_visual (cmap);

  switch (v->type) {
    case GDK_VISUAL_STATIC_GRAY:
    case GDK_VISUAL_GRAYSCALE:
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_PSEUDO_COLOR:
      *red   = cmap->colors[pixel].red;
      *green = cmap->colors[pixel].green;
      *blue  = cmap->colors[pixel].blue;
      break;
    case GDK_VISUAL_TRUE_COLOR:
      *red   = ((pixel & v->red_mask)   << (32 - v->red_shift   - v->red_prec))   >> 24;
      *green = ((pixel & v->green_mask) << (32 - v->green_shift - v->green_prec)) >> 24;
      *blue  = ((pixel & v->blue_mask)  << (32 - v->blue_shift  - v->blue_prec))  >> 24;
                 
      break;
    case GDK_VISUAL_DIRECT_COLOR:
      *red   = cmap->colors[((pixel & v->red_mask)   << (32 - v->red_shift   - v->red_prec))   >> 24].red;
      *green = cmap->colors[((pixel & v->green_mask) << (32 - v->green_shift - v->green_prec)) >> 24].green; 
      *blue  = cmap->colors[((pixel & v->blue_mask)  << (32 - v->blue_shift  - v->blue_prec))  >> 24].blue;
      break;
  default:
    g_assert_not_reached ();
  }  
}

/* From gtk+, gtkcolorsel.c */
GtkWidget *
drag_window_create (gint red, gint green, gint blue)
{
  GtkWidget *window;
  GdkColor bg;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  gtk_widget_set_usize (window, 48, 32);
  gtk_widget_realize (window);

  bg.red = (red / 255.0) * 0xffff;
  bg.green = (green / 255.0) * 0xffff;
  bg.blue = (blue / 255.0) * 0xffff;

  gdk_color_alloc (gtk_widget_get_colormap (window), &bg);
  gdk_window_set_background (window->window, &bg);

  return window;
}

