/* GNOME Search Tool 
 * (C) 1998,2000 the Free Software Foundation
 *
 * Author:   George Lebl
 */

#define GTK_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
#define GNOME_DISABLE_DEPRECATED
#define G_DISABLE_DEPRECATED

#include <config.h>
#include <gnome.h>
#include <string.h>

#include <bonobo-activation/bonobo-activation.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h> 
#include <gdk/gdkkeysyms.h>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>

#include "gsearchtool.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define ICON_SIZE 20
#define ISSLASH(C) ((C) == '/')
#define BACKSLASH_IS_PATH_SEPARATOR ISSLASH ('\\')
#define C_STANDARD_STRFTIME_CHARACTERS "aAbBcdHIjmMpSUwWxXyYZ"
#define C_STANDARD_NUMERIC_STRFTIME_CHARACTERS "dHIjmMSUwWyY"

typedef enum {
	NOT_RUNNING,
	RUNNING,
	MAKE_IT_STOP,
	MAKE_IT_QUIT
} RunLevel;

enum {
	COLUMN_ICON,
	COLUMN_NAME,
	COLUMN_PATH,
	COLUMN_READABLE_SIZE,
	COLUMN_SIZE,
	COLUMN_TYPE,
	COLUMN_READABLE_DATE,
	COLUMN_DATE,
	COLUMN_NO_FILES_FOUND,
	NUM_COLUMNS
};

const FindOptionTemplate templates[] = {
	{ FIND_OPTION_TEXT, "-name '%s'", N_("File name is") },
	{ FIND_OPTION_TEXT, "'!' -name '%s'", N_("File name is not") }, 
	{ FIND_OPTION_BOOL, "-mount", N_("Don't search mounted filesystems") },
	{ FIND_OPTION_BOOL, "-follow", N_("Follow symbolic links") },	
	{ FIND_OPTION_BOOL, "-depth", N_("Process folder contents depth first") },
	{ FIND_OPTION_TEXT, "-user '%s'", N_("Owned by user") },
	{ FIND_OPTION_TEXT, "-group '%s'", N_("Owned by group") },
	{ FIND_OPTION_BOOL, "\\( -nouser -o -nogroup \\)", N_("Invalid user or group") },
	{ FIND_OPTION_TIME, "-mtime '%s'", N_("Last time modified") },
	{ FIND_OPTION_BOOL, "-size 0c \\( -type f -o -type d \\)", N_("File is empty") },
	{ FIND_OPTION_TEXT, "-regex '%s'", N_("Matches regular expression") },
	{ FIND_OPTION_END, NULL,NULL}
};

/*this will not include the directories in search*/
const static char defoptions[] = "'!' -type d";
/*this should be done if the above is made optional*/
/*char defoptions[] = "-mindepth 0";*/

GList *criteria_find = NULL;
gboolean  gail_loaded = FALSE;

static GtkWidget *find_box = NULL;

static GtkWidget *start_dir_e = NULL;

static GtkWidget *locate_entry = NULL;

static GtkWidget *locate_path_entry = NULL;

static GtkWidget *locate_browse_entry = NULL;

static GtkWidget *nbook = NULL;

static GtkWidget *app = NULL; 

static GtkWidget *status_bar = NULL;

static GtkWidget *progress_bar = NULL;

static GtkWidget *file_selector = NULL;

static GtkWidget *locate_results = NULL;

static GtkWidget *find_results = NULL;

static GtkWidget *locate_tree = NULL;

static GtkWidget *find_tree = NULL;

static GtkWidget *locate_buttons[2];

static GtkWidget *find_buttons[2];

static GtkListStore *locate_model = NULL;

static GtkListStore *find_model = NULL;

static GtkTreeSelection *locate_selection = NULL;

static GtkTreeSelection *find_selection = NULL;

static GtkTreeIter locate_iter;

static GtkTreeIter find_iter;

static RunLevel find_running = NOT_RUNNING;
static RunLevel locate_running = NOT_RUNNING;
static gboolean lock = FALSE;

static int current_template = 0;

static char *filename = NULL;
static gchar *locate_string = NULL;

static GtkWidget *save_widget = NULL;

static int
count_char (const char *s, char p)
{
	int cnt = 0;
	for(;*s;s++) {
		if(*s == p)
			cnt++;
	}
	return cnt;
}

static char *
quote_quote_string (const char *s)
{
	GString *gs;

	if(count_char(s, '\'') == 0)
		return g_strdup(s);
	gs = g_string_new("");
	for(;*s;s++) {
		if(*s == '\'')
			g_string_append(gs,"'\\''");
		else
			g_string_append_c(gs,*s);
	}

	return g_string_free (gs, FALSE);
}

static char *
make_find_cmd (const char *start_dir)
{
	GString *cmdbuf;
	GList *list;
	gchar *escape_dir = NULL;
	gchar *regex = NULL;

	
	if (criteria_find==NULL) return NULL;

	cmdbuf = g_string_new ("");

	if(start_dir) {
		escape_dir = g_strescape(start_dir, NULL);
		g_string_append_printf(cmdbuf, "exec find \"%s\" %s ", escape_dir, defoptions);
	} else
		g_string_append_printf(cmdbuf, "exec find . %s ", defoptions);

	for(list=criteria_find;list!=NULL;list=g_list_next(list)) {
		FindOption *opt = list->data;
		if(opt->enabled) {
			char *s;
			switch(templates[opt->templ].type) {
			case FIND_OPTION_BOOL:
				g_string_append_printf(cmdbuf,"%s ",
						  templates[opt->templ].option);
				break;
			case FIND_OPTION_TEXT:
				if (!strcmp(templates[opt->templ].option,"-regex '%s'"))
					regex=quote_quote_string(opt->data.text);
				else {
					gchar *locale_s = NULL;
					s = quote_quote_string(opt->data.text);
					locale_s = g_locale_from_utf8(s, -1, NULL, NULL, NULL);
					g_string_append_printf(cmdbuf,
						  templates[opt->templ].option,
						  locale_s);
					g_free(locale_s);
					g_free(s);
					g_string_append_c(cmdbuf, ' ');
				}
				break;
			case FIND_OPTION_NUMBER:
				g_string_append_printf(cmdbuf,
						  templates[opt->templ].option,
						  opt->data.number);
				g_string_append_c(cmdbuf, ' ');
				break;
			case FIND_OPTION_TIME:
				{
					gchar *locale_s = NULL;
					s = quote_quote_string(opt->data.time);
					locale_s = g_locale_from_utf8(s, -1, NULL, NULL, NULL);
					g_string_append_printf(cmdbuf,
						  templates[opt->templ].option,
						  locale_s);
					g_free(locale_s);
					g_free(s);
					g_string_append_c(cmdbuf, ' ');
				}
				break;
			default:
			        break;
			}
		}
	}
	g_string_append (cmdbuf, "-print ");

	if(regex!=NULL) 
	{	
		gchar *locale_regex = NULL;
		locale_regex = g_locale_from_utf8 (regex, -1, NULL, NULL, NULL);
		g_string_append_printf (cmdbuf, " |egrep '%s'", locale_regex);
		g_free (locale_regex);
	}
	g_free(escape_dir);

	return g_string_free(cmdbuf, FALSE);
}

static char *
make_locate_cmd(void)
{
	GString *cmdbuf;
	gchar *locate_path;
	gchar *locate_command;
	gchar *locale_locate_path;
	gchar *locale_locate_string;
	
	locate_string = 
		(gchar *)gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(locate_entry))));
	locate_string = quote_quote_string(locate_string);

	if(!locate_string || !*locate_string) {
		return NULL;
	}
	
	if(locate_path_entry) {
		locate_path = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(locate_path_entry), TRUE);
		if(!locate_path) {
			gnome_app_error (GNOME_APP(app),
					 _("Start folder does not exist."));
			return NULL;
		}
	} else
		locate_path = NULL;

	locale_locate_path = g_locale_from_utf8(locate_path, -1, NULL, NULL, NULL);
	locale_locate_string = g_locale_from_utf8(locate_string, -1, NULL, NULL, NULL);

	cmdbuf = g_string_new ("");
	locate_command = g_find_program_in_path ("locate");

	if (locate_command != NULL)
	{
		g_string_append_printf (cmdbuf, "exec %s '%s*%s'", locate_command, locale_locate_path,
					locale_locate_string);
	} else {
		g_string_append_printf (cmdbuf, "exec find \"%s\" -name '%s' -mount", locale_locate_path, 
					locale_locate_string);
	}	
	g_free (locate_path);
	g_free (locate_command);
	g_free (locale_locate_path);
	g_free (locale_locate_string);
	
	return g_string_free (cmdbuf, FALSE);
}

