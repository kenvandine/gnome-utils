/* meat-grinder, maker of tarballs */

#include <config.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

static GtkWidget *app = NULL;
static GtkWidget *icon_list = NULL;
static GtkWidget *status_label = NULL;
static GtkWidget *compress_button = NULL;
static GtkWidget *app_bar = NULL;
static GtkWidget *progress_bar = NULL;
static GHashTable *file_ht = NULL;
static gint number_of_files = 0;
static gint number_of_dirs = 0;
static gchar *file_icon = NULL;
static gchar *folder_icon = NULL;
static gchar *compress_icon = NULL;
static gchar *tar_prog = NULL;
static gchar *sh_prog = NULL;
static gchar *gzip_prog = NULL;
static gchar *tarstr = NULL;
static gchar *filestr = NULL;
static gchar *gzipstr = NULL;
static gchar *filename = NULL;
static pid_t temporary_pid = 0;
static gchar *temporary_file = NULL;
struct poptOption options;
struct argv_adder argv_adder;

static GtkWidget *create_archive_widget=NULL;
static GtkWidget *remove_widget=NULL;
static GtkWidget *clear_widget=NULL;
static GtkWidget *selectall_widget=NULL;

#define ERRDLG(error) gtk_message_dialog_new (GTK_WINDOW(app), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error);
#define ERRDLGP(error,parent) gtk_message_dialog_new (GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error);

static GtkTargetEntry drop_targets [] = {
  { "text/uri-list", 0, 0 }
};
static guint n_drop_targets = sizeof (drop_targets) / sizeof(drop_targets[0]);

static GtkTargetEntry drag_targets [] = {
  { "x-special/gnome-icon-list", 0, 0 },
  { "text/uri-list", 0, 0 }
};
static guint n_drag_targets = sizeof(drag_targets) / sizeof(drag_targets[0]);

enum {
	TYPE_FOLDER,
	TYPE_REGULAR
};


typedef struct _File File;
struct _File {
	gint type;
	gchar *name;
	gchar *base_name;
};

static void
free_file (File *f)
{
	f->type = -1;
	g_free (f->name);
	f->name = NULL;
	g_free (f);
}

static void 
cleanup_tmpstr (void)
{
	g_free(tarstr);
	tarstr = NULL;
	g_free(filestr);
	filestr = NULL;
	g_free(gzipstr);
	gzipstr = NULL;
}

static char *
make_temp_dir (void)
{
	char *name = NULL;
	do {
		if (name != NULL)
			g_free (name);
		name = g_strdup_printf ("/tmp/gnome-meat-grinder-%d",
					rand ());
	} while (mkdir (name, 0755) < 0);

	return name;
}

static gboolean
query_dialog (const gchar *msg)
{
	gint response;
	GtkWidget *req;

	req = gtk_message_dialog_new (GTK_WINDOW (app),
				       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       msg);
	
	gtk_dialog_add_buttons (GTK_DIALOG (req),
				GTK_STOCK_NO, GTK_RESPONSE_NO,
				GTK_STOCK_YES, GTK_RESPONSE_YES,
				NULL);
	

	response = gtk_dialog_run (GTK_DIALOG (req));

	gtk_widget_destroy(req);

	if (response == GTK_RESPONSE_YES)
		return TRUE;
	else /* this includes -1 which is "destroyed" */
		return FALSE;
}

static void
update_status (void)
{
	if (number_of_files > 0 || number_of_dirs > 0) {
		gchar *msg;
		msg = g_strdup_printf (_("Number of files: %d\nNumber of folders: %d"),
				       number_of_files, number_of_dirs);
		gtk_label_set_text (GTK_LABEL (status_label), msg);
		g_free (msg);
	} else {
		gtk_label_set_text (GTK_LABEL (status_label), 
				    _("No files or folders, drop files or folders\n"
				      "above to add them to the archive"));
	}
}

/* removes icon, positions should not be trusted after this */
static void
remove_pos (gint pos)
{
	File *f;
	gchar *status_msg;

	f = gnome_icon_list_get_icon_data (GNOME_ICON_LIST (icon_list), pos);
	gnome_icon_list_remove (GNOME_ICON_LIST (icon_list), pos);
	if (f == NULL)
		return;

	if (f->type == TYPE_REGULAR)
		number_of_files --;
	else if (f->type == TYPE_FOLDER)
		number_of_dirs --;
	g_hash_table_remove (file_ht, f->base_name);
        
 	status_msg = g_strdup_printf(_("%s removed"), f->base_name);
        gnome_appbar_push (GNOME_APPBAR(app_bar), 
                           status_msg);
        g_free (status_msg);
	free_file (f);

	if (number_of_files + number_of_dirs <= 0) {
		gtk_widget_set_sensitive (compress_button, FALSE);
		gtk_widget_set_sensitive (create_archive_widget, FALSE);
		gtk_widget_set_sensitive (clear_widget, FALSE);
		gtk_widget_set_sensitive (selectall_widget, FALSE);
	}

	update_status ();
}

