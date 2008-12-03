/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * GNOME Search Tool
 *
 *  File:  gsearchtool-spinner.c
 *
 *  Copyright (C) 2006 Dennis Cranston
 *  Copyright (C) 2002 Marco Pesenti Gritti
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author: Andy Hertzfeld <andy@eazel.com>
 *
 *  Ephy port by Marco Pesenti Gritti <marco@it.gnome.org>
 *  Gsearchtool port by Dennis Cranston <dennis_cranston@yahoo.com>
 *
 */

#include "gsearchtool-spinner.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

/* Spinner cache implementation */

#define GSEARCH_TYPE_SPINNER_CACHE		(gsearch_spinner_cache_get_type())
#define GSEARCH_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), GSEARCH_TYPE_SPINNER_CACHE, GSearchSpinnerCache))
#define GSEARCH_SPINNER_CACHE_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST((klass), GSEARCH_TYPE_SPINNER_CACHE, GSearchSpinnerCacheClass))
#define GSEARCH_IS_SPINNER_CACHE(object)	(G_TYPE_CHECK_INSTANCE_TYPE((object), GSEARCH_TYPE_SPINNER_CACHE))
#define GSEARCH_IS_SPINNER_CACHE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GSEARCH_TYPE_SPINNER_CACHE))
#define GSEARCH_SPINNER_CACHE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), GSEARCH_TYPE_SPINNER_CACHE, GSearchSpinnerCacheClass))

typedef struct _GSearchSpinnerCache		GSearchSpinnerCache;
typedef struct _GSearchSpinnerCacheClass	GSearchSpinnerCacheClass;
typedef struct _GSearchSpinnerCachePrivate	GSearchSpinnerCachePrivate;

struct _GSearchSpinnerCacheClass
{
	GObjectClass parent_class;
};

struct _GSearchSpinnerCache
{
	GObject parent_object;

	/*< private >*/
	GSearchSpinnerCachePrivate *priv;
};

#define GSEARCH_SPINNER_CACHE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSEARCH_TYPE_SPINNER_CACHE, GSearchSpinnerCachePrivate))

typedef struct
{
	GtkIconSize size;
	int width;
	int height;
	GdkPixbuf *quiescent_pixbuf;
	GList *images;
} GSearchSpinnerImages;

typedef struct
{
	GdkScreen *screen;
	GtkIconTheme *icon_theme;
	GSearchSpinnerImages *originals;
	/* List of GSearchSpinnerImages scaled to different sizes */
	GList *images;
} GSearchSpinnerCacheData;

struct _GSearchSpinnerCachePrivate
{
	/* Hash table of GdkScreen -> GSearchSpinnerCacheData */
	GHashTable *hash;
};

static void gsearch_spinner_cache_class_init (GSearchSpinnerCacheClass *klass);
static void gsearch_spinner_cache_init	     (GSearchSpinnerCache *cache);

static GObjectClass *cache_parent_class = NULL;

static GType
gsearch_spinner_cache_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo our_info =
		{
			sizeof (GSearchSpinnerCacheClass),
			NULL,
			NULL,
			(GClassInitFunc) gsearch_spinner_cache_class_init,
			NULL,
			NULL,
			sizeof (GSearchSpinnerCache),
			0,
			(GInstanceInitFunc) gsearch_spinner_cache_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GSearchSpinnerCache",
					       &our_info, 0);
	}

	return type;
}

static void
gsearch_spinner_images_free (GSearchSpinnerImages *images)
{
	if (images != NULL)
	{
		g_list_foreach (images->images, (GFunc) g_object_unref, NULL);
		g_object_unref (images->quiescent_pixbuf);

		g_free (images);
	}
}

static GSearchSpinnerImages *
gsearch_spinner_images_copy (GSearchSpinnerImages *images)
{
	GSearchSpinnerImages *copy = g_new (GSearchSpinnerImages, 1);

	copy->size = images->size;
	copy->width = images->width;
	copy->height = images->height;

	copy->quiescent_pixbuf = g_object_ref (images->quiescent_pixbuf);
	copy->images = g_list_copy (images->images);
	g_list_foreach (copy->images, (GFunc) g_object_ref, NULL);

	return copy;
}