static gboolean
kill_after_nth_nl (GString *str, int n)
{
	int i;
	int count = 0;
	for (i = 0; str->str[i] != '\0'; i++) {
		if (str->str[i] == '\n') {
			count++;
			if (count == n) {
				g_string_truncate (str, i);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static gchar *
get_icon_of_file_with_mime_type (const gchar *file_name, const gchar *mime_type) 
{
	const char *icon_name; 
	gchar *icon_path;
	gchar *tmp;
	
	icon_name = gnome_vfs_mime_get_icon(mime_type);
	if (g_file_test ((icon_name), G_FILE_TEST_EXISTS)) {
		icon_path = g_strdup (icon_name);
		return icon_path;
	}
	
	if (icon_name == NULL)
		icon_name = g_strdup ("i-regular.png");
	
	if (g_file_test ((file_name), G_FILE_TEST_IS_DIR)) 
		icon_name = g_strdup ("i-directory-24.png");

	icon_path = gnome_vfs_icon_path_from_filename (icon_name);

	if (icon_path == NULL) {
		tmp = g_strconcat (icon_name, ".png", NULL);
		icon_path = gnome_vfs_icon_path_from_filename (tmp);
		g_free (tmp);
	}

	if (icon_path == NULL) {
		tmp = g_strconcat ("document-icons/", icon_name, NULL);
		icon_path = gnome_vfs_icon_path_from_filename (tmp);
		g_free (tmp);
	}

	if (icon_path == NULL) {
		tmp = g_strconcat ("document-icons/", icon_name, ".png", NULL);
		icon_path = gnome_vfs_icon_path_from_filename (tmp);
		g_free (tmp);
	}

	if (icon_path == NULL) {
		tmp = g_strconcat ("nautilus/gnome/", icon_name, NULL);
		icon_path = gnome_vfs_icon_path_from_filename (tmp);
		g_free (tmp);
	}

	if (icon_path == NULL) {
		tmp = g_strconcat ("nautilus/gnome/", icon_name, ".png", NULL);
		icon_path = gnome_vfs_icon_path_from_filename (tmp);
		g_free (tmp);
	}

	if (icon_path == NULL)
		icon_path = gnome_vfs_icon_path_from_filename ("nautilus/gnome/i-regular.png");

	return icon_path;
}

static char *
strdup_strftime (const char *format, struct tm *time_pieces)
{
	/* based on code from eel */
	GString *string;
	const char *remainder, *percent;
	char code[3], buffer[512];
	char *piece, *result, *converted;
	size_t string_length;
	gboolean strip_leading_zeros, turn_leading_zeros_to_spaces;

	converted = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
	g_return_val_if_fail (converted != NULL, NULL);
	
	string = g_string_new ("");
	remainder = converted;

	for (;;) {
		percent = strchr (remainder, '%');
		if (percent == NULL) {
			g_string_append (string, remainder);
			break;
		}
		g_string_append_len (string, remainder,
				     percent - remainder);

		remainder = percent + 1;
		switch (*remainder) {
		case '-':
			strip_leading_zeros = TRUE;
			turn_leading_zeros_to_spaces = FALSE;
			remainder++;
			break;
		case '_':
			strip_leading_zeros = FALSE;
			turn_leading_zeros_to_spaces = TRUE;
			remainder++;
			break;
		case '%':
			g_string_append_c (string, '%');
			remainder++;
			continue;
		case '\0':
			g_warning ("Trailing %% passed to strdup_strftime");
			g_string_append_c (string, '%');
			continue;
		default:
			strip_leading_zeros = FALSE;
			turn_leading_zeros_to_spaces = FALSE;
			break;
		}
		
		if (strchr (C_STANDARD_STRFTIME_CHARACTERS, *remainder) == NULL) {
			g_warning ("strdup_strftime does not support "
				   "non-standard escape code %%%c",
				   *remainder);
		}

		code[0] = '%';
		code[1] = *remainder;
		code[2] = '\0';
		string_length = strftime (buffer, sizeof (buffer),
					  code, time_pieces);
		if (string_length == 0) {
			buffer[0] = '\0';
		}

		piece = buffer;
		if (strip_leading_zeros || turn_leading_zeros_to_spaces) {
			if (strchr (C_STANDARD_NUMERIC_STRFTIME_CHARACTERS, *remainder) == NULL) {
				g_warning ("strdup_strftime does not support "
					   "modifier for non-numeric escape code %%%c%c",
					   remainder[-1],
					   *remainder);
			}
			if (*piece == '0') {
				do {
					piece++;
				} while (*piece == '0');
				if (!g_ascii_isdigit (*piece)) {
				    piece--;
				}
			}
			if (turn_leading_zeros_to_spaces) {
				memset (buffer, ' ', piece - buffer);
				piece = buffer;
			}
		}
		remainder++;
		g_string_append (string, piece);
	}
	
	result = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);
	g_string_free (string, TRUE);
	g_free (converted);
	return result;
}

static gchar *
get_readable_date (time_t file_time_raw)
{
	struct tm *file_time;
	gchar *format;
	GDate *today;
	GDate *file_date;
	guint32 file_date_age;
	gchar *readable_date;

	file_time = localtime (&file_time_raw);
	file_date = g_date_new_dmy (file_time->tm_mday,
			       file_time->tm_mon + 1,
			       file_time->tm_year + 1900);
	
	today = g_date_new ();
	g_date_set_time (today, time (NULL));

	file_date_age = g_date_get_julian (today) - g_date_get_julian (file_date);
	 
	g_date_free (today);
	g_date_free (file_date);
			
	/* the format varies depending on age of file. */
	if (file_date_age == 0)	{
		format = g_strdup(_("today at %-I:%M %p"));
	} else if (file_date_age == 1) {
		format = g_strdup(_("yesterday at %-I:%M %p"));
	} else if (file_date_age < 7) {
		format = g_strdup(_("%m/%-d/%y, %-I:%M %p"));
	} else {
		format = g_strdup(_("%m/%-d/%y, %-I:%M %p"));
	}
	
	readable_date = strdup_strftime (format, file_time);
	g_free (format);
	
	return readable_date;
} 

static gchar *
get_basic_date (time_t file_time_raw)
{
	struct tm *file_time;
	const char *format = _("%Y%m%d%H%M%S");

	file_time = localtime (&file_time_raw);
	return strdup_strftime (format, file_time);
} 

static gchar *
get_file_type (const char *mime_type, const char *file)
{
	if (mime_type == NULL) {
		return NULL;
	}

	if (g_ascii_strcasecmp (mime_type, GNOME_VFS_MIME_TYPE_UNKNOWN) == 0) {
		if (g_file_test (file, G_FILE_TEST_IS_DIR))
			return _("folder");
		if (g_file_test (file, G_FILE_TEST_IS_EXECUTABLE))
			return _("program");	
		if (g_file_test (file, G_FILE_TEST_IS_SYMLINK))
			return _("symbolic link");
	}

	return (gchar *)gnome_vfs_mime_get_description (mime_type);
} 

static void
add_file_to_list_store(const gchar *file, GtkListStore *store, GtkTreeIter *iter)
{					
	const gchar *mime_type = gnome_vfs_mime_type_from_name (file);
	gchar *description = get_file_type (mime_type, file);
	gchar *icon_path = get_icon_of_file_with_mime_type (file, mime_type);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
	GnomeVFSFileInfo *vfs_file_info = gnome_vfs_file_info_new ();
	gchar *readable_size, *readable_date, *date;
	gchar *utf8_base_name, *utf8_dir_name;
	gchar *base_name, *dir_name;
	
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
				  		  ICON_SIZE,
				  		  ICON_SIZE,
				  		  GDK_INTERP_BILINEAR);
		g_object_unref (G_OBJECT (pixbuf));
		pixbuf = scaled;	
	}
	gnome_vfs_get_file_info (file, vfs_file_info, GNOME_VFS_FILE_INFO_DEFAULT);
	readable_size = gnome_vfs_format_file_size_for_display (vfs_file_info->size);
	readable_date = get_readable_date (vfs_file_info->mtime);
	date = get_basic_date (vfs_file_info->mtime);
	
	base_name = g_path_get_basename(file);
	dir_name = g_path_get_dirname(file);
	
	utf8_base_name = g_locale_to_utf8 (base_name, -1, NULL, NULL, NULL);
	utf8_dir_name = g_locale_to_utf8 (dir_name, -1, NULL, NULL, NULL);

	gtk_list_store_append (GTK_LIST_STORE(store), iter); 
	gtk_list_store_set (GTK_LIST_STORE(store), iter,
			    COLUMN_ICON, pixbuf, 
			    COLUMN_NAME, utf8_base_name,
			    COLUMN_PATH, utf8_dir_name,
			    COLUMN_READABLE_SIZE, readable_size,
			    COLUMN_SIZE, (gdouble) vfs_file_info->size,
			    COLUMN_TYPE, description,
			    COLUMN_READABLE_DATE, readable_date,
			    COLUMN_DATE, date,
			    COLUMN_NO_FILES_FOUND, FALSE,
			    -1);
			    
	gnome_vfs_file_info_unref (vfs_file_info);
	g_object_unref (G_OBJECT (pixbuf));
	g_free (base_name);
	g_free (dir_name);
	g_free (utf8_base_name);
	g_free (utf8_dir_name);
	g_free (icon_path); 
	g_free (readable_size);
	g_free (readable_date);
	g_free (date);
}


/*
 * add_atk_namedesc
 * @widget    : The Gtk Widget for which @name and @desc are added.
 * @name      : Accessible Name
 * @desc      : Accessible Description
 * Description: This function adds accessible name and description to a
 *              Gtk widget.
 */

void
add_atk_namedesc(GtkWidget *widget, const gchar *name, const gchar *desc)
{
	AtkObject *atk_widget;

	g_return_if_fail (GTK_IS_WIDGET(widget));

	atk_widget = gtk_widget_get_accessible(widget);

	if (name != NULL)
		atk_object_set_name(atk_widget, name);
	if (desc !=NULL)
		atk_object_set_description(atk_widget, desc);
}


/*
 * add_atk_relation
 * @obj1      : The first widget in the relation @rel_type
 * @obj2      : The second widget in the relation @rel_type.
 * @rel_type  : Relation type which relates @obj1 and @obj2
 * Description: This function establishes Atk Relation between two given
 *              objects.
 */

void
add_atk_relation(GtkWidget *obj1, GtkWidget *obj2, AtkRelationType rel_type)
{
	AtkObject *atk_obj1, *atk_obj2;
	AtkRelationSet *relation_set;
	AtkRelation *relation;
	
	g_return_if_fail (GTK_IS_WIDGET(obj1));
	g_return_if_fail (GTK_IS_WIDGET(obj2));
	
	atk_obj1 = gtk_widget_get_accessible(obj1);
			
	atk_obj2 = gtk_widget_get_accessible(obj2);
	
	relation_set = atk_object_ref_relation_set (atk_obj1);
	relation = atk_relation_new(&atk_obj2, 1, rel_type);
	atk_relation_set_add(relation_set, relation);
	g_object_unref(G_OBJECT (relation));
	
}


static void
add_no_files_found_to_list_store(GtkListStore *store, GtkTreeIter *iter)
{
	/* when the list is empty append a 'No Files Found.' message */
	gtk_list_store_append (GTK_LIST_STORE(store), iter); 
	gtk_list_store_set (GTK_LIST_STORE(store), iter,
		    		COLUMN_ICON, NULL, 
				COLUMN_NAME, _("No Files Found."),
		    		COLUMN_PATH, "",
			    	COLUMN_READABLE_SIZE, "",
			    	COLUMN_SIZE, (gdouble) 0,
			    	COLUMN_TYPE, "",
		    		COLUMN_READABLE_DATE, "",
		    		COLUMN_DATE, 0,
				COLUMN_NO_FILES_FOUND, TRUE,
		    		-1);
}

static gint
update_progress_bar (gpointer p)
{
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress_bar));
	
	return TRUE;
}

