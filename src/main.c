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

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/param.h>
#include <errno.h>
#include <netpacket/packet.h>

#include "config.h"
#include "dhcp.h"
#include "inet.h"
#include "wrappers.h"
#include "recvfromflags.h"

#include "assertions.h"
#include "logging.h"

static struct ServerInfoList		servers;
static struct FdInfoList		fds;

static void
fillFDSet(/*@out@*/fd_set			*fd_set,
	  /*@out@*/int				*max)
{
  size_t	i = 0;
  FD_ZERO(fd_set);
  *max = -1;
  
  for (i=0; i<fds.len; ++i) {
    *max = MAX(*max, fds.dta[i].fd);
    FD_SET(fds.dta[i].fd, fd_set);
  }
}

static bool
isValidHeader(struct DHCPHeader *header)
{
  char const	*reason = 0;
  
  if      (header->flags.mbz!=0)   { reason = "Invalid flags field"; }
  else if (header->hops>=MAX_HOPS) { reason = "Looping detected"; }
  else if (header->hlen!=6)        { reason = "Unknown/unsupported hardware-type"; }
  else switch (header->op) {
    case opBOOTREPLY	:
    case opBOOTREQUEST	:  break;
    default		:  reason = "Unknown operation"; break;
  };

  if (reason!=0) {
    LOGSTR(reason);
    LOG("\n");
  }

    
  return reason==0;
}

static bool
isValidOptions(/*@in@*/struct DHCPOptions const	*options,
	       size_t				o_len)
{
  bool				seen_end     = false;
  struct DHCPSingleOption const	*opt         = reinterpret_cast(struct DHCPSingleOption const *)(options->data);
  struct DHCPSingleOption const	*end_options = reinterpret_cast(struct DHCPSingleOption const *)(reinterpret_cast(uint8_t const *)(options) + o_len);
  
  if (o_len==0) return true;
  if (o_len<=4) return false;
  if (options->cookie != DHCP_COOKIE) return false;

  do {
    switch (opt->code) {
      case cdEND	:  seen_end = true; break;
      default		:  break;
    }

    opt = DHCP_nextSingleOptionConst(opt);
  } while (opt < end_options);

  return (seen_end && opt==end_options);
}

static size_t
fillOptions(void				*option_ptr,
	    /*@in@*/struct InterfaceInfo const	*iface)
{
  struct DHCPSingleOption 		*opt     = static_cast(struct DHCPSingleOption *)(option_ptr);
  struct DHCPSingleOption		*end_opt = 0, *relay_opt = 0;
  size_t				len;
  
  do {
    switch (opt->code) {
      case cdRELAY_AGENT	:  relay_opt = opt; break;
      case cdEND		:  end_opt   = opt; break;
      default			:  break;
    }

    opt = DHCP_nextSingleOption(opt);
  } while (end_opt==0);

    /* Determine used space until end-tag and add space for the end-tag itself
     * (1 octet). */
  len  = (reinterpret_cast(char *)(end_opt) -
	  static_cast(char *)(option_ptr) + 1u);
  
    /* Check if a relay-agent field exists already; if not, replace the end-tag
       
     *  - - - - - - -----
     * |           | END |
     *  - - - - - - -----
     * with
     *  - - - - - - ----- ----- ----- ----- ------------ -----
     * |           | 82  | len | sub | sub | ... id ... | END |
     * |           |     |     | opt | len |            |     |
     *  - - - - - - ----- ----- ----- ----- ------------ -----
     * */
  if (relay_opt==0) {
    assert(strlen(iface->aid)<=IFNAMSIZ);
    
      /* Add space needed for our RFC 3046 agent id. See figure above for
       * details. */
    len += 4 + strlen(iface->aid);

      /* 'len' should now have the length of the complete option-field. RFC 2131
       * sets a lower limit of 312 octets, so we are checking against this
       * value. Since the function got only the real options without the
       * magic-cookie, 4 octets must be added.
       *
       * Because the underlying buffer was declared to hold more than this
       * minimum amount, we can exclude overflows here.
       *
       * Further versions of this software should make it possible to configure
       * the maximum size at runtime. */
    if (len+4 < 312) {
	/* replace old end-tag with our information */
      end_opt->code    = cdRELAY_AGENT;
      end_opt->data[0] = agCIRCUITID;	/* code for circuit id as specified by RFC 3046 */
      end_opt->data[1] = static_cast(uint8_t)(strlen(iface->aid));
      end_opt->len     = 2 + end_opt->data[1];
      memcpy(end_opt->data+2, iface->aid, end_opt->data[1]);

	/* set new end-tag */
      end_opt = DHCP_nextSingleOption(end_opt);
      end_opt->code = cdEND;
    }
  }

  return len;
}

