/* Gnome Color Browser. By that one tcd guy. <timg@means.net> */

#include <config.h>
#include <gnome.h>

typedef enum {
    DECIMAL_8BIT, 
    DECIMAL_16BIT,
    HEX_8BIT, 
    HEX_16BIT,
    FLOAT
} FormatType;

typedef struct 
{
    char name[256];
    int r,g,b;
} RGBColor;

GtkWidget *window, *clist;
GdkGC *gc;
GdkColor black;

static 		GtkWidget *create_clist(void);
void 		set_swatch(RGBColor *c, GtkWidget *clist);
void		load_rgb(GtkWidget *clist);
void 		format_color(FormatType type, char *str, int r, int g, int b);
static 		GtkWidget *create_menu(void);
void 		about_cb(GtkWidget *widget, void *data);
void 		delete_event(GtkWidget *widget, gpointer *data);
void 		select_row_cb(GtkWidget *widget,
			      gint row,
			      gint column,
			      GdkEventButton *event,
			      gpointer data);
void 		menu_item_cb(GtkWidget *menuitem, gpointer data);
void make_menus(GnomeApp *app);

void delete_event(GtkWidget *widget, gpointer *data)
{
    gtk_main_quit();
}

int main(int argc, char **argv)
{
    GtkWidget *clist, *vbox, *hbox;
    GtkWidget *menu, *dropdown;
    GtkWidget *label, *sw;
    GtkWidget *name_e, *value_e;

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, GNOMELOCALEDIR);
    textdomain(PACKAGE);
#endif
      
    gnome_init("gcolorsel", VERSION, argc, argv);
    window = gnome_app_new("gcolorsel", _("Gnome Color Browser"));
    gtk_widget_set_usize(window, 300, 328);
    gtk_window_set_title(GTK_WINDOW(window), _("Gnome Color Browser"));
    gtk_window_set_wmclass(GTK_WINDOW(window), "main_window","gcolorsel");
    gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
    gtk_signal_connect( GTK_OBJECT(window), "delete_event",
			GTK_SIGNAL_FUNC(delete_event), NULL);
    make_menus(GNOME_APP(window));

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(vbox), 2);

    /* Create menu */
    hbox = gtk_hbox_new(TRUE, 2);
    label = gtk_label_new("Format Style:");
    menu = create_menu();
    dropdown = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(dropdown), menu);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), dropdown);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Create entries */
    hbox = gtk_hbox_new(TRUE, 2);
    label = gtk_label_new(_("Color Name:"));
    name_e = gtk_entry_new();
    gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), name_e);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    hbox = gtk_hbox_new(TRUE, 2);
    label = gtk_label_new(_("Color Values:"));
    value_e = gtk_entry_new();
    gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), value_e);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    /* Create color list */
    gtk_widget_realize(window);
    gc = gdk_gc_new(window->window);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    clist = create_clist();
    gtk_container_add(GTK_CONTAINER(sw), clist);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), sw);

    /* some silly stuff to avoid too many globals */
    gtk_object_set_data(GTK_OBJECT(clist), "name_entry", name_e);
    gtk_object_set_data(GTK_OBJECT(clist), "value_entry", value_e);
	
    gnome_app_set_contents(GNOME_APP(window), vbox);
	
    gtk_widget_show_all(window);

    gtk_main();
    return EXIT_SUCCESS;
}

void select_row_cb(GtkWidget *widget,
		   gint row,
		   gint column,
		   GdkEventButton *event,
		   gpointer data)     
{
    GtkWidget *name_e, *value_e;
    char *tmp;

    name_e = gtk_object_get_data(GTK_OBJECT(widget), "name_entry");
    value_e = gtk_object_get_data(GTK_OBJECT(widget), "value_entry");

    gtk_clist_get_text(GTK_CLIST(widget), row, 1, &tmp);
    gtk_entry_set_text(GTK_ENTRY(value_e), tmp);
		
    gtk_clist_get_text(GTK_CLIST(widget), row, 2, &tmp);
    gtk_entry_set_text(GTK_ENTRY(name_e), tmp);
}	        

