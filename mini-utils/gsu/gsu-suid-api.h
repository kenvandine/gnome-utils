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

#ifndef GSU_SUID_API_H
#define GSU_SUID_API_H

#include <unistd.h>

#define GSU_MAX_MESSAGE_LEN 512
#define GSU_MAX_PASSWD_LEN 512
#define GSU_FD_LEN 10			/* Leave room for FOPEN_MAX */


/* NOTE: gsu_call_suid() blocks waiting for a message from the called
 * executable (this shouldn't be long).  The message pipe is closed on exec,
 * (read: when su calls the command) but the suid executable itself
 * cannot block for input (sent after calling gsu_call_suid by the calling
 * process). */

/* Hook to set up gsu, modifies args. Call ASAP. */
void gsu_suid_init(int *argc, char **argv);

/* Returns the password in an allocated buffer.  gsu_suid_init or
 * GSU_PASSWD_STDIN_START can only be used once. */
char *gsu_suid_provide_password(void);

/* Same format as printf, returns message to the graphical client,
 * and only fails to return on I/O errors.  Calls after the first are
 * ignored. */
void gsu_suid_error(const char *message, ...);

/* Provide the password on fd 0, (creates and ends a scope) */
#define GSU_PASSWD_STDIN_START \
	if(1) { \
		extern size_t gsu_passwd_fd; /* in gsu-suid-api.c */ \
		size_t gsu_stdin_copy_fd; \
		size_t gsu_prefix; \
		\
		void gsu_suid_io_error(const char *message, ...); \
		\
		read(gsu_passwd_fd, &gsu_prefix, sizeof gsu_prefix); \
		gsu_stdin_copy_fd = dup(0); \
		if(gsu_stdin_copy_fd < 0) \
			gsu_suid_io_error(NULL); \
		if(dup2(gsu_passwd_fd, 0) != 0) \
			gsu_suid_io_error(NULL);

#define GSU_PASSWD_STDIN_END \
		if(dup2(gsu_stdin_copy_fd, 0) != 0) \
			gsu_suid_io_error(NULL); \
		if(close(gsu_stdin_copy_fd)) \
			gsu_suid_io_error(NULL); \
	}

#endif