static uint16_t
calculateCheckSum(/*@in@*/void const * const	dta,
		  size_t size,
		  uint32_t sum)
{
  size_t		i;
  uint16_t const	*data = reinterpret_cast(uint16_t const *)(dta);
  
  for (i=0; i<size/2; ++i) sum += ntohs(data[i]);
  if (size%2 != 0) {
    union {
	uint8_t		aval[2];
	uint16_t	ival;
    } end_data;

    end_data.ival    = 0;
    end_data.aval[0] = reinterpret_cast(uint8_t const *)(data)[size-1];
    end_data.aval[1] = 0;
    sum += ntohs(end_data.ival);
  }

  while ( (sum>>16)!=0 )
    sum = (sum & 0xFFFF) + (sum >> 16);
  
  return sum;
}

static void
fixCheckSumIP(struct iphdr * const	ip)
{
  ip->check = 0;
  ip->check = htons(~calculateCheckSum(ip, sizeof(*ip), 0));
}

static void
fixCheckSumUDP(struct udphdr * const			udp,
	       /*@in@*/struct iphdr const * const	ip,
	       void const * const			data)
{
  uint32_t		sum;
  struct {
      uint32_t		src;
      uint32_t		dst;
      uint8_t		mbz;
      uint8_t		proto;
      uint16_t		len;
  } __attribute__((__packed__))	const pseudo_hdr = {
    ip->saddr, ip->daddr,
    0,
    ip->protocol, udp->len };

#if 0  
  pseudo_hdr.src   = ip->ip_src.s_addr;
  pseudo_hdr.dst   = ip->ip_dst.s_addr;
  pseudo_hdr.mbz   = 0;
  pseudo_hdr.proto = ip->ip_p;
  pseudo_hdr.len   = udp->len;
#endif
  
  udp->check = 0;
  sum = calculateCheckSum(&pseudo_hdr, sizeof(pseudo_hdr), 0);
  sum = calculateCheckSum(udp,         sizeof(*udp),       sum);
  sum = calculateCheckSum(data,        ntohs(udp->len)-sizeof(*udp), sum);

  sum = ~ntohs(sum);
  if (sum==0) sum=~sum;
  udp->check = sum;
}

static void
sendEtherFrame(struct InterfaceInfo const	*iface,
	       struct DHCPllPacket		*frame,
	       char const			*buffer,
	       size_t				size)
{
  struct sockaddr_ll		sock;
  struct msghdr			msg;
  struct iovec			iovec_data[2];

    /* We support ethernet only and the config-part shall return ethernet-macs
     * only... */
  assert(iface->if_maclen == sizeof(frame->eth.ether_dhost));
  
  memset(&sock, 0, sizeof(sock));
  sock.sll_family    = AF_PACKET;
  sock.sll_ifindex   = reinterpret_cast(int)(iface->if_idx);
    /* We do not need to initialize the other attributes of rcpt_sock since
     * dst-hwaddr et.al. are determined by the ethernet-frame defined below */

  memcpy(frame->eth.ether_shost, iface->if_mac,  iface->if_maclen);
  
  frame->ip.version  = 4;
  frame->ip.ihl      = sizeof(frame->ip)/4u;
  frame->ip.tos      = 0;
  frame->ip.tot_len  = htons(sizeof(frame->ip) + sizeof(struct udphdr) + size);
  frame->ip.id       = 0;
  frame->ip.frag_off = htons(IP_DF);
  frame->ip.ttl      = 64;
  frame->ip.protocol = IPPROTO_UDP;
  frame->ip.saddr    = iface->if_ip;

  frame->udp.len     = htons(sizeof(struct udphdr) + size);

  fixCheckSumIP(&frame->ip);
  fixCheckSumUDP(&frame->udp, &frame->ip, buffer);

  iovec_data[0].iov_base = frame;
  iovec_data[0].iov_len  = sizeof(*frame);
  iovec_data[1].iov_base = const_cast(void *)(buffer);
  iovec_data[1].iov_len  = size;
  
  msg.msg_name       = &sock;
  msg.msg_namelen    = sizeof(sock);
  msg.msg_iov        = iovec_data;
  msg.msg_iovlen     = 2;
  msg.msg_control    = 0;
  msg.msg_controllen = 0;
  msg.msg_flags      = 0;

  Wsendmsg(fds.raw_fd, &msg, 0);
}

