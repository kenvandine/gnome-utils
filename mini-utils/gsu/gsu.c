/* This is a hacked up version of GNU su which requests the password
   via a Gnome dialog box. It also has some Debian patches. 
   - Havoc Pennington <hp@pobox.com> */

/* su for GNU.  Run a shell with substitute user and group IDs.
   Copyright (C) 92, 93, 94, 95, 1996 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Run a shell with the real and effective UID and GID and groups
   of USER, default `root'.

   The shell run is taken from USER's password entry, /bin/sh if
   none is specified there.  If the account has a password, su
   prompts for a password unless run by a user with real UID 0.

   Does not change the current directory.
   Sets `HOME' and `SHELL' from the password entry for USER, and if
   USER is not root, sets `USER' and `LOGNAME' to USER.
   The subshell is not a login shell.

   If one or more ARGs are given, they are passed as additional
   arguments to the subshell.

   Does not handle /bin/sh or other shells specially
   (setting argv[0] to "-su", passing -c only to certain shells, etc.).
   I don't see the point in doing that, and it's ugly.

   This program intentionally does not support a "wheel group" that
   restricts who can su to UID 0 accounts.  RMS considers that to
   be fascist.

   Options:
   -, -l, --login	Make the subshell a login shell.
			Unset all environment variables except
			TERM, HOME and SHELL (set as above), and USER
			and LOGNAME (set unconditionally as above), and
			set PATH to a default value.
			Change to USER's home directory.
			Prepend "-" to the shell's name.
   -c, --commmand=COMMAND
			Pass COMMAND to the subshell with a -c option
			instead of starting an interactive shell.
   -f, --fast		Pass the -f option to the subshell.
   -m, -p, --preserve-environment
			Do not change HOME, USER, LOGNAME, SHELL.
			Run $SHELL instead of USER's shell from /etc/passwd
			unless not the superuser and USER's shell is
			restricted.
			Overridden by --login and --shell.
   -s, --shell=shell	Run SHELL instead of USER's shell from /etc/passwd
			unless not the superuser and USER's shell is
			restricted.

   Compile-time options:
   -DSYSLOG_SUCCESS	Log successful su's (by default, to root) with syslog.
   -DSYSLOG_FAILURE	Log failed su's (by default, to root) with syslog.

   -DSYSLOG_NON_ROOT	Log all su's, not just those to root (UID 0).
   Never logs attempted su's to nonexistent accounts.

   Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

/* For Gnome for now just turn on the logging */
#define SYSLOG_SUCCESS
#define SYSLOG_FAILURE


#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
/* #include "system.h" */ /* shared header for GNU shellutils, not in Gnome */
#include <gnome.h>

/* Fixme many of these conditionals don't actually exist in the 
   Gnome configure script. */

#if defined(HAVE_SYSLOG_H) && defined(HAVE_SYSLOG)
#include <syslog.h>
#else /* !HAVE_SYSLOG_H */
#ifdef SYSLOG_SUCCESS
#undef SYSLOG_SUCCESS
#endif
#ifdef SYSLOG_FAILURE
#undef SYSLOG_FAILURE
#endif
#ifdef SYSLOG_NON_ROOT
#undef SYSLOG_NON_ROOT
#endif
#endif /* !HAVE_SYSLOG_H */

#ifdef _POSIX_VERSION
#include <limits.h>
#else /* not _POSIX_VERSION */
struct passwd *getpwuid ();
struct group *getgrgid ();
uid_t getuid ();
#include <sys/param.h>
#endif /* not _POSIX_VERSION */

#ifndef HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#ifndef HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

/* error.h no longer included, use g_error instead */

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

/* The default PATH for simulated logins to non-superuser accounts.  */
#ifdef _PATH_DEFPATH
#define DEFAULT_LOGIN_PATH _PATH_DEFPATH
#else
#define DEFAULT_LOGIN_PATH ":/usr/ucb:/bin:/usr/bin"
#endif

