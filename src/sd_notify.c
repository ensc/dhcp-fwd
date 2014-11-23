/***
  Copyright 2010 Lennart Poettering

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "sd_notify.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

union sockaddr_union { // stripped down from the sd-daemon version
	struct sockaddr sa;
	struct sockaddr_un un;
};

int sd_notify_supported(void)
{
	const char *e;

	e = getenv("NOTIFY_SOCKET");
	if (!e)
		return 0;

	/* Must be an abstract socket, or an absolute path */
	if ((e[0] != '@' && e[0] != '/') || e[1] == 0)
		return 0;

	return 1;
}

int sd_notify(int unset_environment, const char *state) {
	int fd = -1, r;
	struct msghdr msghdr;
	struct iovec iovec;
	union sockaddr_union sockaddr;
	const char *e;

	if (!state) {
		r = -EINVAL;
		goto finish;
	}

	e = getenv("NOTIFY_SOCKET");
	if (!e)
		return 0;

	/* Must be an abstract socket, or an absolute path */
	if ((e[0] != '@' && e[0] != '/') || e[1] == 0) {
		r = -EINVAL;
		goto finish;
	}

	fd = socket(AF_UNIX, SOCK_DGRAM|SOCK_CLOEXEC, 0);
	if (fd < 0) {
		r = -errno;
		goto finish;
	}

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sa.sa_family = AF_UNIX;
	strncpy(sockaddr.un.sun_path, e, sizeof(sockaddr.un.sun_path));

	if (sockaddr.un.sun_path[0] == '@')
		sockaddr.un.sun_path[0] = 0;

	memset(&iovec, 0, sizeof(iovec));
	iovec.iov_base = (char*) state;
	iovec.iov_len = strlen(state);

	memset(&msghdr, 0, sizeof(msghdr));
	msghdr.msg_name = &sockaddr;
	msghdr.msg_namelen = offsetof(struct sockaddr_un, sun_path) + strlen(e);

	if (msghdr.msg_namelen > sizeof(struct sockaddr_un))
		msghdr.msg_namelen = sizeof(struct sockaddr_un);

	msghdr.msg_iov = &iovec;
	msghdr.msg_iovlen = 1;

	if (sendmsg(fd, &msghdr, MSG_NOSIGNAL) < 0) {
		r = -errno;
		goto finish;
	}

	r = 1;

finish:
	if (unset_environment)
		unsetenv("NOTIFY_SOCKET");

	if (fd >= 0)
		close(fd);

	return r;
}