static void
sendToClient(/*@in@*/struct FdInfo const * const	fd,
	     /*@in@*/struct DHCPHeader const * const	header,
	     /*@in@*/char const * const			buffer,
	     size_t const				size)
{
  struct DHCPllPacket		frame;
  struct InterfaceInfo const	*iface = fd->iface;

    /* Should be checked by isValidHeader() already */
  assert(header->hlen == sizeof(frame.eth.ether_dhost));
    //  assert(header->op   == opBOOTREPLY);

  memset(&frame, 0, sizeof(frame));
  memcpy(frame.eth.ether_dhost, header->chaddr, header->hlen);
  frame.eth.ether_type = htons(ETHERTYPE_IP);

  if (iface->allow_bcast && header->flags.bcast)
    frame.ip.daddr  = -1;
  else
    frame.ip.daddr  = header->ciaddr;
  
  frame.udp.source  = htons(DHCP_PORT_SERVER);
  frame.udp.dest    = htons(DHCP_PORT_CLIENT);

  sendEtherFrame(iface, &frame, buffer, size);
}

inline static void
sendServerBcast(struct ServerInfo const	* const		server,
		char const * const			buffer,
		size_t const				size)
{
  struct DHCPllPacket		frame;
  struct InterfaceInfo const	*iface = server->info.iface;

  memset(&frame, 0, sizeof(frame));
  memset(frame.eth.ether_dhost, 255, sizeof(frame.eth.ether_dhost));

  frame.eth.ether_type = htons(ETHERTYPE_IP);

  frame.ip.daddr       = -1;
  
  frame.udp.source     = htons(DHCP_PORT_CLIENT);
  frame.udp.dest       = htons(DHCP_PORT_SERVER);
  
  sendEtherFrame(iface, &frame, buffer, size);
}

inline static void
sendServerUnicast(int const				fd,
		  struct ServerInfo const * const	server,
		  char const * const			buffer,
		  size_t const				size)
{
  struct sockaddr_in	sock;
  
  memset(&sock, 0, sizeof(sock));
  sock.sin_family = AF_INET;

  sock.sin_addr = server->info.ip;
  sock.sin_port = htons(DHCP_PORT_SERVER);
	      
  Wsendto(fd, buffer, size, 0,
	  reinterpret_cast(struct sockaddr *)(&sock),
	  sizeof(sock));
}

static void
sendToServer(char const * const				buffer,
	     size_t const				size)
{
  size_t		j;

  for (j=0; j<servers.len; ++j) {
    struct ServerInfo const * const	server = servers.dta + j;

    switch (server->type) {
      case svUNICAST	:
	sendServerUnicast(fds.sender_fd, server, buffer, size);
	break;
      case svBCAST	:
	sendServerBcast(server, buffer, size);
	break;
      default		:
	assert(false);
    }
  }
}

