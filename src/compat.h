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

#ifndef DHCP_FORWARDER_COMPAT_H
#define DHCP_FORWARDER_COMPAT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <features.h>
#include <inttypes.h>

  /* Shamelessly stolen from glibc's <sys/cdefs.h> */
#ifndef __flexarr
/* Support for flexible arrays.  */
# if defined(__GNUC__) && (__GNUC__>=3)
/* GCC 2.97 supports C99 flexible array members.  */
#  define __flexarr      []
# else
#  ifdef __GNUC__
#   define __flexarr     [0]
#  else
#   if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#    define __flexarr    []
#   else
/* Some other non-C99 compiler.  Approximate with [1].  */
#    define __flexarr    [1]
#   endif
#  endif
# endif
#endif

#ifndef ETH_ALEN
#  define ETH_ALEN		6
#endif

#ifndef ETHERTYPE_IP
#  define ETHERTYPE_IP		0x0800
#endif

#ifndef IP_DF
#  define IP_DF			0x4000
#endif

#ifndef __GLIBC__
struct ether_header
{
  uint8_t  ether_dhost[ETH_ALEN];      /* destination eth addr */
  uint8_t  ether_shost[ETH_ALEN];      /* source ether addr    */
  uint16_t ether_type;                 /* packet type ID field */
} __attribute__ ((__packed__));
#endif

#ifndef TEMP_FAILURE_RETRY
# define TEMP_FAILURE_RETRY(expression) \
  (__extension__                                                              \
    ({ long int __result;                                                     \
       do __result = (long int) (expression);                                 \
       while (__result == -1L && errno == EINTR);                             \
       __result; }))
#endif

#endif	/* DHCP_FORWARDER_COMPAT_H */
