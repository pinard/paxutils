# Makefile for tar sources.
# Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

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

AUTOMAKE_OPTIONS = gnits ansi2knr
EXTRA_PROGRAMS = mt rmt
bin_PROGRAMS = cpio tar $(extra_bin_progs)
libexec_PROGRAMS = $(extra_libexec_progs)
## Note: we don't install pax for now; it hasn't been tested at all.
noinst_PROGRAMS = pax

noinst_HEADERS = arith.h common.h cpio.h cpiohdr.h defer.h extern.h \
rmt.h system.h tar.h tarhdr.h ustar.h
shared_SOURCES = cdf.c copyin.c copyout.c copypass.c defer.c fmtcpio.c \
fmttar.c format.c getfile.c global.c rename.c util.c

cpio_SOURCES = cpio.c $(shared_SOURCES) rtapelib.c
mt_SOURCES = mt.c rtapelib.c
pax_SOURCES = pax.c $(shared_SOURCES) rtapelib.c
rmt_SOURCES = rmt.c
tar_SOURCES = arith.c buffer.c compare.c create.c delete.c extract.c \
header.c incremen.c list.c mangle.c misc.c names.c open3.c reading.c \
rtapelib.c tar.c update.c

EXTRA_DIST = rmt.cpio.c rmt.cpio.h rtapelib.cpio.c

## INCLUDES = -I../intl -I$(top_srcdir)/intl -I$(top_srcdir)/lib
## LDADD = ../lib/libpu.a @INTLLIBS@
##
INCLUDES = -I$(top_srcdir)/lib
LDADD = ../lib/libpu.a

localedir = $(prefix)/share/locale

cpio.o: cpio.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/cpio.c

cpio._o: cpio._c
	@rm -f _cpio.c
	@ln cpio._c _cpio.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _cpio.c
	@mv _cpio.o $@
	@rm _cpio.c

mt.o: mt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/mt.c

mt._o: mt._c
	@rm -f _mt.c
	@ln mt._c _mt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _mt.c
	@mv _mt.o $@
	@rm _mt.c

pax.o: pax.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/pax.c

pax._o: pax._c
	@rm -f _pax.c
	@ln pax._c _pax.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _pax.c
	@mv _pax.o $@
	@rm _pax.c

tar.o: tar.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/tar.c

tar._o: tar._c
	@rm -f _tar.c
	@ln tar._c _tar.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _tar.c
	@mv _tar.o $@
	@rm _tar.c

rmt.o: rmt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/rmt.c

rmt._o: rmt._c
	@rm -f _rmt.c
	@ln rmt._c _rmt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _rmt.c
	@mv _rmt.o $@
	@rm _rmt.c