static void
really_run_command(char *cmd, char sepchar, gchar *utf8_pattern_str, RunLevel *running, GtkWidget *tree, GtkListStore *store, GtkTreeIter *iter)
{
	int timeout;
	GString *string;
	char ret[PIPE_READ_BUFFER];
	int fd[2], fderr[2];
	int i,n;
	int pid;
	int files_found;
	char* str;
	GPatternSpec *pattern;
	GString *errors = NULL;
	gboolean add_to_errors = TRUE;
	
	lock = TRUE;

	pattern = g_pattern_spec_new (utf8_pattern_str);
	/* reset scroll position and clear the tree view */
	gtk_tree_view_scroll_to_point (GTK_TREE_VIEW(tree), 0, 0);
	gtk_list_store_clear (GTK_LIST_STORE(store)); 
	
	/* running = NOT_RUNNING, RUNNING, MAKE_IT_STOP */
	*running = RUNNING;
	
	pipe(fd);
	pipe(fderr);
	
	if ( (pid = fork()) == 0) {
		close(fd[0]);
		close(fderr[0]);
		
		close(STDOUT); 
		close(STDIN);
		close(STDERR);

		dup2(fd[1],STDOUT);
		dup2(fderr[1],STDERR);

		close(fd[1]);
		close(fderr[1]);

		{
			gchar *shell = NULL;

			shell = g_find_program_in_path ("sh");
			if (shell == NULL)
			{
				GtkWidget *dialog;

				dialog = gtk_message_dialog_new(NULL,
					0,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					_("Cannot perform the search.\n"
					"The Bourne shell was not found\n"
					"It is typically installed as /bin/sh or /usr/bin/sh."));
				gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (app));
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			}
			execl(shell, shell, "-c", cmd, NULL);
			g_free(shell);
		}
		_exit(0); /* in case it fails */
	}
	close(fd[1]);
	close(fderr[1]);

	gnome_appbar_pop (GNOME_APPBAR (status_bar));
	gnome_appbar_push (GNOME_APPBAR (status_bar), _("Searching..."));
	timeout = gtk_timeout_add (100, update_progress_bar, NULL);
		
	fcntl(fd[0], F_SETFL, O_NONBLOCK);
	fcntl(fderr[0], F_SETFL, O_NONBLOCK);

	string = g_string_new (NULL);

	while (*running == RUNNING) {
		n = read (fd[0], ret, PIPE_READ_BUFFER);
		for (i = 0; i < n; i++) {
			if (*running != RUNNING)
				break;
			if(ret[i] == sepchar) {
				gchar *locale_base_name = NULL;
				gchar *base_name = NULL;

				base_name = g_path_get_basename (string->str);
			        locale_base_name = g_locale_to_utf8 (base_name, -1, NULL, NULL, NULL);
				
				if (g_pattern_match_string (pattern, locale_base_name)) {
				        add_file_to_list_store (string->str, store, iter);
				}

				g_string_assign (string, "");
				g_free (locale_base_name);
				g_free (base_name);
			} else {
				g_string_append_c (string, ret[i]);
			}
			if(gtk_events_pending())
				gtk_main_iteration_do (TRUE);
		}

		n = read (fderr[0], ret, PIPE_READ_BUFFER-1);
		if (n > 0) {
			ret[n] = '\0';
			if (add_to_errors) {
				if (strstr(ret, "ermission denied") == NULL) {
					if (errors == NULL)
						errors = g_string_new (ret);
					else
						errors = g_string_append (errors, ret);
					
					add_to_errors =
						! kill_after_nth_nl (errors, 20);
				}
			}
			fprintf (stderr, "%s", ret);
		}
		
		if (waitpid (-1, NULL, WNOHANG) == -1)
			break;
		if(gtk_events_pending())
			gtk_main_iteration_do(TRUE);
		if ((*running == MAKE_IT_STOP) || (*running == MAKE_IT_QUIT)) {
			kill(pid, SIGKILL);
			wait(NULL);
		}
	}
	/* now we got it all ... so finish reading from the pipe */
	while ((n = read (fd[0], ret, PIPE_READ_BUFFER)) > 0) {
		for (i = 0; i < n; i++) {
			if (*running != RUNNING) 
				break;
			if(ret[i] == sepchar) {
				gchar *locale_base_name = NULL;
				gchar *base_name = NULL;
				
				base_name = g_path_get_basename (string->str);
			        locale_base_name = g_locale_to_utf8 (base_name, -1, NULL, NULL, NULL);
				
				if (g_pattern_match_string (pattern, locale_base_name)) {
				        add_file_to_list_store (string->str, store, iter);
				}
				
				g_string_assign (string, "");
				g_free (locale_base_name);
				g_free (base_name);
			} else {
				g_string_append_c (string, ret[i]);
			}
			if(gtk_events_pending())
				gtk_main_iteration_do (TRUE);
		}
	}
	while((n = read(fderr[0], ret, PIPE_READ_BUFFER-1)) > 0) {
		ret[n]='\0';
		if (add_to_errors) {
			if (strstr(ret, "ermission denied") == NULL) {			
				if (errors == NULL)
					errors = g_string_new (ret);
				else
					errors = g_string_append (errors, ret); 
				
				add_to_errors =
						! kill_after_nth_nl (errors, 20);
			}
		}
		fprintf (stderr, "%s", ret);
	}

	close(fd[0]);
	close(fderr[0]);
	
	gtk_timeout_remove (timeout);
	gnome_appbar_pop (GNOME_APPBAR (status_bar));
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0.0);
		
	/* if any errors occured */
	if(errors) {
		if ( ! add_to_errors) {
			errors = g_string_append
				(errors,
				 _("\n... Too many errors to display ..."));
		}
		/* make an error message */
		gnome_app_error (GNOME_APP (app), errors->str);
		/* freeing allocated memory */
		g_string_free (errors, TRUE);
	}

	files_found = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(store), NULL);

	if (files_found == 0)
		str = g_strdup (_("No files found"));
	else
		if (files_found == 1)
			str = g_strdup (_("One file found"));
		else
			str = g_strdup_printf (_("%d files found"), files_found);
	
	gnome_appbar_push (GNOME_APPBAR (status_bar), str);

	if (files_found == 0)
		add_no_files_found_to_list_store (store, iter);
	
	g_free (str);
	g_string_free (string, TRUE);
	g_pattern_spec_free (pattern);

	if (*running == MAKE_IT_QUIT)
	{
	 	*running = NOT_RUNNING;
		lock = FALSE;
		gtk_main_quit ();
 	}
 	else
 	{
		*running = NOT_RUNNING;
		lock = FALSE;
	}
}

