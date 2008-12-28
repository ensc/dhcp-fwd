// $Id$    --*- c++ -*--

// Copyright (C) 2002, 2008
//               Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 3 of the License.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program. If not, see http://www.gnu.org/licenses/.

#ifndef H_DHCP_FORWARDER_COMPAT_IF_PACKET_H
#define H_DHCP_FORWARDER_COMPAT_IF_PACKET_H

#include <sys/socket.h>
#include <features.h>    /* for the glibc version number */
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#  include <netpacket/packet.h>
#  include <net/ethernet.h>     /* the L2 protocols */
#else
#  include <asm/types.h>
#  include <linux/if_packet.h>
#  include <linux/if_ether.h>   /* The L2 protocols */
#endif

#endif	//  H_DHCP_FORWARDER_COMPAT_IF_PACKET_H
