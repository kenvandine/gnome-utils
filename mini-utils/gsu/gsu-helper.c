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

#ifdef USE_PAM

   Actually, with PAM, su has nothing to do with whether or not a
   wheel group is enforced by su.  RMS tries to restrict your access
   to a su which implements the wheel group, but PAM considers that
   to be fascist, and gives the user/sysadmin the opportunity to
   enforce a wheel group by proper editing of /etc/pam.conf

#endif

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

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <signal.h>
#include <sys/wait.h>
#endif
#include "system.h"

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

#include "error.h"

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
#define DEFAULT_ROOT_LOGIN_PATH "/usr/ucb:/bin:/usr/bin:/etc"
#endif

/* The default paths which get set are both bogus and oddly influenced
   by <paths.h> and -D on the commands line. Just to be clear, we'll set
   these explicitly. -ewt */
#undef DEFAULT_LOGIN_PATH
#undef DEFAULT_ROOT_LOGIN_PATH
#define DEFAULT_LOGIN_PATH "/bin:/usr/bin:/usr/local/bin:/usr/bin/X11"
#define DEFAULT_ROOT_LOGIN_PATH \
    "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/bin/X11"

/* The shell to run if none is given in the user's passwd entry.  */
#define DEFAULT_SHELL "/bin/sh"

/* The user to become if none is specified.  */
#define DEFAULT_USER "root"

#ifndef USE_PAM
char *crypt ();
#endif
char *getpass ();
char *getusershell ();
void endusershell ();
void setusershell ();

char *basename ();
char *xmalloc ();
char *xrealloc ();
char *xstrdup ();

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
    error (1, 0, _("virtual memory exhausted"));
}

/* Return a newly-allocated string whose contents concatenate
   those of S1, S2, S3.  */

static char *
concat (const char *s1, const char *s2, const char *s3)
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

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

#ifndef SYSLOG_NON_ROOT
  if (pw->pw_uid)
    return;
#endif
  new_user = pw->pw_name;
  /* The utmp entry (via getlogin) is probably the best way to identify
     the user, especially if someone su's from a su-shell.  */
  old_user = getlogin ();
  if (old_user == NULL)
    old_user = "";
  tty = ttyname (2);
  if (tty == NULL)
    tty = "";
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

#ifdef USE_PAM
/* conversation function and static variables for communication */

/*
 * A generic conversation function for text based applications
 *
 * Written by Andrew Morgan <morgan@physics.ucla.edu>
 *
 */

#include <stdlib.h>
#include <unistd.h>

#define INPUTSIZE PAM_MAX_MSG_SIZE

#define CONV_ECHO_ON  1
#define CONV_ECHO_OFF 0

#define _pam_overwrite(x) \
{ \
     register char *xx; \
     if ((xx=x)) \
          while (*xx) \
               *xx++ = '\0'; \
}

static char *read_string(int echo, const char *remark)
{
     char buffer[INPUTSIZE];
     char *text,*tmp;

     if (!echo) {
	  tmp = getpass(remark);
	  text = strdup(tmp);       /* get some space for this text */
	  _pam_overwrite(tmp);       /* overwrite the old record of
				      * the password */
     } else {
	  fprintf(stderr,"%s",remark);
	  text = fgets(buffer,INPUTSIZE-1,stdin);
	  if (text) {
	       tmp = buffer + strlen(buffer);
	       while (tmp > buffer && (*--tmp == '\n'))
		    *tmp = '\0';
	       text = strdup(buffer);  /* get some space for this text */
	  }
     }

     return (text);
}

#define REPLY_CHUNK 5

static void drop_reply(struct pam_response *reply, int replies)
{
     int i;

     for (i=0; i<replies; ++i) {
	  _pam_overwrite(reply[i].resp);      /* might be a password */
	  free(reply[i].resp);
     }
     if (reply)
	  free(reply);
}

static pam_handle_t *pamh = NULL;
static int retval;
static struct pam_conv conv = {
  misc_conv,
  NULL
};

#define PAM_BAIL_P if (retval) { \
  pam_end(pamh, PAM_SUCCESS); \
  return 0; \
}
#endif

/* Ask the user for a password.
   If PAM is in use, let PAM ask for the password if necessary.
   Return 1 if the user gives the correct password for entry PW,
   0 if not.  Return 1 without asking for a password if run by UID 0
   or if PW has an empty password.  */

