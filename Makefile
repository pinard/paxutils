# Makefile for GNU tar program.
#
# In order to disable remote-tape support, add -DNO_REMOTE to the
# appropriate DEFS line, and remove rtape_lib.* from LOCAL_{SRC,OBJ}
#
# If tar fails to properly print error msgs, or core-dumps doing same,
# you may need to change which version of msg...() you are using.
# To do so, add one of the following to your DEFS= line
# -DSTDC_MSG		If you are using an ANSI compiler, and have vfprintf().
# -DVARARGS_MSG		If you have varargs.h and vfprintf()
# -DDOPRNT_MSG		If you have _doprnt(), and no useful varargs support
# -DLOSING_MSG		If nothing else works.
#
# Some non-BSD systems may have to add -DNEED_TZSET in order to have getdate.y
# compile correctly.
#
# getdate.y has 8 shift/reduce conflicts.
#
# In addition to setting DEFS appropriately for your system, you might
# have to hand edit the #defines and #undefs in port.c and rtape_lib.c.
# For Ultrix 3.1, you will have to compile rtape_lib.c with -DUSG
#

## GNU version
#DEFS = -DBSD42
#LOCAL_SRC = 
#LOCAL_OBJ = 
#LDFLAGS =
#LIBS =  -lutils
#LINT = lint
#LINTFLAGS = -abchx
#DEF_AR_FILE = \"-\"
#DEFBLOCKING = 20
#O = o

# Berserkeley version
#CC=ngcc
DEFS = -DBSD42
LOCAL_SRC = getdate.y  rtape_lib.c
LOCAL_OBJ = getdate.$O rtape_lib.$O
LDFLAGS =
LIBS =
LINT = lint
LINTFLAGS = -abchx
DEF_AR_FILE = \"/dev/rmt8\"
DEFBLOCKING = 20
O = o

# USG version
# Add -DNDIR to DEFS if your system uses ndir.h instead of dirent.h
# Add -DDIRECT to DEFS if your system uses 'struct direct' instead of
# 'struct dirent' (this is the case at least with one add-on ndir 
# library)
# Add -DHAVE_MTIO to DEFS if your system has sys/mtio.h and defines MTIOCTOP
# Add -DDAYLIGHT_MISSING to DEFS if your system doesn't define the
# external variable `daylight'.
# Add -lndir to LIBS if your ndir routines aren't in libc.a
# Add -lPW to LIBS if you don't compile with gcc (to get alloca)
#DEFS = -DUSG #-DNDIR -DDIRECT -DHAVE_MTIO
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = #-lndir -lPW
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# UniSoft's Uniplus SVR2 with NFS
#DEFS = -DUSG -DUNIPLUS -DNFS -DSVR2
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -bx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# MASSCOMP version
#CC = ucb cc
#DEFS = -DBSD42
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = 
#LINT = lint
#LINTFLAGS = -bx
#DEF_AR_FILE = \"/dev/rmt0\"
#DEFBLOCKING = 20
#O = o

# (yuk) MS-DOS (Microsoft C 4.0) version
#MODEL = S
#DEFS = -DNONAMES -A$(MODEL) -DNO_REMOTE
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = $(MODEL)dir.lib
#LINT =	$(CC)
#LINTFLAGS = -W3
#DEF_AR_FILE = \"tar.out\"
#DEFBLOCKING = 20
#O = obj

# V7 version
# Pick open3 emulation or nonexistence.  See open3.h, port.c.
##DEFS = -DV7 -DEMUL_OPEN3 -Dvoid=int
##DEFS = -DV7 -DNO_OPEN3 -Dvoid=int
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -abchx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# Minix version
# No lint, so no lintflags.  Default file is stdin/out.  (Minix "tar"
# doesn't even take an "f" flag, it assumes argv[2] is the archive name!)
# Minix "make" doesn't expand macros right, so Minix users will have
# to expand CFLAGS, SRCS, O, etc by hand, or fix your make.  Not my problem!
# You'll also need to come up with ctime(), the directory
# library, and a fixed doprintf() that handles %*s.  Put this stuff in
# the "SUBSRC/SUBOBJ" macro below if you didn't put it in your C library.
# Note that Minix "cc" produces ".s" files, not .o's, so O = s has been set.
#
# Pick open3 emulation or nonexistence.  See open3.h, port.c.
##DEFS = -DV7 -DMINIX -DEMUL_OPEN3
##DEFS = -DV7 -DMINIX -DNO_OPEN3
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS =
#DEF_AR_FILE = \"-\"
#DEFBLOCKING = 8	/* No good reason for this, change at will */
#O = s

