# Main Makefile for paxutils.
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

AUTOMAKE_OPTIONS = gnits dist-shar
ACLOCAL_AMFLAGS = -I m4

BABYL = ../rmail/* ../admin/*/RMAIL
EXTRA_DIST = PATCHES-AC PATCHES-BI README.dos config.bat rebox.el \
stamp-pot

SUBDIRS = m4 doc i18n lib src scripts tests contrib

POTFILES = src/common.h src/buffer.c src/create.c src/compare.c	\
src/delete.c src/extract.c src/incremen.c src/list.c src/mangle.c \
src/names.c src/rmt.c src/rtapelib.c src/tar.c src/update.c \
tests/genfile.c

all-local: $(srcdir)/stamp-pot

$(srcdir)/stamp-pot: $(POTFILES)
	@echo "$(POTFILES)" > $@

dist-zoo: $(DISTFILES)
	rm -rf $(distdir)
	mkdir $(distdir)
	distdir=`cd $(distdir) && pwd` \
	  && cd $(srcdir) \
	  && automake --include-deps --output-dir=$$distdir
	@for file in $(DISTFILES); do		\
	  test -f $(distdir)/$$file \
	    || cp -p $(srcdir)/$$file $(distdir)/$$file; \
	done
	for subdir in $(SUBDIRS); do		\
	  test -d $(distdir)/$$subdir		\
	  || mkdir $(distdir)/$$subdir		\
	  || exit 1;				\
	  chmod 777 $(distdir)/$$subdir;	\
	  (cd $$subdir && $(MAKE) dist) || exit 1; \
	done
	@sublist="$(DIST_SUBDIRS)";		\
	for dir in $$sublist; do		\
	  echo copying directory $$dir;		\
	  tar -chf - $$dir | (cd $(distdir) && tar -xBpf -); \
	done
	chmod -R a+r $(distdir)
	find $(distdir) -type f | xargs dosfn
#	find $(distdir) -type f | xargs recode :ibmpc
	mv $(distdir) $(PACKAGE)
	find $(PACKAGE) -type f | zoo ahIq $(PACKAGE).zoo
	rm -rf $(PACKAGE)

## ## Check our path lengths.
## path-check: distdir
## 	   (cd $(distdir) && \
## ## FIXME there's got to be a better way!  pathchk should take the list
## ## of files on stdin, at least.
## 	     find . -print | xargs pathchk -p); \
## 	     status=$$?; \
## 	     rm -rf $(distdir); \
## 	     exit $$status
