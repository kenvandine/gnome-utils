/*
    Copyright (C) 1999 Martin Baulig <martin@home-of-linux.org>
    Copyright (C) 2000 Adrian Johnston

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <gnome.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "gsu-suid-api.h"
#include "gsu-gui-api.h"

#define GSU_SUID "gsu-suid"
#define paranoid 1		/* Require root control of GSU_SUID */

/* Private prototypes */
static gchar *gsu_find_helper(void);
static void gsu_complain(gchar * msg);
static void gsu_complain_and_die(gchar * msg);

static void gsu_complain(gchar * msg)
{
	gnome_dialog_run(
		GNOME_DIALOG(gnome_message_box_new
			(msg, GNOME_MESSAGE_BOX_ERROR, _("Close"), NULL)));
}

static void gsu_complain_and_die(gchar * msg)
{
	gsu_complain(msg);
	exit(1);
}

/* While user can be NULL, command and passwd are required.
 *
 * io_fd if non-NULL points to a fd number that is to be
 * a pipe in the called process.  The fd for the other end of that
 * pipe is placed in io_fd.  If io_fd was 0 then it may be 
 * written to, otherwise the fd is for reading.
 *
 * This call blocks until the message pipe is closed by gsu-helper.
 * That occurs when gsu-helper execs the command.
 *
 * Returns 0 if a shell was invoked with the given command and privlages.
 * Otherwise, any error is provided to the user through a dialoge.
 * In any case, io_fd if non-NULL may refer to an open file handle.
 */
extern int
gsu_call_suid(gchar * command,
			  gchar * user, gchar * password, gint * io_fd)
{
	gchar *helper_pathname;

	gchar passwd_pipe_str[GSU_FD_LEN], message_pipe_str[GSU_FD_LEN];

	static gchar message[GSU_MAX_MESSAGE_LEN];
	int passwd_pipe[2], message_pipe[2], io_pipe[2];
	pid_t pid;

	/* Used by the parent */
	int len, retval;

	helper_pathname = gsu_find_helper();
	if (!helper_pathname) {
		gsu_complain("A required program " GSU_SUID
			" cannot be found\nor has the wrong permissions.  "
			"Contact your\nlocal system administrator.");
		return 1;
	}

	if (pipe(passwd_pipe))
		gsu_complain_and_die(strerror(errno));
	if (pipe(message_pipe))
		gsu_complain_and_die(strerror(errno));
	if (io_fd) {
		if (pipe(io_pipe))
			gsu_complain_and_die(strerror(errno));
	}

	sprintf(passwd_pipe_str, "%d", passwd_pipe[0]);
	sprintf(message_pipe_str, "%d", message_pipe[1]);

	/* Ye ol' fork and exec of GSU_SUID */
	if ((pid = fork()) < 0)
		gsu_complain_and_die(strerror(errno));

	if (pid == 0) {				/* Child */
		gchar *commandargs[10];
		int argindex = 3;

		commandargs[0] = GSU_SUID;
		commandargs[1] = "-c";
		commandargs[2] = command;
		if (user)
			commandargs[argindex++] = user;
		commandargs[argindex++] = passwd_pipe_str;
		commandargs[argindex++] = message_pipe_str;
		commandargs[argindex] = NULL;

		close(passwd_pipe[1]);
		close(message_pipe[0]);

		/* Replace stdin/out with pipe */
		if (io_fd) {
			dup2(io_pipe[*io_fd != 0], *io_fd);
			close(io_pipe[0]);
			close(io_pipe[1]);
		}

		execv(helper_pathname, commandargs);

		perror("execv");
		gsu_complain_and_die(strerror(errno));	/* target was verified */
	}
	/* Parent */
	close(passwd_pipe[0]);
	close(message_pipe[1]);
	g_free(helper_pathname);

	/* Return I/O pipe opened to child */
	if (io_fd) {
		close(io_pipe[*io_fd != 0]);
		*io_fd = io_pipe[*io_fd == 0];
	}

	/* Write password */
	len = strlen(password) + 1;
	if (write(passwd_pipe[1], &len, sizeof(len)) != sizeof(len)) {
		perror("write(passwd_pipe)");
		gsu_complain_and_die(strerror(errno));
	}
	if (write(passwd_pipe[1], password, len) != len) {
		perror("write(passwd_pipe)");
		gsu_complain_and_die(strerror(errno));
	}
	/* The trailing newline is for compatability if passed on stdin */
	if (write(passwd_pipe[1], "\n", 1) != 1) {
		perror("write(passwd_pipe)");
		gsu_complain_and_die(strerror(errno));
	}
	close(passwd_pipe[1]);

	/* Check for errors indicated by messages from GSU_SUID */
	retval = read(message_pipe[0], &len, sizeof(len));
	if (retval == 0) {
		close(message_pipe[0]);	/* No complaints... return */
		return 0;
	}

	if (retval != sizeof(len)) {
		perror("first read(message_pipe)");
		gsu_complain("Response from " GSU_SUID " was lost");
	} else if (len >= GSU_MAX_MESSAGE_LEN)
		gsu_complain("Response from " GSU_SUID " was too large");
	else if (read(message_pipe[0], message, len) != len)
		gsu_complain("Response from " GSU_SUID " was lost");
	else
		gsu_complain(message);	/* You can't win... */

	close(message_pipe[0]);

	return 1;		/* A Message was returned */
}

static gchar *gsu_find_helper(void)
{
	gchar *helper_pathname = NULL;

	helper_pathname = gnome_is_program_in_path(GSU_SUID);
	if (!helper_pathname)
		return NULL;

#if paranoid >= 1

	/* We pass the target user's password to that program so let
	 * me be a little paranoid here.
	 */

	{
		struct stat statb;

		/* stat the helper program. */
		if (stat(helper_pathname, &statb))
			goto no_helper;

		/* a regular file ? */
		if (!S_ISREG(statb.st_mode))
			goto no_helper;

		/* owned by root ? */
		if (statb.st_uid)
			goto no_helper;

		/* not writable by anyone but root ? */
		if (statb.st_mode & (S_IWGRP | S_IWOTH))
			goto no_helper;

		/* suid ? */
		if (!statb.st_mode & S_ISUID)
			goto no_helper;
	}
#endif
	/* Ok, now we can trust that program. */

	return helper_pathname;

#if paranoid >= 1
  no_helper:
	g_free(helper_pathname);
	return NULL;
#endif
}
