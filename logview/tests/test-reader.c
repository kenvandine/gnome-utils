#include "logview-log.h"

#include <glib.h>
#include <gio/gio.h>

static GMainLoop *loop;

static void
new_lines_cb (LogviewLog *log,
              char **lines,
              GError *error,
              gpointer user_data)
{
  int i;

  for (i = 0; lines[i]; i++) {
    g_print ("line %d: %s\n", i, lines[i]);
  }

  g_strfreev (lines);
  g_object_unref (log);

  g_main_loop_quit (loop);
}

static void
callback (LogviewLog *log,
          GError *error,
          gpointer user_data)
{
  g_print ("callback! err %p, log %p\n", error, log);

  logview_log_read_new_lines (log, new_lines_cb, NULL);
}

int main (int argc, char **argv)
{
  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);
  logview_log_create ("/var/log/messages", callback, NULL);
  g_main_loop_run (loop);

  return 0;
}
