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

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "config.h"
#include "util.h"
#include "wrappers.h"
#include "compat.h"

static int	look_ahead = -1;
static int	fd;

inline static int
getNext()
{
  char		c;
  int		cnt;
  
  cnt = TEMP_FAILURE_RETRY(read(fd, &c, 1));
  if (cnt==-1) {
    write(2, "read() failed\n", 14);
    exit(3);
  }
  look_ahead = -1;
  
  return cnt==0 ? -1 : c;
}

inline static int
getLookAhead()
{
  if (look_ahead==-1) look_ahead = getNext();

  return look_ahead;
}

inline static void
match(char c)
{
  int		got = getLookAhead();
  
  if (got==-1) {
    write(2, "unexpected EOF while parsing\n", 29);
    exit(3);
  }

  if (reinterpret_cast(char)(got)!=c) {
    write(2, "unexpected symbol\n", 14);
    assert(false);
    exit(3);
  }

  look_ahead = -1;
}

inline static void
matchStr(char const *str)
{
  assert(str!=0);
  for (; *str!=0; ++str) match(*str);
}

inline static struct InterfaceInfo *
newInterface(struct InterfaceInfoList *ifs)
{
  ++ifs->len;
  ifs->dta = Erealloc(ifs->dta, ifs->len * (sizeof(ifs->dta[0])));
  
  return ifs->dta + ifs->len - 1;
}

inline static struct ServerInfo *
newServer(struct ServerInfoList *servers)
{
  ++servers->len;
  servers->dta = Erealloc(servers->dta, servers->len * (sizeof(servers->dta[0])));
  
  return servers->dta + servers->len - 1;
}

inline static struct InterfaceInfo *
searchInterface(struct InterfaceInfoList *ifs, char const *name)
{
  struct InterfaceInfo	*iface;
  
  for (iface=ifs->dta; iface < ifs->dta + ifs->len; ++iface)
    if (strcmp(name, iface->name)==0) break;

  if (iface==ifs->dta + ifs->len) {
    write(2, "unknown interface\n", 18);
    exit(3);
  }

  return iface;
}

inline static void
matchEOL()
{
  int state	= 0xFF00;
  while (state!=0xFFFF) {
    int		c = getLookAhead();
    
    switch (state) {
      case 0xFF00	:
	switch (c) {
	  case ' '	:
	  case '\t'	:  break;
	  case '\n'	:  state = 0;    break;
	  case '\r'	:  state = 0x10; break;
	  default	:  goto err;
	}
	match(c);
	break;
	
      case 0		:
	switch (c) {
	  case '\n'	:  state = 0;    match(c); break;
	  case '\r'	:  state = 0x10; match(c); break;
	  default	:  state = 0xFFFF; break;
	}
	break;

      case 0x10		:
	switch (c) {
	  case '\n'	:  state = 0;    break;
	  default	:  goto err;
	}
	match(c);
	break;

      default		:  assert(false); goto err;
    }
  }

  return;

  err:
  write(2, "unexpected character\n", 21);
  exit(3);    
}

inline static void
readBlanks()
{
  int		c   = 0;
  size_t	cnt = 0;
  
  while (c!=-1) {
    c = getLookAhead();
    
    switch (c) {
      case ' '	:
      case '\t'	:  match(c); ++cnt; break;
      default	:  c=-1; break;
    }
  }

  if (cnt==0) {
    write(2, "Expected blank, got character\n", 30);
    exit(3);
  }
}

inline static void
readName(char buffer[], size_t len)
{
  char		*ptr = buffer;
  
  while (ptr-buffer+1 < len) {
    char	c = getLookAhead();

    if ( (c>='a' && c<='z') || (c>='A' && c<='Z') ||
	 (c>='0' && c<='9') ||
	 c=='-' || c=='_' || c=='/' || c=='.')
    {
      *ptr++ = c;
      match(c);
    }
    else break;
  }

  if (len>0) *ptr = 0;
}

inline static void
readIfname(char *iface)
{
  readName(iface, IFNAMSIZ);
  
  if (iface[0]==0) {
    write(2, "Invalid interface name\n", 23);
    exit(3);
  }
}

