#include <gsu-lib.h>

#include <sys/stat.h>
#include <unistd.h>

#include <pwd.h>

#define paranoid 1

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

static gchar *
gsu_getpass(const gchar *new_user)
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
		prompt = g_strconcat
			(_("You are trying to change your user identity.\n"
			   "Please enter the password for user `"),
			 new_user, "'.", NULL);
	
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
	int passwd_pipe [2], message_pipe [2];
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

	sprintf (passwd_pipe_str, "%d", passwd_pipe [0]);
	sprintf (message_pipe_str, "%d", message_pipe [1]);

	fprintf (stderr, "Pipes: (%d,%d) - (%d,%d)\n",
		 passwd_pipe [0], passwd_pipe [1],
		 message_pipe [0], message_pipe [1]);

	/* Enter password. */

	password = gsu_getpass ("martin");
	if (!password) return -1;

	/* Let's fork () a child ... */

	pid = fork ();
	if (pid < 0)
		no_helper ();

	if (pid) {
		int len = strlen (password)+1;
		gchar message [BUFSIZ];
		
		close (passwd_pipe [0]);
		close (message_pipe [1]);

#if 0
		if (write (passwd_pipe [1], &len, sizeof (len)) != sizeof (len))
			su_error ("first write");
#endif

		if (write (passwd_pipe [1], password, len) != len)
			su_error ("second write");

		close (passwd_pipe [1]);

		if (read (message_pipe [0], &len, sizeof (len)) != sizeof (len))
			su_error ("first read");

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

	close (passwd_pipe [1]);
	close (message_pipe [0]);

	execl (helper_pathname, "gsu", "-",
	       passwd_pipe_str, message_pipe_str);

	su_error ("execl");
}