static void
run_command(GtkWidget *w, gpointer data)
{
	char *cmd;
	GtkWidget **buttons = data;

	char *start_dir;

	gtk_widget_set_sensitive(save_widget, FALSE);

	if (buttons == find_buttons)
	{
		gtk_widget_set_sensitive (locate_buttons[0], FALSE);
		gtk_widget_set_sensitive (locate_buttons[1], FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (find_buttons[0], FALSE);
		gtk_widget_set_sensitive (find_buttons[1], FALSE);
	}
	
	if (buttons[0] == w) { /*we are in the stop button*/
		if(find_running == RUNNING) {
			find_running = MAKE_IT_STOP;
			gtk_widget_set_sensitive(buttons[1], FALSE);
			gtk_widget_set_sensitive(save_widget, TRUE);
			gtk_widget_hide(buttons[0]);
			gtk_widget_show(buttons[1]);
		}

		goto out;
	}

	if(start_dir_e) {
		start_dir = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(start_dir_e), TRUE);
		if(!start_dir) {
			gnome_app_error (GNOME_APP(app),
					 _("Start folder does not exist."));
			goto out;
		}
	} else
		start_dir = NULL;

	cmd = make_find_cmd(start_dir);
	g_free(start_dir);

	if (cmd == NULL) {
		gnome_app_error (GNOME_APP(app),
				 _("Cannot perform the search. Please specify a search criteria."));
		goto out;
	}

	if (!lock) {
		gtk_widget_set_sensitive(find_results, TRUE);
		gtk_widget_set_sensitive(buttons[0], TRUE);
		gtk_widget_show(buttons[0]);
		gtk_widget_hide(buttons[1]);

		really_run_command(cmd, '\n', "*", &find_running, find_tree, find_model, &find_iter);
		
		gtk_widget_set_sensitive(buttons[1], TRUE);
		gtk_widget_hide(buttons[0]);
		gtk_widget_show(buttons[1]);
	} else {
		gnome_app_error(GNOME_APP(app),
				_("A search is already running.  Please wait for the search "
				  "to complete or cancel it."));
	}
	gtk_widget_set_sensitive(save_widget, TRUE);
	g_free(cmd);

out:
	if (buttons == find_buttons)
	{
		gtk_widget_set_sensitive (locate_buttons[0], TRUE);
		gtk_widget_set_sensitive (locate_buttons[1], TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (find_buttons[0], TRUE);
		gtk_widget_set_sensitive (find_buttons[1], TRUE);
	}

}

static void
run_cmd_dialog(GtkWidget *wid, gpointer data)
{
	char *cmd;
	char *start_dir;
	gchar *locale_cmd = NULL;
	GtkWidget *dlg;
	GtkWidget *w, *label;
	GtkWidget *vbox;

	if(start_dir_e) {
		start_dir = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(start_dir_e), TRUE);
		if(!start_dir) {
			gnome_app_error (GNOME_APP(app),
					 _("Start folder does not exist"));
			return;
		}
	} else
		start_dir = NULL;

	if (!gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook))) 
		cmd = make_locate_cmd();
	else
		cmd = make_find_cmd(start_dir);
		
	g_free(start_dir);
	
	if (cmd == NULL) {
		gnome_app_error (GNOME_APP(app),
				 _("Cannot show the search command line. Please specify a search criteria."));
		return;
	}
	
	dlg = gtk_dialog_new_with_buttons(_("Search Command Line"), GTK_WINDOW(app),
			       GTK_DIALOG_DESTROY_WITH_PARENT,
			       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			       NULL);
	
	gtk_window_set_modal (GTK_WINDOW(dlg), TRUE);
				      
	g_object_set (G_OBJECT (dlg),
		      "allow_grow", FALSE,
		      "allow_shrink", FALSE,
		      "resizable", FALSE,
		      NULL);
	
	vbox = gtk_vbox_new (FALSE, 4);
    	g_object_set (G_OBJECT (vbox), "border_width", 6, NULL);
	
	gtk_dialog_set_default_response (GTK_DIALOG(dlg), GTK_RESPONSE_CLOSE); 

	label = gtk_label_new_with_mnemonic (_("This is the command line that can be used to "
			    "execute this search\nfrom the console."
			    "\n\nC_ommand:"));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
	
	w = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(w), FALSE);
	locale_cmd = g_locale_to_utf8(cmd, -1, NULL, NULL, NULL);
	gtk_entry_set_text(GTK_ENTRY(w), locale_cmd);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), w);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), vbox);

	g_signal_connect_swapped (GTK_OBJECT (dlg), 
                             	  "response", 
                             	  G_CALLBACK (gtk_widget_destroy),
                                  GTK_OBJECT (dlg));
	if (gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET(label), _("Command"), NULL);
		add_atk_namedesc (GTK_WIDGET(w), _("Command Line Entry"), _("Use this command line to execute the search from console"));
		add_atk_relation (w, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
		add_atk_relation (GTK_WIDGET(label), w, ATK_RELATION_LABEL_FOR);
	}

	gtk_widget_show_all(dlg); 
	
	g_free(locale_cmd);
	g_free(cmd);
}

static gboolean
view_file_with_application(char *file)
{
	const char *mimeType = gnome_vfs_mime_type_from_name(file);	
	GnomeVFSMimeApplication *mimeApp = gnome_vfs_mime_get_default_application(mimeType);
		
	if (mimeApp) {
		char **argv;
		int argc = 2;

		argv = g_new0 (char *, argc);
		argv[0] = mimeApp->command;
		argv[1] = file;	
		
		if (mimeApp->requires_terminal) 
			gnome_prepend_terminal_to_vector(&argc, &argv);	
		
		gnome_execute_async(NULL, argc, argv);	
		gnome_vfs_mime_application_free(mimeApp);	
		g_free(argv);
		return TRUE;
	}
	return FALSE;
}

static gboolean
nautilus_is_running()
{
	CORBA_Environment ev;
	CORBA_Object obj;
	gboolean ret;
	
	CORBA_exception_init (&ev); 
	obj = bonobo_activation_activate_from_id ("OAFIID:Nautilus_Factory",
		Bonobo_ACTIVATION_FLAG_EXISTING_ONLY, NULL, &ev);
		
	ret = !CORBA_Object_is_nil (obj, &ev);
	
	CORBA_Object_release (obj, &ev);	
	CORBA_exception_free (&ev);
	
	return ret;
}

static void
launch_nautilus (char *file)
{
	int argc = 2;
	char **argv = g_new0 (char *, argc);
	
	argv[0] = "nautilus";
	argv[1] = file;
	
	gnome_execute_async(NULL, argc, argv);
	g_free(argv);
}

static gboolean
launch_file (GtkWidget *w, GdkEventButton *event, gpointer data)
{
	gchar *file = NULL;
	gchar *locale_file = NULL;
	gchar *utf8_name = NULL;
	gchar *utf8_path = NULL;
	gboolean no_files_found = FALSE;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	if (event->type==GDK_2BUTTON_PRESS) {
		
		if (!gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook))) {
			store = locate_model;
			selection = locate_selection;
		}
		else {
			store = find_model;
			selection = find_selection;
		}
		
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    			return TRUE;
		
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,COLUMN_NAME, &utf8_name,
							       COLUMN_PATH, &utf8_path,
							       COLUMN_NO_FILES_FOUND, &no_files_found,
								-1);

		if (!no_files_found) {
			file = g_build_filename (utf8_path, utf8_name, NULL);
			locale_file = g_locale_from_utf8(file, -1, NULL, NULL, NULL);
			
			if (nautilus_is_running ()) {
				launch_nautilus (locale_file);
			}
			else {
				if (!view_file_with_application(locale_file))	
					gnome_error_dialog_parented(_("No viewer available for this mime type."),
							    	    GTK_WINDOW(app));
			}

			g_free (file);
			g_free (locale_file);
		}
		
		g_free (utf8_name);
		g_free (utf8_path);

		return TRUE;
	}
	return FALSE;
}

static void
quit_cb(GtkWidget *w, gpointer data)
{
	if(find_running == RUNNING)
	{
		find_running = MAKE_IT_QUIT;
		return;
	}
	if(locate_running == RUNNING)
	{
		locate_running = MAKE_IT_QUIT;
		return;
	}
	gtk_main_quit ();
}

static void
menu_toggled(GtkWidget *w, gpointer data)
{
	if(GTK_CHECK_MENU_ITEM(w)->active)
		current_template = (long)data;
}

static GtkWidget *
make_list_of_templates(void)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList *group=NULL;
	int i;

	menu = gtk_menu_new ();

	for(i=0;templates[i].type!=FIND_OPTION_END;i++) {
		menuitem=gtk_radio_menu_item_new_with_label(group,
							    _(templates[i].desc));
		g_signal_connect(G_OBJECT(menuitem),"toggled",
				 G_CALLBACK(menu_toggled),
			         (gpointer)(long)i);
		group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show (menuitem);
	}
	return menu;
}

static void
remove_option(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	
	gtk_container_remove(GTK_CONTAINER(find_box),w->parent);
	criteria_find = g_list_remove(criteria_find,opt);
}

static void
enable_option(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	GtkWidget *frame = GTK_WIDGET (g_object_get_data (G_OBJECT(w), "frame"));
	gtk_widget_set_sensitive(GTK_WIDGET(frame),
				 GTK_TOGGLE_BUTTON(w)->active);
	opt->enabled = GTK_TOGGLE_BUTTON(w)->active;
}

static void
entry_changed(GtkWidget *w, gpointer data)
{
	FindOption *opt = data;
	switch(templates[opt->templ].type) {
	case FIND_OPTION_TEXT:
		opt->data.text = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
		break;
	case FIND_OPTION_NUMBER:
		sscanf(gtk_entry_get_text(GTK_ENTRY(w)),"%d",
		       &opt->data.number);
		break;
	case FIND_OPTION_TIME:
		opt->data.time = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
		break;
	default:
		g_warning(_("Entry changed called for a non entry option!"));
		break;
	}
}

char empty_str[]="";

static void
set_option_defaults(FindOption *opt)
{
	switch(templates[opt->templ].type) {
	case FIND_OPTION_BOOL:
		break;
	case FIND_OPTION_TEXT:
		opt->data.text = empty_str;
		break;
	case FIND_OPTION_NUMBER:
		opt->data.number = 0;
		break;
	case FIND_OPTION_TIME:
		opt->data.time = empty_str;
		break;
	default:
	        break;
	}
}

/*
 * This function starts advanced search when the user presses Enter/Return
 */

static void
advanced_search_activate (GtkWidget *entry, gpointer data)
{
	GtkWidget **buttons = data;
	run_command (buttons[1], buttons);
}

static GtkWidget *
create_option_box(FindOption *opt, gboolean enabled)
{
	GtkWidget *hbox;
	GtkWidget *option;
	GtkWidget *frame;
	GtkWidget *w, *label;

	hbox = gtk_hbox_new(FALSE,5);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(hbox),frame,TRUE,TRUE,0);

	switch(templates[opt->templ].type) {
	case FIND_OPTION_BOOL:
		option = gtk_label_new(_(templates[opt->templ].desc));
		gtk_misc_set_alignment(GTK_MISC(option), 0.0, 0.5);
		break;
	case FIND_OPTION_TEXT:
	case FIND_OPTION_NUMBER:
	case FIND_OPTION_TIME:
		option = gtk_hbox_new(FALSE,5);
		label = gtk_label_new(_(templates[opt->templ].desc));
		gtk_box_pack_start(GTK_BOX(option),label,FALSE,FALSE,0);
		w = gtk_entry_new();
		g_signal_connect(G_OBJECT(w),"changed",
				 G_CALLBACK(entry_changed),opt);
		gtk_box_pack_start(GTK_BOX(option),w,TRUE,TRUE,0);

		if (gail_loaded)
		{
			add_atk_namedesc (GTK_WIDGET(w), _("Search Rule Value Entry"), _("Enter a value for search rule"));
			add_atk_relation (w, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
			add_atk_relation (GTK_WIDGET(label), w, ATK_RELATION_LABEL_FOR);
		}
		g_signal_connect (G_OBJECT (w), "activate",
				  G_CALLBACK (advanced_search_activate),
				  find_buttons);
		break;
	default:
		/* This should never happen, if it does, there is a bug */
		option = gtk_label_new("???");
	        break;
	}
	gtk_container_add(GTK_CONTAINER(frame), option);

	w = gtk_check_button_new_with_label(_("Enabled"));
	g_object_set_data (G_OBJECT (w), "frame", frame);
	g_signal_connect(G_OBJECT(w), "toggled",
			 G_CALLBACK(enable_option), opt);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), enabled);
	enable_option(w, opt);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE,FALSE,0);
	if (gail_loaded)
		add_atk_namedesc (GTK_WIDGET(w), NULL, _("Toggle On/Off to enable/disable the rule"));

	w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	gtk_widget_set_size_request (GTK_WIDGET(w), 88, -1);
	g_signal_connect(G_OBJECT(w), "clicked",
			 G_CALLBACK(remove_option), opt);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE,FALSE, 0);
	if (gail_loaded)
		add_atk_namedesc (GTK_WIDGET(w), NULL, _("Click to Remove the Rule"));

	return hbox;
}

static void
add_option(int templ, gboolean enabled)
{
	FindOption *opt = g_new(FindOption,1);
	GtkWidget *w;

	opt->templ = templ;
	opt->enabled = TRUE;

	set_option_defaults(opt);
	
	w = create_option_box(opt, enabled);
	gtk_widget_show_all(w);

	criteria_find = g_list_append(criteria_find,opt);
	gtk_box_pack_start(GTK_BOX(find_box),w,FALSE,FALSE,0);
}

static void
add_option_cb(GtkWidget *w, gpointer data)
{
	add_option(current_template, TRUE);
}