inline static void
handlePacket(/*@in@*/struct FdInfo const * const		fd,
	     /*@dependent@*/char * const			buffer,
	     size_t						size)
{
  struct DHCPHeader * const	header  = reinterpret_cast(struct DHCPHeader *)(buffer);
  struct DHCPOptions * const	options = reinterpret_cast(struct DHCPOptions *)(buffer + sizeof(*header));
  size_t			options_len = size - sizeof(*header);
  

    /** Discard broken header (e.g. too much hops or bad values) */
  if (!isValidHeader(header)) return;
  ++header->hops;

    /* Check if we are the first agent; if so, set 'giaddr' and the relay-agent
     * field, else do not touch the packet */
  if (header->giaddr==0) {
    header->giaddr = fd->iface->if_ip;
  
    if (!isValidOptions(options, options_len)) {
      LOG("Invalid options\n");
      return;
    }

      /* Fill agent-info and adjust size-information */
    options_len  = fillOptions(options->data, fd->iface);
    options_len += sizeof(options->cookie);
    size         = options_len + sizeof(*header);

    assert(isValidOptions(options, options_len));
  }

  switch (header->op) {
    case opBOOTREPLY	:
      if (fd->iface->has_servers) sendToClient(fd, header, buffer, size);
      break;
    case opBOOTREQUEST	:
      if (fd->iface->has_clients) sendToServer(buffer, size);
      break;
	/* isValidHeader() checked the correctness of header->op already and it
	 * should be impossible to reach this code... */
    default		:  assert(false); 
  }
}

static size_t
determineMaxMTU()
{
  size_t		result = 0;
  struct FdInfo	const	*fd;
  
  for (fd=fds.dta; fd<fds.dta+fds.len; ++fd)
    if (fd->iface->if_mtu>result) result = fd->iface->if_mtu;

  return result;
}

inline static struct FdInfo const *
lookupFD(/*@in@*/struct in_addr const			addr)
{
  register size_t	i;
  
  for (i=0; i<fds.len; ++i) {
    struct FdInfo const	* const		fd = &fds.dta[i];
    if (fd->iface->if_ip==addr.s_addr) return fd;
  }

  return 0;
}

/*@noreturn@*/
static void
execRelay()
{
  fd_set			fd_set;
  int				c = 5;
  size_t			max_mtu   = determineMaxMTU();
  size_t			total_len = max_mtu + IFNAMSIZ + 4;
  char				*buffer   = 0;

  buffer = static_cast(char *)(alloca(total_len));

  while (c-- > 0) {
    int				max;
    size_t			i;
    
    fillFDSet(&fd_set, &max);
    if (Wselect(max+1, &fd_set, 0, 0, 0)==-1) continue;

    for (i=0; i<fds.len; ++i) {
      struct FdInfo const	*fd_info = &fds.dta[i];
      size_t			size;
      struct sockaddr_in	addr;
      struct in_pktinfo		pkinfo;
      int			flags = 0;
      
      if (!FD_ISSET(fd_info->fd, &fd_set)) continue;

      size = WrecvfromFlagsInet4(fd_info->fd, buffer, total_len, &flags,
				 &addr, &pkinfo);

#if 0      
      printf("spec_dst=%s, ", inet_ntoa(pkinfo.ipi_spec_dst));
      printf("addr=%s, ifindex=%i\n",
	     inet_ntoa(pkinfo.ipi_addr), pkinfo.ipi_ifindex);
#endif
      
      if (reinterpret_cast(ssize_t)(size)==-1) {
	int		error = errno;
	LOG("recvfrom(): ");
	LOGSTR(strerror(error));
      }
      else if (size < sizeof(struct DHCPHeader)) {
	LOG("Malformed package\n");
      }
      else {
	if (pkinfo.ipi_addr.s_addr!=-1)
	  fd_info = lookupFD(pkinfo.ipi_addr);

	if (fd_info==0) LOG("Received package on unknown interface\n");
	else if (size > fd_info->iface->if_mtu) {
	  LOG("Unexpected large packet\n");
	}
	else 
	  handlePacket(fd_info, buffer, size);
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  struct InterfaceInfoList		ifs;

  checkCompileTimeAssertions();

  switch (initializeSystem(argc, argv, &ifs, &servers, &fds)) {
    case -1	:  return 5;
    case 0	:  execRelay();
    default	:  return 0;
  }
}

  // splint +matchanyintegral -exitarg -fullinitblock -boolfalse false -booltrue true -booltype bool +unixlib -D_GNU_SOURCE main.c

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
