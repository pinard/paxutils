# Local additions to Autoconf macros.
# Copyright (C) 1992, 1994, 1995 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1992.

## ------------------------------- ##
## Check for function prototypes.  ##
## ------------------------------- ##

AC_DEFUN(fp_C_PROTOTYPES,
[AC_REQUIRE([fp_PROG_CC_STDC])
AC_MSG_CHECKING([for function prototypes])
if test "$ac_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES)
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=ansi2knr
fi
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])

## ----------------------------------------- ##
## ANSIfy the C compiler whenever possible.  ##
## ----------------------------------------- ##

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it defines @code{__STDC__} to 1 and
# handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{ac_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN(fp_PROG_CC_STDC,
[AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(ac_cv_prog_cc_stdc,
[ac_cv_prog_cc_stdc=no
ac_save_CFLAGS="$CFLAGS"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX			-Aa -D_HPUX_SOURCE
# SVR4			-Xc
for ac_arg in "" -qlanglvl=ansi -std1 "-Aa -D_HPUX_SOURCE" -Xc
do
  CFLAGS="$ac_save_CFLAGS $ac_arg"
  AC_TRY_COMPILE(
[#if !defined(__STDC__) || __STDC__ != 1
choke me
#endif	
], [int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};],
[ac_cv_prog_cc_stdc="$ac_arg"; break])
done
CFLAGS="$ac_save_CFLAGS"
])
AC_MSG_RESULT($ac_cv_prog_cc_stdc)
case "x$ac_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $ac_cv_prog_cc_stdc" ;;
esac
])

## --------------------------------------------------------- ##
## Use AC_PROG_INSTALL, supplementing it with INSTALL_SCRIPT ##
## substitution.					     ##
## --------------------------------------------------------- ##

AC_DEFUN(fp_PROG_INSTALL,
[AC_PROG_INSTALL
test -z "$INSTALL_SCRIPT" && INSTALL_SCRIPT='${INSTALL} -m 755'
AC_SUBST(INSTALL_SCRIPT)dnl
])

## ----------------------------------- ##
## Check if --with-dmalloc was given.  ##
## ----------------------------------- ##

# I just checked, and GNU rx seems to work fine with a slightly
# modified GNU m4.  So, I put out the test below in my aclocal.m4,
# and will try to use it in my things.  The idea is to distribute
# rx.[hc] and regex.[hc] together, for a while.  The WITH_REGEX symbol
# (which should also be documented in acconfig.h) is used to decide
# which of regex.h or rx.h should be included in the application.
#
# If `./configure --with-regex' is given, the package will use
# the older regex.  Else, a check is made to see if rx is already
# installed, as with newer Linux'es.  If not found, the package will
# use the rx from the distribution.  If found, the package will use
# the system's rx which, on Linux at least, will result in a smaller
# executable file.

AC_DEFUN(fp_WITH_DMALLOC,
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
[  --with-dmalloc          use dmalloc, as in
                          ftp://ftp.letters.com/src/dmalloc/dmalloc.tar.gz],
[if test "$withval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC)
  LIBS="$LIBS -ldmalloc"
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])

## --------------------------------- ##
## Check if --with-regex was given.  ##
## --------------------------------- ##

AC_DEFUN(fp_WITH_REGEX,
[AC_MSG_CHECKING(which of rx or regex is wanted)
AC_ARG_WITH(regex,
[  --with-regex            use older regex in lieu of GNU rx for matching],
[if test "$withval" = yes; then
  ac_with_regex=1
  AC_MSG_RESULT(regex)
  AC_DEFINE(WITH_REGEX)
  LIBOBJS="$LIBOBJS regex.o"
fi])
if test -z "$ac_with_regex"; then
  AC_MSG_RESULT(rx)
  AC_CHECK_FUNC(re_rx_search, , [LIBOBJS="$LIBOBJS rx.o"])
fi
AC_SUBST(LIBOBJS)dnl
])

## ------------------- ##
## Check NLS options.  ##
## ------------------- ##

AC_DEFUN(ud_LC_MESSAGES,
[if test $ac_cv_header_locale_h = yes; then
  AC_MSG_CHECKING([for LC_MESSAGES])
  AC_CACHE_VAL(ud_cv_val_LC_MESSAGES,
    [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
     ud_cv_val_LC_MESSAGES=yes, ud_cv_val_LC_MESSAGES=no)])
  AC_MSG_RESULT($ud_cv_val_LC_MESSAGES)
  if test $ud_cv_val_LC_MESSAGES = yes; then
    AC_DEFINE(HAVE_LC_MESSAGES)
  fi
fi
])

