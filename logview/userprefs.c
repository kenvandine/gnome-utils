/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */


#include <string.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include "userprefs.h"
#include <sys/stat.h>
#include "logview.h"

#define LOG_CANVAS_H             400
#define LOG_CANVAS_W             600

#define GCONF_WIDTH_KEY  "/apps/gnome-system-log/width"
#define GCONF_HEIGHT_KEY "/apps/gnome-system-log/height"
#define GCONF_LOGFILE "/apps/gnome-system-log/logfile"
#define GCONF_LOGFILES "/apps/gnome-system-log/logfiles"
#define GCONF_FONTSIZE_KEY "/apps/gnome-system-log/fontsize"
#define GCONF_MONOSPACE_FONT_NAME "/desktop/gnome/interface/monospace_font_name"
#define GCONF_MENUS_HAVE_TEAROFF "/desktop/gnome/interface/menus_have_tearoff"

static GConfClient *client = NULL;
static UserPrefs *prefs;

static GSList*
parse_syslog(gchar *syslog_file) {

	/* Most of this stolen from sysklogd sources */

	char *logfile = NULL;
	char cbuf[BUFSIZ];
	char *cline, *p;
	FILE *cf;
	GSList *logfiles = NULL;

	if ((cf = fopen(syslog_file, "r")) == NULL) {
		return NULL;
	}
	cline = cbuf;
	while (fgets(cline, sizeof(cbuf) - (cline - cbuf), cf) != NULL) {
		for (p = cline; isspace(*p); ++p);
		if (*p == '\0' || *p == '#')
			continue;
		for (;*p && !strchr("\t ", *p); ++p);
		while (*p == '\t' || *p == ' ')
			p++;
		if (*p == '-')
			p++;
		if (*p == '/') {
			logfile = g_strdup (p);
			/* remove trailing newline */
			if (logfile[strlen(logfile)-1] == '\n')
				logfile[strlen(logfile)-1] = '\0';
			logfiles = g_slist_append (logfiles, logfile);
		}
	}
	return logfiles; 
}

static void
prefs_create_defaults (UserPrefs *p)
{
	int i;
	gchar *logfiles[] = {"/var/adm/messages",
                         "/var/log/messages",
                         "/var/log/sys.log",
                         "/var/log/secure",
                         "/var/log/maillog",
                         "/var/log/cron",
                         "/var/log/Xorg.0.log",
                         "/var/log/XFree86.0.log",
                         "/var/log/auth.log",
                         "/var/log/cups/error_log"};
	struct stat filestat;
	GSList *logs = NULL;
	GnomeVFSResult result;
	GnomeVFSHandle *handle;

    g_assert (p != NULL);
	
	/* For first time running, try parsing various logfiles */
	/* Try to parse syslog.conf to get logfile names */

	result = gnome_vfs_open (&handle, "/etc/syslog.conf", GNOME_VFS_OPEN_READ);
	if (result == GNOME_VFS_OK) {
		gnome_vfs_close (handle);
		logs = parse_syslog ("/etc/syslog.conf");
	}
	
	for (i=0; i<7; i++) {
		if (file_is_log (logfiles[i], FALSE))
			logs = g_slist_append (logs, g_strdup(logfiles[i]));
	}

	if (logs != NULL) {
		p->logs = logs;
		p->logfile = logs->data;
	}
}

static UserPrefs *
prefs_load (GConfClient *client)
{
	gchar *logfile = NULL;
	int width, height, fontsize;
	UserPrefs *p;
	GSList *list;
	gboolean found;
    GError *err;

    g_assert (client != NULL);

	p = g_new0 (UserPrefs, 1);

	p->logs = gconf_client_get_list (client, GCONF_LOGFILES, GCONF_VALUE_STRING, &err);
	if (err != NULL)
		prefs_create_defaults (p);

	logfile = gconf_client_get_string (client, GCONF_LOGFILE, NULL);
	if (logfile != NULL && strcmp (logfile, "") && file_is_log(logfile, FALSE)) {
		p->logfile = g_strdup (logfile);
		g_free (logfile);
	}

	found = FALSE;
	for (list = p->logs; list!=NULL; list = g_slist_next (list)) {
		if (g_ascii_strncasecmp (list->data, p->logfile, 255) == 0)
			found = TRUE;
	}

	width = gconf_client_get_int (client, GCONF_WIDTH_KEY, NULL);
	height = gconf_client_get_int (client, GCONF_HEIGHT_KEY, NULL);
	fontsize = gconf_client_get_int (client, GCONF_FONTSIZE_KEY, NULL);

	p->width = (width == 0 ? LOG_CANVAS_W : width);
	p->height = (height == 0 ? LOG_CANVAS_H : height);
	p->fontsize = fontsize;

	return p;
}

