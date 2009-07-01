## ------------------------------------------- ##
## Check if --with-included-malloc was given.  ##
## From François Pinard                        ##
## ------------------------------------------- ##

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