inline static void
readIp(struct in_addr *ip)
{
  int		state = 0;
  char		buffer[1024];
  char 		*ptr = buffer;

  while (state!=0xFFFF) {
    int		c = getLookAhead();
    switch (c) {
      case -1	:
      case ' '	:
      case '\t'	:
      case '\n'	:
      case '\r'	:  state = 0xFFFF;       break;
      default	:
	if (ptr+1 < buffer+sizeof(buffer)) {
	  *ptr++ = c;
	  match(c);
	}
	else {
	  write(2, "Invalid IP\n", 11);
	  exit(3);
	}
	break;
    }
  }

  *ptr = 0;
  if (inet_aton(buffer, ip)==0) {
    write(2, "Invalid ip\n", 11);
    exit(3);
  }
}

inline static void
readBool(bool *val)
{
  int		state = 0;
  while (state!=0xFFFF) {
    int		c = getLookAhead();
    
    switch (state) {
      case 0	:
	switch (c) {
	  case 'f'	:  matchStr("false"); *val=false; break;
	  case 't'	:  matchStr("true");  *val=true;  break;
	  case '0'	:  match('0');        *val=false; break;
	  case '1'	:  match('1');        *val=true;  break;
	  case 'N'	:
	  case 'n'	:  state = 0x1000; break;
	  case 'Y'	:
	  case 'y'	:  state = 0x2000; break;
	  default	:  goto err;
	}
	if (state==0) state=0xFFFF;
	else          match(c);
	break;

      case 0x1000	:
	switch (c) {
	  case 'o'	:  match(c);
	  case -1	:
	  case ' '	:
	  case '\t'	:
	  case '\n'	:
	  case '\r'	:  *val = false; state = 0xFFFF; break;
	  default	:  goto err;
	}
	break;

      case 0x2000	:
	switch (c) {
	  case 'e'	:  matchStr("yes");
	  case -1	:
	  case ' '	:
	  case '\t'	:
	  case '\n'	:
	  case '\r'	:  *val = true; state = 0xFFFF; break;
	  default	:  goto err;
	}
	break;

      default		:  assert(false);  goto err;
    }
  }

  return;

  err:
  write(2, "unexpected character\n", 21);
  exit(3);    
}

void
parse(char const		filename[],
      struct ConfigInfo		*cfg)
{
  int			state = 0x0;
  char			ifname[IFNAMSIZ];
  char			agent_id[IFNAMSIZ];
  char			name[PATH_MAX];
  long			nr = 0;
  struct in_addr	ip;
  bool			has_clients;
  bool			allow_bcast;
  bool			is_active;

  fd = open(filename, O_RDONLY);
  if (fd==-1) {
    perror("open()");
    exit(1);
  }
  
