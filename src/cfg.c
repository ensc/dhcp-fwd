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
#include "cfg.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <sys/param.h>
#include <net/if_arp.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "output.h"
#include "parser.h"
#include "wrappers.h"
#include "inet.h"


/*@noreturn@*/ static void
exitFatal(char const msg[], register size_t len) __attribute__ ((noreturn))
  /*:requires maxRead(msg)+1 >= len@*/
  /*@*/ ;

  /*@noreturn@*/
static void scEXITFATAL(/*@in@*//*@sef@*/char const *msg) /*@*/; 
#define scEXITFATAL(msg)	exitFatal(msg, sizeof(msg)-1)


inline static void
exitFatal(char const msg[], register size_t len)
{
  (void)write(2, msg, len);
  (void)write(2, "\n", 1);

  exit(2);
}


inline static in_addr_t
sockaddrToInet4(/*@in@*//*@sef@*/struct sockaddr const *addr)
    /*@*/
{

  if (/*@-type@*/addr->sa_family!=AF_INET/*@=type@*/)
    scEXITFATAL("Interface has not IPv4 address");

  return (reinterpret_cast(struct sockaddr_in const *)(addr))->sin_addr.s_addr;
}

inline static void
initClientFD(struct FdInfo *fd,
	     /*@in@*//*@observer@*/struct InterfaceInfo const *iface)
    /*@globals internalState, fileSystem@*/
    /*@modifies internalState, fileSystem, *fd@*/
{
  struct sockaddr_in	s;
  int const		ON = 1;

  assert(fd!=0 && iface!=0);
  
  fd->iface = iface;
  fd->fd    = Esocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /*@-boundsread@*/
  Esetsockopt(fd->fd, SOL_IP,     IP_PKTINFO,      &ON, sizeof ON);
  Esetsockopt(fd->fd, SOL_SOCKET, SO_BROADCAST,    &ON, sizeof ON);
  Esetsockopt(fd->fd, SOL_SOCKET, SO_BINDTODEVICE, iface->name, strlen(iface->name)+1);
    /*@=boundsread@*/

    /*@-boundswrite@*/
  memset(&s, 0, sizeof(s));
    /*@=boundswrite@*/
  
    /*@-type@*/
  s.sin_family      = AF_INET; /*@=type@*/
  s.sin_port        = htons(DHCP_PORT_SERVER);
  s.sin_addr.s_addr = htonl(INADDR_ANY);

  Ebind(fd->fd, &s);
}

inline static void
initRawFD(/*@out@*/int *fd)
    /*@globals internalState@*/
    /*@modifies internalState, *fd@*/
    /*@requires maxSet(fd)==0@*/
{
  assert(fd!=0);

  *fd = Esocket(AF_PACKET, SOCK_RAW, 0xFFFF);
}

inline static void
initSenderFD(/*@out@*/int *fd)
    /*@globals internalState, fileSystem@*/
    /*@modifies internalState, fileSystem, *fd@*/
    /*@requires maxRead(fd)==0 /\ maxSet(fd)==0@*/
{
  struct sockaddr_in	s;

  assert(fd!=0);

  *fd = Esocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /*@-boundswrite@*/
  memset(&s, 0, sizeof(s));
    /*@=boundswrite@*/
  
    /*@-type@*/
  s.sin_family      = AF_INET; /*@=type@*/
  s.sin_port        = htons(DHCP_PORT_CLIENT);
  s.sin_addr.s_addr = htonl(INADDR_ANY);

  Ebind(*fd, &s);
}

inline static void
sockaddrToHwAddr(/*@in@*/struct sockaddr const	*addr,
		 /*@out@*/uint8_t		mac[],
		 /*@out@*/size_t		*len)
    /*@modifies *mac, *len@*/
    /*@requires maxSet(len)==1 /\ maxSet(mac)>=ETH_ALEN @*/
    /*@ensures  maxRead(mac)==(*len)@*/
{
  assert(addr!=0);

  switch (addr->sa_family) {
    case ARPHRD_EETHER	:
    case ARPHRD_IEEE802	:
    case ARPHRD_ETHER	:  *len = ETH_ALEN; break;
    default		:  scEXITFATAL("Unsupported hardware-type"); 
  }

    /*@-boundsread@*/
  assert(*len <= ETH_ALEN);
    /*@=boundsread@*/

    /*@-boundswrite@*/
  memcpy(mac, addr->sa_data, *len);
    /*@=boundswrite@*/
}

