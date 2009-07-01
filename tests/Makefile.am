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
check_PROGRAMS = genfile
BUILT_SOURCES = preset

TESTS = none.sh

# FIXME: Nothing works, waiting for overhaul.
PREPONED_TESTS = version.sh ignfail.sh create01.sh extrac01.sh extrac02.sh \
extrac03.sh extrac04.sh extrac05.sh old.sh volume.sh gzip.sh \
append01.sh delete01.sh increm01.sh increm02.sh

# Postponed until after the 2.5 release.
POSTPONED_TESTS = delete02.sh

check_PROGRAMS = echotime genfile rediff swapb swaphw verify wrsparse

EXTRA_DIST = README README2 after before cpiotest $(TESTS) \
$(PREPONED_TESTS) $(POSTPONED_TESTS)

DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
INCLUDES = -I.. -I$(top_srcdir)/src -I$(top_srcdir)/lib
LDADD = ../lib/libpu.a
localedir = $(prefix)/share/locale

# Set this to test a different cpio.
GCPIO = `cd ../src && pwd`/cpio
check-local: cpiotest defs
# FIXME: Nothing works, waiting for overhaul.
#	srcdir=$(srcdir) GCPIO=$(GCPIO) $(srcdir)/cpiotest

mostlyclean:
	rm -rf tmp-*

## create01.sh delete01.sh delete02.sh: genfile
