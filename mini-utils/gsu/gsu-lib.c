#include <gsu-lib.h>

#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <pwd.h>

#define paranoid 1

#ifndef max
#define max(a,b) ((a > b) ? a : b)
#endif

static void
no_helper (void)
{
	gnome_dialog_run
		(GNOME_DIALOG
		 (gnome_message_box_new
		  (_("This program needs a SUID helper program called "
		     "gsu-helper\nwhich is either not installed or has "
		     "the wrong permissions.\n\n"
		     "Please contact your local system administrator."),
		   GNOME_MESSAGE_BOX_ERROR,
		   _("Close"), NULL)));

	exit (1);
}

static void
su_failed (gchar *message)
{
	gchar *msg_text;

	if (!strcmp (message, "incorrect password"))
		msg_text = _("You gave an incorrect password.");
	else
		msg_text = message;

	gnome_dialog_run
		(GNOME_DIALOG
		 (gnome_message_box_new
		  (msg_text,
		   GNOME_MESSAGE_BOX_ERROR,  _("Close"), NULL)));

	exit (1);
}

static void
su_error (gchar *error)
{
	perror (error);
	exit (1);
}

static void
su_ok (void)
{
	gnome_dialog_run
		(GNOME_DIALOG
		 (gnome_message_box_new
		  (_("SU was successful."),
		   GNOME_MESSAGE_BOX_INFO,
		   _("Close"), NULL)));

	exit (0);
}

static void
dialog_callback (gchar *string, gpointer password_ptr)
{
	*(gchar**) password_ptr = string ? g_strdup (string) : NULL;
}

gchar *
gsu_getpass (const gchar *new_user)
{
    GtkWidget *dialog, *label, *entry;
    gchar *prompt, *password = NULL;
    gint button;

    if (strcmp (new_user, "root") == 0)
	prompt = g_strdup
	    (_("You are trying to do something which requires\n"
	       "root (system administrator) privileges.\n"
	       "To do this, you must give the root password.\n"
	       "Please enter the root password now, or choose\n"
	       "Cancel if you do not know it."));
    else
	prompt = g_strdup_printf
	    (_("You are trying to change your user identity.\n"
	     "Please enter the password for user `%s'."),
	     new_user);
	
    dialog = gnome_request_dialog (TRUE, prompt, NULL, 32,
				   dialog_callback, &password,
				   NULL);

    g_free (prompt);

    button = gnome_dialog_run_and_close (GNOME_DIALOG(dialog));

    if (button && password) {
	g_free (password);
	password = NULL;
    }

    return password;
}

