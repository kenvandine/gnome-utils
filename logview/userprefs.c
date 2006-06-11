/* userprefs.c - logview user preferences handling
 *
 * Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)
 * Copyright (C) 2004  Vincent Noel
 * Copyright (C) 2006  Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include "userprefs.h"
#include <sys/stat.h>
#include "logview.h"

#define LOGVIEW_DEFAULT_HEIGHT 400
#define LOGVIEW_DEFAULT_WIDTH 600

/* logview settings */
#define GCONF_DIR 		"/apps/gnome-system-log"
#define GCONF_WIDTH_KEY 	GCONF_DIR "/width"
#define GCONF_HEIGHT_KEY 	GCONF_DIR "/height"
#define GCONF_LOGFILE 		GCONF_DIR "/logfile"
#define GCONF_LOGFILES 		GCONF_DIR "/logfiles"
#define GCONF_FONTSIZE_KEY 	GCONF_DIR "/fontsize"

/* desktop-wide settings */
#define GCONF_MONOSPACE_FONT_NAME "/desktop/gnome/interface/monospace_font_name"
#define GCONF_MENUS_HAVE_TEAROFF  "/desktop/gnome/interface/menus_have_tearoff"

static GConfClient *gconf_client = NULL;
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
		gchar **list;
		gint i;
		for (p = cline; g_ascii_isspace(*p); ++p);
		if (*p == '\0' || *p == '#' || *p == '\n')
			continue;
		list = g_strsplit_set (p, ", -\t()\n", 0);
		for (i = 0; list[i]; ++i) {
			if (*list[i] == '/' &&
				g_slist_find_custom(logfiles, list[i],
				g_ascii_strcasecmp) == NULL) {
					logfiles = g_slist_insert (logfiles,
						g_strdup(list[i]), 0);
			}
		}
		g_strfreev(list);
	}
	fclose(cf);
	return logfiles;
}

static void
prefs_create_defaults (UserPrefs *p)
{
	int i;
	gchar *logfiles[] = {
		"/var/log/sys.log",
#ifndef ON_SUN_OS
		"/var/log/messages",
		"/var/log/secure",
		"/var/log/maillog",
		"/var/log/cron",
		"/var/log/Xorg.0.log",
		"/var/log/XFree86.0.log",
		"/var/log/auth.log",
		"/var/log/cups/error_log",
#else
		"/var/adm/messages",
		"/var/adm/sulog",
		"/var/log/authlog",
		"/var/log/brlog",
		"/var/log/postrun.log",
		"/var/log/scrollkeeper.log",
		"/var/log/snmpd.log",
		"/var/log/sysidconfig.log",
		"/var/log/swupas/swupas.log",
		"/var/log/swupas/swupas.error.log",
#endif
	NULL};
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
	
	for (i=0; logfiles[i]; i++) {
	    if (g_slist_find_custom(logs, logfiles[i], g_ascii_strcasecmp) == NULL &&
		file_is_log (logfiles[i], FALSE))
		logs = g_slist_insert (logs, g_strdup(logfiles[i]), 0);
	}

	if (logs != NULL) {
		p->logs = logs;
		p->logfile = logs->data;
	}
}

static UserPrefs *
prefs_load (void)
{
	gchar *logfile;
	int width, height, fontsize;
	UserPrefs *p;
	GError *err;

	g_assert (gconf_client != NULL);

	p = g_new0 (UserPrefs, 1);

	err = NULL;
	p->logs = gconf_client_get_list (gconf_client,
					 GCONF_LOGFILES,
					 GCONF_VALUE_STRING,
					 &err);
	if (p->logs == NULL)
		prefs_create_defaults (p);
	
	logfile = NULL;
	logfile = gconf_client_get_string (gconf_client, GCONF_LOGFILE, NULL);
	if (logfile && logfile[0] != '\0' && file_is_log (logfile, FALSE)) {
		gboolean found;
		GSList *iter;
		
		p->logfile = g_strdup (logfile);
		g_free (logfile);
	
		for (found = FALSE, iter = p->logs;
		     iter != NULL;
		     iter = g_slist_next (iter)) {
			if (g_ascii_strncasecmp (iter->data, p->logfile, 255) == 0)
				found = TRUE;
		}
	}

	width = gconf_client_get_int (gconf_client, GCONF_WIDTH_KEY, NULL);
	height = gconf_client_get_int (gconf_client, GCONF_HEIGHT_KEY, NULL);
	fontsize = gconf_client_get_int (gconf_client, GCONF_FONTSIZE_KEY, NULL);

	p->width = (width == 0 ? LOGVIEW_DEFAULT_WIDTH : width);
	p->height = (height == 0 ? LOGVIEW_DEFAULT_HEIGHT : height);
	p->fontsize = fontsize;

	return p;
}