static GtkWidget *create_clist(void)
{
    GdkColormap *colormap;
        
    clist = gtk_clist_new(3);
    gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_IN);

    gtk_clist_set_row_height(GTK_CLIST(clist), 18);

    gtk_clist_set_column_width(GTK_CLIST(clist), 0, 52);
    gtk_clist_set_column_width(GTK_CLIST(clist), 1, 72);
    gtk_clist_set_column_title(GTK_CLIST(clist), 0, _("Color"));
    gtk_clist_set_column_title(GTK_CLIST(clist), 1, _("Value"));
    gtk_clist_set_column_title(GTK_CLIST(clist), 2, _("Name"));
    gtk_clist_column_titles_show(GTK_CLIST(clist));

    gtk_signal_connect(GTK_OBJECT(clist), "select_row", 
		       GTK_SIGNAL_FUNC(select_row_cb), NULL);

    colormap = gtk_widget_get_colormap(clist);
    gdk_color_parse("black", &black);
    gdk_color_alloc(colormap, &black);
	
    gtk_object_set_data(GTK_OBJECT(clist), "colormap", colormap);

    gtk_clist_freeze(GTK_CLIST(clist));
    load_rgb(clist);
    gtk_clist_thaw(GTK_CLIST(clist));
	
    return clist;
}

void set_swatch(RGBColor *c, GtkWidget *clist)
{
    char *string[3];
    static int row=0;
    GdkPixmap *pixmap;
    GdkColor color;

    if(!c || !c->name)
	return;
		
    gdk_color_parse(c->name, &color);
    gdk_color_alloc(gtk_object_get_data(GTK_OBJECT(clist), "colormap"), &color);
    
    pixmap = gdk_pixmap_new(window->window,
			    48, 16, gtk_widget_get_visual(window)->depth);
	
    gdk_gc_set_foreground(gc, &color);
    gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, 47, 15);
    gdk_gc_set_foreground(gc, &black);
    gdk_draw_rectangle(pixmap, gc, FALSE, 0, 0, 47, 15);

    string[0] = NULL;
    string[1] = g_malloc(64);
    string[2] = c->name;
	
    format_color(DECIMAL_8BIT, string[1], c->r, c->g, c->b);
	
    gtk_clist_append(GTK_CLIST(clist), string);
    gtk_clist_set_pixmap(GTK_CLIST(clist), row, 0, pixmap, NULL);
    gtk_clist_set_row_data(GTK_CLIST(clist), row, c);

    row++;
    gtk_object_set_data(GTK_OBJECT(clist), "last_row", GINT_TO_POINTER(row));
		
    g_free(string[1]);
}

/* grabbed from ee */
#define GTK_FLUSH \
    while (gtk_events_pending()) \
            gtk_main_iteration();
            
void load_rgb(GtkWidget *clist)
{
    GtkWidget *prog_window, *hbox, *bar, *label;
    char tmp[256];
    int t;
    FILE *file;
    long flen;
	
    file = fopen("/usr/X11R6/lib/X11/rgb.txt", "r");
    if(!file)
    {
	gtk_widget_show(gnome_message_box_new(
	    _("Error, cannot open /usr/X11R6/lib/X11/rgb.txt.\n"), "error",
	    GNOME_STOCK_BUTTON_OK, NULL));
	return;
    }		

    fseek(file, 0, SEEK_END);
    flen = ftell(file);
    fseek(file, 0, SEEK_SET);

    prog_window = gnome_dialog_new(N_("Gnome Color Browser"), GNOME_STOCK_BUTTON_CANCEL, NULL);

    bar = gtk_progress_bar_new();
    hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    label = gtk_label_new(N_("Parsing Colors"));
    gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), bar);
    gtk_progress_bar_update(GTK_PROGRESS_BAR(bar), 0);

    gtk_box_pack_end_defaults(GTK_BOX(GNOME_DIALOG(prog_window)->vbox), hbox);
	
    gtk_widget_show_all(prog_window);

    gtk_grab_add(prog_window);
    GTK_FLUSH;

    for(;;)
    {
	gfloat p;
	RGBColor *color;
		
	fgets(tmp, 255, file);

	if(feof(file))
	    break;

	color = g_new(RGBColor, 1);
	t = sscanf(tmp, "%d %d %d\t\t%[a-zA-Z0-9 ]\n", &color->r,
		   &color->g,
		   &color->b,
		   color->name);
	if(t==4)
	    set_swatch(color, clist);

	p = (gfloat)ftell(file)/(gfloat)flen;
	gtk_progress_bar_update(GTK_PROGRESS_BAR(bar), p);
	GTK_FLUSH;
    }
    gtk_widget_destroy(prog_window);
    return;
}

