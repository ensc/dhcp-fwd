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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "logging.h"
#include "dhcp.h"

void
logDHCPPackage(char const *data, size_t	len,
	       struct in_pktinfo const		*pkinfo,
	       void const			*addr)
{
  /*@temp@*/char		buffer[256];	/* see max. calculation below */
  char				addr_buffer[128];	/* adjust if needed */
  /*@dependent@*/char const	*msg = 0;
  struct tm			tm;
  struct timeval		tv;
  int				error = errno;
  struct sockaddr const		*saddr = reinterpret_cast(struct sockaddr const *)(addr);
  struct DHCPHeader const	*header = reinterpret_cast(struct DHCPHeader const *)(data);
  

  (void)gettimeofday(&tv, 0);
  (void)localtime_r(&tv.tv_sec, &tm);
  
  if (strftime(buffer, sizeof buffer, "%T", &tm)==-1) goto err;		/*   8 chars */
  if (snprintf(buffer+strlen(buffer), 10,
	       ".%06li: ", tv.tv_usec)==-1) goto err;			/*  +9 chars
									 => 17 chars */
  
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

      /* max. 48 chars */
    if (inet_ntop(saddr->sa_family, ptr, addr_buffer, sizeof addr_buffer)==0) {
      strcpy(addr_buffer, "< ???? >");
    }

      /* max 14 + 48 + 10 = 72 chars */
    snprintf(buffer, 48+10+1, "from %s (if #%i): ", addr_buffer, pkinfo->ipi_ifindex);
    
    if (len<sizeof(struct DHCPHeader)) {
	/* + max 24 + 10 = 34 chars  ==> max 106 chars */
      snprintf(buffer + strlen(buffer), 34+1, "Broken package with len %lu", len);
    }
    else {
      struct in_addr		ip;
      bool			is_faulty = false;

	/* + 9 chars ==> max 115 chars */
      snprintf(buffer+strlen(buffer), 9+1, "%08x ", header->xid);
      switch (header->op) {	/* + <=24 chars  ==> max 139 chars */
	case opBOOTREQUEST:
	  strcat(buffer, "BOOTREQUEST from ");	/* + 17 chars */
	  ip.s_addr = header->ciaddr;
	  break;
	case opBOOTREPLY:
	  strcat(buffer, "BOOTREPLY to ");	/* + 17 chars */
	  ip.s_addr = header->yiaddr;
	  break;
	default:
	    /* + max 14 + 10 = 24 chars */
	  snprintf(buffer+strlen(buffer), 24+1, "<UNKNOWN> (%u), ", header->op);
	  is_faulty = true;
	  break;
      }

	/* + <=15 chars ==> max 154 chars */
      if (!is_faulty) {
	assertDefined(&ip);
	strncat(buffer, inet_ntoa(ip), 15);
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
