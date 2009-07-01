dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])


# Single argument says where are built sources to test, relative to the
# built test directory.  Maybe omitted if the same (flat distribution).

AC_DEFUN(AT_CONFIG,
[AT_TESTPATH=ifelse($1, , ., $1)
AC_SUBST(AT_TESTPATH)
fp_PROG_ECHO
])


# Once this macro is called, you may output with no echo in a Makefile or
# script using:  echo @ECHO_N@ "STRING_TO_OUTPUT@ECHO_C@".

AC_DEFUN(fp_PROG_ECHO,
[AC_CACHE_CHECK(how to suppress newlines using echo, fp_cv_prog_echo_nonl,
[if (echo "testing\c"; echo 1,2,3) | grep c >/dev/null; then
  if (echo -n testing; echo 1,2,3) | sed s/-n/xn/ | grep xn >/dev/null; then
    fp_cv_prog_echo_nonl=no
  else
    fp_cv_prog_echo_nonl=option
  fi
else
  fp_cv_prog_echo_nonl=escape
fi
])
test $fp_cv_prog_echo_nonl = no \
  && echo 2>&1 "WARNING: \`echo' not powerful enough for \`make check'"
case $fp_cv_prog_echo_nonl in
  no) ECHO_N= ECHO_C= ;;
  option) ECHO_N=-n ECHO_C= ;;
  escape) ECHO_N= ECHO_C='\c' ;;
esac
AC_SUBST(ECHO_N)dnl
AC_SUBST(ECHO_C)dnl
])


# serial 1

AC_DEFUN(AM_C_PROTOTYPES,
[AC_REQUIRE([AM_PROG_CC_STDC])
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for function prototypes])
if test "$am_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES,1,[Define if compiler has function prototypes])
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=./ansi2knr
  # Ensure some checks needed by ansi2knr itself.
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h)
fi
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])


# serial 1

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN(AM_PROG_CC_STDC,
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX			-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;
], [
return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];
],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
if test -z "$am_cv_prog_cc_stdc"; then
  AC_MSG_RESULT([none needed])
else
  AC_MSG_RESULT($am_cv_prog_cc_stdc)
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])


# SunOS 4.1.3's printf treats %llu as %lu.  From Alexandre Oliva.

AC_DEFUN(fp_HAVE_PRINTF_LLU,
[AC_CACHE_CHECK(whether printf understands %llu, tar_cv_have_printf_llu,
[AC_TRY_RUN([#include <stdio.h>
main ()
{
  unsigned long long before = 1;
  char buffer[1024];
  while (before)
    {
       unsigned long long after = 0;
       sprintf(buffer, "%llu", before);
       sscanf(buffer, "%llu", &after);
       if (after != before)
	 exit (1);
       before <<= 1;
    }
  exit (0);
}
], tar_cv_have_printf_llu=yes, tar_cv_have_printf_llu=no,
  test "$tar_cv_have_printf_llu" = '' && tar_cv_have_printf_llu=cross)])
test "$tar_cv_have_printf_llu" = yes && AC_DEFINE(HAVE_PRINTF_LLU)
])


# serial 1

AC_DEFUN(AM_WITH_DMALLOC,
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
[  --with-dmalloc          use dmalloc, as in
                          ftp://ftp.letters.com/src/dmalloc/dmalloc.tar.gz],
[if test "$withval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC,1,
            [Define if using the dmalloc debugging malloc package])
  LIBS="$LIBS -ldmalloc"
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])


# In 1992, Michael Bushnell (now Thomas Bushnell <thomas@gnu.org>)
# devised a test for avoiding HP/UX malloc and using GNU malloc instead.
# Bruno Haible <haible@ma2s2.mathematik.uni-karlsruhe.de> recycled this
# test for CLISP Common LISP and extended it to cover broken mallocs
# from SGI.  I (<pinard@iro.umontreal.ca>) reworked it a little so it is
# independent of config.guess, and overridable by the installer.

