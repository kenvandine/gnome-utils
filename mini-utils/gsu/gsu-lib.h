#ifndef _GSU_LIB_H
#define _GSU_LIB_H 1

#include <config.h>
#include <gnome.h>

#ifdef DBG
#define debug(args) fprintf args
#else
#define debug(args)
#endif


gchar *gsu_get_helper (void);
gchar *gsu_getpass (const gchar *new_user);
int gsu_call_helper (char *user, char *command, char *shell,
					 int login, int preserve, int fast);

#endif