void format_color(FormatType type, char *str, int r, int g, int b)
{
    switch(type)
    {
    case DECIMAL_8BIT:
	g_snprintf(str, 64, "%d %d %d", r,g,b);
	return;
    case DECIMAL_16BIT:
	g_snprintf(str, 64, "%d %d %d", r*256,g*256,b*256);
	return;
    case HEX_8BIT:
	g_snprintf(str, 64, "#%02x%02x%02x", r,g,b);
	return;
    case HEX_16BIT:
	g_snprintf(str, 64, "#%04x%04x%04x", r*256,g*256,b*256);
	return;
    case FLOAT:
	g_snprintf(str, 64, "%1.4g %1.4g %1.4g", (float)r/256,
		   (float)g/256,
		   (float)b/256);
	return;
    }
}

char *menu_options[]=
{
    N_("Decimal (8 bit)"),
    N_("Decimal (16 bit)"),
    N_("Hex (8 bit)"),
    N_("Hex (16 bit)"),
    N_("Float"),
    NULL,
};

void menu_item_cb(GtkWidget *menuitem, gpointer data)
{
    RGBColor *color;
    int type, rows, i;
    char tmp[64];
	
    type = GPOINTER_TO_INT(data);
    rows = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(clist), "last_row"));
	
    for(i=0; i < rows; i++)
    {
	color = gtk_clist_get_row_data(GTK_CLIST(clist), i);
	format_color(type, tmp, color->r, color->g, color->b);
	gtk_clist_set_text(GTK_CLIST(clist), i, 1, tmp);
    }
}
 
static GtkWidget *create_menu(void)
{
    int i=0;
    GtkWidget *menu, *menuitem;
        
    menu = gtk_menu_new();
        
    while(menu_options[i])
    {
	menuitem = gtk_menu_item_new_with_label(menu_options[i]);  
	gtk_menu_append(GTK_MENU(menu), menuitem);                 
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   GTK_SIGNAL_FUNC(menu_item_cb),
			   GINT_TO_POINTER(i));
	i++;    
    }
    return menu;
}

void about_cb(GtkWidget *widget, void *data)
{
    GtkWidget *about;
    gchar *authors[] = {
	"Tim P. Gerla",
	NULL
    };
		
    about = gnome_about_new (
	_("Gnome Color Browser"),
	"0.1",
	"(C) 1997-98 Tim P. Gerla", 
	(const gchar**)authors,
	_("Small utility to browse available X11 colors."),
	NULL);                                
    gtk_widget_show(about);
          
    return;
}
 
/* Menus */
static GnomeUIInfo help_menu[] = {
    GNOMEUIINFO_ITEM_STOCK(N_("_About"), NULL, about_cb,
                           GNOME_STOCK_MENU_ABOUT),
    GNOMEUIINFO_END
};
  
static GnomeUIInfo program_menu[] = {
	{GNOME_APP_UI_ITEM, N_("E_xit"), NULL, delete_event, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'x',
	 GDK_CONTROL_MASK, NULL},
    GNOMEUIINFO_END
};      

static GnomeUIInfo main_menu[] = {
        GNOMEUIINFO_SUBTREE(N_("_Program"), &program_menu),
        GNOMEUIINFO_SUBTREE(N_("_Help"), &help_menu),
        GNOMEUIINFO_END
};
/* End of menus */

void make_menus(GnomeApp *app)
{
    gnome_app_create_menus(app, main_menu);
}