# On IRIX 5.2, libc malloc is broken, but the -lmalloc one was usable.
# So in my packages, I once unconditionally used -lmalloc if it existed.
# This does not do anymore, because the -lmalloc malloc is broken on
# Solaris 2.4 to 2.5.1 (alignment is 4 bytes instead of 8 bytes, as
# reported by John Wells <john@bitsmart.com>).

# Bruno also notes: "HP-UX has two different malloc() implementations.
# Both are broken.  When used with CLISP, the one in the default libc.a
# leads to a SIGSEGV, the one in libmalloc.a leads to a SIGBUS."

# If the installer does not give a preference, we use the included GNU
# malloc if we have the slightest doubt that malloc could be broken,
# this includes cross compilation, and *all* HP/UX or IRIX systems.
# It is crude indeed, but I just do not have enough information for truly
# benchmarking malloc in all cases, but want safe packages nevertheless.

AC_DEFUN(fp_WITH_MALLOC,
[AC_MSG_CHECKING(if included GNU malloc is wanted)
AC_ARG_WITH(included-malloc,
[  --with-included-malloc  use the GNU malloc which is included here], ,
[if test $cross_compiling = yes; then
  withval=yes
else
  case `uname -s 2> /dev/null` in
    HP-UX | IRIX* ) withval=yes ;;
    *) withval=no ;;
  esac
fi])
test "$withval" = yes && LIBOBJS="$LIBOBJS gmalloc.o"
AC_MSG_RESULT($withval)
])

#serial 1

dnl From Jim Meyering.
dnl Determine whether malloc accepts 0 as its argument.
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_malloc if the replacement function should be used.  */
dnl  #undef malloc
dnl

AC_DEFUN(jm_FUNC_MALLOC,
[
 if test x = y; then
   dnl This code is deliberately never run via ./configure.
   dnl FIXME: this is a gross hack to make autoheader put an entry
   dnl for this symbol in config.h.in.
   AC_CHECK_FUNCS(DONE_WORKING_MALLOC_CHECK)
 fi
 dnl xmalloc.c requires that this symbol be defined so it doesn't
 dnl mistakenly use a broken malloc -- as it might if this test were omitted.
 ac_kludge=HAVE_DONE_WORKING_MALLOC_CHECK
 AC_DEFINE_UNQUOTED($ac_kludge)

 AC_CACHE_CHECK([for working malloc], jm_cv_func_working_malloc,
  [AC_TRY_RUN([
    char *malloc ();
    int
    main ()
    {
      exit (malloc (0) ? 0 : 1);
    }
	  ],
	 jm_cv_func_working_malloc=yes,
	 jm_cv_func_working_malloc=no,
	 dnl When crosscompiling, assume malloc is broken.
	 jm_cv_func_working_malloc=no)
  ])
  if test $jm_cv_func_working_malloc = no; then
    LIBOBJS="$LIBOBJS malloc.o"
    AC_DEFINE_UNQUOTED(malloc, rpl_malloc)
  fi
])

#serial 1

dnl From Jim Meyering.
dnl Determine whether realloc works when both arguments are 0.
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_realloc if the replacement function should be used.  */
dnl  #undef realloc
dnl

AC_DEFUN(jm_FUNC_REALLOC,
[
 if test x = y; then
   dnl This code is deliberately never run via ./configure.
   dnl FIXME: this is a gross hack to make autoheader put an entry
   dnl for this symbol in config.h.in.
   AC_CHECK_FUNCS(DONE_WORKING_REALLOC_CHECK)
 fi
 dnl xmalloc.c requires that this symbol be defined so it doesn't
 dnl mistakenly use a broken realloc -- as it might if this test were omitted.
 ac_kludge=HAVE_DONE_WORKING_REALLOC_CHECK
 AC_DEFINE_UNQUOTED($ac_kludge)

 AC_CACHE_CHECK([for working realloc], jm_cv_func_working_realloc,
  [AC_TRY_RUN([
    char *realloc ();
    int
    main ()
    {
      exit (realloc (0, 0) ? 0 : 1);
    }
	  ],
	 jm_cv_func_working_realloc=yes,
	 jm_cv_func_working_realloc=no,
	 dnl When crosscompiling, assume realloc is broken.
	 jm_cv_func_working_realloc=no)
  ])
  if test $jm_cv_func_working_realloc = no; then
    LIBOBJS="$LIBOBJS realloc.o"
    AC_DEFINE_UNQUOTED(realloc, rpl_realloc)
  fi
])