static int
correct_password (const struct passwd *pw)
{
#ifdef USE_PAM

  /* root always succeeds; this isn't an authentication question (no
   * extra privs are being granted) so it shouldn't authenticate with PAM.
   * However, we want to create the pam_handle so that proper credentials
   * are created later with pam_setcred(). */
  retval = pam_start("su", pw->pw_name, &conv, &pamh);
  PAM_BAIL_P;
  if (getuid () == 0)
    return 1;
  retval = pam_authenticate(pamh, 0);
  PAM_BAIL_P;
  retval = pam_acct_mgmt(pamh, 0);
  if (retval == PAM_NEW_AUTHTOK_REQD) {
    /* password has expired.  Offer option to change it. */
    retval = pam_chauthtok(pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
    PAM_BAIL_P;
  }
  PAM_BAIL_P;
  /* must be authenticated if this point was reached */
  return 1;
#else /* !USE_PAM */
  char *unencrypted, *encrypted, *correct;
#ifdef HAVE_SHADOW_H
  /* Shadow passwd stuff for SVR3 and maybe other systems.  */
  struct spwd *sp = getspnam (pw->pw_name);

  endspent ();
  if (sp)
    correct = sp->sp_pwdp;
  else
#endif
  correct = pw->pw_passwd;

  if (getuid () == 0 || correct == 0 || correct[0] == '\0')
    return 1;

  unencrypted = getpass (_("Password:"));
  if (unencrypted == NULL)
    {
      error (0, 0, _("getpass: cannot open /dev/tty"));
      return 0;
    }
  encrypted = crypt (unencrypted, correct);
  memset (unencrypted, 0, strlen (unencrypted));
  return strcmp (encrypted, correct) == 0;
#endif /* !USE_PAM */
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
      environ = (char **) xmalloc (2 * sizeof (char *));
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
      if (change_environment)
	{
	  xputenv (concat ("HOME", "=", pw->pw_dir));
	  xputenv (concat ("SHELL", "=", shell));
	  if (pw->pw_uid)
	    {
	      xputenv (concat ("USER", "=", pw->pw_name));
	      xputenv (concat ("LOGNAME", "=", pw->pw_name));
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
    error (1, errno, _("cannot set groups"));
  endgrent ();
#endif
#ifdef USE_PAM
  retval = pam_setcred(pamh, PAM_ESTABLISH_CRED);
  if (retval != PAM_SUCCESS)
    error (1, 0, pam_strerror(pamh, retval));
#endif /* USE_PAM */
  if (setgid (pw->pw_gid))
    error (1, errno, _("cannot set group id"));
  if (setuid (pw->pw_uid))
    error (1, errno, _("cannot set user id"));
}

#ifdef USE_PAM
static int caught=0;
/* Signal handler for parent process later */
static void su_catch_sig(int sig)
{
  ++caught;
}
#endif

/* Run SHELL, or DEFAULT_SHELL if SHELL is empty.
   If COMMAND is nonzero, pass it to the shell with the -c option.
   If ADDITIONAL_ARGS is nonzero, pass it to the shell as more
   arguments.  */

static void
run_shell (const char *shell, const char *command, char **additional_args)
{
  const char **args;
  int argno = 1;
#ifdef USE_PAM
  int child;
  sigset_t ourset;
  int status;

  retval = pam_open_session(pamh,0);
  if (retval != PAM_SUCCESS) {
    fprintf (stderr, "could not open session\n");
    exit (1);
  }
  child = fork();
  if (child == 0) {  /* child shell */
  pam_end(pamh, 0);
#endif
  if (additional_args)
    args = (const char **) xmalloc (sizeof (char *)
				    * (10 + elements (additional_args)));
  else
    args = (const char **) xmalloc (sizeof (char *) * 10);
  if (simulate_login)
    {
      char *arg0;
      char *shell_basename;

      shell_basename = basename (shell);
      arg0 = xmalloc (strlen (shell_basename) + 2);
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
  error (1, errno, _("cannot run %s"), shell);
#ifdef USE_PAM
  }
  /* parent only */
  sigfillset(&ourset);
  if (sigprocmask(SIG_BLOCK, &ourset, NULL)) {
    fprintf(stderr, "su: signal malfunction\n");
    caught = 1;
  }
  if (!caught) {
    struct sigaction action;
    action.sa_handler = su_catch_sig;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigemptyset(&ourset);
    if (sigaddset(&ourset, SIGTERM)
        || sigaddset(&ourset, SIGALRM)
        || sigaction(SIGTERM, &action, NULL)
        || sigprocmask(SIG_UNBLOCK, &ourset, NULL)) {
      fprintf(stderr, "su: signal masking malfunction\n");
      caught = 1;
    }
  }
  if (!caught) {
    do {
      int pid;

      pid = waitpid(-1, &status, WUNTRACED);

      if (WIFSTOPPED(status)) {
          kill(getpid(), SIGSTOP);
          /* once we get here, we must have resumed */
          kill(pid, SIGCONT);
      }
    } while (WIFSTOPPED(status));
  } else
    status = -1;

  if (caught) {
    fprintf(stderr, "\nSession terminated, killing shell...");
    kill (child, SIGTERM);
  }
  retval = pam_close_session(pamh, 0);
  PAM_BAIL_P;
  retval = pam_end(pamh, PAM_SUCCESS);
  PAM_BAIL_P;
  if (caught) {
    sleep(2);
    kill(child, SIGKILL);
    fprintf(stderr, " ...killed.\n");
  }
  exit (status);
#endif /* USE_PAM */
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
  const char *new_user = DEFAULT_USER;
  char *command = 0;
  char **additional_args = 0;
  char *shell = 0;
  struct passwd *pw;
  struct passwd pw_copy;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

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
      printf ("su (%s) %s\n", GNU_PACKAGE, VERSION);
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
    error (1, 0, _("user %s does not exist"), new_user);
  endpwent ();

  /* Make a copy of the password information and point pw at the local
     copy instead.  Otherwise, some systems (e.g. Linux) would clobber
     the static data through the getlogin call from log_su.  */
  pw_copy = *pw;
  pw = &pw_copy;
  pw->pw_name = xstrdup (pw->pw_name);
  pw->pw_dir = xstrdup (pw->pw_dir);
  pw->pw_shell = xstrdup (pw->pw_shell);

  if (!correct_password (pw))
    {
#ifdef SYSLOG_FAILURE
      log_su (pw, 0);
#endif
      error (1, 0, _("incorrect password"));
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
      error (0, 0, _("using restricted shell %s"), pw->pw_shell);
      shell = 0;
    }
  if (shell == 0)
    {
      shell = xstrdup (pw->pw_shell);
    }
  modify_environment (pw, shell);

  change_identity (pw);
  if (simulate_login && chdir (pw->pw_dir))
    error (0, errno, _("warning: cannot change directory to %s"), pw->pw_dir);

  run_shell (shell, command, additional_args);
}
