// $Id$    --*- c++ -*--

/* Based on advio/recvfromflags.c in W.R.Stevens's "Unix Network Programming,
 * Vol I", section 20.2 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/param.h>		/* ALIGN macro for CMSG_NXTHDR() macro */
#include <string.h>

#include "util.h"
#include "recvfromflags.h"

ssize_t
recvfrom_flags(int fd, void *ptr, size_t nbytes,
	       int *flagsp,
	       struct sockaddr *sa, socklen_t *salenptr,
	       struct in_pktinfo *pktp)
{
  struct msghdr			msg;
  struct iovec			iov[1];
  ssize_t			n;

  struct cmsghdr	*cmptr;
  union {
      struct cmsghdr	cm;
      char		control[CMSG_SPACE(sizeof(struct in_addr)) +
				CMSG_SPACE(sizeof(struct in_pktinfo))];
  } control_un;

  msg.msg_control    = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);
  msg.msg_flags      = 0;

  msg.msg_name       = sa;
  msg.msg_namelen    = *salenptr;
  iov[0].iov_base    = ptr;
  iov[0].iov_len     = nbytes;
  msg.msg_iov        = iov;
  msg.msg_iovlen     = 1;

  n = recvmsg(fd, &msg, *flagsp);
  if (n<0) return(n);

  *salenptr          = msg.msg_namelen;	/* pass back results */
  *flagsp            = msg.msg_flags;	/* pass back results */
  
  if (pktp)
    memset(pktp, 0, sizeof(struct in_pktinfo));	/* 0.0.0.0, i/f = 0 */


  if (msg.msg_controllen < sizeof(struct cmsghdr) ||
      (msg.msg_flags & MSG_CTRUNC) || pktp == NULL)
    return(n);

  for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL;
       cmptr = CMSG_NXTHDR(&msg, cmptr)) {

    if (cmptr->cmsg_level == IPPROTO_IP &&
	cmptr->cmsg_type == IP_PKTINFO)
    {
      *pktp = *reinterpret_cast(struct in_pktinfo *)(CMSG_DATA(cmptr));
      continue;
    }
	  
  }
  return(n);
}

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