static void
gsearch_spinner_cache_data_unload (GSearchSpinnerCacheData *data)
{
	g_return_if_fail (data != NULL);

	g_list_foreach (data->images, (GFunc) gsearch_spinner_images_free, NULL);
	data->images = NULL;
	data->originals = NULL;
}

static GdkPixbuf *
extract_frame (GdkPixbuf *grid_pixbuf,
	       int x,
	       int y,
	       int size)
{
	GdkPixbuf *pixbuf;

	if (x + size > gdk_pixbuf_get_width (grid_pixbuf) ||
	    y + size > gdk_pixbuf_get_height (grid_pixbuf))
	{
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_subpixbuf (grid_pixbuf,
					   x, y,
					   size, size);
	g_return_val_if_fail (pixbuf != NULL, NULL);

	return pixbuf;
}

static void
gsearch_spinner_cache_data_load (GSearchSpinnerCacheData *data)
{
	GSearchSpinnerImages *images;
	GdkPixbuf *icon_pixbuf, *pixbuf;
	GtkIconInfo *icon_info;
	int grid_width, grid_height, x, y, size, h, w;
	const char *icon;

	g_return_if_fail (data != NULL);

	gsearch_spinner_cache_data_unload (data);

	/* Load the animation */
	icon_info = gtk_icon_theme_lookup_icon (data->icon_theme,
						"gnome-searchtool-animation", -1, 0);
	if (icon_info == NULL)
	{
		g_warning ("Throbber animation not found\n");
		return;
	}

	size = gtk_icon_info_get_base_size (icon_info);
	icon = gtk_icon_info_get_filename (icon_info);
	g_return_if_fail (icon != NULL);

	icon_pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
	if (icon_pixbuf == NULL)
	{
		g_warning ("Could not load the spinner file\n");
		gtk_icon_info_free (icon_info);
		return;
	}

	grid_width = gdk_pixbuf_get_width (icon_pixbuf);
	grid_height = gdk_pixbuf_get_height (icon_pixbuf);

	images = g_new (GSearchSpinnerImages, 1);
	data->images = g_list_prepend (NULL, images);
	data->originals = images;

	images->size = GTK_ICON_SIZE_INVALID;
	images->width = images->height = size;
	images->images = NULL;
	images->quiescent_pixbuf = NULL;

	for (y = 0; y < grid_height; y += size)
	{
		for (x = 0; x < grid_width ; x += size)
		{
			pixbuf = extract_frame (icon_pixbuf, x, y, size);

			if (pixbuf)
			{
				images->images =
					g_list_prepend (images->images, pixbuf);
			}
			else
			{
				g_warning ("Cannot extract frame from the grid\n");
			}
		}
	}
	images->images = g_list_reverse (images->images);

	gtk_icon_info_free (icon_info);
	g_object_unref (icon_pixbuf);

	/* Load the rest icon */
	icon_info = gtk_icon_theme_lookup_icon (data->icon_theme,
						"gnome-searchtool-animation-rest", -1, 0);
	if (icon_info == NULL)
	{
		g_warning ("Throbber rest icon not found\n");
		return;
	}

	size = gtk_icon_info_get_base_size (icon_info);
	icon = gtk_icon_info_get_filename (icon_info);
	g_return_if_fail (icon != NULL);

	icon_pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
	gtk_icon_info_free (icon_info);

	if (icon_pixbuf == NULL)
	{
		g_warning ("Could not load spinner rest icon\n");
		gsearch_spinner_images_free (images);
		return;
	}

	images->quiescent_pixbuf = icon_pixbuf;

	w = gdk_pixbuf_get_width (icon_pixbuf);
	h = gdk_pixbuf_get_height (icon_pixbuf);
	images->width = MAX (images->width, w);
	images->height = MAX (images->height, h);
}

static GSearchSpinnerCacheData *
gsearch_spinner_cache_data_new (GdkScreen *screen)
{
	GSearchSpinnerCacheData *data;

	data = g_new0 (GSearchSpinnerCacheData, 1);

	data->screen = screen;
	data->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect_swapped (data->icon_theme, "changed",
				  G_CALLBACK (gsearch_spinner_cache_data_load),
				  data);

	gsearch_spinner_cache_data_load (data);

	return data;
}

static void
gsearch_spinner_cache_data_free (GSearchSpinnerCacheData *data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (data->icon_theme != NULL);

	g_signal_handlers_disconnect_by_func
		(data->icon_theme,
		 G_CALLBACK (gsearch_spinner_cache_data_load), data);

	gsearch_spinner_cache_data_unload (data);

	g_free (data);
}

static int
compare_size (gconstpointer images_ptr,
	      gconstpointer size_ptr)
{
	const GSearchSpinnerImages *images = (const GSearchSpinnerImages *) images_ptr;
	GtkIconSize size = GPOINTER_TO_INT (size_ptr);

	if (images->size == size)
	{
		return 0;
	}

	return -1;
}

static GdkPixbuf *
scale_to_size (GdkPixbuf *pixbuf,
	       int dw,
	       int dh)
{
	GdkPixbuf *result;
	int pw, ph;

	pw = gdk_pixbuf_get_width (pixbuf);
	ph = gdk_pixbuf_get_height (pixbuf);

	if (pw != dw || ph != dh)
	{
		result = gdk_pixbuf_scale_simple (pixbuf, dw, dh,
						  GDK_INTERP_BILINEAR);
	}
	else
	{
		result = g_object_ref (pixbuf);
	}

	return result;
}

static GSearchSpinnerImages *
gsearch_spinner_cache_get_images (GSearchSpinnerCache *cache,
                                  GdkScreen *screen,
                                  GtkIconSize size)
{
	GSearchSpinnerCachePrivate *priv = cache->priv;
	GSearchSpinnerCacheData *data;
	GSearchSpinnerImages *images;
	GtkSettings *settings;
	GdkPixbuf *pixbuf, *scaled_pixbuf;
	GList *element, *l;
	int h, w;

	data = g_hash_table_lookup (priv->hash, screen);
	if (data == NULL)
	{
		data = gsearch_spinner_cache_data_new (screen);
		g_hash_table_insert (priv->hash, screen, data);
	}

	if (data->images == NULL || data->originals == NULL)
	{
		/* Load failed, but don't try endlessly again! */
		return NULL;
	}

	element = g_list_find_custom (data->images,
				      GINT_TO_POINTER (size),
				      (GCompareFunc) compare_size);
	if (element != NULL)
	{
		return gsearch_spinner_images_copy ((GSearchSpinnerImages *) element->data);
	}

	settings = gtk_settings_get_for_screen (screen);
	if (!gtk_icon_size_lookup_for_settings (settings, size, &w, &h))
	{
		g_warning ("Failed to look up icon size\n");
		return NULL;
	}

	images = g_new (GSearchSpinnerImages, 1);
	images->size = size;
	images->width = w;
	images->height = h;
	images->images = NULL;

	for (l = data->originals->images; l != NULL; l = l->next)
	{
		pixbuf = (GdkPixbuf *) l->data;
		scaled_pixbuf = scale_to_size (pixbuf, w, h);

		images->images = g_list_prepend (images->images, scaled_pixbuf);
	}
	images->images = g_list_reverse (images->images);

	images->quiescent_pixbuf =
		scale_to_size (data->originals->quiescent_pixbuf, w, h);

	/* store in cache */
	data->images = g_list_prepend (data->images, images);

	return gsearch_spinner_images_copy (images);
}

static void
gsearch_spinner_cache_init (GSearchSpinnerCache *cache)
{
	GSearchSpinnerCachePrivate *priv;

	priv = cache->priv = GSEARCH_SPINNER_CACHE_GET_PRIVATE (cache);

	priv->hash = g_hash_table_new_full (g_direct_hash, g_direct_equal,
					    NULL,
					    (GDestroyNotify) gsearch_spinner_cache_data_free);
}

static void
gsearch_spinner_cache_finalize (GObject *object)
{
	GSearchSpinnerCache *cache = GSEARCH_SPINNER_CACHE (object); 
	GSearchSpinnerCachePrivate *priv = cache->priv;

	g_hash_table_destroy (priv->hash);

	G_OBJECT_CLASS (cache_parent_class)->finalize (object);
}

static void
gsearch_spinner_cache_class_init (GSearchSpinnerCacheClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	cache_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gsearch_spinner_cache_finalize;

	g_type_class_add_private (object_class, sizeof (GSearchSpinnerCachePrivate));
}

static GSearchSpinnerCache *spinner_cache = NULL;

static GSearchSpinnerCache *
gsearch_spinner_cache_ref (void)
{
	if (spinner_cache == NULL)
	{
		GSearchSpinnerCache **cache_ptr;

		spinner_cache = g_object_new (GSEARCH_TYPE_SPINNER_CACHE, NULL);
		cache_ptr = &spinner_cache;
		g_object_add_weak_pointer (G_OBJECT (spinner_cache),
					   (gpointer *) cache_ptr);

		return spinner_cache;
	}
	else
	{
		return g_object_ref (spinner_cache);
	}
}

/* Spinner implementation */

#define SPINNER_TIMEOUT 175 /* ms */

#define GSEARCH_SPINNER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSEARCH_TYPE_SPINNER, GSearchSpinnerDetails))

struct _GSearchSpinnerDetails
{
	GtkIconTheme *icon_theme;
	GSearchSpinnerCache *cache;
	GtkIconSize size;
	GSearchSpinnerImages *images;
	GList *current_image;
	guint timeout;
	guint timer_task;
	guint spinning : 1;
};

static void gsearch_spinner_class_init (GSearchSpinnerClass *class);
static void gsearch_spinner_init	    (GSearchSpinner *spinner);

static GObjectClass *parent_class = NULL;

GType
gsearch_spinner_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo our_info =
		{
			sizeof (GSearchSpinnerClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) gsearch_spinner_class_init,
			NULL,
			NULL, /* class_data */
			sizeof (GSearchSpinner),
			0, /* n_preallocs */
			(GInstanceInitFunc) gsearch_spinner_init
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX,
					       "GSearchSpinner",
					       &our_info, 0);
	}

	return type;
}

