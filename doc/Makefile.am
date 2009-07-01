# Makefile for tar documentation.
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
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

AUTOMAKE_OPTIONS = gnits
info_TEXINFOS = paxutils.texi
man_MANS = cpio.1 mt.1 pax.1 tar.1

EXTRA_DIST = convtexi.pl getdate.texi header.texi help2man $(MANS)

PERL = perl

CLEANFILES = tmp-*

# The rendering level is anyone of PUBLISH, DISTRIB or PROOF.
# Just call `make dvi RENDITION=PROOF' if you want PROOF rendition.
RENDITION = DISTRIB

paxutils.info: paxutils.texi getdate.texi header.texi version.texi
	@echo "WARNING: \`makeinfo' will not preprocess Texinfo input properly"
	@echo "         for expanding Texinfo macros, if it comes from a"
	@echo "         Texinfo distribution which is earlier than version 3.7."
	cd $(srcdir) && $(MAKEINFO) -D$(RENDITION) paxutils.texi \
	  || ( rm $@ && exit 1 )

paxutils.dvi: paxutils.texi getdate.texi header.texi version.texi
	@echo "WARNING: \`makeinfo' will not preprocess Texinfo input properly"
	@echo "         for expanding Texinfo macros, if it comes from a"
	@echo "         Texinfo distribution which is earlier than version 3.7."
	$(MAKEINFO) -D$(RENDITION) -Etmp-paxutils.tmp -otmp-paxutils.info \
	  -I$(srcdir) paxutils.texi
	rm -f tmp-paxutils.sed tmp-paxutils.info*
	test $(RENDITION) = DISTRIB \
	  || echo >>tmp-paxutils.sed 's/^@set DISTRIB/@set $(RENDITION)/'
	test $(RENDITION) = PUBLISH \
	  || echo >>tmp-paxutils.sed '/^@smallbook/d'
	sed -f tmp-paxutils.sed tmp-paxutils.tmp > tmp-paxutils.texi
	rm -f tmp-paxutils.sed tmp-paxutils.tmp
	TEXINPUTS=$(srcdir):$$TEXINPUTS $(TEXI2DVI) tmp-paxutils.texi
	mv tmp-paxutils.dvi $@

header.texi: $(top_srcdir)/src/tar.h
	sed -n '/archive format/,/End of format/p' $(top_srcdir)/src/tar.h \
	  | expand | sed 's/\([{}]\)/@\1/g' > $(srcdir)/header.texi

cpio.1: help2man $(top_srcdir)/src/cpio.c
	@if test -f ../src/cpio; then \
	  echo "Updating the \`man' page for \`cpio'"; \
	  LANGUAGE=C $(PERL) $(srcdir)/help2man ../src/cpio > cpio.1-tmp; \
	  mv cpio.1-tmp $(srcdir)/cpio.1; \
	else \
	  echo "WARNING: The \`man' page for \`cpio' cannot be updated yet."; \
	  echo "         Retry once the program executable will be ready."; \
	fi

mt.1: help2man $(top_srcdir)/src/mt.c
	@if test -f ../src/mt; then \
	  echo "Updating the \`man' page for \`mt'"; \
	  LANGUAGE=C $(PERL) $(srcdir)/help2man ../src/mt > mt.1-tmp; \
	  mv mt.1-tmp $(srcdir)/mt.1; \
	else \
	  echo "WARNING: The \`man' page for \`mt' cannot be updated yet."; \
	  echo "         Retry once the program executable will be ready."; \
	fi

pax.1: help2man $(top_srcdir)/src/pax.c
	@if test -f ../src/pax; then \
	  echo "Updating the \`man' page for \`pax'"; \
	  LANGUAGE=C $(PERL) $(srcdir)/help2man ../src/pax > pax.1-tmp; \
	  mv pax.1-tmp $(srcdir)/pax.1; \
	else \
	  echo "WARNING: The \`man' page for \`pax' cannot be updated yet."; \
	  echo "         Retry once the program executable will be ready."; \
	fi

tar.1: help2man $(top_srcdir)/src/tar.c
	@if test -f ../src/tar; then \
	  echo "Updating the \`man' page for \`tar'"; \
	  LANGUAGE=C $(PERL) $(srcdir)/help2man ../src/tar > tar.1-tmp; \
	  mv tar.1-tmp $(srcdir)/tar.1; \
	else \
	  echo "WARNING: The \`man' page for \`tar' cannot be updated yet."; \
	  echo "         Retry once the program executable will be ready."; \
	fi
