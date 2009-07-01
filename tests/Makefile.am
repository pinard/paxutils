# Makefile for tar regression tests.
# Copyright © 1996, 1997, 1998 Free Software Foundation, Inc.
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
check_PROGRAMS = echotime genfile rediff swapb swaphw verify wrsparse

SUITE = append.m4 c-append.m4 c-base.m4 c-block.m4 c-crc.m4 \
c-interop.m4 c-junk.m4 c-long.m4 c-newc.m4 c-nopipe.m4 c-odd.m4	\
c-owner.m4 c-pass.m4 c-reset.m4 c-skip.m4 c-swap.m4 c-tar.m4 \
c-unpack.m4 create.m4 delete.m4 exclude.m4 extract.m4 gzip.m4 \
incremen.m4 volume.m4

EXTRA_DIST = atgeneral.m4 make-structure test-one testsuite \
testsuite.m4 $(SUITE)

DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
INCLUDES = -I.. -I$(top_srcdir)/src -I$(top_srcdir)/lib
LDADD = ../lib/libpu.a
localedir = $(prefix)/share/locale

all-local: atconfig atlocal testsuite

check-local: atconfig atlocal testsuite
	$(SHELL) $(srcdir)/testsuite

$(srcdir)/testsuite: atgeneral.m4 testsuite.m4 $(SUITE)
	m4 -I $(srcdir) atgeneral.m4 testsuite.m4 | cat -s > $@-tmp
	mv $@-tmp $@
	chmod +x $@

clean-local:
	rm -f FAIL-*.sh