struct argv_adder {
	gchar *link_dir;
	gchar **argv;
	gint pos;
};


static void
make_argv_fe (gpointer key, gpointer value, gpointer user_data)
{
	File *f = value;
	gchar *file;
	struct argv_adder *argv_adding = user_data;
	file = g_build_filename ((argv_adding->link_dir), (f->base_name), NULL);
	argv_adding->argv[argv_adding->pos ++] = f->base_name;
	/* FIXME: catch errors */
	symlink (f->name, file);
}

static void
whack_links_fe (gpointer key, gpointer value, gpointer user_data)
{
	File *f = value;
	gchar *link_dir = user_data;
	gchar *file = g_build_filename ((link_dir), (f->base_name), NULL);

	unlink (file);
	g_free (file);
}

static gint
update_progress_bar (gpointer p)
{
        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress_bar));

        return TRUE;
}

static void
start_progress_bar (gboolean flag)
{
	static gint timeout;

	if (flag)
		timeout = gtk_timeout_add (100, update_progress_bar, NULL);
	else {
		gtk_timeout_remove (timeout);
        	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0.0);
	}
}

static void
setup_busy (GtkWidget *w, gboolean busy)
{

	if (busy) {
		GdkCursor *cursor;
		/* Change cursor to busy */
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (w->window, cursor);
		gdk_cursor_unref (cursor);
	} else {
		gdk_window_set_cursor (w->window, NULL);
	}

	gdk_flush ();
}

static gboolean
handle_io (GIOChannel *ioc, GIOCondition condition, gpointer data) 
{
	start_progress_bar (FALSE);

	g_hash_table_foreach (file_ht, whack_links_fe, argv_adder.link_dir);

	/* FIXME: handle errors */
	rmdir (argv_adder.link_dir);
	g_free (argv_adder.link_dir);
	g_free (argv_adder.argv);
	cleanup_tmpstr ();
}