define(ud_WITH_NLS,
[AC_MSG_CHECKING([whether NLS is requested])
  dnl Default is enabled NLS
  AC_ARG_ENABLE(nls,
    [  --disable-nls           do not use Native Language Support],
    [nls_cv_use_nls=$enableval], [nls_cv_use_nls=yes])
  AC_MSG_RESULT($nls_cv_use_nls)

  dnl If we use NLS figure out what method
  if test "$nls_cv_use_nls" = "yes"; then
    AC_DEFINE(ENABLE_NLS)
    AC_MSG_CHECKING([for explicitly using GNU gettext])
    AC_ARG_WITH(gnu-nls,
      [  --with-gnu-nls          use the GNU NLS library],
      [nls_cv_force_use_gnu_nls=$withval], [nls_cv_force_use_gnu_nls=no])
    AC_MSG_RESULT($nls_cv_force_use_gnu_nls)

    if test "$nls_cv_force_use_gnu_nls" = "yes"; then
      nls_cv_use_gnu_nls=yes
    else
      dnl User does not insist on using GNU NLS library.  Figure out what
      dnl to use.  If gettext or catgets are available (in this order) we
      dnl use this.  Else we have to fall back to GNU NLS library.
      AC_CHECK_LIB(intl, main)
      AC_CHECK_LIB(i, main)
      AC_FUNC_CHECK(gettext,
        [AC_DEFINE(HAVE_GETTEXT)
         AC_PATH_PROG(MSGFMT, msgfmt, msgfmt)dnl
	 AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)dnl
	 AC_PATH_PROG(XGETTEXT, xgettext, xgettext)dnl
         INTLOBJS="\$(GETTOBJS)"
         CATOBJEXT=.mo
	 DATADIRNAME=lib],

        dnl No gettext in C library.  Try catgets next.
        [AC_FUNC_CHECK(catgets,
          [AC_DEFINE(HAVE_CATGETS)
           INTLOBJS="\$(CATOBJS)"
           AC_PATH_PROG(GENCAT, gencat, [echo gencat])dnl
	   AC_PATH_PROGS(GMSGFMT, [gmsgfmt msgfmt], msgfmt)dnl
	   AC_PATH_PROG(XGETTEXT, xgettext, xgettext)dnl
           CATOBJEXT=.cat
	   DATADIRNAME=lib
	   INTLDEPS="../intl/libintl.a"
	   INTLLIBS=$INTLDEPS
	   nls_cv_header_intl=intl/libintl.h
	   nls_cv_header_libgt=intl/libgettext.h],

          dnl Even catgets is not included.  Fall back on GNU NLS library.
          [AC_MSG_RESULT([using GNU gettext])
	   nls_cv_use_gnu_nls=yes])])
    fi
    if test "$nls_cv_use_gnu_nls" = "yes"; then
      dnl Mark actions used to generate GNU NLS library.
      INTLOBJS="\$(GETTOBJS)"
      AC_PATH_PROG(MSGFMT, msgfmt, msgfmt)dnl
      AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)dnl
      AC_PATH_PROG(XGETTEXT, xgettext, xgettext)dnl
      AC_SUBST(MSGFMT)
      CATOBJEXT=.gmo
      DATADIRNAME=share
      INTLDEPS="../intl/libintl.a"
      INTLLIBS=$INTLDEPS
      nls_cv_header_intl=intl/libintl.h
      nls_cv_header_libgt=intl/libgettext.h
    fi
    # We need to process the intl/ directory
    INTLSUB=intl
  fi
  AC_SUBST(INTLOBJS)
  if test -n "$CATOBJEXT"; then
    for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
  fi
  AC_SUBST(CATALOGS)
  AC_SUBST(CATOBJEXT)
  AC_SUBST(DATADIRNAME)
  AC_SUBST(INTLDEPS)
  AC_SUBST(INTLLIBS)
  AC_SUBST(INTLSUB)
])

AC_DEFUN(ud_GNU_GETTEXT,
[AC_CHECK_HEADERS(locale.h nl_types.h)
AC_CHECK_FUNCS(setlocale)

ud_LC_MESSAGES
ud_WITH_NLS

if test "x$prefix" = xNONE; then
  AC_DEFINE_UNQUOTED(STD_INC_PATH, "$ac_default_prefix/share/nls/src")
else
  AC_DEFINE_UNQUOTED(STD_INC_PATH, "$prefix/share/nls/src")
fi
])