int
gsu_call_helper (int argc, char **argv)
{
	gchar *password, *helper_pathname;
	gchar passwd_pipe_str [BUFSIZ], message_pipe_str [BUFSIZ];
	gchar error_pipe_str [BUFSIZ], message [BUFSIZ];
	int passwd_pipe [2], message_pipe [2], error_pipe [2];
	int len, max_fd, retval;
	fd_set rdfds;
	pid_t pid;

	helper_pathname = gnome_is_program_in_path ("gsu-helper");
	if (!helper_pathname)
		no_helper ();

#if paranoid >= 1

	/* We pass the target user's password to that program so let
	 * me be a little paranoid here.
	 */

	{
		struct stat statb;

		/* stat the helper program. */
		if (stat (helper_pathname, &statb))
			no_helper ();

		/* a regular file ? */
		if (!S_ISREG (statb.st_mode))
			no_helper ();

		/* owned by root ? */
		if (statb.st_uid)
			no_helper ();

		/* not writable by anyone but root ? */
		if (statb.st_mode & (S_IWGRP|S_IWOTH))
			no_helper ();

		/* suid ? */
		if (!statb.st_mode & S_ISUID)
			no_helper ();
	}

	/* Ok, now we can trust that program. */

#endif /* paranoid >= 1 */

	if (pipe (passwd_pipe))
		no_helper ();

	if (pipe (message_pipe))
		no_helper ();

	if (pipe (error_pipe))
		no_helper ();

	sprintf (passwd_pipe_str, "%d", passwd_pipe [0]);
	sprintf (message_pipe_str, "%d", message_pipe [1]);
	sprintf (error_pipe_str, "%d", error_pipe [1]);

	fprintf (stderr, "Pipes: (%d,%d) - (%d,%d) - (%d,%d)\n",
		 passwd_pipe [0], passwd_pipe [1],
		 message_pipe [0], message_pipe [1],
		 error_pipe [0], error_pipe [1]);

	/* Enter password. */

	password = gsu_getpass ("martin");
	if (!password) return -1;

	/* Let's fork () a child ... */

	pid = fork ();
	if (pid < 0)
		no_helper ();

	if (pid) {
		close (passwd_pipe [1]);
		close (message_pipe [0]);
		close (error_pipe [0]);

		execl (helper_pathname, "gsu", "-",
		       passwd_pipe_str, error_pipe_str,
		       message_pipe_str, NULL);

		su_error ("execl");
	}

	/* We are the child. */

	len = strlen (password)+1;
		
	close (passwd_pipe [0]);
	close (message_pipe [1]);
	close (error_pipe [1]);

	if (write (passwd_pipe [1], &len, sizeof (len)) != sizeof (len))
		su_error ("first write");

	if (write (passwd_pipe [1], password, len) != len)
		su_error ("second write");

	sleep (3);

	close (passwd_pipe [1]);

 select_loop:

	FD_ZERO (&rdfds);
	FD_SET (message_pipe [0], &rdfds);
	FD_SET (error_pipe [0], &rdfds);

	max_fd = max (message_pipe [0], error_pipe [0]);

	fprintf (stderr, "Starting select on %d and %d ...\n",
		 message_pipe [0], error_pipe [0]);

	retval = select (max_fd+1, &rdfds, NULL, NULL, NULL);

	fprintf (stderr, "select: %d - %s - %d - %d\n",
		 retval, strerror (errno),
		 FD_ISSET (message_pipe [0], &rdfds),
		 FD_ISSET (error_pipe [0], &rdfds));

	if (FD_ISSET (error_pipe [0], &rdfds)) {
		retval = read (error_pipe [0], &len, sizeof (len));
		fprintf (stderr, "error read: %d - %d\n", retval, len);

		if (!retval)
			su_ok ();
	}

	if (FD_ISSET (message_pipe [0], &rdfds)) {
		retval = read (message_pipe [0], &len, sizeof (len));
		fprintf (stderr, "first read: %d - %d\n", retval, len);

		if (!retval) {
			fprintf (stderr, "SU OK!\n");
			close (message_pipe [0]);
			message_pipe [0] = error_pipe [0];
			goto select_loop;
		} else if (retval != sizeof (len))
			su_error ("first read");
	}

#if 0
	if (read (message_pipe [0], &len, sizeof (len)) != sizeof (len))
		su_error ("first read");
#endif

	if (len+1 > BUFSIZ)
		su_error ("too large");

	if (read (message_pipe [0], message, len) != len)
		su_error ("second read");

	close (message_pipe [0]);

	/* both of them will never return */
	if (!strcmp (message, "OK"))
		su_ok ();
	else
		su_failed (message);
}

gchar *
gsu_get_helper (void)
{
    gchar *helper_pathname = NULL;

    helper_pathname = gnome_is_program_in_path ("gsu-helper");
    if (!helper_pathname)
	return NULL;

#if paranoid >= 1

    /* We pass the target user's password to that program so let
	 * me be a little paranoid here.
	 */

    {
	struct stat statb;

	/* stat the helper program. */
	if (stat (helper_pathname, &statb))
	    goto no_helper;

	/* a regular file ? */
	if (!S_ISREG (statb.st_mode))
	    goto no_helper;

	/* owned by root ? */
	if (statb.st_uid)
	    goto no_helper;

	/* not writable by anyone but root ? */
	if (statb.st_mode & (S_IWGRP|S_IWOTH))
	    goto no_helper;

	/* suid ? */
	if (!statb.st_mode & S_ISUID)
	    goto no_helper;
    }

    /* Ok, now we can trust that program. */

#endif /* paranoid >= 1 */

    return helper_pathname;

 no_helper:
    g_free (helper_pathname);
    return NULL;
}
