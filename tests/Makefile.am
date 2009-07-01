# Makefile for Free paxutils regression tests.
# Copyright © 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1988.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AUTOMAKE_OPTIONS = gnits ../src/ansi2knr
check_PROGRAMS = echotime genfile swapb swaphw verify wrsparse

SUITE = c-append.m4 c-block.m4 c-crc.m4 c-default.m4 c-junk.m4 \
c-long.m4 c-newc.m4 c-nopipe.m4 c-oldc.m4 c-owner.m4 c-pass.m4 \
c-reset.m4 c-skip.m4 c-swap.m4 c-tar.m4 suite.m4 t-append.m4 \
t-create.m4 t-delete.m4 t-exclude.m4 t-extract.m4 t-gzip.m4 \
t-incremen.m4 t-volume.m4

EXTRA_DIST = atgeneral.m4 make-mix test-one testsuite $(SUITE)

DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
INCLUDES = -I.. -I$(top_srcdir)/src -I$(top_srcdir)/lib
LDADD = ../lib/libpu.a
localedir = $(prefix)/share/locale

all-local: atconfig atlocal testsuite

check-local: atconfig atlocal testsuite
	$(SHELL) $(srcdir)/testsuite

$(srcdir)/testsuite: atgeneral.m4 $(SUITE)
	m4 -I $(srcdir) atgeneral.m4 suite.m4 | cat -s > $@-tmp
	mv $@-tmp $@
	chmod +x $@

clean-local:
	rm -f debug-*.sh