static GtkWidget *
create_find_page(void)
{
	GtkWidget *vbox, *vbox2;
	GtkWidget *hbox, *hbox2;
	GtkWidget *label, *w, *gnome_entry;
	GtkWidget *vpaned, *image, *align;
	GtkWidget *frame, *frame2;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	char *s;

	vbox = gtk_vbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),GNOME_PAD_SMALL);

	hbox = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,GNOME_PAD_SMALL);
	label = gtk_label_new_with_mnemonic(_("S_tarting in folder:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	start_dir_e = gnome_file_entry_new("directory", _("Browse"));
	gnome_file_entry_set_directory_entry(GNOME_FILE_ENTRY(start_dir_e), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox),start_dir_e,TRUE,TRUE,GNOME_PAD_SMALL);
	w = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(start_dir_e));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
	gnome_entry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY(start_dir_e));
	if (gail_loaded)
	{ 
		add_atk_namedesc (GTK_WIDGET(gnome_entry), _("Starting folder entry"), _("Enter the folder name where you want to start the search"));
		add_atk_namedesc (GTK_WIDGET(w), _("Starting folder entry"), _("Enter the folder name where you want to start the search"));
		add_atk_relation (gnome_entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY); 
	}
	/* Clicking on 'OK' in the Browse dialog will start a search -- commenting out until someone can figure this out
	g_signal_connect (G_OBJECT (gnome_entry), "activate",
			  G_CALLBACK (advanced_search_activate),
			  find_buttons);
	*/

	s = g_get_current_dir();
	gtk_entry_set_text(GTK_ENTRY(w), s);
	g_free(s);

	find_box = gtk_vbox_new(FALSE,0);
	
	vpaned = gtk_vpaned_new();
	frame2 = gtk_frame_new(NULL);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_widget_set_size_request (vpaned, 200, 270);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_label ( GTK_FRAME(frame), _("Rules:"));
	w = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(w),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(w), find_box);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_frame_set_shadow_type(GTK_FRAME(frame2), GTK_SHADOW_NONE);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(w), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX(vbox),vpaned,TRUE,TRUE,0);
	
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(w));
	gtk_box_pack_start (GTK_BOX(vbox2),frame,TRUE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame2), GTK_WIDGET(vbox2));
	gtk_paned_pack1 (GTK_PANED (vpaned), frame2, TRUE, FALSE);
	gtk_widget_set_size_request (frame2, -1, 150);
	
	hbox = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox2),hbox,FALSE,FALSE,0);
	label = gtk_label_new_with_mnemonic(_("A_dd Rule:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	w = gtk_option_menu_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), GTK_WIDGET(w));
	gtk_option_menu_set_menu(GTK_OPTION_MENU(w), make_list_of_templates());
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);

	if (gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET(w), _("Search Rules Menu"), _("Select a search rule from the menu"));
		add_atk_relation (w, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY);
	}

	w = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_set_size_request (GTK_WIDGET(w), 88, -1);
	g_signal_connect(G_OBJECT(w),"clicked",
			 G_CALLBACK(add_option_cb),NULL);
	add_option(0, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox),w,FALSE,FALSE,0);
	
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox2),hbox,FALSE,FALSE,0);

	/* create find button */
	find_buttons[1] = gtk_button_new ();
	gtk_widget_set_size_request (GTK_WIDGET(find_buttons[1]), 88, -1);
	
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_container_add( GTK_CONTAINER (find_buttons[1]), align);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(align), hbox2);
	
	image = gtk_image_new_from_stock("gtk-find", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX(hbox2), image, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic(_("_Find"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	
	find_buttons[0] = gtk_button_new_from_stock(GTK_STOCK_STOP);
	gtk_widget_set_size_request (GTK_WIDGET(find_buttons[0]), 88, -1);
	g_signal_connect(G_OBJECT(find_buttons[1]),"clicked",
			 G_CALLBACK(run_command), find_buttons);
	g_signal_connect(G_OBJECT(find_buttons[0]),"clicked",
			 G_CALLBACK(run_command), find_buttons);
	gtk_box_pack_end(GTK_BOX(hbox),find_buttons[0],FALSE,FALSE,GNOME_PAD_SMALL);
	gtk_box_pack_end(GTK_BOX(hbox),find_buttons[1],FALSE,FALSE,GNOME_PAD_SMALL);
	gtk_widget_set_sensitive(find_buttons[0],FALSE);
 
	if (gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET(w), NULL, _("Click to Add a Rule"));
		add_atk_namedesc (GTK_WIDGET(find_buttons[1]), NULL , _("Click to Start the Search"));
		add_atk_namedesc (GTK_WIDGET(find_buttons[0]), NULL, _("Click to Stop the Search"));
	}
             
	/* create search results section */
	frame = gtk_frame_new(NULL);
	label = gtk_label_new_with_mnemonic (_("S_earch Results"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_paned_pack2 (GTK_PANED (vpaned), frame, TRUE, FALSE);
	gtk_widget_set_size_request (frame, -1, 90);
	find_results = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_usize(find_results,600,90);
	gtk_widget_set_sensitive(find_results, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(find_results), GNOME_PAD_SMALL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(find_results),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
	find_model = gtk_list_store_new (NUM_COLUMNS, 
					 GDK_TYPE_PIXBUF, 
					 G_TYPE_STRING, 
					 G_TYPE_STRING, 
					 G_TYPE_STRING,
					 G_TYPE_DOUBLE,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_BOOLEAN);				 
	
	find_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(find_model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (find_tree), TRUE);
  	g_object_unref (G_OBJECT (find_model));
	
	find_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(find_tree));
	
	g_signal_connect(G_OBJECT(find_tree), "button_press_event",
		         G_CALLBACK(launch_file),NULL);		   

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), find_tree);

	gtk_container_add(GTK_CONTAINER(find_results),GTK_WIDGET(find_tree));
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(find_results));
	
	/* create the name column */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", COLUMN_ICON,
                                             NULL);
					     
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_NAME,
					     NULL);
					     
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 175);
	gtk_tree_view_column_set_resizable (column, TRUE);				     
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(find_tree), column);

	/* create the folder column */
	renderer = gtk_cell_renderer_text_new (); 
	column = gtk_tree_view_column_new_with_attributes (_("Folder"), renderer,
							   "text", COLUMN_PATH,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 125);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_PATH); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(find_tree), column);
	
	/* create the size column */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set( renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer,
							   "text", COLUMN_READABLE_SIZE,
							   NULL);						   
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 75);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
	gtk_tree_view_column_set_alignment (column, 1.0); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(find_tree), column);
 
	/* create the type column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer,
							   "text", COLUMN_TYPE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 75);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_TYPE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(find_tree), column);

	/* create the date modified column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date Modified"), renderer,
							   "text", COLUMN_READABLE_DATE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 75);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(find_tree), column);

	return vbox;
}

static void
run_locate_command(GtkWidget *w, gpointer data)
{
	gchar *cmd;
	GtkWidget **buttons = data;

        gtk_widget_set_sensitive(save_widget, FALSE);

	if (buttons == find_buttons)
	{
		gtk_widget_set_sensitive (locate_buttons[0], FALSE);
		gtk_widget_set_sensitive (locate_buttons[1], FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (find_buttons[0], FALSE);
		gtk_widget_set_sensitive (find_buttons[1], FALSE);
	}

	if (buttons[0] == w) { /*we are in the stop button*/
		if(locate_running == RUNNING)
			locate_running = MAKE_IT_STOP;
			gtk_widget_hide(buttons[0]);
			gtk_widget_show(buttons[1]);
			gtk_widget_set_sensitive(buttons[1], FALSE);
			gtk_widget_set_sensitive(save_widget, TRUE);
		goto out;
	}

	cmd = make_locate_cmd();
	
	if (cmd == NULL) {
		gnome_app_error (GNOME_APP(app),
				 _("Cannot perform the search. "
				   "Please specify a search criteria."));
		goto out;
	}
	
	if (!lock) {	
		gtk_widget_show(buttons[0]);
		gtk_widget_set_sensitive(buttons[0], TRUE);
		gtk_widget_hide(buttons[1]);
		gtk_widget_set_sensitive(buttons[1], FALSE);
		gtk_widget_set_sensitive(locate_results, TRUE);

		really_run_command(cmd, '\n', locate_string, &locate_running, locate_tree, locate_model, &locate_iter);
			
		gtk_widget_hide(buttons[0]);
		gtk_widget_set_sensitive(buttons[0], FALSE);
		gtk_widget_show(buttons[1]);
		gtk_widget_set_sensitive(buttons[1], TRUE);

	} else {
		gnome_app_error(GNOME_APP(app),
				_("A search is already running.  Please wait for the search "
				  "to complete or cancel it."));
	}
	
	gtk_widget_set_sensitive(save_widget, TRUE);
	
	g_free(cmd);

