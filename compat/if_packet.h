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

#ifndef H_DHCP_FORWARDER_COMPAT_IF_PACKET_H
#define H_DHCP_FORWARDER_COMPAT_IF_PACKET_H

#ifdef HAVE_NETPACKET_PACKET_H
#  include <netpacket/packet.h>
#elif defined(HAVE_LINUX_IF_PACKET_H)
#  include <linux/if_packet.h>
#else
#  error Can not find <netpacket/packet.h> or <linux/if_packet.h>
#endif

#endif	//  H_DHCP_FORWARDER_COMPAT_IF_PACKET_H
