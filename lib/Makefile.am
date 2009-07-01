# Makefile for paxutils library.
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

AUTOMAKE_OPTIONS = gnits

noinst_LIBRARIES = libpu.a

EXTRA_DIST = alloca.c fileblocks.c fnmatch.c ftruncate.c execlp.c \
full-read.c full-write.c gettext.c gmalloc.c malloc.c memset.c mkdir.c \
modechange.h realloc.c rename.c rmdir.c stpcpy.c stpncpy.c strstr.c

noinst_HEADERS = argmatch.h backupfile.h dstring.h error.h filetypes.h \
fnmatch.h getopt.h getdate.h getpagesize.h gettext.h hash.h makepath.h \
pathmax.h regex.h save-cwd.h xalloc.h

libpu_a_SOURCES = addext.c argmatch.c backupfile.c basename.c \
dirname.c dstring.c error.c filemode.c full-read.c full-write.c	\
getdate.y getopt.c getopt1.c hash.c idcache.c makepath.c modechange.c \
msleep.c octal.c regex.c save-cwd.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c

## INCLUDES = -I.. -I$(srcdir) -I../intl
INCLUDES = -I.. -I$(srcdir)
libpu_a_LIBADD = @ALLOCA@ @LIBOBJS@
libpu_a_DEPENDENCIES = $(libpu_a_LIBADD)
localedir = $(prefix)/share/locale
aliaspath = $(localedir):.

# Say $(srcdir), so GNU make does not report an ambiguity with the .y.c rule.
$(srcdir)/getdate.c: getdate.y
	@echo Expect 13 shift/reduce conflicts...
	cd $(srcdir) && \
	  $(YACC) $(YFLAGS) getdate.y; \
	  mv -f y.tab.c getdate.c

gettext.o: gettext.c
	$(COMPILE) -DALIASPATH=\"$(aliaspath)\" -DLOCALEDIR=\"$(localedir)\" \
	  -c $(srcdir)/gettext.c

gettext._o: gettext._c
	@rm -f _gettext.c
	@ln gettext._c _gettext.c
	$(COMPILE) -DALIASPATH=\"$(aliaspath)\" -DLOCALEDIR=\"$(localedir)\" \
	  -c _gettext.c
	@mv _gettext.o $@
	@rm _gettext.c