static gboolean
create_archive (GtkWidget *fsel,
		 const gchar *fname,
		const gchar *dir,
		gboolean gui_errors)
{
	GIOChannel *ioc;
	gboolean status = TRUE;
	gchar *tmpfname = NULL;
	gint child_pid;
	gint child_fd;

	
	if (fsel) {
		setup_busy (fsel, FALSE);
		gtk_widget_destroy (fsel);
	} 

	if (dir == NULL)
		argv_adder.link_dir = make_temp_dir ();
	else
		argv_adder.link_dir = (gchar *)dir;

	/* Added code for appending .tar.gz to archive file */
	tmpfname = g_strstr_len ((gchar *)fname, strlen((gchar *)fname), ".tar.gz");
	if (tmpfname == NULL)
		tmpfname = g_strstr_len ((gchar *)fname, strlen((gchar *)fname), ".tgz");
	if (tmpfname != NULL) {
		if (!((g_ascii_strcasecmp(tmpfname, ".tar.gz") == 0) || (g_ascii_strcasecmp(tmpfname, ".tgz") == 0)))
			fname = g_strconcat ((gchar *)fname, ".tar.gz", NULL);
	} else {
		fname = g_strconcat ((gchar *)fname, ".tar.gz", NULL);
	}

        /* Since tar -z option is not available in Solaris, doing tar & gzip */

	argv_adder.argv = g_new (gchar *, number_of_files + number_of_dirs + 4);
	argv_adder.pos = 0;
	g_hash_table_foreach (file_ht, make_argv_fe, &argv_adder);
	argv_adder.argv[argv_adder.pos ++] = NULL;

	tarstr = g_strconcat(tar_prog, " -chf - ", NULL);
	filestr = g_strjoinv(" ", argv_adder.argv);
	gzipstr = g_strconcat(" | ", gzip_prog, " - > ", (gchar *)fname, NULL);

	filestr = g_strescape (filestr, NULL);

	argv_adder.argv[0] = sh_prog;
	argv_adder.argv[1] = "-c";
	argv_adder.argv[2] = g_strconcat(tarstr, filestr, gzipstr, NULL);	
	argv_adder.argv[3] = NULL;

	/* FIXME: get error output, check for errors, etc... */

	/* setup_busy (app, TRUE); */
	start_progress_bar (TRUE);
	if (!g_spawn_async_with_pipes (argv_adder.link_dir, argv_adder.argv, 
				       NULL, (G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
		                       NULL, NULL, &child_pid, NULL, &child_fd, NULL, NULL)) {
		status = FALSE;
		if (gui_errors) {
			gchar *tmp = NULL;
			tmp = g_strdup_printf (_("Failed to create %s"), filestr);
			ERRDLG (tmp);
			g_free (tmp);
		}
		else
			fprintf (stderr, _("Failed to create %s"), filestr);
	}
	ioc = g_io_channel_unix_new (child_fd);
        g_io_add_watch (ioc, G_IO_HUP,
                        handle_io, NULL);
        g_io_channel_unref (ioc);
/*	setup_busy (app, FALSE);  */

	return status;
}

static void
start_temporary (void)
{
	gchar *dir;
	gchar *file = NULL;

	if (temporary_file != NULL) {
		if (access (temporary_file, F_OK) == 0)
			return;

		/* Note: nautilus is a wanker and will happily do a move when
		 * we explicitly told him that we just support copy, so in case
		 * this file is missing, we let nautilus have it and hope
		 * he chokes on it */

		dir = g_path_get_dirname (temporary_file);
		rmdir (dir);
		g_free (dir);

		g_free (temporary_file = NULL);
		temporary_file = NULL;

		/* just paranoia */
		if (temporary_pid > 0)
			kill (temporary_pid, SIGKILL);
	}

	/* make a temporary dirname */
	dir = make_temp_dir ();
	file = g_build_filename ((dir), (_("Archive.tar.gz")), NULL);

	temporary_pid = fork ();

	if (temporary_pid == 0) {
		if ( ! create_archive (NULL, file, dir, FALSE /* gui_errors */)) {
			_exit (1);
		} else {
			_exit (0);
		}
	}

	/* can't fork? don't dispair, do synchroniously */
	if (temporary_pid < 0) {
		setup_busy (app, TRUE);
		if ( ! create_archive (NULL, file, dir, TRUE /* gui_errors */)) {
			setup_busy (app, FALSE);
			g_free (file);
			g_free (dir);
			temporary_pid = 0;
			return;
		}
		setup_busy (app, FALSE);
		temporary_pid = 0;
	}
	g_free (dir);

	temporary_file = file;
}

static gboolean
ensure_temporary (void)
{
	gint status;

	start_temporary ();

	if (temporary_file == NULL)
		return FALSE;

	if (temporary_pid == 0)
		return TRUE;

	/* ok, gotta wait */
	waitpid (temporary_pid, &status, 0);

	temporary_pid = 0;

	if (WIFEXITED (status) &&
	    WEXITSTATUS (status) == 0) {
		return TRUE;
	} else {
		g_free (temporary_file);
		temporary_file = NULL;
		temporary_pid = 0;
		return FALSE;
	}
}

static void
cleanup_temporary (void)
{
	gchar *file = temporary_file;
	pid_t pid = temporary_pid;

	temporary_file = NULL;
	temporary_pid = 0;

	if (pid > 0) {
		if (kill (pid, SIGTERM) == 0)
			waitpid (pid, NULL, 0);
	}
	
	if (file != NULL) {
		gchar *dir;

		unlink (file);

		dir = g_path_get_dirname (file);
		rmdir (dir);
		g_free (dir);
	}

	g_free (file);
}

static void
about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	GdkPixbuf	 *pixbuf;
	GError		 *error = NULL;
	gchar		 *file;
	
	gchar *authors[] = {
		"George Lebl <jirka@5z.com>",
		NULL
	};
	gchar *documenters[] = {
		NULL
	};
	/* Translator credits */
	gchar *translator_credits = _("translator_credits");

	if (about != NULL) {
		gtk_widget_show_now (about);
		gdk_window_raise (about->window);
		return;
	}
	
	file = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, 
			"document-icons/gnome-compressed.png", FALSE, NULL);
	pixbuf = gdk_pixbuf_new_from_file (file, &error);
	
	if (error) {
		g_warning (G_STRLOC ": cannot open %s: %s", file, error->message);
		g_error_free (error);
	}
	g_free (file);
	
	about = gnome_about_new (_("GNOME Archive Generator"), VERSION,
				 "(C) 2001 George Lebl",
				 _("Drag files in to make archives"),
				 (const gchar **)authors,
				 (const gchar **)documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 pixbuf);
				 
	if (pixbuf) {
		gdk_pixbuf_unref (pixbuf);
	}			 
	
	g_signal_connect (G_OBJECT (about), "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &about);
	gtk_widget_show (about);
}

