// $Id$    --*- c++ -*--

// Copyright (C) 2002 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  

#ifndef DHCP_FORWARDER_SRC_WRAPPERS_H
#define DHCP_FORWARDER_SRC_WRAPPERS_H

#include "splint.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <alloca.h>
#include <sys/resource.h>
#include <grp.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "util.h"
#include "recvfromflags.h"
#include "compat.h"

  /*@-internalglobs@*//*@-modfilesys@*/
/*@unused@*//*@noreturnwhentrue@*/
inline static void
FatalErrnoError(bool condition, int retval, char const msg[]) /*@*/
{
  if (!condition)	return;

#if 0  
  char		*str = strerror(errno);
  write(2, msg, strlen(msg));
  write(2, ": ", 2);
  write(2, str, strlen(str));
  write(2, "\n", 1);
#else
  perror(msg);
#endif

  exit(retval);
}
  /*@=internalglobs@*//*@=modfilesys@*/

/*@unused@*/
inline static /*@observer@*/ struct group const *
Egetgrnam(char const *name)
   /*@*/
{
  /*@observer@*/struct group const	*res = getgrnam(name);
  FatalErrnoError(res==0, 1, "getgrnam()");

    /*@-freshtrans@*/
    /*@-mustfreefresh@*/
  return res;
}
  /*@=mustfreefresh@*/
  /*@=freshtrans@*/

/*@unused@*/
inline static /*@observer@*/ struct passwd const *
Egetpwnam(char const *name)
    /*@*/
{
  struct passwd const	*res = getpwnam(name);
  FatalErrnoError(res==0, 1, "getpwnam()");

  return res;
}

/*@unused@*/
inline static void
Echroot(char const path[])
  /*@globals internalState, errno@*/
  /*@modifies internalState, errno@*/
  /*@warn superuser "Only super-user processes may call Echroot."@*/
{
    /*@-superuser@*/
  FatalErrnoError(chroot(path)==-1, 1, "chroot()");
    /*@=superuser@*/  
}

/*@unused@*/
inline static void
Echdir(char const path[])
  /*@globals internalState, errno@*/
  /*@modifies internalState, errno@*/
{
  FatalErrnoError(chdir(path)==-1, 1, "chdir()");
}

/*@unused@*/
inline static void
Esetuid(uid_t uid)
  /*@globals internalState, fileSystem, errno@*/
  /*@modifies internalState, fileSystem, errno@*/
{
  FatalErrnoError(setuid(uid)==-1, 1, "setuid()");
}

/*@unused@*/
inline static void
Esetgid(gid_t gid)
  /*@globals internalState, fileSystem, errno@*/
  /*@modifies internalState, fileSystem, errno@*/
{
  FatalErrnoError(setgid(gid)==-1, 1, "setgid()");
}

/*@unused@*/
inline static void
Esetgroups(size_t size, const gid_t *list)
    /*@globals internalState@*/
    /*@modifies internalState@*/
{
  FatalErrnoError(setgroups(size, list)==-1, 1, "setgroups()");
}

/*@unused@*/
inline static /*@null@*//*@only@*/ void *
Erealloc(/*@only@*//*@out@*//*@null@*/ void *ptr,
	 size_t new_size)
    /*@ensures maxSet(result) == new_size@*/
    /*@modifies *ptr@*/
{
  register void		*res = realloc(ptr, new_size);
  FatalErrnoError(res==0 && new_size!=0, 1, "realloc()");

  return res;
}

/*@unused@*/
inline static /*@null@*//*@only@*/ void *
Emalloc(size_t size)
    /*@*/
    /*@ensures maxSet(result) == size@*/
{
  register void /*@out@*/		*res = malloc(size);
  FatalErrnoError(res==0 && size!=0, 1, "malloc()");
    /*@-compdef@*/
  return res;
    /*@=compdef@*/
}

/*@unused@*/
inline static /*@null@*//*@temp@*//*@only@*/ void *
Ealloca(size_t size)
    /*@ensures maxSet(result) == size@*/
    /*@*/
{
    /*@-modunconnomods@*/
  register void /*@temp@*/		*res = alloca(size);
    /*@=modunconnomods@*/
  
  FatalErrnoError(res==0 && size!=0, 1, "malloc()");
    /*@-freshtrans -mustfreefresh@*/
  return res;
    /*@=freshtrans =mustfreefresh@*/
}

/*@unused@*/
inline static int
Edup(int fd)
    /*@globals internalState, fileSystem@*/
    /*@modifies internalState, fileSystem@*/
{
  int		res = dup(fd);

  FatalErrnoError(res==-1, 1, "dup()");

  return res;
}

/*@unused@*/
inline static int
Edup2(int oldfd, int newfd)
    /*@globals internalState, fileSystem@*/
    /*@modifies internalState, fileSystem@*/
{
  int		res = dup2(oldfd, newfd);

  FatalErrnoError(res==-1, 1, "dup2()");

  return res;
}

/*@unused@*/
inline static int
Eopen(char const *pathname, int flags)
    /*@globals internalState@*/
    /*@modifies internalState@*/
{
  int		res = open(pathname, flags);
  FatalErrnoError(res==-1, 1, "open()");

  return res;
}