out:
	if (buttons == find_buttons)
	{
		gtk_widget_set_sensitive (locate_buttons[0], TRUE);
		gtk_widget_set_sensitive (locate_buttons[1], TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (find_buttons[0], TRUE);
		gtk_widget_set_sensitive (find_buttons[1], TRUE);
	}


}

static void
locate_activate (GtkWidget *entry, gpointer data)
{
	GtkWidget **buttons = data;
	run_locate_command (buttons[1], buttons);
}

/*
 * This function stops the search when user presses Escape while the search
 * is underway.
 */

static gboolean
stop_search (GtkWidget *w, GdkEventKey *event)
{
	GtkWidget **buttons;
	g_return_if_fail (GTK_IS_WIDGET(w));

	if (event->keyval == GDK_Escape)
	{
		if (locate_running == RUNNING)
		{
			locate_running = MAKE_IT_STOP;
			return TRUE;
		}
		else if (find_running == RUNNING)
		{
			find_running = MAKE_IT_STOP;
			return TRUE;
		}
	}
	return FALSE;
}

static GtkWidget *
create_locate_page(void)
{
	GtkWidget *w, *vbox, *hbox, *hbox2, *entry, *table;
	GtkWidget *image, *label, *frame, *align;
	GtkTreeViewColumn *column;	
	static gchar *history = NULL;
	GtkCellRenderer *renderer;
	gchar *s;

	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), GNOME_PAD_SMALL);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, GNOME_PAD_SMALL);
	
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (hbox), table);
	
	w = gtk_label_new_with_mnemonic(_("Find files _named:"));
	gtk_label_set_justify (GTK_LABEL (w), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (w), "xalign", 0.0, NULL);
	
	gtk_table_attach(GTK_TABLE(table), w, 0, 1, 0, 1, GTK_FILL, 0, 0, 1);

	locate_entry = gnome_entry_new(history);
	gtk_label_set_mnemonic_widget(GTK_LABEL(w), gnome_entry_gtk_entry(GNOME_ENTRY(locate_entry)));
	gnome_entry_set_max_saved(GNOME_ENTRY(locate_entry), 10);
	gtk_table_attach(GTK_TABLE(table), locate_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	entry =  gnome_entry_gtk_entry (GNOME_ENTRY(locate_entry));
       
	if (GTK_IS_ACCESSIBLE (gtk_widget_get_accessible(locate_entry)))
	{
		gail_loaded = TRUE;
		add_atk_namedesc (locate_entry, _("File Name Entry"), _("Enter the file name you want to search"));
		add_atk_namedesc (entry, _("File Name Entry"), _("Enter the file name you want to search"));
		add_atk_relation (locate_entry, GTK_WIDGET(w), ATK_RELATION_LABELLED_BY);
	}	
     
	g_signal_connect (G_OBJECT (locate_entry), "activate",
			  G_CALLBACK (locate_activate),
			  locate_buttons);
			  
	label = gtk_label_new_with_mnemonic(_("S_tarting in folder:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	
	locate_path_entry = gnome_file_entry_new("directory", _("Browse"));
	gnome_file_entry_set_directory_entry(GNOME_FILE_ENTRY(locate_path_entry), TRUE);
	w = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(locate_path_entry));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
	locate_browse_entry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY(locate_path_entry));
	gtk_table_attach(GTK_TABLE(table), locate_path_entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	
	if (gail_loaded)
	{ 
		add_atk_namedesc (GTK_WIDGET(locate_browse_entry), _("Starting folder entry"), _("Enter the folder name where you want to start the search"));
		add_atk_namedesc (GTK_WIDGET(w), _("Starting folder entry"), _("Enter the folder name where you want to start the search"));
		add_atk_relation (locate_browse_entry, GTK_WIDGET(label), ATK_RELATION_LABELLED_BY); 
	}
	/* Clicking on 'OK' in the Browse dialog will start a search -- commenting out until someone can figure this out
	g_signal_connect (G_OBJECT (locate_browse_entry), "activate",
			  G_CALLBACK (locate_activate),
			  locate_buttons);
	*/
			  
	s = g_get_current_dir();
	gtk_entry_set_text(GTK_ENTRY(w), s);
	g_free(s);
	
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	/* create a custom find button */
	locate_buttons[1] = gtk_button_new ();
	gtk_widget_set_size_request (GTK_WIDGET(locate_buttons[1]), 88, -1);
	
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_container_add( GTK_CONTAINER (locate_buttons[1]), align);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(align), hbox2);
	
	image = gtk_image_new_from_stock("gtk-find", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX(hbox2), image, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic(_("_Find"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

	locate_buttons[0] = gtk_button_new_from_stock(GTK_STOCK_STOP);
	gtk_widget_set_size_request (GTK_WIDGET(locate_buttons[0]), 88, -1);
	g_signal_connect(G_OBJECT(locate_buttons[1]),"clicked",
			 G_CALLBACK(run_locate_command), locate_buttons);
	g_signal_connect(G_OBJECT(locate_buttons[0]),"clicked",
			 G_CALLBACK(run_locate_command), locate_buttons);
	gtk_box_pack_end(GTK_BOX(hbox),locate_buttons[0],FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(hbox),locate_buttons[1],FALSE,FALSE,0);
	gtk_widget_set_sensitive(locate_buttons[0],FALSE);
  
	if (gail_loaded)
	{
		add_atk_namedesc (GTK_WIDGET(locate_buttons[1]), NULL, _("Click to Start the search"));
		add_atk_namedesc (GTK_WIDGET(locate_buttons[0]), NULL, _("Click to Stop the search"));
	}

	/* create search results section */
	frame = gtk_frame_new(NULL);
	label = gtk_label_new_with_mnemonic (_("S_earch Results"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_box_pack_start(GTK_BOX(vbox),frame,TRUE,TRUE,0);
	locate_results = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(locate_results), FALSE); 
	gtk_container_set_border_width(GTK_CONTAINER(locate_results), GNOME_PAD_SMALL);
	gtk_widget_set_usize(locate_results,600,260);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(locate_results),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
	
	locate_model = gtk_list_store_new(NUM_COLUMNS, 
					  GDK_TYPE_PIXBUF, 
					  G_TYPE_STRING, 
					  G_TYPE_STRING, 
					  G_TYPE_STRING,
					  G_TYPE_DOUBLE,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_BOOLEAN);
	
	locate_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(locate_model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (locate_tree), TRUE);
  	g_object_unref (G_OBJECT (locate_model));
	
	locate_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(locate_tree));
	
	g_signal_connect(G_OBJECT(locate_tree), "button_press_event",
		         G_CALLBACK(launch_file),NULL);		   

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), locate_tree);
	
	gtk_container_add(GTK_CONTAINER(locate_results),GTK_WIDGET(locate_tree));
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(locate_results));
	
	/* create the name column */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", COLUMN_ICON,
                                             NULL);
					     
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", COLUMN_NAME,
					     NULL);
					     
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 175);
	gtk_tree_view_column_set_resizable (column, TRUE);				     
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(locate_tree), column);

	/* create the folder column */
	renderer = gtk_cell_renderer_text_new (); 
	column = gtk_tree_view_column_new_with_attributes (_("Folder"), renderer,
							   "text", COLUMN_PATH,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 125);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_PATH); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(locate_tree), column);
	
	/* create the size column */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set( renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer,
							   "text", COLUMN_READABLE_SIZE,
							   NULL);						   
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 75);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
	gtk_tree_view_column_set_alignment (column, 1.0); 
	gtk_tree_view_append_column (GTK_TREE_VIEW(locate_tree), column);
 
	/* create the type column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer,
							   "text", COLUMN_TYPE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 75);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_TYPE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(locate_tree), column);

	/* create the date modified column */ 
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date Modified"), renderer,
							   "text", COLUMN_READABLE_DATE,
							   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 75);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(locate_tree), column);


	return vbox;
}