inline static void
fillInterfaceInfo(struct InterfaceInfoList *ifs)
    /*@globals fileSystem, internalState@*/
    /*@modifies ifs->dta, fileSystem, internalState@*/
    /*@requires maxRead(ifs->dta)==ifs->len /\ maxRead(ifs)==0@*/
{
  int			fd = Esocket(AF_INET, SOCK_DGRAM, 0);
  size_t		i;

  assert(ifs->len==0 || ifs->dta!=0);

  for (i=0; i<ifs->len; ++i) {
    struct ifreq		iface;
    struct InterfaceInfo	*ifinfo;

    assert(ifs->dta!=0);
    ifinfo = &ifs->dta[i];

    memcpy(iface.ifr_name, ifinfo->name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFINDEX,  &iface)==-1) goto err;
      /*@-usedef@*/
    if (iface.ifr_ifindex<0)                  goto err;
    ifinfo->if_idx = (unsigned int)(iface.ifr_ifindex);
      /*@=usedef@*/

    if (ioctl(fd, SIOCGIFADDR,   &iface)==-1) goto err;
    ifinfo->if_real_ip = sockaddrToInet4(&iface.ifr_addr);

    if (ioctl(fd, SIOCGIFMTU,    &iface)==-1) goto err;
    ifinfo->if_mtu = static_cast(size_t)(iface.ifr_mtu);

    if (ioctl(fd, SIOCGIFHWADDR, &iface)==-1) goto err;
    sockaddrToHwAddr(&iface.ifr_hwaddr,
		     ifinfo->if_mac, &ifinfo->if_maclen);

    if (ifinfo->if_ip==INADDR_NONE)
      ifinfo->if_ip = ifinfo->if_real_ip;
  }
  
  Eclose(fd);

  return;
  err:
  perror("ioctl()");
  scEXITFATAL("Can not get interface information");
}

inline static void
initFDs(/*@out@*/struct FdInfoList 		*fds,
	/*@in@*/struct ConfigInfo const	* const	cfg)
    /*@globals internalState, fileSystem@*/
    /*@modifies internalState, fileSystem, *fds@*/
    /*@requires maxRead(fds)>=0 /\ maxSet(fds)>=0 /\ maxSet(fds->sender_fd)>=0@*/
{
  size_t					i, idx;
  struct InterfaceInfoList const * const	ifs = &cfg->interfaces;

  initSenderFD(&fds->sender_fd);
  initRawFD(&fds->raw_fd);
  
  fds->len = ifs->len;
  fds->dta = static_cast(struct FdInfo*)(Emalloc(fds->len *
						 (sizeof(*fds->dta))));

  assert(fds->dta!=0 || fds->len==0);
  assert(ifs->dta!=0 || ifs->len==0);
  
  for (idx=0, i=0; i<ifs->len; ++i, ++idx) {
    assert(ifs->dta!=0);
    assert(fds->dta!=0);
    
    initClientFD(&fds->dta[idx], &ifs->dta[i]);
  }
}

inline static void
getConfig(/*@in@*/char const				*filename,
	  /*@out@*//*@dependent@*/struct ConfigInfo	*cfg)
    /*@globals internalState, fileSystem@*/
    /*@modifies *cfg, internalState, fileSystem@*/
    /*@requires maxRead(cfg)>=0
             /\ PATH_MAX >= 1
             /\ (maxSet(cfg->chroot_path)+1)  == PATH_MAX
	     /\ (maxSet(cfg->logfile_name)+1) == PATH_MAX
	     /\ (maxSet(cfg->pidfile_name)+1) == PATH_MAX@*/
    /*@ensures  maxRead(cfg->chroot_path)>=0
             /\ maxRead(cfg->logfile_name)>=0
	     /\ maxRead(cfg->pidfile_name)>=0@*/
{
  cfg->interfaces.dta = 0;
  cfg->interfaces.len = 0;

  cfg->servers.dta    = 0;
  cfg->servers.len    = 0;

  cfg->ulimits.dta    = 0;
  cfg->ulimits.len    = 0;

  cfg->uid            = 99;
  cfg->gid            = 99;

  cfg->chroot_path[0]  = '\0';
  cfg->logfile_name[0] = '\0';
  cfg->loglevel        = 0;

  cfg->pidfile_name[0] = '\0';
  
  parse(filename, cfg);
    /*@-boundsread@*/
  fillInterfaceInfo(&cfg->interfaces);
    /*@=boundsread@*/
}

  /*@maynotreturn@*/