static void
prefs_monospace_font_changed (GConfClient *client,
                              guint id,
                              GConfEntry *entry,
                              gpointer data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (data);

	if (entry->value && (entry->value->type == GCONF_VALUE_STRING)) {
		const gchar *monospace_font_name;

		monospace_font_name = gconf_value_get_string (entry->value);
		logview_set_font (logview, monospace_font_name);
	}
}

static void
prefs_menus_have_tearoff_changed (GConfClient *client,
                                  guint id,
                                  GConfEntry *entry,
                                  gpointer data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (data);

	if (entry->value && (entry->value->type == GCONF_VALUE_BOOL)) {
		gboolean add_tearoffs;

		add_tearoffs = gconf_value_get_bool (entry->value);
		gtk_ui_manager_set_add_tearoffs (logview->ui_manager,
						 add_tearoffs);
	}
}

gchar *
prefs_get_monospace (void)
{
	return (gconf_client_get_string (gconf_client, GCONF_MONOSPACE_FONT_NAME, NULL));
}

gboolean
prefs_get_have_tearoff (void)
{
	return (gconf_client_get_bool (gconf_client, GCONF_MENUS_HAVE_TEAROFF, NULL));
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
	if (name && name[0] != '\0')
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

	g_assert (gconf_client != NULL);

	if (prefs->logfile) {
		if (gconf_client_key_is_writable (gconf_client, GCONF_LOGFILE, NULL))
			gconf_client_set_string (gconf_client,
						 GCONF_LOGFILE,
						 prefs->logfile,
						 NULL);
	}

	if (gconf_client_key_is_writable (gconf_client, GCONF_LOGFILES, NULL))
		gconf_client_set_list (gconf_client,
				       GCONF_LOGFILES,
				       GCONF_VALUE_STRING,
				       prefs->logs,
				       NULL);

	if (prefs->width > 0 && prefs->height > 0) {
		if (gconf_client_key_is_writable (gconf_client, GCONF_WIDTH_KEY, NULL))
			gconf_client_set_int (gconf_client,
					      GCONF_WIDTH_KEY,
					      prefs->width,
					      NULL);
		
		if (gconf_client_key_is_writable (gconf_client, GCONF_HEIGHT_KEY, NULL))
			gconf_client_set_int (gconf_client,
					      GCONF_HEIGHT_KEY,
					      prefs->height,
					      NULL);
	}

	if (prefs->fontsize > 0) {
		if (gconf_client_key_is_writable (gconf_client, GCONF_FONTSIZE_KEY, NULL))
			gconf_client_set_int (gconf_client,
					      GCONF_FONTSIZE_KEY,
					      prefs->fontsize,
					      NULL);
	}
}

void
prefs_store_window_size (GtkWidget *window)
{
	gint width, height;

	g_return_if_fail (GTK_IS_WINDOW (window));
      
	gtk_window_get_size (GTK_WINDOW(window), &width, &height);
	if (width > 0 && height > 0) {
		prefs->width = width;
		prefs->height = height;
	}
}

void 
prefs_connect (LogviewWindow *logview)
{
	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

	g_assert (gconf_client != NULL);

	gconf_client_notify_add (gconf_client,
				 GCONF_MONOSPACE_FONT_NAME,
				 (GConfClientNotifyFunc) prefs_monospace_font_changed,
				 logview, NULL,
				 NULL);
	gconf_client_notify_add (gconf_client,
				 GCONF_MENUS_HAVE_TEAROFF,
				 (GConfClientNotifyFunc) prefs_menus_have_tearoff_changed,
				 logview, NULL,
				 NULL);
}

void
prefs_init (void)
{
	if (!gconf_client) {
		gconf_client = gconf_client_get_default ();

		prefs = prefs_load ();
	}
}