/* The default PATH for simulated logins to superuser accounts.  */
#ifdef _PATH_DEFPATH_ROOT
#define DEFAULT_ROOT_LOGIN_PATH _PATH_DEFPATH_ROOT
#else
#define DEFAULT_ROOT_LOGIN_PATH "/sbin:/usr/ucb:/bin:/usr/sbin:/usr/bin:/etc"
#endif

/* The shell to run if none is given in the user's passwd entry.  */
#define DEFAULT_SHELL "/bin/sh"

/* The user to become if none is specified.  */
#define DEFAULT_USER "root"

char *crypt ();
/* char *getpass (); */ /* This is the function we're replacing for Gnome. */
char * gui_getpass(void);
char *getusershell ();
void endusershell ();
void setusershell ();

char *basename ();
/* Pretty sure the g_ functions are the same, error checking */
/*
char *xmalloc ();
char *xrealloc ();
char *xstrdup ();
*/

extern char **environ;

/* The name this program was run with.  */
char *program_name;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

/* If nonzero, pass the `-f' option to the subshell.  */
static int fast_startup;

/* If nonzero, simulate a login instead of just starting a shell.  */
static int simulate_login;

/* If nonzero, change some environment vars to indicate the user su'd to.  */
static int change_environment;

/* Made global in Gnome version. Shadowed by local var in log_su */
static const char *new_user = DEFAULT_USER;

static struct option const longopts[] =
{
  {"command", required_argument, 0, 'c'},
  {"fast", no_argument, &fast_startup, 1},
  {"help", no_argument, &show_help, 1},
  {"login", no_argument, &simulate_login, 1},
  {"preserve-environment", no_argument, &change_environment, 0},
  {"shell", required_argument, 0, 's'},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

/* Add VAL to the environment, checking for out of memory errors.  */

static void
xputenv (const char *val)
{
  if (putenv (val))
    g_error (_("virtual memory exhausted"));
}

/* Return a newly-allocated string whose contents concatenate
   those of S1, S2, S3.  */

static char *
concat (const char *s1, const char *s2, const char *s3)
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) g_malloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  result[len1 + len2 + len3] = 0;

  return result;
}

/* Return the number of elements in ARR, a null-terminated array.  */

static int
elements (char **arr)
{
  int n = 0;

  for (n = 0; *arr; ++arr)
    ++n;
  return n;
}

#if defined (SYSLOG_SUCCESS) || defined (SYSLOG_FAILURE)
/* Log the fact that someone has run su to the user given by PW;
   if SUCCESSFUL is nonzero, they gave the correct password, etc.  */

static void
log_su (const struct passwd *pw, int successful)
{
  const char *new_user, *old_user, *tty;
  struct passwd * check;

#ifndef SYSLOG_NON_ROOT
  if (pw->pw_uid)
    return;
#endif
  new_user = pw->pw_name;
  /* The utmp entry (via getlogin) is probably the best way to identify
     the user, especially if someone su's from a su-shell.  */
  /* It's not, however, foolproof.  su now uses an algorithm suggested
     by Ian Jackson, which only uses the utmp entry from getlogin() as
     a first try.  If it comes back NULL, use the name from getpwuid(). */
  old_user = getlogin ();
  if (old_user == NULL)
    {
      check = getpwuid (getuid ());
      if (check != NULL)
	old_user = check->pw_name;
      else
	/* We tried everything... */
	old_user = "";
    }

  tty = ttyname (2);
  if (tty == NULL)
    tty = "none";
  /* 4.2BSD openlog doesn't have the third parameter.  */
  openlog (basename (program_name), 0
#ifdef LOG_AUTH
	   , LOG_AUTH
#endif
	   );
  syslog (LOG_NOTICE,
#ifdef SYSLOG_NON_ROOT
	  "%s(to %s) %s on %s",
#else
	  "%s%s on %s",
#endif
	  successful ? "" : "FAILED SU ",
#ifdef SYSLOG_NON_ROOT
	  new_user,
#endif
	  old_user, tty);
  closelog ();
}
#endif

