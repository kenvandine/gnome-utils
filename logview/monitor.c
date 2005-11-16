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


#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "logview.h"
#include "logrtns.h"
#include "monitor.h"
#include <libgnomevfs/gnome-vfs-ops.h>

void
monitor_stop (Log *log)
{
  g_return_if_fail (log);

  if (log->mon_handle != NULL) {
      gnome_vfs_monitor_cancel (log->mon_handle);
      log->mon_handle = NULL;
      log->mon_offset = 0;
      
      gnome_vfs_close (log->mon_file_handle);
      log->mon_file_handle = NULL;
      
      log->monitored = FALSE;
  }
}

static void
monitor_callback (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri,
                  const gchar *info_uri, GnomeVFSMonitorEventType event_type,
                  gpointer data)
{
  LogviewWindow *logview;
  Log *log = data;
  
  g_assert (log);

  logview  = LOGVIEW_WINDOW (log->window);
  loglist_bold_log (LOG_LIST (logview->loglist), log);
  log->needs_refresh = TRUE;
  if (logview->curlog == log)
    logview_repaint (logview);
}

void
monitor_start (Log *log)
{
  GnomeVFSResult result;
  GnomeVFSFileSize size;
  gchar *main = NULL, *second = NULL;
	 
  g_return_if_fail (log);

  result = gnome_vfs_open (&(log->mon_file_handle), log->name, 
                           GNOME_VFS_OPEN_READ);
  result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_END, 0L);
  result = gnome_vfs_tell (log->mon_file_handle, &size);
  log->mon_offset = size;

  result = gnome_vfs_monitor_add (&(log->mon_handle), log->name,
                         GNOME_VFS_MONITOR_FILE, monitor_callback,
                         log);

  if (result != GNOME_VFS_OK) {
    main = g_strdup (_("This file cannot be monitored."));
    switch (result) {
    case GNOME_VFS_ERROR_NOT_SUPPORTED :
        second = g_strdup (_("File monitoring is not supported on this file system.\n"));
    default:
        second = g_strdup (_("Gnome-VFS Error.\n"));
    }
    error_dialog_show (NULL, main, second);
    g_free (main);
    g_free (second);
    gnome_vfs_close (log->mon_file_handle);
    return;
  }

  log->monitored = TRUE;
}
