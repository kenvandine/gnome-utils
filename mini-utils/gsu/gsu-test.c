#include <gsu-lib.h>

/* Our options */
char *user;
char *command;
char *shell;

int fast;
int preserve;
int login;

static const struct poptOption options[] = {
	{"user", 'u', POPT_ARG_STRING, &user, 0,
	 N_("login as USER"),
	 N_("USER")
	},

	{"command", 'c', POPT_ARG_STRING, &command, 0,
	 N_("pass a single COMMAND to the shell with -c"),
	 N_("COMMAND")
	},
	
	{"shell", 's', POPT_ARG_STRING, &shell, 0,
	 N_("run SHELL if /etc/SHELLS allows it"),
	 N_("SHELL")
	},

	{"preserve-environment", 'm', POPT_ARG_NONE, &preserve, 0,
	 N_("do not reset environment variables."),
	 NULL
	},

	{NULL, 'p', POPT_ARG_NONE, &preserve, 0,
	 N_("same as -m"),
	 NULL
	},
	
	{"login", 'l', POPT_ARG_NONE, &login, 0,
	 N_("make the shell a login shell"),
	 NULL
	},

	{"fast", 'f', POPT_ARG_NONE, &fast, 0,
	 N_("pass -f to the shell (for csh or tcsh)"),
	 NULL
	},

	{NULL, '\0', 0, NULL, 0} /* end the list */
};


int
main (int argc, char **argv)
{
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init_with_popt_table(PACKAGE, VERSION,
							   argc, argv, options, 0, NULL);
	
	gsu_call_helper (user, command, shell, login, preserve, fast);

	return 0;
}
