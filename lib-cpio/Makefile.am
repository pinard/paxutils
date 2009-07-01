AUTOMAKE_OPTIONS = gnits

INCLUDES = -I../intl -I$(top_srcdir)/intl

noinst_LIBRARIES = libcpio.a

libcpio_a_SOURCES = ansidecl.h dstring.h dirname.c dstring.c error.c \
filemode.c filetypes.h fnmatch.h getopt.c getopt.h getopt1.c idcache.c \
makepath.c octal.c stpcpy.c stripslash.c userspec.c xmalloc.c \
xstrdup.c

libcpio_a_LIBADD = @LIBOBJS@ @ALLOCA@
