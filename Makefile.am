## $Id$

# Copyright (C) 2002 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
#  
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#  

DIET			=  @DIET@
CC			=  $(DIET) @CC@

confdir			=  $(sysconfdir)
cfg_filename		=  $(confdir)/dhcp-fwd.cfg


sbin_PROGRAMS		=  dhcp-fwd
dhcp_fwd_SOURCES	=  src/config.c src/recvfromflags.c src/main.c		\
			   src/parser.c

AM_CPPFLAGS		=  -DCFG_FILENAME='"$(cfg_filename)"'

noinst_HEADERS		=  src/compat.h src/config.h src/dhcp.h src/inet.h	\
			   src/recvfromflags.h src/splint_compat.h		\
			   src/util.h src/wrappers.h src/assertions.h		\
			   src/parser.h