static GtkWidget *
create_window(void)
{
	nbook = gtk_notebook_new();
	gtk_container_set_border_width(GTK_CONTAINER(nbook),GNOME_PAD_SMALL);

	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),create_locate_page(),
				 gtk_label_new (_("Simple")));  /* Can we connect to this  */
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook),create_find_page(),
				 gtk_label_new (_("Advanced")));

	return nbook;
}

static void
about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	GdkPixbuf  	 *pixbuf;
	GError		 *error = NULL;
	gchar		 *file;
	
	static const char *authors[] = {
		"George Lebl <jirka@5z.com>",
		"Dennis M. Cranston <dennis_cranston@yahoo.com>",
		NULL
	};
	gchar *documenters[] = {
		NULL
	};
	/* Translator credits */
	gchar *translator_credits = _("translator_credits");

	if (about != NULL) {
		gtk_window_present (GTK_WINDOW (about));
		return;
	}

	file = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, "gnome-searchtool.png", FALSE, NULL);
	pixbuf = gdk_pixbuf_new_from_file (file, &error);
	
	if (error) {
		g_warning (G_STRLOC ": cannot open %s: %s", file, error->message);
		g_error_free (error);
	}
	g_free (file);

	about = gnome_about_new(_("GNOME Search Tool"), VERSION,
				_("(C) 1998,2002 the Free Software Foundation"),
				_("Frontend to the unix find/locate "
				  "commands"),
				authors,
				(const char **)documenters,
				strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				pixbuf);
	if (pixbuf) {
		gdk_pixbuf_unref (pixbuf);
	}
										
	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW (data));
	g_signal_connect (G_OBJECT (about), "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &about);
	gtk_widget_show (about);
}

static void
save_results(GtkFileSelection *selector, gpointer user_data)
{
	FILE *fp;
	GtkListStore *store;
	GtkTreeIter iter;
	gint n_children, i;
	
	if (!gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook)))
		store = locate_model;
	else
		store = find_model;
	
	filename = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));
	
	if (access (filename, F_OK) == 0) {
		GtkWidget *dialog;
		gint response;
		
		dialog = gtk_message_dialog_new
			(GTK_WINDOW (file_selector),
			 0 /* flags */,
			 GTK_MESSAGE_QUESTION,
			 GTK_BUTTONS_YES_NO,
			 _("File %s already exists. Overwrite?"),
			 filename);
		gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW (app));
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		
		gtk_widget_destroy (GTK_WIDGET(dialog));
		
		if (response != GTK_RESPONSE_YES) return;
	}
	
	if (fp = fopen(filename, "w")) {
	
		if (gtk_tree_model_get_iter_root(GTK_TREE_MODEL(store), &iter)) {
			
			n_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL);
			
			for (i = 0; i < n_children; i++)
			{
				gchar *utf8_path, *utf8_name, *file;
				
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_PATH, &utf8_path, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COLUMN_NAME, &utf8_name, -1);
				gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);	
				
				file = g_build_filename (utf8_path, utf8_name, NULL);					    
				fprintf(fp, "%s\n", file);
				
				g_free(utf8_path);
				g_free(utf8_name);
				g_free(file);
			}
		}		 
		fclose(fp);
	} 
	else {
		gnome_app_error(GNOME_APP(app), _("Cannot save the results file."));
	}
}

static void
show_file_selector(GtkWidget *widget)
{
	file_selector = gtk_file_selection_new(_("Save Results"));
		
	if (filename) gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), filename);

	g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button), "clicked",
				G_CALLBACK (save_results), NULL);
	
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                             	"clicked",
                             	G_CALLBACK (gtk_widget_destroy), 
                             	(gpointer) file_selector); 

   	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
                             	"clicked",
                             	G_CALLBACK (gtk_widget_destroy),
                             	(gpointer) file_selector); 

	gtk_window_set_modal (GTK_WINDOW(file_selector), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW(file_selector), GTK_WINDOW(app));
	gtk_window_set_position (GTK_WINDOW (file_selector), GTK_WIN_POS_MOUSE);

	gtk_widget_show (GTK_WIDGET(file_selector));
}

/* thy evil easter egg */
static gboolean
window_click(GtkWidget *w, GdkEventButton *be)
{
	static int foo = 0;
	if(be->button == 3 && (++foo)%3 == 0)
		gnome_ok_dialog("9\\_/_-\n  /\\ /\\\n\nGEGL!");
	return TRUE;
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
	quit_cb (NULL, NULL);
	return TRUE;
}

static GnomeUIInfo search_menu[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("S_how Command..."), 
				N_("Show the command used to perform the search"), 
				run_cmd_dialog,
				GTK_STOCK_NEW),
	GNOMEUIINFO_ITEM_STOCK (N_("Save Results _As..."), 
			        N_("Save the search results"), 
				show_file_selector,
				GTK_STOCK_SAVE_AS),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_QUIT_ITEM (quit_cb,NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {  
	GNOMEUIINFO_HELP ("gnome-search-tool"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (about_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo gsearch_menu[] = {
	GNOMEUIINFO_SUBTREE (N_("Sear_ch"), search_menu),
	GNOMEUIINFO_MENU_HELP_TREE (help_menu),
        GNOMEUIINFO_END
};

int
main(int argc, char *argv[])
{
	GnomeProgram *gsearchtool;
	GnomeClient *client;
	GdkGeometry hints;
	GtkWidget *search;

	/* Initialize the i18n stuff */
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gsearchtool = gnome_program_init ("gnome-search-tool", VERSION, LIBGNOMEUI_MODULE, argc, argv, GNOME_PARAM_APP_DATADIR,DATADIR, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-searchtool.png");

	if (!bonobo_init (bonobo_activation_orb_get (), NULL, NULL))
		g_error (_("Cannot initialize bonobo."));

	bonobo_activate();

        app = gnome_app_new("gnome-search-tool", _("Search Tool"));
	gtk_window_set_wmclass (GTK_WINDOW (app), "gnome-search-tool", "gnome-search-tool");
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, TRUE);
	
        g_signal_connect(G_OBJECT(app), "delete_event",
			 G_CALLBACK(quit_cb), NULL);

	g_signal_connect(G_OBJECT(app), "button_press_event",
			 G_CALLBACK(window_click), NULL);
	g_signal_connect(G_OBJECT(app), "key_press_event",
		         G_CALLBACK(stop_search), NULL);

	/*set up session management*/		 
	client = gnome_master_client ();		 
			 
	g_signal_connect (client, "save_yourself",
			  G_CALLBACK (save_session), (gpointer)argv[0]);

	g_signal_connect (client, "die", G_CALLBACK (die), NULL);
		
	/* set up status bar */
	status_bar = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_NEVER);
	gnome_app_set_statusbar (GNOME_APP (app), status_bar);
	progress_bar = GTK_WIDGET (gnome_appbar_get_progress (
				GNOME_APPBAR (status_bar)));
	g_object_set (G_OBJECT (progress_bar), 
		      "pulse_step", 0.1,
		      NULL);
				      
	/*set up the menu*/
        gnome_app_create_menus_with_data (GNOME_APP (app), gsearch_menu, app);
	gnome_app_install_menu_hints (GNOME_APP (app), gsearch_menu);

	search = create_window();
	gtk_widget_show_all(search);
	gtk_widget_hide(locate_buttons[0]);
	gtk_widget_hide(find_buttons[0]);

	gnome_app_set_contents(GNOME_APP(app), search);
	
      	hints.min_width = 460;
      	hints.min_height = 360;
	gtk_window_set_geometry_hints(GTK_WINDOW(app), GTK_WIDGET(app),
				      &hints, GDK_HINT_MIN_SIZE);
	
	gtk_widget_show(app);

	gtk_window_set_focus(GTK_WINDOW(app), GTK_WIDGET(gnome_entry_gtk_entry(GNOME_ENTRY(locate_entry))));
        save_widget = search_menu[1].widget;

	gtk_main ();

	return 0;
}