static void
quit_cb (GtkWidget *item)
{
	gtk_main_quit ();
}

static void
select_all_cb (GtkWidget *w, gpointer data)
{
	gint i, num_icons;

	num_icons = gnome_icon_list_get_num_icons(GNOME_ICON_LIST (icon_list));
	for (i = 0; i < num_icons; i++) {
		gnome_icon_list_select_icon (GNOME_ICON_LIST (icon_list), i);
	}
        gnome_appbar_push (GNOME_APPBAR (app_bar), 
                           _("All files selected"));
}

static gboolean
file_remove_fe (gpointer key, gpointer value, gpointer user_data)
{
	File *f = value;
	free_file (f);
	return TRUE;
}

static void
clear_cb (GtkWidget *w, gpointer data)
{
	g_hash_table_foreach_remove (file_ht, file_remove_fe, NULL);
	number_of_files = number_of_dirs = 0;
	gnome_icon_list_clear (GNOME_ICON_LIST (icon_list));

        gnome_appbar_push (GNOME_APPBAR (app_bar), 
                           _("All files removed"));

	gtk_widget_set_sensitive (compress_button, FALSE);
	gtk_widget_set_sensitive (create_archive_widget, FALSE);
	gtk_widget_set_sensitive (remove_widget, FALSE);
	gtk_widget_set_sensitive (clear_widget, FALSE);
	gtk_widget_set_sensitive (selectall_widget, FALSE);
	update_status ();
}

static void
icon_select_cb (GnomeIconList *iconlist,
		gint arg1,
		GdkEvent *event,
		gpointer user_data)
{
	gtk_widget_set_sensitive (remove_widget, TRUE);
}

static void
icon_unselect_cb (GnomeIconList *iconlist,
		  gint arg1,
		  GdkEvent *event,
		  gpointer user_data) 
{
	gtk_widget_set_sensitive (remove_widget, FALSE);
}

static void
remove_cb (GtkWidget *w, gpointer data)
{
	GList *list;
	
	list = gnome_icon_list_get_selection(GNOME_ICON_LIST (icon_list));
	while (list != NULL) {
		gint pos = GPOINTER_TO_INT (list->data);
		remove_pos (pos);
		list = gnome_icon_list_get_selection
			(GNOME_ICON_LIST (icon_list));
	}

	if (number_of_files + number_of_dirs <= 0) {
		gtk_widget_set_sensitive (compress_button, FALSE);
		gtk_widget_set_sensitive (create_archive_widget, FALSE);
		gtk_widget_set_sensitive (clear_widget, FALSE);
		gtk_widget_set_sensitive (selectall_widget, FALSE);
	}
}

static void
save_ok (GtkWidget *widget, GtkFileSelection *fsel)
{
	gchar *fname;
        gchar *status_msg;
        gchar *tmpfname = NULL;

	g_return_if_fail (GTK_IS_FILE_SELECTION(fsel));

	setup_busy (GTK_WIDGET (fsel), TRUE);

	fname = (gchar *)gtk_file_selection_get_filename (fsel);
	if (fname == NULL || fname[0] == '\0') {
		ERRDLGP (_("No filename selected"), fsel);
		setup_busy (GTK_WIDGET (fsel), FALSE);
		return;
	} else if (access (fname, F_OK) == 0 &&
		   ! query_dialog (_("File exists, overwrite?"))) {
		setup_busy (GTK_WIDGET (fsel), FALSE);
		return;
	} else if ( ! create_archive (GTK_WIDGET (fsel),
				      fname,
				      NULL /*dir*/,
				      TRUE /* gui_errors */)) {
		/* the above should do error dialog itself */
		return;
	}


	/* Added code for appending .tar.gz to archive file */
	tmpfname = g_strstr_len ((gchar *)fname, strlen((gchar *)fname), ".tar.gz");
	if (tmpfname == NULL)
		tmpfname = g_strstr_len ((gchar *)fname, strlen((gchar *)fname), ".tgz");
	if (tmpfname != NULL) {
		if (!((g_ascii_strcasecmp(tmpfname, ".tar.gz") == 0) || (g_ascii_strcasecmp(tmpfname, ".tgz") == 0)))
			fname = g_strconcat ((gchar *)fname, ".tar.gz", NULL);
	} else {
		fname = g_strconcat ((gchar *)fname, ".tar.gz", NULL);
	}

        status_msg = g_strdup_printf(_("%s created"), g_path_get_basename(fname));
	g_free (filename);
	filename = g_strdup (fname);
        gnome_appbar_push (GNOME_APPBAR (app_bar), 
                           status_msg);
       g_free (status_msg);
}

static void
archive_cb (GtkWidget *w, gpointer data)
{
	GtkFileSelection *fsel;

	if (number_of_files + number_of_dirs <= 0) {
		ERRDLG (_("No items to archive"));
		return;
	}

	fsel = GTK_FILE_SELECTION(gtk_file_selection_new(_("Save Results")));
	if (filename != NULL)
		gtk_file_selection_set_filename (fsel, filename);

	g_signal_connect (G_OBJECT (fsel->ok_button), "clicked",
			  G_CALLBACK (save_ok), fsel);
	g_signal_connect_swapped
		(G_OBJECT (fsel->cancel_button), "clicked",
		 G_CALLBACK (gtk_widget_destroy), 
		 fsel);

	gtk_window_set_position (GTK_WINDOW (fsel), GTK_WIN_POS_MOUSE);

	gtk_window_set_transient_for (GTK_WINDOW (fsel),
				      GTK_WINDOW (app));

	gtk_widget_show (GTK_WIDGET (fsel));
}

static void
add_file (const gchar *file)
{
	struct stat s;
	File *f, *oldf;
	gint type;
	const gchar *icon;
	gint pos;
	gint i;
	const gchar *base_name;
	gchar *fullname;
        gchar *status_msg;

	if (file == NULL ||
	    file[0] == '\0')
		return;

	fullname = g_strdup (file);

	while (strcmp (fullname, "/") != 0 &&
	       fullname[0] != '\0' &&
	       fullname[strlen(fullname)-1] == '/') {
		fullname[strlen(fullname)-1] = '\0';
	}

	if (strcmp (fullname, "/") != 0)
		base_name = g_path_get_basename (fullname);
	else
		base_name = "/";

	oldf = g_hash_table_lookup (file_ht, base_name);
	i = 1;
	while (oldf != NULL) {
		gchar *temp;
		if (strcmp (oldf->name, fullname) == 0) {
			g_free (fullname);
			return;
		}
		temp = g_strdup_printf ("%s-%d", base_name, i++);
		oldf = g_hash_table_lookup (file_ht, temp);
		g_free (temp);
	}

	if (access (fullname, R_OK) != 0) {
		/* FIXME: perhaps an error of some sort ??? */
		g_free (fullname);
		return;
	}

	if (stat (fullname, &s) < 0) {
		/* FIXME: perhaps an error of some sort ??? */
		g_free (fullname);
		return;
	}

	type = 0;
	icon = NULL;
	if (S_ISDIR (s.st_mode)) {
		type = TYPE_FOLDER;
		icon = folder_icon;
		number_of_dirs ++;
	} else if (S_ISREG (s.st_mode) ||
		   S_ISLNK (s.st_mode)) {
		type = TYPE_REGULAR;
		icon = file_icon;
		number_of_files ++;
	} else {
		/* FIXME: error of some sort */
		g_free (fullname);
		return;
	}

	gtk_widget_set_sensitive (compress_button, TRUE);
	gtk_widget_set_sensitive (create_archive_widget, TRUE);
	gtk_widget_set_sensitive (clear_widget, TRUE);
	gtk_widget_set_sensitive (selectall_widget, TRUE);

        filename = g_strdup_printf("%s/",g_path_get_dirname(fullname));

	f = g_new0 (File, 1);
	f->type = type;
	f->name = g_strdup (fullname);
	if (oldf == NULL) {
		f->base_name = g_strdup (base_name);
	} else {
		i = 1;
		do {
			g_free (f->base_name);
			f->base_name = g_strdup_printf ("%s-%d",
							base_name, i++);
		} while (g_hash_table_lookup (file_ht, f->base_name) != NULL);
	}

	pos = gnome_icon_list_append (GNOME_ICON_LIST (icon_list), icon,
				      f->base_name);
	gnome_icon_list_set_icon_data (GNOME_ICON_LIST (icon_list), pos, f);

	g_hash_table_insert (file_ht, f->base_name, f);
    
	update_status ();
        status_msg = g_strdup_printf(_("%s added"), base_name);
        gnome_appbar_push (GNOME_APPBAR (app_bar), 
                           status_msg);
        g_free (status_msg);
	g_free (fullname);
}

static void
add_ok (GtkWidget *widget, GtkFileSelection *fsel)
{
	gchar *fname;

	g_return_if_fail (GTK_IS_FILE_SELECTION(fsel));

	fname = (gchar *)gtk_file_selection_get_filename (fsel);
	if (fname == NULL || fname[0] == '\0') {
		ERRDLGP (_("No filename selected"), fsel);
		return;
	} else if (access (fname, F_OK) != 0) {
		ERRDLGP (_("File does not exists"), fsel);
		return;
	}
	
	add_file (fname);

	gtk_widget_destroy (GTK_WIDGET (fsel));
}

static void
add_cb (GtkWidget *w, gpointer data)
{
	GtkFileSelection *fsel;

	fsel = GTK_FILE_SELECTION(gtk_file_selection_new(_("Add file or folder")));
	if (filename != NULL)
		gtk_file_selection_set_filename (fsel, filename);

	g_signal_connect (G_OBJECT (fsel->ok_button), "clicked",
			  G_CALLBACK (add_ok), fsel);
	g_signal_connect_swapped
		(G_OBJECT (fsel->cancel_button), "clicked",
		 G_CALLBACK (gtk_widget_destroy), 
		 fsel);

	gtk_window_set_position (GTK_WINDOW (fsel), GTK_WIN_POS_MOUSE);

	gtk_window_set_transient_for (GTK_WINDOW (fsel),
				      GTK_WINDOW (app));

	gtk_widget_show (GTK_WIDGET (fsel));
}


static void
drag_data_received (GtkWidget          *widget,
		    GdkDragContext     *context,
		    gint                x,
		    gint                y,
		    GtkSelectionData   *data,
		    guint               info,
		    guint               time)
{
	if (data->length >= 0 &&
	    data->format == 8) {
		gint i;
		gchar **files = g_strsplit ((gchar *)data->data, "\r\n", -1);
		for (i = 0; files != NULL && files[i] != NULL; i++) {
			/* FIXME: EVIL!!!! */
			if (strncmp (files[i], "file:", strlen ("file:")) == 0)
				add_file (files[i] + strlen ("file:"));
		}
		g_strfreev (files);
		gtk_drag_finish (context, TRUE, FALSE, time);
	} else {
		gtk_drag_finish (context, FALSE, FALSE, time);
	}
}

static void
drag_data_get (GtkWidget          *widget,
	       GdkDragContext     *context,
	       GtkSelectionData   *selection_data,
	       guint               info,
	       guint               time,
	       gpointer            data)
{
	gchar *string;

	if ( ! ensure_temporary ()) {
		/*FIXME: cancel the drag*/
		return;
	}

	string = g_strdup_printf ("file:%s\r\n", temporary_file);
	gtk_selection_data_set (selection_data,
				selection_data->target,
				8, string, strlen (string)+1);
	g_free (string);
}
GtkWidget*
archive_button_new_with_image (const gchar* text, const gchar* image_path)
{
        GtkWidget *button;
        GtkStockItem item;
        GtkWidget *label;
        GtkWidget *image;
        GtkWidget *hbox;
        GtkWidget *align;

        button = gtk_button_new ();

        if (GTK_BIN (button)->child)
                gtk_container_remove (GTK_CONTAINER (button),
                                      GTK_BIN (button)->child);

	if (image) {
                label = gtk_label_new_with_mnemonic (text);

                gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

                image = gtk_image_new_from_file (image_path);
                hbox = gtk_hbox_new (FALSE, 2);

                align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

                gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
                gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

                gtk_container_add (GTK_CONTAINER (button), align);
                gtk_container_add (GTK_CONTAINER (align), hbox);
                gtk_widget_show_all (align);

                return button;
        }

        label = gtk_label_new_with_mnemonic (text);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

        gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

        gtk_widget_show (label);
        gtk_container_add (GTK_CONTAINER (button), label);

        return button;
}