static gboolean
gsearch_spinner_load_images (GSearchSpinner *spinner)
{
	GSearchSpinnerDetails *details = spinner->details;

	if (details->images == NULL)
	{
		details->images =
			gsearch_spinner_cache_get_images
				(details->cache,
				 gtk_widget_get_screen (GTK_WIDGET (spinner)),
				 details->size);

		if (details->images != NULL)
		{
			details->current_image = details->images->images;
		}

	}

	return details->images != NULL;
}

static void
gsearch_spinner_unload_images (GSearchSpinner *spinner)
{
	gsearch_spinner_images_free (spinner->details->images);
	spinner->details->images = NULL;
	spinner->details->current_image = NULL;
}

static void
icon_theme_changed_cb (GtkIconTheme *icon_theme,
		       GSearchSpinner *spinner)
{
	gsearch_spinner_unload_images (spinner);
	gtk_widget_queue_resize (GTK_WIDGET (spinner));
}

static void
gsearch_spinner_init (GSearchSpinner *spinner)
{
	GSearchSpinnerDetails *details = spinner->details;
	GtkWidget *widget = GTK_WIDGET (spinner);

	gtk_widget_set_events (widget,
			       gtk_widget_get_events (widget)
			       | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

	details = spinner->details = GSEARCH_SPINNER_GET_PRIVATE (spinner);

	details->cache = gsearch_spinner_cache_ref ();
	details->size = GTK_ICON_SIZE_INVALID;
	details->spinning = FALSE;
	details->timeout = SPINNER_TIMEOUT;
}

static GdkPixbuf *
select_spinner_image (GSearchSpinner *spinner)
{
	GSearchSpinnerDetails *details = spinner->details;
	GSearchSpinnerImages *images = details->images;

	g_return_val_if_fail (images != NULL, NULL);

	if (spinner->details->timer_task == 0)
	{
		if (images->quiescent_pixbuf != NULL)
		{
			return g_object_ref (details->images->quiescent_pixbuf);
		}

		return NULL;
	}

	g_return_val_if_fail (details->current_image != NULL, NULL);

	return g_object_ref (details->current_image->data);
}

static int
gsearch_spinner_expose (GtkWidget *widget,
                        GdkEventExpose *event)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (widget);
	GdkPixbuf *pixbuf;
	GdkGC *gc;
	int x_offset, y_offset, width, height;
	GdkRectangle pix_area, dest;

	if (!GTK_WIDGET_DRAWABLE (spinner))
	{
		return TRUE;
	}

	if (!gsearch_spinner_load_images (spinner))
	{
		return TRUE;
	}

	pixbuf = select_spinner_image (spinner);
	if (pixbuf == NULL)
	{
		return FALSE;
	}

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	/* Compute the offsets for the image centered on our allocation */
	x_offset = (widget->allocation.width - width) / 2;
	y_offset = (widget->allocation.height - height) / 2;

	pix_area.x = x_offset + widget->allocation.x;
	pix_area.y = y_offset + widget->allocation.y;
	pix_area.width = width;
	pix_area.height = height;

	if (!gdk_rectangle_intersect (&event->area, &pix_area, &dest))
	{
		g_object_unref (pixbuf);
		return FALSE;
	}

	gc = gdk_gc_new (widget->window);
	gdk_draw_pixbuf (widget->window, gc, pixbuf,
			 dest.x - x_offset - widget->allocation.x,
			 dest.y - y_offset - widget->allocation.y,
			 dest.x, dest.y,
			 dest.width, dest.height,
			 GDK_RGB_DITHER_MAX, 0, 0);
	g_object_unref (gc);

	g_object_unref (pixbuf);

	return FALSE;
}