# Xenix version
#DEFS = -DUSG -DXENIX
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS = 
#LIBS = -lx
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# SGI 4D version
#DEFS = -DUSG -I/usr/include/bsd
#LOCAL_SRC = getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS = 
#LIBS =
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/tape\"
#DEFBLOCKING = 20
#O = o

#CC = gcc
#TARGET_ARCH = 

CFLAGS = $(COPTS) $(ALLDEFS)
ALLDEFS = $(DEFS) \
	-DDEF_AR_FILE=$(DEF_AR_FILE) \
	-DDEFBLOCKING=$(DEFBLOCKING)
# next line for Debugging
COPTS = -g
# next line for Production
#COPTS = -O

# Add things here like readdir that aren't in your standard libraries.
# (E.g. MSDOS needs msd_dir.c, msd_dir.obj)
SUBSRC=
SUBOBJ=	

# Destination directory and installation program for make install
DESTDIR = /usr/local
INSTALL = cp
RM = rm -f

SRC1 =	tar.c create.c extract.c buffer.c getoldopt.c update.c gnu.c mangle.c
SRC2 =  version.c list.c names.c diffarch.c port.c wildmat.c getopt.c getopt1.c
SRC3 =  $(LOCAL_SRC) $(SUBSRC)
SRCS =	$(SRC1) $(SRC2) $(SRC3)
OBJ1 =	tar.$O create.$O extract.$O buffer.$O getoldopt.$O list.$O update.$O
OBJ2 =	version.$O names.$O diffarch.$O port.$O wildmat.$O getopt.$O getopt1.$O
OBJ3 =  gnu.$O mangle.$O $(LOCAL_OBJ) $(SUBOBJ)
OBJS =	$(OBJ1) $(OBJ2) $(OBJ3)
# AUX =	README PORTING Makefile TODO tar.h port.h open3.h \
#	msd_dir.h msd_dir.c
AUX =   README COPYING ChangeLog Makefile tar.texinfo tar.h port.h open3.h \
	rmt.h msd_dir.h msd_dir.c rtape_server.c rtape_lib.c getdate.y getopt.h

all:	tar rmt

tar:	$(OBJS)
	$(CC) $(LDFLAGS) -o tar $(COPTS) $(OBJS) $(LIBS)

rmt:	rtape_server.c
	$(CC) $(CFLAGS) -o rmt rtape_server.c

# command is too long for Messy-Dos (128 char line length limit) so
# this kludge is used...
#	@echo $(OBJ1) + > command
#	@echo $(OBJ2) >> command
#	link @command, $@,,$(LIBS) /NOI;
#	@$(RM) command

install: all
	$(RM) $(DESTDIR)/bin/tar
	$(INSTALL) tar   $(DESTDIR)/bin/tar
	$(INSTALL) tar.texinfo $(DESTDIR)/man/tar.texinfo
	$(INSTALL) rmt /etc/rmt

lint:	$(SRCS)
	$(LINT) $(LINTFLAGS) $(ALLDEFS) $(SRCS)

clean:
	$(RM) errs $(OBJS) tar rmt

shar: $(SRCS) $(AUX)
	shar $(SRCS) $(AUX) | compress > tar-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q version.c`.shar.Z

dist: $(SRC1) $(SRC2) $(AUX)
	echo tar-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q < version.c` > .fname
	mkdir `cat .fname`

	ln $(SRC1) $(SRC2) $(AUX) `cat .fname`
	tar cvhZf `cat .fname`.tar.Z `cat .fname`
	-rm -r `cat .fname` .fname

tar.zoo: $(SRCS) $(AUX)
	-mkdir tmp.dir
	-rm tar.zoo
	for X in $(SRCS) $(AUX) ; do echo $$X ; sed 's/$$/
	cd tmp.dir ; zoo aM ../tar.zoo *
	-rmdir tmp.dir

$(OBJS): tar.h port.h