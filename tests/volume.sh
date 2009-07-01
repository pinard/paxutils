#! /bin/sh
# Volume labels are checked on read by fnmatch.

. ./preset

# Skip test when /dev/null is not available.
# Hmph!  Seems it was only for DOS, where DJGPP now supports it.
#test -r /dev/null && exit 77

. $srcdir/before

tar cfVT archive label /dev/null || exit 1

tar xfV archive label || exit 1
tar xfV archive 'la?el' || exit 1
tar xfV archive 'l*l' || exit 1

echo 1>&2 -----
tar xfV archive lab
test $? = 2 || exit 1
echo 1>&2 -----
tar xfV archive bel
test $? = 2 || exit 1
echo 1>&2 -----
tar xfV archive babel
test $? = 2 || exit 1

err="\
-----
tar: Volume \`label' does not match \`lab'
tar: Error is not recoverable: exiting now
-----
tar: Volume \`label' does not match \`bel'
tar: Error is not recoverable: exiting now
-----
tar: Volume \`label' does not match \`babel'
tar: Error is not recoverable: exiting now
"

. $srcdir/after