# Select gettext and choose translations to install.
# François Pinard <pinard@iro.umontreal.ca>, 1998.

AC_DEFUN(fp_WITH_GETTEXT, [

  AC_MSG_CHECKING(whether NLS is wanted)
  AC_ARG_ENABLE(nls,
    [  --disable-nls           disallow Native Language Support],
    enable_nls=$enableval, enable_nls=yes)
  AC_MSG_RESULT($enable_nls)
  use_nls=$enable_nls
  AM_CONDITIONAL(USE_NLS, test $use_nls = yes)

  if test $enable_nls = yes; then
    AC_DEFINE(ENABLE_NLS)

    AC_ARG_WITH(catgets,
      [  --with-catgets          say that catgets is not supported],
      [AC_MSG_WARN([catgets not supported, --with-catgets ignored])])

    AC_CHECK_FUNCS(gettext)
    AC_CHECK_LIB(intl, gettext, :)
    if test $ac_cv_lib_intl_gettext$ac_cv_func_gettext != nono; then
      AC_MSG_CHECKING(whether the included gettext is preferred)
      AC_ARG_WITH(included-gettext,
	[  --without-included-gettext avoid our provided version of gettext],
	with_included_gettext=$withval, with_included_gettext=yes)
      AC_MSG_RESULT($with_included_gettext)
      if test $with_included_gettext$ac_cv_func_gettext = nono; then
        LIBS="$LIBS -lintl"
      fi
    else
      with_included_gettext=yes
    fi
    if test $with_included_gettext = yes; then
      LIBOBJS="$LIBOBJS gettext.o"
      AC_DEFINE(HAVE_GETTEXT)
      AC_DEFINE(HAVE_DCGETTEXT)
    else
      AC_CHECK_HEADERS(libintl.h)
      AC_CHECK_FUNCS(dcgettext gettext)
    fi

    AC_CHECK_HEADERS(locale.h)
    AC_CHECK_FUNCS(setlocale)
    AM_LC_MESSAGES

    if test -z "$ALL_LINGUAS"; then
      AC_MSG_WARN(This package does not install translations yet.)
    else
      ac_items="$ALL_LINGUAS"
      for ac_item in $ac_items; do
	ALL_POFILES="$ALL_POFILES $ac_item.po"
	ALL_MOFILES="$ALL_MOFILES $ac_item.mo"
      done
    fi
    AC_SUBST(ALL_LINGUAS)
    AC_SUBST(ALL_POFILES)
    AC_SUBST(ALL_MOFILES)

    AC_MSG_CHECKING(which translations to install)
    if test -z "$LINGUAS"; then
      ac_print="$ALL_LINGUAS"
      MOFILES="$ALL_MOFILES"
    else
      ac_items="$LINGUAS"
      for ac_item in $ac_items; do
	case "$ALL_LINGUAS" in
	  *$ac_item*)
	    ac_print="$ac_print $ac_item"
	    MOFILES="$MOFILES $ac_item.mo"
	    ;;
	esac
      done
    fi
    AC_SUBST(MOFILES)
    if test -z "$ac_print"; then
      AC_MSG_RESULT(none)
    else
      AC_MSG_RESULT($ac_print)
    fi

  fi])

# Define a conditional.

AC_DEFUN(AM_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])

# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 1

AC_DEFUN(AM_LC_MESSAGES,
  [if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES)
    fi
  fi])

