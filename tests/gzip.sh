#! /bin/sh
# tar should detect that its gzip child failed.

. ./preset

# Skip test when /dev/null is not available.
# Hmph!  Seems it was only for DOS, where DJGPP now supports it.
#test -r /dev/null && exit 77

. $srcdir/before

tar xfvz /dev/null
test $? = 2 || exit 1

err="\

gzip: stdin: unexpected end of file
tar: Child returned status 1
tar: Processed all files possible, despite earlier errors
"

. $srcdir/after