/* Ask the user for a password.
   Return 1 if the user gives the correct password for entry PW,
   0 if not.  Return 1 without asking for a password if run by UID 0
   or if PW has an empty password.  */

static int
correct_password (const struct passwd *pw)
{
  char *unencrypted, *encrypted, *correct;
#ifdef HAVE_SHADOW_H
  /* Shadow passwd stuff for SVR3 and maybe other systems.  */
  struct spwd *sp = getspnam (pw->pw_name);

  endspent ();
  if (sp) {
    correct = sp->sp_pwdp;
  }
  else
#endif
    correct = pw->pw_passwd;

  if (getuid () == 0 || correct == 0 || correct[0] == '\0')
    return 1;

  unencrypted = gui_getpass ();

  if (unencrypted == NULL)
    {
      /* Cancel was clicked, or error. */
      gtk_main_quit();
      return 0;
    }
  encrypted = crypt (unencrypted, correct);

  memset (unencrypted, 0, strlen (unencrypted));
  g_free(unencrypted);
  return strcmp (encrypted, correct) == 0;
}

/* Update `environ' for the new shell based on PW, with SHELL being
   the value for the SHELL environment variable.  */

static void
modify_environment (const struct passwd *pw, const char *shell)
{
  char *term;

  if (simulate_login)
    {
      /* Leave TERM unchanged.  Set HOME, SHELL, USER, LOGNAME, PATH.
         Unset all other environment variables.  */
      term = getenv ("TERM");
      environ = (char **) g_malloc (2 * sizeof (char *));
      environ[0] = 0;
      if (term)
	xputenv (concat ("TERM", "=", term));
      xputenv (concat ("HOME", "=", pw->pw_dir));
      xputenv (concat ("SHELL", "=", shell));
      xputenv (concat ("USER", "=", pw->pw_name));
      xputenv (concat ("LOGNAME", "=", pw->pw_name));
      xputenv (concat ("PATH", "=", pw->pw_uid
		       ? DEFAULT_LOGIN_PATH : DEFAULT_ROOT_LOGIN_PATH));
    }
  else
    {
      /* Set HOME, SHELL, and if not becoming a super-user,
	 USER and LOGNAME.  */
      /* Also, set PATH to DEFAULT_ROOT_LOGIN_PATH if becoming a
         super-user, so that /sbin and /usr/sbin are correctly in
         PATH. --imurdock */
      if (change_environment)
	{
	  xputenv (concat ("HOME", "=", pw->pw_dir));
	  xputenv (concat ("SHELL", "=", shell));
	  if (pw->pw_uid)
	    {
	      xputenv (concat ("USER", "=", pw->pw_name));
	      xputenv (concat ("LOGNAME", "=", pw->pw_name));
	    }
	  else
	    {
	      xputenv (concat ("PATH", "=", pw->pw_uid
			       ? DEFAULT_LOGIN_PATH
			       : DEFAULT_ROOT_LOGIN_PATH));
	    }
	}
    }
}

/* Become the user and group(s) specified by PW.  */

static void
change_identity (const struct passwd *pw)
{
#ifdef HAVE_INITGROUPS
  errno = 0;
  if (initgroups (pw->pw_name, pw->pw_gid) == -1)
    g_error (_("Cannot set groups: %s"), g_unix_error_string(errno));
  endgrent ();
#endif
  if (setgid (pw->pw_gid))
    g_error (_("Cannot set group id: %s"), g_unix_error_string(errno));
  if (setuid (pw->pw_uid))
    g_error (_("Cannot set user id: %s"), g_unix_error_string(errno));
}

/* Run SHELL, or DEFAULT_SHELL if SHELL is empty.
   If COMMAND is nonzero, pass it to the shell with the -c option.
   If ADDITIONAL_ARGS is nonzero, pass it to the shell as more
   arguments.  */

