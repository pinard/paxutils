/* Special definitions for paxutils, processed by autoheader.
   Copyright © 1994, 1997, 1998, 1999 Free Software Foundation, Inc.
   François Pinard <pinard@iro.umontreal.ca>, 1993.
*/

/* Define if old GNU tar format support should be included.  (Doesn't work) */
#undef CPIO_USE_GNUTAR

/* Define if internal debugging code should be enabled.  */
#undef DEBUG_CPIO

/* Define to a string giving the full name of the default archive file.  */
#undef DEFAULT_ARCHIVE

/* Define to a number giving the default blocking size for archives.  */
#undef DEFAULT_BLOCKING

/* Define to 1 if density may be indicated by [lmh] at end of device.  */
#undef DENSITY_LETTER

/* Define to a string giving the prefix of the default device, without the
   part specifying the unit and density.  */
#undef DEVICE_PREFIX

/* Define to 1 if you lack a 3-argument version of open, and want to
   emulate it with system calls you do have.  */
#undef EMUL_OPEN3

/* Define to 1 if --per-file-compress option should be compiled in.  */
#undef ENABLE_DALE_CODE

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Define as 1 if you have dcgettext.  */
#undef HAVE_DCGETTEXT

/* Define to 1 if you have getgrgid(3).  */
#undef HAVE_GETGRGID

/* Define to 1 if you have getpwuid(3).  */
#undef HAVE_GETPWUID

/* Define as 1 if you have gettext.  */
#undef HAVE_GETTEXT

/* Define to 1 if mknod function is available.  */
#undef HAVE_MKNOD

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

/* Define to 1 if printf() supports "%llu" format.  */
#undef HAVE_PRINTF_LLU

/* Define to 1 if some rsh exists, or if you have <netdb.h>.  */
#undef HAVE_RTAPELIB

/* Define to 1 if stpcpy function is available.  */
#undef HAVE_STPCPY

/* Define to 1 if you have the valloc function.  */
#undef HAVE_VALLOC

/* Define if `union wait' is the type of the first arg to wait functions.  */
#undef HAVE_UNION_WAIT

/* Define to 1 if utime.h exists and declares struct utimbuf.  */
#undef HAVE_UTIME_H

/* Define to mt_model (v.g., for DG/UX), else to mt_type.  */
#undef MTIO_CHECK_FIELD

/* Define to the installation directory for locales.  */
#undef LOCALEDIR

/* Define to the name of the distribution.  */
#undef PACKAGE

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES

/* Define to the full path of your rsh, if any.  */
#undef REMOTE_SHELL

/* Path to directory containing system wide message catalog sources.  */
#undef STD_INC_PATH

/* Define to the version of the distribution.  */
#undef VERSION

/* Define to 1 if using the `glocale' package for message catalogs.  */
#undef WITH_CATALOGS

/* Define to 1 for better use of the debugging malloc library.  See
   site ftp.antaire.com in antaire/src, file dmalloc/dmalloc.tar.gz.  */
#undef WITH_DMALLOC

/* Define to 1 if GNU regex should be used instead of GNU rx.  */
#undef WITH_REGEX

/* Define to rpl_malloc if the replacement function should be used.  */
#undef malloc

/* Define to type of major device number, if not otherwise defined.  */
#undef major_t

/* Define to type of minor device number, if not otherwise defined.  */
#undef minor_t

/* Define to rpl_realloc if the replacement function should be used.  */
#undef realloc

/* Define to type of signed memory size, if not otherwise defined.  */
#undef ssize_t

@BOTTOM@

/* This is recommended for all GNU sources.  We conditionalize it in
   case some library file defines it.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

/* Long longs are of no use if we cannot print them easily.  */
#if !HAVE_PRINTF_LLU
# undef SIZEOF_UNSIGNED_LONG_LONG
# define SIZEOF_UNSIGNED_LONG_LONG 0
#endif
