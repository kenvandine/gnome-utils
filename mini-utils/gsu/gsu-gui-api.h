#ifndef GSU_GUI_API_H
#define GSU_GUI_API_H

#include <glib.h>

extern int
gsu_call_suid(gchar * command, gchar * user, gchar * passwd, gint * io_fd);

#endif