static void
run_shell (const char *shell, const char *command, char **additional_args)
{
  const char **args;
  int argno = 1;

  if (additional_args)
    args = (const char **) g_malloc (sizeof (char *)
				    * (10 + elements (additional_args)));
  else
    args = (const char **) g_malloc (sizeof (char *) * 10);
  if (simulate_login)
    {
      char *arg0;
      char *shell_basename;

      shell_basename = basename (shell);
      arg0 = g_malloc (strlen (shell_basename) + 2);
      arg0[0] = '-';
      strcpy (arg0 + 1, shell_basename);
      args[0] = arg0;
    }
  else
    args[0] = basename (shell);
  if (fast_startup)
    args[argno++] = "-f";
  if (command)
    {
      args[argno++] = "-c";
      args[argno++] = command;
    }
  if (additional_args)
    for (; *additional_args; ++additional_args)
      args[argno++] = *additional_args;
  args[argno] = NULL;
  execv (shell, (char **) args);
  g_error (_("cannot run %s"), shell);
}

/* Return 1 if SHELL is a restricted shell (one not returned by
   getusershell), else 0, meaning it is a standard shell.  */

static int
restricted_shell (const char *shell)
{
  char *line;

  setusershell ();
  while ((line = getusershell ()) != NULL)
    {
      if (*line != '#' && strcmp (line, shell) == 0)
	{
	  endusershell ();
	  return 0;
	}
    }
  endusershell ();
  return 1;
}

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [-] [USER [ARG]...]\n"), program_name);
      printf (_("\
Change the effective user id and group id to that of USER.\n\
\n\
  -, -l, --login               make the shell a login shell\n\
  -c, --commmand=COMMAND       pass a single COMMAND to the shell with -c\n\
  -f, --fast                   pass -f to the shell (for csh or tcsh)\n\
  -m, --preserve-environment   do not reset environment variables\n\
  -p                           same as -m\n\
  -s, --shell=SHELL            run SHELL if /etc/shells allows it\n\
      --help                   display this help and exit\n\
      --version                output version information and exit\n\
\n\
A mere - implies -l.   If USER not given, assume root.\n\
"));
      puts (_("\nReport bugs to sh-utils-bugs@gnu.ai.mit.edu"));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc;
  char *command = 0;
  char **additional_args = 0;
  char *shell = 0;
  struct passwd *pw;
  struct passwd pw_copy;
  GtkWidget * d;

  program_name = argv[0];
  /*  setlocale (LC_ALL, ""); */  /* What to do with this? */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gtk_init(&argc, &argv);
  gdk_imlib_init();
  gnomelib_init(program_name); /* FIXME. Can't use gnome_init() because
				  it would require redoing all the 
				  existing arg parsing. */

  fast_startup = 0;
  simulate_login = 0;
  change_environment = 1;

  while ((optc = getopt_long (argc, argv, "c:flmps:", longopts, (int *) 0))
	 != EOF)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'c':
	  command = optarg;
	  break;

	case 'f':
	  fast_startup = 1;
	  break;

	case 'l':
	  simulate_login = 1;
	  break;

	case 'm':
	case 'p':
	  change_environment = 0;
	  break;

	case 's':
	  shell = optarg;
	  break;

	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("gsu (%s) %s\n", PACKAGE, VERSION);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (optind < argc && !strcmp (argv[optind], "-"))
    {
      simulate_login = 1;
      ++optind;
    }
  if (optind < argc)
    new_user = argv[optind++];
  if (optind < argc)
    additional_args = argv + optind;

  pw = getpwnam (new_user);
  if (pw == 0)
    g_error (_("user %s does not exist"), new_user);
  endpwent ();

  /* Make a copy of the password information and point pw at the local
     copy instead.  Otherwise, some systems (e.g. Linux) would clobber
     the static data through the getlogin call from log_su.  */
  pw_copy = *pw;
  pw = &pw_copy;
  pw->pw_name = g_strdup (pw->pw_name);
  pw->pw_dir = g_strdup (pw->pw_dir);
  pw->pw_shell = g_strdup (pw->pw_shell);

  if (!correct_password (pw))
    {
#ifdef SYSLOG_FAILURE
      log_su (pw, 0);
#endif
      d = gnome_message_box_new("You gave an incorrect password.",
				GNOME_MESSAGE_BOX_ERROR,
				GNOME_STOCK_BUTTON_OK, NULL);
      gtk_signal_connect(GTK_OBJECT(d), "destroy",
			 GTK_SIGNAL_FUNC(gtk_main_quit),
			 NULL);
      gtk_widget_show(d);
      gtk_main(); /* wait for dialog to die, then quit */
      g_error (_("incorrect password")); /* just to be sure */
    }
