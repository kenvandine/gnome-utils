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
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include "userprefs.h"
#include <sys/stat.h>

#define LOG_CANVAS_H             400
#define LOG_CANVAS_W             600

#define GCONF_WIDTH_KEY  "/apps/gnome-system-log/width"
#define GCONF_HEIGHT_KEY "/apps/gnome-system-log/height"
#define GCONF_LOGFILE "/apps/gnome-system-log/logfile"
#define GCONF_LOGFILES "/apps/gnome-system-log/logfiles"
#define GCONF_FONTSIZE_KEY "/apps/gnome-system-log/fontsize"

GSList*
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

void
prefs_create_defaults (UserPrefsStruct *prefs)
{
	int i;
	gchar *logfiles[] = {"/var/adm/messages",
                       "/var/log/messages",
                       "/var/log/sys.log",
                       "/var/log/secure",
                       "/var/log/maillog",
                       "/var/log/cron",
                       "/var/log/XFree86.0.log"};
	struct stat filestat;
	GSList *logs = NULL;
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	
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
		prefs->logs = logs;
		prefs->logfile = logs->data;
	}
}

void
prefs_free_loglist (UserPrefsStruct *prefs)
{
	GSList *list;
	g_slist_free (prefs->logs);
	prefs->logs = NULL;
}

UserPrefsStruct *
prefs_load (GConfClient *client)
{
	gchar *logfile = NULL;
	int width, height, fontsize;
	UserPrefsStruct *prefs;
	GSList *list;
	gboolean found;

	prefs = g_new0 (UserPrefsStruct, 1);

	prefs->logs = gconf_client_get_list (client, GCONF_LOGFILES, GCONF_VALUE_STRING, NULL);
	if (prefs->logs == NULL)
		prefs_create_defaults (prefs);

	logfile = gconf_client_get_string (client, GCONF_LOGFILE, NULL);
	if (logfile != NULL && strcmp (logfile, "") && file_is_log(logfile, FALSE)) {
		prefs->logfile = g_strdup (logfile);
		g_free (logfile);
	}

	found = FALSE;
	for (list = prefs->logs; list!=NULL; list = g_slist_next (list)) {
		if (g_ascii_strncasecmp (list->data, prefs->logfile, 255) == 0)
			found = TRUE;
	}

	width = gconf_client_get_int (client, GCONF_WIDTH_KEY, NULL);
	height = gconf_client_get_int (client, GCONF_HEIGHT_KEY, NULL);
	fontsize = gconf_client_get_int (client, GCONF_FONTSIZE_KEY, NULL);

	prefs->width = (width == 0 ? LOG_CANVAS_W : width);
	prefs->height = (height == 0 ? LOG_CANVAS_H : height);
	prefs->fontsize = fontsize;

	return prefs;
}

void
prefs_save (GConfClient *client, UserPrefsStruct *prefs)
{
  GSList *logs;
	if (gconf_client_key_is_writable (client, GCONF_LOGFILE, NULL) &&
	    prefs->logfile != NULL) {
		gconf_client_set_string (client, GCONF_LOGFILE, prefs->logfile, NULL);
	}
	if (prefs->width > 0 && prefs->height > 0) {
		if (gconf_client_key_is_writable (client, GCONF_WIDTH_KEY, NULL))
			gconf_client_set_int (client, GCONF_WIDTH_KEY, prefs->width, NULL);
		if (gconf_client_key_is_writable (client, GCONF_HEIGHT_KEY, NULL))
			gconf_client_set_int (client, GCONF_HEIGHT_KEY, prefs->height, NULL);
	}
	if (prefs->logs != NULL)
		if (gconf_client_key_is_writable (client, GCONF_LOGFILES, NULL))
			gconf_client_set_list (client, GCONF_LOGFILES, GCONF_VALUE_STRING, prefs->logs, NULL);

	if (prefs->fontsize > 0)
		if (gconf_client_key_is_writable (client, GCONF_FONTSIZE_KEY, NULL))
			gconf_client_set_int (client, GCONF_FONTSIZE_KEY, prefs->fontsize, NULL);
}

void
prefs_store_size (GtkWidget *window, UserPrefsStruct *prefs)
{
	int width, height;
	gtk_window_get_size (GTK_WINDOW(window), &width, &height);
	/* FIXME : we should check the state of the window, maximized or not */
	if (width > 0 && height > 0) {
		prefs->width = width;
		prefs->height = height;
	}
}