static gboolean
bump_spinner_frame_cb (GSearchSpinner *spinner)
{
	GSearchSpinnerDetails *details = spinner->details;
	GList *frame;

	if (!GTK_WIDGET_DRAWABLE (spinner)) return TRUE;

	/* This can happen when we've unloaded the images on a theme
	 * change, but haven't been in the queued size request yet.
	 * Just skip this update.
	 */
	if (details->images == NULL) return TRUE;

	frame = details->current_image;
	g_assert (frame != NULL);

	if (frame->next != NULL)
	{
		frame = frame->next;
	}
	else
	{
		frame = g_list_first (frame);
	}

	details->current_image = frame;

	gtk_widget_queue_draw (GTK_WIDGET (spinner));

	/* run again */
	return TRUE;
}

/**
 * gsearch_spinner_start:
 * @spinner: a #GSearchSpinner
 *
 * Start the spinner animation.
 **/
void
gsearch_spinner_start (GSearchSpinner *spinner)
{
	GSearchSpinnerDetails *details = spinner->details;

	details->spinning = TRUE;

	if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)) &&
	    details->timer_task == 0 &&
	    gsearch_spinner_load_images (spinner))
	{
		if (details->images != NULL)
		{
			/* reset to first frame */
			details->current_image = details->images->images;
		}

		details->timer_task =
			g_timeout_add (details->timeout,
				       (GSourceFunc) bump_spinner_frame_cb,
				       spinner);
	}
}

