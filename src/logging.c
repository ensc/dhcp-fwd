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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "splint.h"

#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "output.h"
#include "logging.h"
#include "dhcp.h"

static void WRITE(int FD, /*@sef@*//*@observer@*/char const MSG[])
  /*@globals internalState@*/
  /*@modifies internalState@*/ ;

  /*@-sizeofformalarray@*/
#define WRITE(FD, MSG)	(void)write(FD, (MSG), sizeof(MSG)-1)
  /*@=sizeofformalarray@*/

inline static char *
Xinet_ntop(sa_family_t af, /*@in@*/void const *src,
	   /*@returned@*//*@out@*/char *dst, size_t cnt)
    /*@requires cnt>=8 /\ maxSet(dst) >= cnt@*/
    /*@modifies *dst@*/
{
  if (inet_ntop(af, src, dst, cnt)==0) {
    strcpy(dst, "< ???? >");
  }

  return dst;
}

typedef /*@out@*/ char *	char_outptr;

inline static void
Xsnprintf(/*@out@*/char_outptr * const buffer, size_t * const len,
	  /*@in@*/char const * const format, ...)
    /*@requires notnull *buffer@*/
    /*@requires maxRead(*buffer) >= 0@*/
    /*@globals internalState@*/
    /*@modifies internalState, *buffer, *len@*/
{
  va_list	ap;
  int		l;
  
  va_start(ap, format);
  l = vsnprintf(*buffer, *len, format, ap);
  if (l==-1 || l>*len) {
    WRITE(2, "\n\nBuffer not large enough for snprintf(\"");
    (void)write(2, format, strlen(format)-1);
    WRITE(2, "\");\nthere are ");
    (void)writeUInt(2, *len);
    WRITE(2, " chars available but ");
    writeUInt(2, static_cast(unsigned int)(l));
    WRITE(2, " required\n\n");
  }
  else {
    *len    -= l;
    *buffer += l;
  }
}

inline static void
Xstrncat(/*@unique@*/char * const buffer,
	 /*@in@*/char const * const what, size_t *len)
    /*@modifies *buffer, *len@*/
{
  size_t const		what_len = strlen(what);
  
  if (what_len<*len) {
    strcat(buffer, what);
    *len -= what_len;
  }
}


void
logDHCPPackage(char const *data, size_t	len,
	       struct in_pktinfo const		*pkinfo,
	       void const			*addr)
{
  /*@temp@*/char		buffer[256];
  char 				*buffer_ptr;
  char				addr_buffer[128];	/* adjust if needed */
  /*@dependent@*/char const	*msg = 0;
  struct tm			tmval;
  struct timeval		tv;
  size_t			avail;
  int				error = errno;
  struct sockaddr const		*saddr = reinterpret_cast(struct sockaddr const *)(addr);
  struct DHCPHeader const	*header = reinterpret_cast(struct DHCPHeader const *)(data);
  

  (void)gettimeofday(&tv, 0);
  (void)localtime_r(&tv.tv_sec, &tmval);
  
  if (strftime(buffer, sizeof buffer, "%T", &tmval)==-1) goto err;	/*   8 chars */
  avail      = sizeof(buffer)-strlen(buffer);
  buffer_ptr = buffer + strlen(buffer);
  Xsnprintf(&buffer_ptr, &avail, ".%06li: ", tv.tv_usec);
  
  (void)write(2, buffer, strlen(buffer));

  if (len==-1) {
    msg = strerror(error);
  }
  else {
    void const		*ptr;
    switch (saddr->sa_family) {
      case AF_INET	 : ptr = &reinterpret_cast(struct sockaddr_in  *)(addr)->sin_addr;  break;
      case AF_INET6	:  ptr = &reinterpret_cast(struct sockaddr_in6 *)(addr)->sin6_addr; break;
      default		:  ptr = saddr->sa_data; break;
    }

    (void)Xinet_ntop(saddr->sa_family, ptr, addr_buffer, sizeof addr_buffer);

    buffer_ptr = buffer;
    avail      = sizeof(buffer);
    
#if 1
    Xsnprintf(&buffer_ptr, &avail, "from %s (", addr_buffer) ;
    
    (void)Xinet_ntop(saddr->sa_family, &pkinfo->ipi_addr, addr_buffer, sizeof addr_buffer);
    Xsnprintf(&buffer_ptr, &avail, "%i, %s, ", pkinfo->ipi_ifindex, addr_buffer);

    (void)Xinet_ntop(saddr->sa_family, &pkinfo->ipi_spec_dst, addr_buffer, sizeof addr_buffer);
    Xsnprintf(&buffer_ptr, &avail, "%s)): ", addr_buffer);
#else
    Xsnprintf(&buffer_ptr, &avail, "from %s (if #%i): ", addr_buffer, pkinfo->ipi_ifindex);
#endif
    
    if (len<sizeof(struct DHCPHeader)) {
      Xsnprintf(&buffer_ptr, &avail, "Broken package with len %lu", len);
    }
    else {
      struct in_addr		ip;
      bool			is_faulty = false;

      Xsnprintf(&buffer_ptr, &avail, "%08x ", header->xid);
      switch (header->op) {
	case opBOOTREQUEST:
	  Xstrncat(buffer, "BOOTREQUEST from ", &avail);
	  ip.s_addr = header->ciaddr;
	  break;
	case opBOOTREPLY:
	  Xstrncat(buffer, "BOOTREPLY to ", &avail);
	  ip.s_addr = header->yiaddr;
	  break;
	default:
	  Xsnprintf(&buffer_ptr, &avail, "<UNKNOWN> (%u), ", header->op);
	  is_faulty = true;
	  break;
      }

      if (!is_faulty) {
	assertDefined(&ip);
	Xstrncat(buffer, inet_ntoa(ip), &avail);
      }
    }

    msg = buffer;
  }
  
  (void)write(2, msg, strlen(msg));
  (void)write(2, "\n", 1);

  err:
  errno = error;
}

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
