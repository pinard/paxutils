#! /bin/sh
# Could not extract symlinks over an existing file.

. ./preset

# Skip test when file links are not available, like on DOSish systems.
test -z "$COMSPEC" || exit 77
test -z "$ComSpec" || exit 77

. $srcdir/before

set -e

touch file
ln -s file link 2> /dev/null || exit 77
# But cleanup does not occur when exiting!  FIXME!
tar cf archive link
rm link
touch link
tar xf archive

. $srcdir/after