  while (state!=0xFFFF) {
    int		c = getLookAhead();

    switch (state) {
      case 0xFF00	:  /* comments */
	switch (c) {
	  case '\n'	:
	  case '\r'	:  matchEOL(); state=0; break;
	  default	:  match(c); break;
	}
	break;

      case 0xFFFE	:  matchEOL(); state=0; break;
	  
      case 0		:
	switch (c) {
	  case '\t'	:
	  case ' '	:  match(c);       break;
	  case '\n'	:
	  case '\r'	:  matchEOL();     break;
	  case '#'	:  state = 0xFF00; break;
	  case 'i'	:  state = 0x0100; break;
	  case 'n'	:  state = 0x0200; break;
	  case 's'	:  state = 0x0300; break;
	  case 'u'	:  state = 0x0400; break;
	  case 'g'	:  state = 0x0500; break;
	  case 'c'	:  state = 0x0600; break;
	  case 'l'	:  state = 0x0700; break;
	  case 'p'	:  state = 0x0800; break;
	  case -1	:  state = 0xFFFF; break;
	  default	:  goto err;
	}
	if (state!=0 && state!=0xFFFF) match(c);
	break;

      case 0x0800	:
	matchStr("idfile"); readBlanks();
	++state;
	break;

      case 0x0801	:
	readName(cfg->pidfile_name, sizeof(cfg->pidfile_name));
	state = 0xFFFE;
	break;
	
      case 0x0700	:
	matchStr("og");
	++state;
	break;

      case 0x0701	:
	switch (c) {
	  case 'f'	:  matchStr("file");  state = 0x0710; break;
	  case 'l'	:  matchStr("level"); state = 0x0720; break;
	  default	:  goto err;
	}
	readBlanks();
	break;

      case 0x0710	:
	readName(cfg->logfile_name, sizeof(cfg->logfile_name));
	state = 0xFFFE;
	break;

      case 0x0720	:
	readName(name, sizeof(name));
	cfg->loglevel = atoi(name);
	state = 0xFFFE;
	break;
	
      case 0x0400	:
	matchStr("ser"); readBlanks();
	state = 0x0401;
	break;

      case 0x0500	:
	matchStr("roup"); readBlanks();
	state = 0x501;

      case 0x0401	:
      case 0x0501	:
	readName(name, sizeof(name));
	++state;
	break;

      case 0x0402	:
      case 0x0502	:
      {
	char		*err_ptr;
	
	nr = strtol(name, &err_ptr, 0);
	if (*err_ptr!=0) state += 0x11;
	else             state += 0x01;

	break;
      }

      case 0x0403	:
	cfg->uid = nr;
	state = 0xFFFE;
	break;

      case 0x0413	:
	cfg->uid = Egetpwnam(name)->pw_uid;
	state = 0xFFFE;
	break;

      case 0x0503	:
	cfg->gid = nr;
	state = 0xFFFE;
	break;

      case 0x0513	:
	cfg->gid = Egetgrnam(name)->gr_gid;
	state = 0xFFFE;
	break;

      case 0x0600	:
	matchStr("hroot"); readBlanks();
	readName(name, sizeof(name));
	++state;
	break;

      case 0x0601	:
	strcpy(cfg->chroot_path, name);
	state = 0xFFFE;
	break;

	  /* if <name> ... case */
      case 0x100	:
	match('f');             readBlanks();
	readIfname(ifname);     readBlanks();
	readBool(&has_clients); readBlanks();
	readBool(&allow_bcast); readBlanks();
	readBool(&is_active);
	state = 0x110;
	break;
	
      case 0x110	:
      {
	struct InterfaceInfo *	iface = newInterface(&cfg->interfaces);
	
	strcpy(iface->name, ifname);
	iface->aid[0]      = 0;
	iface->has_clients = has_clients;
	iface->allow_bcast = allow_bcast;
	iface->is_active   = is_active;

	state = 0xFFFE;
	break;
      }

	
      case 0x200	:
	matchStr("ame");      readBlanks();
	state = 0x201;
	break;
	
      case 0x201	:
	readIfname(ifname);   readBlanks();
	readIfname(agent_id);
	state = 0x202;
	break;

      case 0x202	:
      {
	struct InterfaceInfo *	iface = searchInterface(&cfg->interfaces,
							ifname);

	strcpy(iface->aid, agent_id);
	state = 0xFFFE;
	break;
      }

      case 0x300	:
	matchStr("erver");  readBlanks();
	state=0x301;
	break;

      case 0x301	:
	switch (c) {
	  case 'i'	:  state = 0x310; break;
	  case 'b'	:  state = 0x320; break;
	  default	:  goto err;
	}
	match(c);
	break;

      case 0x310	:
	match('p'); readBlanks();
	readIp(&ip);
	state = 0x311;
	break;

      case 0x311	:
      {
	struct ServerInfo	*server = newServer(&cfg->servers);

	server->type    = svUNICAST;
	server->info.ip = ip;
	
	state = 0xFFFE;
	break;
      }


      case 0x320	:
	matchStr("cast"); readBlanks();
	readIfname(ifname);
	state = 0x321;
	break;

      case 0x321	:
      {
	struct ServerInfo	*server = newServer(&cfg->servers);

	server->type       = svBCAST;
	server->info.iface = searchInterface(&cfg->interfaces, ifname);
	
	state = 0xFFFE;
	break;
      }

      default		:  assert(false); goto err;
    }
  }

  return;

  err:
  write(2, "Bad character\n", 14);
  exit(3);
}

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
