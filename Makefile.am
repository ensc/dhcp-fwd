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

ACLOCAL_AMFLAGS		=  -I m4

WWW_HOST		=  janus
WWW_DIR			=  public_html/dhcpfwd
WWW_FILES_ROOT		=  $(srcdir)/doc/index.html
WWW_FILES_FILES		=  $(distdir).tar.bz2 $(distdir).tar.bz2.asc $(srcdir)/README $(srcdir)/NEWS


SUBDIRS			=  .
DIST_SUBDIRS		=  $(SUBDIRS) m4

confdir			=  $(sysconfdir)
cfg_filename		=  $(confdir)/dhcp-fwd.cfg

if ENABLE_LOGGING
logging_SRCS		=  src/logging.c
else
logging_SRCS		=
endif

sbin_PROGRAMS		=  dhcp-fwd
EXTRA_dhcp_fwd_SOURCES	=  src/logging.c
dhcp_fwd_SOURCES	=  src/config.c src/recvfromflags.c src/main.c		\
			   src/parser.c src/output.c $(logging_SRCS)

AM_CPPFLAGS		=  -DCFG_FILENAME='"$(cfg_filename)"'

noinst_HEADERS		=  src/compat.h src/config.h src/dhcp.h src/inet.h	\
			   src/recvfromflags.h src/splint_compat.h		\
			   src/util.h src/wrappers.h src/assertions.h		\
			   src/parser.h src/output.h src/logging.h

EXTRA_DIST		=  dhcp-forwarder.spec Makefile.m4.am			\
		 	   contrib/dhcp-fwd.init contrib/dhcp-fwd.conf

install-contrib:
			$(mkinstalldirs) $(DESTDIR)/etc/init.d
			$(INSTALL_SCRIPT) contrib/dhcp-fwd.init $(DESTDIR)/etc/init.d/dhcp-fwd

uninstall-contrib:
			rm -f $(DESTDIR)/etc/init.d/dhcp-fwd

www:			.www-timestamp

.www-timestamp:		sftp.cmds
			sftp -b $< $(WWW_HOST)
			touch $@

sftp.cmds:		$(WWW_FILES_ROOT) $(WWW_FILES_FILES)
			@echo 'chdir $(WWW_DIR)' >$@.tmp
			list="$(WWW_FILES_ROOT)"; for f in $$list; do			\
				test $$f -ot $@ || echo "put -P $$f";			\
			done >>$@.tmp

			@echo 'chdir files' >>$@.tmp
			list="$(WWW_FILES_FILES)"; for f in $$list; do			\
				test $$f -ot $@ || echo "put -P $$f";			\
			done >>$@.tmp
			mv -f $@.tmp $@



include $(top_srcdir)/m4/validate.am
include $(top_srcdir)/m4/changelog.am
include $(top_srcdir)/m4/diet.am
include $(top_srcdir)/m4/gpgsig.am
