# Makefile for tar sources.
# Copyright (C) 1994, 95, 96, 97, 98, 99 Free Software Foundation, Inc.

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
EXTRA_PROGRAMS = cpio mt pax rmt
bin_PROGRAMS = tar @extra_bin_progs@
libexec_PROGRAMS = @extra_libexec_progs@

noinst_HEADERS = arith.h common.h cpio.h defer.h global.h rmt.h	\
system.h tar.h ustar.h
shared_SOURCES = cdf.c copyin.c copyout.c copypass.c defer.c fmtcpio.c \
fmttar.c format.c getfile.c global.c misc.c rename.c util.c

cpio_SOURCES = cpio.c $(shared_SOURCES) rtapelib.c
mt_SOURCES = mt.c rtapelib.c
pax_SOURCES = pax.c $(shared_SOURCES) rtapelib.c
rmt_SOURCES = rmt.c
tar_SOURCES = arith.c buffer.c compare.c create.c delete.c extract.c \
global.c header.c incremen.c list.c mangle.c misc.c names.c open3.c \
reading.c rtapelib.c tar.c update.c

INCLUDES = -I$(top_srcdir)/lib
LDADD = ../lib/libpu.a