/* Menus */
static GnomeUIInfo file_menu[] = {
        GNOMEUIINFO_ITEM_STOCK 
		(N_("_Add file or folder..."),
                 N_("Add a file or folder to archive"),
                 add_cb, GTK_STOCK_DND_MULTIPLE),

	{ GNOME_APP_UI_ITEM, N_("_Create archive..."), N_("Create a new archive from the items"), 
	(gpointer) archive_cb, NULL, NULL, GNOME_APP_PIXMAP_FILENAME, "document-icons/gnome-compressed.png", 
	0, (GdkModifierType) 0, NULL },

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_QUIT_ITEM (quit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit_menu[] = {
	GNOMEUIINFO_ITEM_NONE (N_("_Remove"),
			       N_("Remove the selected item"),
			       remove_cb),
	GNOMEUIINFO_ITEM_NONE (N_("R_emove All"),
			       N_("Remove all items"),
			       clear_cb),
	GNOMEUIINFO_MENU_SELECT_ALL_ITEM (select_all_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP ("archive-generator"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (about_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo grinder_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file_menu),
	GNOMEUIINFO_MENU_EDIT_TREE (edit_menu),
	GNOMEUIINFO_MENU_HELP_TREE (help_menu),
        GNOMEUIINFO_END
};

static void
init_gui (void)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *sw;
	GtkWidget *w;
	GtkWidget *image;
	GtkTooltips *tips;

	tips = gtk_tooltips_new ();

        app = gnome_app_new ("archive-generator",
			     _("GNOME Archive Generator"));
	gtk_window_set_wmclass (GTK_WINDOW (app),
				"archive-generator",
				"archive-generator");
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);

	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/document-icons/gnome-compressed.png");
        
	gnome_app_create_menus (GNOME_APP (app), grinder_menu);

        g_signal_connect (G_OBJECT (app), "delete_event",
			  G_CALLBACK (quit_cb), NULL);


	vbox = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_size_request (sw, 250, 150);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (sw);
	gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

	icon_list = gnome_icon_list_new (/*evil*/66, NULL, 0);
	gnome_icon_list_set_selection_mode  (GNOME_ICON_LIST (icon_list),
					     GTK_SELECTION_MULTIPLE);

	g_signal_connect (G_OBJECT (icon_list), "select_icon",
			  G_CALLBACK (icon_select_cb), NULL);	

	g_signal_connect (G_OBJECT (icon_list), "unselect_icon",
			  G_CALLBACK (icon_unselect_cb), NULL);	

	gtk_widget_show (icon_list);
	gtk_container_add (GTK_CONTAINER (sw), icon_list);

	gtk_tooltips_set_tip (tips, icon_list,
			      _("Drop files here to add them to the archive."),
			      NULL);

	w = gtk_hseparator_new ();
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	/* tarball icon */
	compress_button = archive_button_new_with_image (_("_Create Archive"), 
							 compress_icon);
	gtk_widget_show (compress_button);

	gtk_box_pack_start (GTK_BOX (hbox), compress_button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (compress_button), "clicked",
			  G_CALLBACK (archive_cb), NULL);
	gtk_tooltips_set_tip (tips, compress_button,
			      _("Drag this button to the destination where you "
				"want the archive to be created or press it to "
				"pop up a file selection dialog."),
			      NULL);

	gtk_widget_set_sensitive (compress_button, FALSE);
	gtk_widget_set_sensitive (file_menu[1].widget, FALSE);
	gtk_widget_set_sensitive (edit_menu[0].widget, FALSE);
	gtk_widget_set_sensitive (edit_menu[1].widget, FALSE);
	gtk_widget_set_sensitive (edit_menu[2].widget, FALSE);

	/* setup dnd */
	gtk_drag_source_set (compress_button,
			     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			     drag_targets, n_drag_targets,
			     GDK_ACTION_COPY);
	/* just in case some wanker like nautilus took our image */
	g_signal_connect (G_OBJECT (compress_button), "drag_begin",
			  G_CALLBACK (start_temporary), NULL);
	g_signal_connect (G_OBJECT (compress_button), "drag_data_get",
			  G_CALLBACK (drag_data_get), NULL);

	status_label = gtk_label_new ("");
	gtk_label_set_justify (GTK_LABEL (status_label), GTK_JUSTIFY_LEFT);
	gtk_widget_show (status_label);
	gtk_box_pack_start (GTK_BOX (hbox), status_label, FALSE, FALSE, 0);
	update_status ();

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	/* setup dnd */
	gtk_drag_dest_set (app,
			   GTK_DEST_DEFAULT_ALL,
			   drop_targets, n_drop_targets,
			   GDK_ACTION_LINK);
	g_signal_connect (G_OBJECT (app), "drag_data_received",
			  G_CALLBACK (drag_data_received), NULL);

	gnome_app_set_contents (GNOME_APP (app), vbox);

        app_bar = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_NEVER);

	gnome_app_set_statusbar (GNOME_APP (app), app_bar);
        progress_bar = GTK_WIDGET (gnome_appbar_get_progress (
                                GNOME_APPBAR (app_bar)));
        g_object_set (G_OBJECT (progress_bar),
                      "pulse_step", 0.1,
                      NULL);
	
	/*set up the menu*/
	gnome_app_install_menu_hints (GNOME_APP (app), grinder_menu);

	gtk_widget_show_all (vbox);
	gtk_widget_show (app);
}

static void
got_signal (gint sig)
{
	cleanup_temporary ();
	
	/* whack thyself */
	signal (sig, SIG_DFL);
	kill (getpid (), sig);
}

static gint
save_session (GnomeClient *client, gint phase,
 	      GnomeRestartStyle save_style, gint shutdown,
 	      GnomeInteractStyle interact_style, gint fast,
 	      gpointer client_data)
{
 	gchar *argv[] = { NULL };
 
 	argv[0] = (gchar *) client_data;
 	gnome_client_set_clone_command (client, 1, argv);
 	gnome_client_set_restart_command (client, 1, argv);
 
 	return TRUE;
}
 
static gint
die (GnomeClient *client, gpointer client_data)
{
	gtk_main_quit ();
}

gint
main (gint argc, gchar *argv [])
{
	gint i;
	poptContext ctx;
	const gchar **files;
	GnomeClient *client;
	
	/* Initialize the i18n stuff */
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gnome_program_init ("archive-generator", VERSION, LIBGNOMEUI_MODULE,
			    argc, argv, GNOME_PARAM_POPT_TABLE,
			    &options,GNOME_PARAM_APP_DATADIR,DATADIR,NULL);

	client = gnome_master_client ();

	g_signal_connect (client, "save_yourself",
			  G_CALLBACK (save_session), (gpointer)argv[0]);

	g_signal_connect (client, "die", G_CALLBACK (die), NULL);

	file_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "mc/i-regular.png", TRUE, NULL);
	if (file_icon == NULL)
		file_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "nautilus/i-regular.png", TRUE, NULL);
	if (file_icon == NULL)
		file_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "nautilus/gnome/i-regular.png", TRUE, NULL);
	if (file_icon == NULL)
		file_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "gnome-file.png", TRUE, NULL);
	if (file_icon == NULL)
		file_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "gnome-unknown.png", TRUE, NULL);

	folder_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "gnome-folder.png", TRUE, NULL);
	if (folder_icon == NULL)
		folder_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "nautilus/gnome-folder.png", TRUE, NULL);
	if (folder_icon == NULL)
		folder_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "nautilus/gnome/gnome/gnome-folder.png", TRUE, NULL);
	if (folder_icon == NULL)
		folder_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "mc/i-directory.png", TRUE, NULL);
	if (folder_icon == NULL)
		folder_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "gnome-unknown.png", TRUE, NULL);

	compress_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "document-icons/gnome-compressed.png", TRUE, NULL);
	if (compress_icon == NULL)
		compress_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "nautilus/gnome-compressed.png", TRUE, NULL);
	if (compress_icon == NULL)
		compress_icon = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "mc/gnome-compressed.png", TRUE, NULL);

	if (folder_icon == NULL || file_icon == NULL) {
		ERRDLG (_("Cannot find proper icons anywhere!"));
		exit (1);
	}

	tar_prog = g_find_program_in_path ("gtar");
	if (tar_prog == NULL)
		tar_prog = g_find_program_in_path ("tar");
	if (tar_prog == NULL) {
		ERRDLG (_("Cannot find the archive (tar) program!\n"
			  "This is the program used for creating archives."));
		exit (1);
	}
	sh_prog = g_find_program_in_path ("sh");
	if (sh_prog == NULL) {
		g_print("Cannot find the shell (sh) program! Aborting");
		exit (1);
	}

	gzip_prog = g_find_program_in_path ("gzip");
	if (gzip_prog == NULL) {
		g_print("Cannot find the compression (gzip) program!\n"
			 "This is the program used for compressing archives.\n");
		exit (1);
	}

	ctx = poptGetContext (PACKAGE, argc, (const char **) argv, &options, 0);

	files = poptGetArgs (ctx);

	init_gui ();

	create_archive_widget = file_menu[1].widget;
	remove_widget = edit_menu[0].widget;
	clear_widget = edit_menu[1].widget;
	selectall_widget = edit_menu[2].widget;

	file_ht = g_hash_table_new (g_str_hash, g_str_equal);

	gnome_icon_list_freeze (GNOME_ICON_LIST (icon_list));

	for (i = 0; files != NULL && files[i] != NULL; i++) {
		if (files[i][0] == '/') {
			add_file (files[i]);
		} else {
			gchar *curdir = g_get_current_dir ();
			gchar *file = g_build_filename ((curdir), (files[i]), NULL);
			add_file (file);
			g_free (file);
			g_free (curdir);
		}
	}

	gnome_icon_list_thaw (GNOME_ICON_LIST (icon_list));

	signal (SIGINT, got_signal);
	signal (SIGTERM, got_signal);

	gtk_main ();

	cleanup_temporary ();

	g_free(tar_prog);
	g_free(sh_prog);
	g_free(gzip_prog);
	
	return 0;
}