/*@unused@*/
inline static int 
Esocket(int domain, int type, int protocol)
    /*@globals internalState@*/
    /*@modifies internalState@*/
{
  register int		res = socket(domain, type, protocol);
  FatalErrnoError(res==-1, 1, "socket()");

  return res;
}

/*@unused@*/
inline static void
Ebind(int s, struct sockaddr_in const *address)
    /*@globals fileSystem, errno@*/
    /*@modifies fileSystem, errno@*/
{
  FatalErrnoError(bind(s,
		       reinterpret_cast(struct sockaddr const *)(address),
		       sizeof(*address))==-1,
		  1, "bind()");
}

/*@unused@*/
inline static void
Eclose(int s)
    /*@globals internalState, fileSystem, errno@*/
    /*@modifies internalState, fileSystem, errno@*/
{
  FatalErrnoError(close(s)==-1, 1, "close()");
}

/*@unused@*/
inline static void
Esetsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
    /*@requires maxRead(optval) >= optlen@*/
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno@*/
{
  FatalErrnoError(setsockopt(s, level, optname, optval, optlen)==-1,
		  1, "setsockopt()");
}

/*@unused@*/
inline static void
Esetrlimit(int resource, /*@in@*/struct rlimit const *rlim)
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno@*/
{
#if (defined(__GLIBC__) && __GLIBC__>=2) && defined(__cplusplus) && defined(_GNU_SOURCE)
  FatalErrnoError(setrlimit(static_cast(__rlimit_resource)(resource), rlim)==-1, 1, "setrlimit()");
#else  
  FatalErrnoError(setrlimit(resource, rlim)==-1, 1, "setrlimit()");
#endif  
}

/*@unused@*/
inline static int
Wselect(int n,
	/*@null@*/fd_set *readfds,
	/*@null@*/fd_set *writefds,
	/*@null@*/fd_set *exceptfds,
	/*@null@*/struct timeval *timeout)
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno, *readfds, *writefds, *exceptfds, *timeout@*/
{
  register int			res;
  
  retry:
  res = select(n, readfds, writefds, exceptfds, timeout);
  if (res==-1) {
    if (errno==EINTR) goto retry;
  }

  return res;
}

/*@unused@*/
inline static size_t
Wrecv(int s,
      /*@out@*/void *buf, size_t len, int flags)
    /*@requires maxSet(buf) >= len@*/
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno, *buf@*/
{
  register ssize_t		res;

  retry:
  res = recv(s, buf, len, flags);
  
  if (res==-1) {
    if (errno==EINTR) goto retry;
  }

  return static_cast(size_t)(res);
}

/*@unused@*/
inline static size_t
WrecvfromInet4(int s,
	       /*@out@*/void *buf, size_t len, int flags,
	       struct sockaddr_in *from)
    /*@requires maxSet(buf) >= len@*/
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno, *buf, *from@*/
{
  register ssize_t		res;
  socklen_t			size = sizeof(*from);

  retry:
  res = recvfrom(s, buf, len, flags,
		 reinterpret_cast(struct sockaddr *)(from), &size);
  
  if (res==-1) {
    if (errno==EINTR) goto retry;
  }

  if (res==-1 || size!=sizeof(*from) ||
    /*@-type@*/from->sin_family!=AF_INET/*@=type@*/) 
    res = -1;

  return static_cast(size_t)(res);
}

/*@unused@*/
inline static size_t
WrecvfromFlagsInet4(int					s,
		    /*@out@*//*@dependent@*/void	*buf,
		    size_t				len,
		    int					*flags,
		    /*@out@*/struct sockaddr_in		*from,
		    /*@out@*/struct in_pktinfo		*pktp)
    /*@requires maxSet(buf) >= len@*/
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno, *buf, *flags, *from, *pktp@*/
{
  register ssize_t		res;
  socklen_t			size = sizeof(*from);

  retry:
  res = recvfrom_flags(s, buf, len, flags,
		       reinterpret_cast(struct sockaddr *)(from), &size,
		       pktp);
  
  if (res==-1) {
    if (errno==EINTR) goto retry;
  }

  assertDefined(from);

  if (res==-1 || size!=sizeof(*from) ||
      /*@-type@*/from->sin_family!=AF_INET/*@=type@*/)
    res = -1;

  return static_cast(size_t)(res);
}

/*@unused@*/
inline static void
Wsendto(int s,
	/*@in@*/const void *msg, size_t len,
	int flags,
	/*@in@*/const struct sockaddr *to, socklen_t to_len)
    /*@requires maxRead(msg) >= len@*/
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno@*/  
{
  register ssize_t		res;

  retry:
  res = sendto(s, msg, len, flags, to, to_len);

  if (res==-1) {
    if (errno==EINTR) goto retry;
  }
}

/*@unused@*/
inline static void
Wsendmsg(int s, /*@dependent@*//*@in@*/struct msghdr const *msg, int flags)
    /*@globals internalState, errno@*/
    /*@modifies internalState, errno@*/
{
  register ssize_t		res;

  retry:
  res = sendmsg(s, msg, flags);

  if (res==-1) {
    if (errno==EINTR) goto retry;
  }
}


#endif	/* DHCP_FORWARDER_SRC_WRAPPERS_H */

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
