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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <net/if_arp.h>

#include <stdio.h>

#include "output.h"
#include "parser.h"
#include "wrappers.h"
#include "inet.h"

#define EXITFATAL(msg)	exitFatal(msg, sizeof(msg))

static void
exitFatal(char const msg[], register size_t len) __attribute__ ((noreturn));
  
inline static void
exitFatal(char const msg[], register size_t len)
{
  write(2, msg, len);
  write(2, "\n", 1);

  exit(2);
}


inline static in_addr_t
sockaddrToInet4(/*@dependent@*/struct sockaddr const *addr)
{

  if (addr->sa_family!=AF_INET) EXITFATAL("Interface has not IPv4 address");

  return (reinterpret_cast(struct sockaddr_in const *)(addr))->sin_addr.s_addr;
}

inline static void
initClientFD(struct FdInfo *fd,
	     struct InterfaceInfo const *iface)
{
  struct		sockaddr_in	s;
  int const		ON = 1;

  assert(fd!=0 && iface!=0);
  
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

inline static void
initRawFD(int *fd)
{
  assert(fd!=0);

  *fd = Esocket(AF_PACKET, SOCK_RAW, 0xFFFF);
}

inline static void
initSenderFD(int *fd)
{
  struct sockaddr_in	s;

  assert(fd!=0);

  *fd = Esocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  memset(&s, 0, sizeof(s));
  s.sin_family      = AF_INET;
  s.sin_port        = htons(DHCP_PORT_CLIENT);
  s.sin_addr.s_addr = htonl(INADDR_ANY);

  Ebind(*fd, &s);
}

inline static void
sockaddrToHwAddr(/*@in@*/struct sockaddr const	*addr,
		 /*@out@*/uint8_t		mac[],
		 /*@out@*/size_t		*len)
{
  assert(addr!=0);

  switch (addr->sa_family) {
    case ARPHRD_EETHER	:
    case ARPHRD_IEEE802	:
    case ARPHRD_ETHER	:  *len = ETH_ALEN; break;
    default		:  EXITFATAL("Unsupported hardware-type");
  }

  memcpy(mac, addr->sa_data, *len);
}

inline static void
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
    ifs->dta[i].if_mtu = static_cast(size_t)(iface.ifr_mtu);

    if (ioctl(fd, SIOCGIFHWADDR, &iface)==-1) goto err;
    sockaddrToHwAddr(&iface.ifr_hwaddr,
		     ifs->dta[i].if_mac, &ifs->dta[i].if_maclen);
  }
  Eclose(fd);

  return;
  
  err:
  perror("ioctl()");
  EXITFATAL("Can not set interface information");
}

inline static void
initFDs(/*@out@*/struct FdInfoList 		*fds,
	/*@in@*/struct ConfigInfo const	* const	cfg)
{
  register size_t				i;
  register size_t				idx;
  struct InterfaceInfoList const * const	ifs = &cfg->interfaces;
  
  initSenderFD(&fds->sender_fd);
  initRawFD(&fds->raw_fd);
  
  fds->len = ifs->len;
  fds->dta = reinterpret_cast(struct FdInfo*)(Emalloc(fds->len * (sizeof(fds->dta[0]))));

  
  for (idx=0, i=0; i<ifs->len; ++i) {
    initClientFD(fds->dta + idx, ifs->dta+i);
    ++idx;
  }
}

inline static void
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

int
initializeSystem(int argc, char *argv[],
		 struct InterfaceInfoList *	ifs,
		 struct ServerInfoList *	servers,
		 struct FdInfoList *		fds)
{
  struct ConfigInfo		cfg;
  char const *			cfg_name = CFG_FILENAME;
  register bool			do_fork  = true;
  register int			i = 1;
  register pid_t		pid;
  register int			pidfile_fd;

  while (i<argc) {
    if      (strcmp(argv[i], "-c")==0) { cfg_name = argv[i+1]; i+=2; }
    else if (strcmp(argv[i], "-d")==0) { do_fork  = false;     ++i;  }
    else EXITFATAL("Bad cmd-line option");
  }
  
  getConfig(cfg_name, &cfg);

  initFDs(fds, &cfg);

  pidfile_fd = open(cfg.pidfile_name, O_WRONLY | O_CREAT);
  if (pidfile_fd==-1) {
    perror("open()");
    exit(1);
  }
  
  *ifs     = cfg.interfaces;
  *servers = cfg.servers;
  
  if (do_fork) pid = fork();
  else         pid = 0;

  switch (pid) {
    case -1	:
    case 0	:
      if (cfg.chroot_path[0]!=0) {
	Echdir (cfg.chroot_path);
	Echroot(cfg.chroot_path);
      }
  
      Esetgid(cfg.uid);
      Esetuid(cfg.gid);
    default	:
      writeUInt(pidfile_fd, pid);
      break;
  }
  
  close(pidfile_fd);
  return pid;
}


  // Local Variables:
  // compile-command: "make -k -C .."
  // fill-column: 80
  // End:
