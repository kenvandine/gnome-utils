#include <config.h>
#if HAS_GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include "menus.h"
#include "gtt-features.h"

#if HAS_GNOME && defined(GNOME_USE_TOOLBAR)
#define WANT_GNOME 1
#else
#define WANT_GNOME 0
#endif

#if !WANT_GNOME
# include "tb_new.xpm"

# include "tb_open.xpm"
# include "tb_save.xpm"

# include "tb_cut.xpm"
# include "tb_copy.xpm"
# include "tb_paste.xpm"

# include "tb_properties.xpm"
# include "tb_timer.xpm"
# include "tb_timer_stopped.xpm"

# include "tb_preferences.xpm"
# ifdef EXTENDED_TOOLBAR
#  include "tb_unknown.xpm"
#  include "tb_exit.xpm"
# endif /* EXTENDED_TOOLBAR */
#endif /* !WANT_GNOME */



#if !WANT_GNOME
static GtkWidget *win;
static GtkBox *hbox;
static GtkTooltips *tt;

typedef struct _ToggleData {
	GtkContainer *button;
	GtkWidget *pmap1, *pmap2, *cur_pmap;
	char *path;
} ToggleData;

ToggleData *toggle_timer = NULL;
#endif /* !WANT_GNOME */



static void sigfunc(GtkWidget *w,
		    gpointer *data)
{
	if (!data) return;
	menus_activate((char *)data);
}



#if !WANT_GNOME
static void set_toggle_state(ToggleData *data)
{
	if (!data) return;
	if (menus_get_toggle_state(data->path)) {
		if (data->cur_pmap != data->pmap1) {
			if (data->cur_pmap)
				gtk_container_remove(data->button, data->cur_pmap);
			gtk_container_add(data->button, data->pmap1);
			data->cur_pmap = data->pmap1;
		}
	} else {
		if (data->cur_pmap != data->pmap2) {
			if (data->cur_pmap)
				gtk_container_remove(data->button, data->cur_pmap);
			gtk_container_add(data->button, data->pmap2);
			data->cur_pmap = data->pmap2;
		}
	}
}
#endif /* !WANT_GNOME */



#if !WANT_GNOME
static void sigfunc_toggle(GtkWidget *w,
			   ToggleData *data)
{
	if (!data) return;
	if (!data->path) return;
	menus_activate(data->path);
	set_toggle_state(data);
}
#endif /* !WANT_GNOME */



#if !WANT_GNOME
static void add_space(gint spacing)
{
	GtkWidget *w;
	
	w = gtk_label_new(" ");
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, spacing);
}
#endif /* !WANT_GNOME */



void toolbar_set_states(void)
{
#if !WANT_GNOME
	if (toggle_timer) set_toggle_state(toggle_timer);
#endif /* !WANT_GNOME */
}



#if !WANT_GNOME
static ToggleData *add_toggle_button(gchar **xpm, gchar **xpm2,
				     char *text,
				     gchar *path)
{
	GtkWidget *w;
	GdkPixmap *pmap;
	GdkBitmap *bmap;
	GtkStyle *style;
	ToggleData *toggle_data;
	
	toggle_data = (ToggleData *)g_malloc(sizeof(ToggleData));
	toggle_data->path = path;
	toggle_data->cur_pmap = NULL;

	style = gtk_widget_get_style(win);
	pmap = gdk_pixmap_create_from_xpm_d(win->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    xpm);
	toggle_data->pmap1 = gtk_pixmap_new(pmap, bmap);
	gtk_widget_show(toggle_data->pmap1);
	pmap = gdk_pixmap_create_from_xpm_d(win->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    xpm2);
	toggle_data->pmap2 = gtk_pixmap_new(pmap, bmap);
	gtk_widget_show(toggle_data->pmap2);
	w = gtk_button_new();
	toggle_data->button = GTK_CONTAINER(w);
	set_toggle_state(toggle_data);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 2);
	if (path) {
		gtk_signal_connect(GTK_OBJECT(w), "clicked",
				   GTK_SIGNAL_FUNC(sigfunc_toggle),
				   (gpointer *)toggle_data);
	}
	if (text)
		gtk_tooltips_set_tips(tt, w, text);
	return toggle_data;
}
#endif /* !WANT_GNOME */



#if !WANT_GNOME
static void add_button(gchar **xpm, char *text, gchar *path)
{
	GtkWidget *w, *pixmap;
	GdkPixmap *pmap;
	GdkBitmap *bmap;
	GtkStyle *style;

	style = gtk_widget_get_style(win);
	pmap = gdk_pixmap_create_from_xpm_d(win->window, &bmap,
					    &style->bg[GTK_STATE_NORMAL],
					    xpm);
	pixmap = gtk_pixmap_new(pmap, bmap);
	gtk_widget_show(pixmap);
	w = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(w), pixmap);
	gtk_widget_show(w);
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 2);
	if (path) {
		gtk_signal_connect(GTK_OBJECT(w), "clicked",
				   GTK_SIGNAL_FUNC(sigfunc),
				   (gpointer *)path);
	}
	if (text)
		gtk_tooltips_set_tips(tt, w, text);
}
#endif /* !WANT_GNOME */



GtkWidget *build_toolbar(GtkWidget *window, GtkTooltips **tips)
{
#if WANT_GNOME
# warning uahhh, that gnome-toolbar suxx!
	GnomeToolbar *tbar;

	tbar = gnome_create_toolbar(window, GNOME_TB_PACK_START);
	gnome_toolbar_add(tbar, "New Project...", "tb_new.xpm",
			  (GnomeToolbarFunc)sigfunc, "<Main>/File/New Project...");
	gnome_toolbar_add_default(tbar, GNOME_TOOLBAR_RELOAD,
				  (GnomeToolbarFunc)sigfunc, "<Main>/File/Reload rc");
	gnome_toolbar_add_default(tbar, GNOME_TOOLBAR_CLOSE,
				  (GnomeToolbarFunc)sigfunc, "<Main>/File/Save rc");
	gnome_toolbar_add_default(tbar, GNOME_TOOLBAR_EXIT,
				  (GnomeToolbarFunc)sigfunc, "<Main>/File/Quit");
	gnome_toolbar_set_style(tbar, GNOME_TB_TEXT|GNOME_TB_ICONS|GNOME_TB_HORIZONTAL);
	if (tips) *tips = NULL;
	return tbar->toolbar;
#else /* WANT_GNOME */ 
	win = window;

	tt = gtk_tooltips_new();

	hbox = GTK_BOX(gtk_hbox_new(FALSE, 0));
	gtk_widget_show(GTK_WIDGET(hbox));

	add_button(tb_new_xpm, "New Project...", "<Main>/File/New Project...");
	add_space(2);
	add_button(tb_open_xpm, "Reload", "<Main>/File/Reload rc");
	add_button(tb_save_xpm, "Save", "<Main>/File/Save rc");
	add_space(2);
	add_button(tb_cut_xpm, "Cut", "<Main>/Edit/Cut");
	add_button(tb_copy_xpm, "Copy", "<Main>/Edit/Copy");
	add_button(tb_paste_xpm, "Paste", "<Main>/Edit/Paste");
	add_space(2);
	add_button(tb_properties_xpm, "Edit Properties...", "<Main>/Edit/Properties...");
	toggle_timer = add_toggle_button(tb_timer_xpm, tb_timer_stopped_xpm,
					 "start/stop Timer",
					 "<Main>/Timer/Timer running");
	add_space(2);
	add_button(tb_preferences_xpm, "Edit Preferences...", "<Main>/Edit/Preferences...");
#ifdef EXTENDED_TOOLBAR
	add_button(tb_unknown_xpm, "About...", "<Main>/Help/About...");
	add_button(tb_exit_xpm, "Quit", "<Main>/File/Quit");
#endif /* EXTENDED_TOOLBAR */

	if (tips) *tips = tt;
	return GTK_WIDGET(hbox);
#endif /* WANT_GNOME */
}

