/*
    gsu-suid-api.h - support for altering almost any su to be gsu-suid

  This code is intended for inclusion in any distribution of the program
  su.  Two licences are available in the hope that one of them is suitable
  for your needs.  You must use one of them.  It is preferred that you
  use the following:

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

  At your option, you may use this alternative BSD style licence:

    Copyright (C) 2000 Adrian Johnston
    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this file (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies of
    the Software, and to permit persons to whom the Software is furnished
    to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
    OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
    THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    
  You may remove the one licence that you choose not to use.
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>

#include "gsu-suid-api.h"

/* Globals (Referenced by the gsu-suid-api.h GSU_PASSWD_STDIN macros.) */
size_t gsu_passwd_fd = -1;
size_t gsu_message_fd = -1;

/* Private prototype */
void gsu_suid_io_error(const char *message, ...);

/*	Removes the last 2 elements of the command line and uses them
 *	as the fileno's of the message and passwd pipes.
 *  usage:  name commands... passwd_fileno message_fileno */
void gsu_suid_init(int *argc, char **argv)
{
	/* The worst they can do here is pass a fd number that is
	 * too large to read(). */

	if (*argc < 4)
		gsu_suid_io_error("Do not call directly");

	if (strlen(argv[*argc - 1]) > GSU_FD_LEN)
		gsu_suid_io_error(NULL);

	gsu_message_fd = atoi(argv[*argc - 1]);
	if (gsu_message_fd < 3)
		gsu_suid_io_error(NULL);

	/* The message pipe is closed automatically on exec. */
	if (fcntl(gsu_message_fd, F_SETFD, FD_CLOEXEC))
		gsu_suid_io_error(NULL);

	if (strlen(argv[*argc - 2]) > GSU_FD_LEN)
		gsu_suid_io_error(NULL);

	gsu_passwd_fd = atoi(argv[*argc - 2]);
	if (gsu_passwd_fd < 3)
		gsu_suid_io_error(NULL);

	argv[(*argc) -= 2] = NULL;	/* Fix up argv and *argc */
}

/* Returns password in an allocated string */
char *gsu_suid_provide_password(void)
{
	char *password;
	size_t size;

	if (read(gsu_passwd_fd, &size, sizeof(size)) != sizeof(size))
		gsu_suid_io_error("read(gsu_passwd_fd)");

	if ((size < 0) || (size > GSU_MAX_PASSWD_LEN))
		gsu_suid_io_error(NULL);

	if ((password = malloc(size)) == NULL)
		gsu_suid_io_error(strerror(errno));

	/* Leave the trailing newline */
	if (read(gsu_passwd_fd, password, size) != size)
		gsu_suid_io_error("read(gsu_passwd_fd)");

	password[size] = '\0';

	return password;
}

/* Returns your error message to the parent process via
 * a pipe.  This function call indicates failure.
 * The value of errno, if non-zero, is also displayed.
 * Calls after the first are ignored. */
void gsu_suid_error(const char *message, ...)
{
	char buf[GSU_MAX_MESSAGE_LEN + 1];
	va_list args;
	int len;
	static int is_not_first = 0;
	if (is_not_first)	/* Report only the first warning */
		return;
	is_not_first = 0;

	va_start(args, message);
	len = vsnprintf(buf, GSU_MAX_MESSAGE_LEN, message, args);
	va_end(args);

	if (errno && len > 0)
		len += strlen(strerror(errno)) + 2;

	if (len > GSU_MAX_MESSAGE_LEN || len < 0)
		gsu_suid_io_error("gsu_suid_error: message too long");

	if (errno)
		strcat(strcat(buf, ": "), strerror(errno));

	if (write(gsu_message_fd, &len, sizeof(len)) != sizeof(len))
		gsu_suid_io_error("write(gsu_message_fd)");

	if (write(gsu_message_fd, buf, len) != len)
		gsu_suid_io_error("write(gsu_message_fd)");
}

/* abort on io errors.  This is mainly for script kiddies... */
void gsu_suid_io_error(const char *message, ...)
{
	va_list args;

	va_start(args, message);
	vfprintf(stderr, message ? message : "I/O Error", args);
	va_end(args);

	if (errno)
		fprintf(stderr, ": %s", strerror(errno));
	fprintf(stderr, "\n");

	exit(errno ? errno : -1);
}

