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

#ifndef DHCP_FORWARDER_DHCP_H
#define DHCP_FORWARDER_DHCP_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "compat.h"
#include "util.h"

struct DHCPHeader  {
    uint8_t	op;
    uint8_t	htype;
    uint8_t	hlen;
    uint8_t	hops;

    uint32_t	xid;
    uint16_t	secs;
    
    struct __attribute__((__packed__)) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int	mbz1:7;
	unsigned int	bcast:1;
	unsigned int	mbz2:8;
#else
	unsigned int	bcast:1;
	unsigned int	mbz1:7;
	unsigned int	mbz2:8;
#endif	
    }		flags;

    in_addr_t	ciaddr;
    in_addr_t	yiaddr;
    in_addr_t	siaddr;
    in_addr_t	giaddr;
    
    uint8_t	chaddr[16];
    uint8_t	sname[64];
    uint8_t	file[128];
} __attribute__((__packed__));

struct DHCPOptions {
    uint32_t			cookie;
    __extension__ char		data __flexarr;
} __attribute__((__packed__));

struct DHCPSingleOption {
    uint8_t			code;
    uint8_t			len;
    __extension__ uint8_t	data __flexarr;
} __attribute__((__packed__));

enum {
  MAX_HOPS	= 16
};

enum {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  DHCP_COOKIE	= 0x63538263
#else
  DHCP_COOKIE	= 0x63825363
#endif
};

enum {
  opBOOTREQUEST	= 1u,
  opBOOTREPLY	= 2u
};

enum {
  cdPAD 	= 0u,
  cdRELAY_AGENT = 82u,
  cdEND 	= 255u
};

enum {
  agCIRCUITID	= 1u,
  agREMOTEID	= 2u
};

inline static size_t
DHCP_getOptionLength(struct DHCPSingleOption const *opt)
{
  switch (opt->code) {
    case cdPAD	:
    case cdEND	:  return 1;
    default	:  return opt->len + 2;
  }
}

inline static struct DHCPSingleOption *
DHCP_nextSingleOption(/*@returned@*/struct DHCPSingleOption *opt)
{
  size_t cnt = DHCP_getOptionLength(opt);

  return (reinterpret_cast(struct DHCPSingleOption *)
	  (reinterpret_cast(char *)(opt) + cnt));
}

inline static struct DHCPSingleOption const *
DHCP_nextSingleOptionConst(/*@returned@*/struct DHCPSingleOption const *opt)
{
  size_t cnt = DHCP_getOptionLength(opt);

  return (reinterpret_cast(struct DHCPSingleOption const *)
	  (reinterpret_cast(char const *)(opt) + cnt));
}

#endif	/* DHCP_FORWARDER_DHCP_H */

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