static void
gsearch_spinner_remove_update_callback (GSearchSpinner *spinner)
{
	if (spinner->details->timer_task != 0)
	{
		g_source_remove (spinner->details->timer_task);
		spinner->details->timer_task = 0;
	}
}

/**
 * gsearch_spinner_stop:
 * @spinner: a #GSearchSpinner
 *
 * Stop the spinner animation.
 **/
void
gsearch_spinner_stop (GSearchSpinner *spinner)
{
	GSearchSpinnerDetails *details = spinner->details;

	details->spinning = FALSE;

	if (spinner->details->timer_task != 0)
	{
		gsearch_spinner_remove_update_callback (spinner);

		if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)))
		{
			gtk_widget_queue_draw (GTK_WIDGET (spinner));
		}
	}
}

/*
 * gsearch_spinner_set_size:
 * @spinner: a #GSearchSpinner
 * @size: the size of type %GtkIconSize
 *
 * Set the size of the spinner. Use %GTK_ICON_SIZE_INVALID to use the
 * native size of the icons.
 **/
void
gsearch_spinner_set_size (GSearchSpinner *spinner,
                          GtkIconSize size)
{
	if (size != spinner->details->size)
	{
		gsearch_spinner_unload_images (spinner);

		spinner->details->size = size;

		gtk_widget_queue_resize (GTK_WIDGET (spinner));
	}
}

