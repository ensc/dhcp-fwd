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

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <net/if_arp.h>

#include <stdio.h>

#include "parser.h"
#include "wrappers.h"
#include "inet.h"


inline static in_addr_t
sockaddrToInet4(/*@dependent@*/struct sockaddr const *addr)
{

  if (addr->sa_family!=AF_INET) {
    (void)write(2, "Interface has not IPv4 address\n", 31);
    exit(2);
  }

  return (reinterpret_cast(struct sockaddr_in const *)(addr))->sin_addr.s_addr;
}

static void
assertInitialized()
{
}

void
initClientFD(struct FdInfo *fd,
	     struct InterfaceInfo const *iface)
{
  struct		sockaddr_in	s;
  int const		ON = 1;

  assert(fd!=0 && iface!=0);
  assertInitialized();
  
  fd->iface = iface;
  fd->fd    = Esocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  Esetsockopt(fd->fd, SOL_IP,     IP_PKTINFO,      &ON, sizeof(ON));
  Esetsockopt(fd->fd, SOL_SOCKET, SO_BROADCAST,    &ON, sizeof(ON));
  Esetsockopt(fd->fd, SOL_SOCKET, SO_BINDTODEVICE, iface->name, strlen(iface->name)+1);
  
  memset(&s, 0, sizeof(s));
  s.sin_family      = AF_INET;
  s.sin_port        = htons(DHCP_PORT_SERVER);
  s.sin_addr.s_addr = htonl(INADDR_ANY);

  Ebind(fd->fd, &s);
}

void
initAnswerFD(int *fd)
{
  assert(fd!=0);
  assertInitialized();

  *fd = Esocket(AF_PACKET, SOCK_RAW, 0xFFFF);
}

inline void
initSenderFD(int *fd)
{
  struct sockaddr_in	s;

  assert(fd!=0);
  assertInitialized();

  *fd = Esocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  memset(&s, 0, sizeof(s));
  s.sin_family      = AF_INET;
  s.sin_port        = htons(DHCP_PORT_CLIENT);
  s.sin_addr.s_addr = htonl(INADDR_ANY);

  Ebind(*fd, &s);
}

static void
sockaddrToHwAddr(/*@in@*/struct sockaddr const	*addr,
		 /*@out@*/uint8_t		mac[],
		 /*@out@*/size_t		*len)
{
  if (addr==0) {
    (void)write(2, "Invalid pointer to hw-address\n", 30);
    abort();
  }
  
  switch (addr->sa_family) {
    case ARPHRD_EETHER	:
    case ARPHRD_IEEE802	:
    case ARPHRD_ETHER	:  *len = ETH_ALEN; break;
    default		:
      (void)write(2, "Unsupported hardware-type\n", 26);
      exit(2);
  }

  memcpy(mac, addr->sa_data, *len);
}
		 

static void
fillInterfaceInfo(struct InterfaceInfoList *ifs)
{
  size_t		i;
  int			fd = Esocket(AF_INET, SOCK_DGRAM, 0);
  
  for (i=0; i<ifs->len; ++i) {
    struct ifreq	iface;
    
    memcpy(iface.ifr_name, ifs->dta[i].name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFINDEX,  &iface)==-1) goto err;
    ifs->dta[i].if_idx = iface.ifr_ifindex;

    if (ioctl(fd, SIOCGIFADDR,   &iface)==-1) goto err;
    ifs->dta[i].if_ip  = sockaddrToInet4(&iface.ifr_addr);

    if (ioctl(fd, SIOCGIFMTU,    &iface)==-1) goto err;
    ifs->dta[i].if_mtu = reinterpret_cast(size_t)(iface.ifr_mtu);

    if (ioctl(fd, SIOCGIFHWADDR, &iface)==-1) goto err;
    sockaddrToHwAddr(&iface.ifr_hwaddr,
		     ifs->dta[i].if_mac, &ifs->dta[i].if_maclen);
  }
  Eclose(fd);

  return;
  
  err:
  (void)write(2, "ioctl() failed\n", 15);
  exit(2);
}

void
getConfig(char const		*filename,
	  struct ConfigInfo	*cfg)
{
  cfg->interfaces.dta = 0;
  cfg->interfaces.len = 0;

  cfg->servers.dta    = 0;
  cfg->servers.len    = 0;

  cfg->uid            = 99;
  cfg->gid            = 99;

  cfg->chroot_path[0]  = 0;
  cfg->logfile_name[0] = 0;
  cfg->loglevel        = 0;

  cfg->pidfile_name[0] = 0;
  
  parse(filename, cfg);
  fillInterfaceInfo(&cfg->interfaces);
}

  // Local Variables:
  // compile-command: "make -k -C .."
  // fill-column: 80
  // End:
