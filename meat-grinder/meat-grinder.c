/* meat-grinder, maker of tarballs */
#include <config.h>
#include <gnome.h>
#include <sys/stat.h>
#include <unistd.h>

static GtkWidget *app = NULL;
static GtkWidget *icon_list = NULL;
static GtkWidget *size_label = NULL;
static GHashTable *file_ht = NULL;
static off_t total_size = 0;
static int number_of_files = 0;
static char *file_icon = NULL;
static char *folder_icon = NULL;
static char *compress_icon = NULL;

enum {
	TYPE_FOLDER,
	TYPE_REGULAR
};


typedef struct _File File;
struct _File {
	int type;
	char *name;
	off_t size;
};

static void
about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	gchar *authors[] = {
		"George Lebl <jirka@5z.com>",
		NULL
	};

	if (about != NULL) {
		gtk_widget_show_now (about);
		gdk_window_raise (about->window);
		return;
	}
	about = gnome_about_new (_("The GNOME Archive Generator"), VERSION,
				 "(C) 2001 George Lebl",
				 (const char **)authors,
				 _("Drag files in to make archives"),
				 NULL);
	gtk_signal_connect (GTK_OBJECT (about), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			    &about);
	gtk_widget_show (about);
}

static void
quit_cb (GtkWidget *item)
{
	gtk_main_quit ();
}

/* Menus */
static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_MENU_EXIT_ITEM (quit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP ("meat-grinder"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (about_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo grinder_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file_menu),
	GNOMEUIINFO_MENU_HELP_TREE (help_menu),
        GNOMEUIINFO_END
};

static void
init_gui (void)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *sw;
	GtkWidget *w, *button;

        app = gnome_app_new ("meat-grinder",
			     _("GNOME Archive Generator"));
	gtk_window_set_wmclass (GTK_WINDOW (app),
				"meat-grinder",
				"meat-grinder");
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, FALSE);

        gtk_signal_connect (GTK_OBJECT (app), "destroy",
			    GTK_SIGNAL_FUNC (quit_cb), NULL);

	/*set up the menu*/
        gnome_app_create_menus (GNOME_APP (app), grinder_menu);

	vbox = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_usize (sw, 250, 150);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (sw);
	gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

	icon_list = gnome_icon_list_new (/*evil*/66, NULL, 0);
	gtk_widget_show (icon_list);
	gtk_container_add (GTK_CONTAINER (sw), icon_list);

	w = gtk_hseparator_new ();
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	/* tarball icon */
	button = gtk_button_new ();
	gtk_widget_show (button);
	w = NULL;
	if (compress_icon != NULL)
		w = gnome_pixmap_new_from_file (compress_icon);
	if (w == NULL)
		w = gtk_label_new (_("Archive"));
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (button), w);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	size_label = gtk_label_new (_("Number of files: 0\nTotal size: 0"));
	gtk_label_set_justify (GTK_LABEL (size_label), GTK_JUSTIFY_LEFT);
	gtk_widget_show (size_label);
	gtk_box_pack_start (GTK_BOX (hbox), size_label, FALSE, FALSE, 0);

	gtk_container_border_width (GTK_CONTAINER (vbox), 5);

	gnome_app_set_contents (GNOME_APP (app), vbox);

	gtk_widget_show (app);
}

static void
add_file (const char *file)
{
	struct stat s;
	File *f;
	int type;
	const char *icon;
	char *msg;
	off_t size;

	if (g_hash_table_lookup (file_ht, file) != NULL)
		return;

	if (stat (file, &s) < 0)
		/* FIXME: perhaps an error of some sort ??? */
		return;

	type = 0;
	icon = NULL;
	if (S_ISDIR (s.st_mode)) {
		type = TYPE_FOLDER;
		icon = folder_icon;
		size = s.st_size;
	} else if (S_ISREG (s.st_mode) ||
		   S_ISLNK (s.st_mode)) {
		type = TYPE_REGULAR;
		icon = file_icon;
		size = 0; /* FIXME: should we even do this? */
	} else {
		/* FIXME: error of some sort */
		return;
	}

	number_of_files ++;
	total_size += s.st_size;

	f = g_new0 (File, 1);
	f->type = type;
	f->name = g_strdup (file);
	f->size = size;

	gnome_icon_list_append (GNOME_ICON_LIST (icon_list),
				icon, file);

	g_hash_table_insert (file_ht, f->name, f);

	msg = g_strdup_printf (_("Number of files: %d\nTotal size: %lu"),
			       number_of_files, (gulong)total_size);
	gtk_label_set_text (GTK_LABEL (size_label), msg);
	g_free (msg);
}

int
main (int argc, char *argv [])
{
	int i;
	poptContext ctx;
	const char **files;
	
	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init_with_popt_table ("meat-grinder", VERSION,
				    argc, argv, NULL, 0, &ctx);
	/* no icon yet */
	/*gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-meat-grinder.png");*/

	file_icon = gnome_pixmap_file ("nautilus/i-regular.png");
	if (file_icon == NULL)
		file_icon = gnome_pixmap_file ("nautilus/gnome/i-regular.png");
	if (file_icon == NULL)
		file_icon = gnome_pixmap_file ("gnome-file.png");
	if (file_icon == NULL)
		file_icon = gnome_pixmap_file ("gnome-unknown.png");

	folder_icon = gnome_pixmap_file ("gnome-folder.png");
	if (folder_icon == NULL)
		folder_icon = gnome_pixmap_file ("nautilus/gnome-folder.png");
	if (folder_icon == NULL)
		folder_icon = gnome_pixmap_file ("nautilus/gnome/gnome/gnome-folder.png");
	if (folder_icon == NULL)
		folder_icon = gnome_pixmap_file ("gnome-unknown.png");

	compress_icon = gnome_pixmap_file ("gnome-compressed.png");
	if (compress_icon == NULL)
		compress_icon = gnome_pixmap_file ("nautilus/gnome-compressed.png");

	if (folder_icon == NULL || file_icon == NULL) {
		gnome_dialog_run_and_close (GNOME_DIALOG (gnome_error_dialog
			  (_("Cannot file proper icons anywhere!"))));
		exit (1);
	}

	files = poptGetArgs (ctx);

	init_gui ();

	file_ht = g_hash_table_new (g_str_hash, g_str_equal);

	gnome_icon_list_freeze (GNOME_ICON_LIST (icon_list));

	for (i = 0; files != NULL && files[i] != NULL; i++) {
		add_file (files[i]);
	}

	gnome_icon_list_thaw (GNOME_ICON_LIST (icon_list));

	gtk_main ();
	
	return 0;
}