#ifdef SYSLOG_SUCCESS
  else
    {
      log_su (pw, 1);
    }
#endif

  if (pw->pw_shell == 0 || pw->pw_shell[0] == 0)
    pw->pw_shell = (char *) DEFAULT_SHELL;
  if (shell == 0 && change_environment == 0)
    shell = getenv ("SHELL");
  if (shell != 0 && getuid () && restricted_shell (pw->pw_shell))
    {
      /* The user being su'd to has a nonstandard shell, and so is
	 probably a uucp account or has restricted access.  Don't
	 compromise the account by allowing access with a standard
	 shell.  */
      g_print (_("using restricted shell %s"), pw->pw_shell);
      exit(0); /* Gnome change: the above was an error() call with exit 0 */
      shell = 0;
    }
  if (shell == 0)
    {
      shell = g_strdup (pw->pw_shell);
    }
  modify_environment (pw, shell);

  change_identity (pw);
  if (simulate_login && chdir (pw->pw_dir)) {
    /* error (0, errno, _("warning: cannot change directory to %s"), pw->pw_dir); */
    g_warning(_("Cannot change directory to %s"), pw->pw_dir);
    exit(0);
  }
  run_shell (shell, command, additional_args);
}

static gchar * entered_password = NULL;
static gboolean password_entered = FALSE;

static void dialog_callback(GtkWidget * dialog, gint button, 
			    gpointer entry)
{
  if ( button == 0 ) {
    /* OK clicked */
    entered_password = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  }
  else if ( button == 1 ) {
    entered_password = NULL;
  }
  else {
    g_assert_not_reached();
  }

  password_entered = TRUE;
  return;
}

char * gui_getpass(void)
{
  GtkWidget * dialog, * label, * entry;
  gchar * s;

  dialog = gnome_dialog_new( _("Root password request"), GNOME_STOCK_BUTTON_OK,
			     GNOME_STOCK_BUTTON_CANCEL, NULL );

  gtk_container_border_width( GTK_CONTAINER(GNOME_DIALOG(dialog)->vbox), 
			      GNOME_PAD );
  gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);
  gnome_dialog_set_modal(GNOME_DIALOG(dialog));
  gnome_dialog_set_default(GNOME_DIALOG(dialog), 0); /* OK button */

  if ( strcmp(new_user, "root") == 0 ) {
    label = gtk_label_new( _("You are trying to do something which requires\n"
			     "root (system administrator) privileges.\n"
			     "To do this, you must give the root password.\n"
			     "Please enter the root password now, or choose\n"
			     "Cancel if you do not know it.") );
  }
  else {
     s = g_copy_strings( _("You are trying to change your user identity.\n"
			   "Please enter the password for user `"),
			 new_user, "'.", NULL );
     label = gtk_label_new(s);
     g_free(s);
  }

  entry = gtk_entry_new_with_max_length(16);
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

  gtk_box_pack_start( GTK_BOX(GNOME_DIALOG(dialog)->vbox), label,
		      FALSE, FALSE, GNOME_PAD );
  gtk_box_pack_start( GTK_BOX(GNOME_DIALOG(dialog)->vbox), entry,
		      FALSE, FALSE, GNOME_PAD );

  gtk_signal_connect( GTK_OBJECT(dialog), "clicked",
		      GTK_SIGNAL_FUNC(dialog_callback), 
		      entry );

  gtk_widget_show_all( dialog );

  /* FIXME I just don't know how to make this function block
     until the dialog is closed */
  while ( password_entered == FALSE ) {
    gtk_main_iteration();
  }

  return entered_password;
}