#if 0
/*
 * gsearch_spinner_set_timeout:
 * @spinner: a #GSearchSpinner
 * @timeout: time delay between updates to the spinner.
 *
 * Sets the timeout delay for spinner updates.
 **/
void
gsearch_spinner_set_timeout (GSearchSpinner *spinner,
                             guint timeout)
{
	GSearchSpinnerDetails *details = spinner->details;

	if (timeout != details->timeout)
	{
		gsearch_spinner_stop (spinner);

		details->timeout = timeout;

		if (details->spinning)
		{
			gsearch_spinner_start (spinner);
		}
	}
}
#endif

static void
gsearch_spinner_size_request (GtkWidget *widget,
                              GtkRequisition *requisition)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (widget);

	if (!gsearch_spinner_load_images (spinner))
	{
		requisition->width = requisition->height = 0;
		gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
						   spinner->details->size,
						   &requisition->width,
					           &requisition->height);
		return;
	}

	requisition->width = spinner->details->images->width;
	requisition->height = spinner->details->images->height;

	/* allocate some extra margin so we don't butt up against toolbar edges */
	if (spinner->details->size != GTK_ICON_SIZE_MENU)
	{
		requisition->width += 4;
		requisition->height += 4;
	}
}

static void
gsearch_spinner_map (GtkWidget *widget)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (widget);
	GSearchSpinnerDetails *details = spinner->details;

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	if (details->spinning)
	{
		gsearch_spinner_start (spinner);
	}
}

static void
gsearch_spinner_unmap (GtkWidget *widget)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (widget);

	gsearch_spinner_remove_update_callback (spinner);

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gsearch_spinner_dispose (GObject *object)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (object);

	g_signal_handlers_disconnect_by_func
			(spinner->details->icon_theme,
		 G_CALLBACK (icon_theme_changed_cb), spinner);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gsearch_spinner_finalize (GObject *object)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (object);

	gsearch_spinner_remove_update_callback (spinner);
	gsearch_spinner_unload_images (spinner);

	g_object_unref (spinner->details->cache);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gsearch_spinner_screen_changed (GtkWidget *widget,
                                GdkScreen *old_screen)
{
	GSearchSpinner *spinner = GSEARCH_SPINNER (widget);
	GSearchSpinnerDetails *details = spinner->details;
	GdkScreen *screen;

	if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
	{
		GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, old_screen);
	}

	screen = gtk_widget_get_screen (widget);

	/* FIXME: this seems to be happening when then spinner is destroyed!? */
	if (old_screen == screen) return;

	/* We'll get mapped again on the new screen, but not unmapped from
	 * the old screen, so remove timeout here.
	 */
	gsearch_spinner_remove_update_callback (spinner);

	gsearch_spinner_unload_images (spinner);

	if (old_screen != NULL)
	{
		g_signal_handlers_disconnect_by_func
			(gtk_icon_theme_get_for_screen (old_screen),
			 G_CALLBACK (icon_theme_changed_cb), spinner);
	}

	details->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect (details->icon_theme, "changed",
			  G_CALLBACK (icon_theme_changed_cb), spinner);
}

static void
gsearch_spinner_class_init (GSearchSpinnerClass *class)
{
	GObjectClass *object_class =  G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->dispose = gsearch_spinner_dispose;
	object_class->finalize = gsearch_spinner_finalize;

	widget_class->expose_event = gsearch_spinner_expose;
	widget_class->size_request = gsearch_spinner_size_request;
	widget_class->map = gsearch_spinner_map;
	widget_class->unmap = gsearch_spinner_unmap;
	widget_class->screen_changed = gsearch_spinner_screen_changed;

	g_type_class_add_private (object_class, sizeof (GSearchSpinnerDetails));
}

/*
 * gsearch_spinner_new:
 *
 * Create a new #GSearchSpinner. The spinner is a widget
 * that gives the user feedback about search status with
 * an animated image.
 *
 * Return Value: the spinner #GtkWidget
 **/
GtkWidget *
gsearch_spinner_new (void)
{
	return GTK_WIDGET (g_object_new (GSEARCH_TYPE_SPINNER,
					 "visible-window", FALSE,
					 NULL));
}