static void
prefs_monospace_font_changed (GConfClient *client, guint id, 
                                GConfEntry *entry, gpointer data)
{
    GtkWidget *view = data;
    gchar *monospace_font_name;
    PangoFontDescription *fontdesc;

    g_assert (client != NULL);
    
    monospace_font_name = gconf_client_get_string (client, GCONF_MONOSPACE_FONT_NAME, NULL);
    widget_set_font (view, monospace_font_name);
    g_free (monospace_font_name);
}

gchar *
prefs_get_monospace (void)
{
    return (gconf_client_get_string (client, GCONF_MONOSPACE_FONT_NAME, NULL));
}

gboolean
prefs_get_have_tearoff (void)
{
    return (gconf_client_get_bool (client, GCONF_MENUS_HAVE_TEAROFF, NULL));
}

GSList *
prefs_get_logs (void)
{
    return prefs->logs;
}

gchar *
prefs_get_active_log (void) 
{
    return prefs->logfile;
}

int
prefs_get_width (void)
{
    return prefs->width;
}

int
prefs_get_height (void)
{
    return prefs->height;
}

void
prefs_free_loglist ()
{
	g_slist_free (prefs->logs);
	prefs->logs = NULL;
}

void
prefs_store_log (gchar *name)
{
    if (name == NULL)
        return;
    prefs->logs = g_slist_append (prefs->logs, name);
}

void
prefs_store_active_log (gchar *name)
{
    /* name can be NULL if no active log */
    prefs->logfile = name;
}

void
prefs_store_fontsize (int fontsize)
{
    prefs->fontsize = fontsize;
}

void
prefs_save (void)
{
  GSList *logs;

    if (gconf_client_key_is_writable (client, GCONF_LOGFILE, NULL))
		gconf_client_set_string (client, GCONF_LOGFILE, prefs->logfile, NULL);

    if (gconf_client_key_is_writable (client, GCONF_LOGFILES, NULL))
        gconf_client_set_list (client, GCONF_LOGFILES, GCONF_VALUE_STRING, prefs->logs, NULL);

	if (prefs->width > 0 && prefs->height > 0) {
		if (gconf_client_key_is_writable (client, GCONF_WIDTH_KEY, NULL))
			gconf_client_set_int (client, GCONF_WIDTH_KEY, prefs->width, NULL);
		if (gconf_client_key_is_writable (client, GCONF_HEIGHT_KEY, NULL))
			gconf_client_set_int (client, GCONF_HEIGHT_KEY, prefs->height, NULL);
	}

	if (prefs->fontsize > 0)
		if (gconf_client_key_is_writable (client, GCONF_FONTSIZE_KEY, NULL))
			gconf_client_set_int (client, GCONF_FONTSIZE_KEY, prefs->fontsize, NULL);
}

void
prefs_store_window_size (GtkWidget *window)
{
	int width, height;

    g_return_if_fail (GTK_IS_WINDOW (window));

	gtk_window_get_size (GTK_WINDOW(window), &width, &height);
	/* FIXME : we should check the state of the window, maximized or not */
	if (width > 0 && height > 0) {
		prefs->width = width;
		prefs->height = height;
	}
}

void 
prefs_connect (LogviewWindow *logview)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

    gconf_client_notify_add (client, GCONF_MONOSPACE_FONT_NAME, 
                             prefs_monospace_font_changed, logview->view, NULL, NULL);
}

void
prefs_init (int argc, char *argv[])
{
    gconf_init (argc, argv, NULL);
    client = gconf_client_get_default ();
    prefs = prefs_load (client);
}