inline static void
showVersion() /*@*/
{
  (void)write(1, PACKAGE_STRING, strlen(PACKAGE_STRING));
  (void)write(1, "\n", 1);
}

  /*@maynotreturn@*/
inline static void
showHelp(/*@in@*//*@nullterminated@*/char const *cmd) /*@*/
{
  char const	msg[] = (" [-v] [-h] [-c <filename>] [-d]\n\n"
			 "  -v              show version\n"
			 "  -h              show help\n"
			 "  -c <filename>   read configuration from <filename>\n"
			 "  -d              debug-mode; do not fork separate process\n\n"
			 "Report bugs to Enrico Scholz <"
			 PACKAGE_BUGREPORT
			 ">\n");

  showVersion();
  (void)write(1, "\nusage: ", 8	);
  (void)write(1, cmd, strlen(cmd));
  (void)write(1, msg, strlen(msg));
}

inline static void
limitResources(/*@in@*/struct UlimitInfoList const *limits)
    /*@globals internalState@*/
    /*@modifies internalState@*/
    /*@requires (maxRead(limits->dta)+1) == limits->len@*/
{
  size_t			i;

  assert(limits->len==0 || limits->dta!=0);
  
  for (i=0; i<limits->len; ++i) {
    assert(limits->dta!=0);
    Esetrlimit(limits->dta[i].code, &limits->dta[i].rlim);
  }
}

inline static void
freeLimitList(struct UlimitInfoList *limits)
    /*@modifies limits->dta, limits->len@*/
    /*@requires only   limits->dta@*/
    /*@ensures  isnull limits->dta@*/
{
  /*@-nullpass@*/free(limits->dta);/*@=nullpass@*/
  limits->dta=0;
  limits->len=0;
}

  /*@-superuser@*/
int
initializeSystem(int argc, char *argv[],
		 struct InterfaceInfoList *	ifs,
		 struct ServerInfoList *	servers,
		 struct FdInfoList *		fds)
{
  struct ConfigInfo		cfg;
  /*@dependent@*/char const *	cfg_name = CFG_FILENAME;
  int				i = 1;
  pid_t				pid;
  int				pidfile_fd;
  bool				do_fork  = true;

  while (i<argc) {
      /*@-boundsread@*/
    if      (strcmp(argv[i], "-v")==0) { showVersion();     exit(0); }
    else if (strcmp(argv[i], "-h")==0) { showHelp(argv[0]); exit(0); }
    else if (strcmp(argv[i], "-d")==0)             {      do_fork  = false;   }
    else if (strcmp(argv[i], "-c")==0 && i+1<argc) { ++i; cfg_name = argv[i]; }
      /*@=boundsread@*/
    else scEXITFATAL("Bad cmd-line option; use '-h' to get help");

    ++i;
  }

    /*@-boundswrite@*/
  getConfig(cfg_name, &cfg);
  initFDs(fds, &cfg);
    /*@=boundswrite@*/

  pidfile_fd = open(cfg.pidfile_name, O_WRONLY|O_CREAT);
  if (pidfile_fd==-1) {
    perror("open()");
    exit(1);
  }
  
  *ifs     = cfg.interfaces;
  *servers = cfg.servers;

  Eclose(0);

  if (do_fork) pid = fork();
  else         pid = 0;

  switch (pid) {
    case -1	:
      perror("fork()");
      break;
      
    case 0	:
      if (cfg.chroot_path[0]!='\0') {
	Echdir (cfg.chroot_path);
	Echroot(cfg.chroot_path);
      }
  
      Esetgid(cfg.uid);
      Esetuid(cfg.gid);

      limitResources(&cfg.ulimits);
      break;
      
    default	:
      writeUInt(pidfile_fd, pid);
      (void)write(pidfile_fd, "\n", 1);
      break;
  }
  
  freeLimitList(&cfg.ulimits);

    /* It is too late to handle an error here. So just ignore the error... */
  (void)close(pidfile_fd);
  return pid;
}
  /*@=superuser@*/

  // Local Variables:
  // compile-command: "make -k -C .."
  // fill-column: 80
  // End:
