#! /bin/sh
# While creating intermediate directories, there was a protection error
# while attemping to create short prefix directories owned by root.

. ./preset
. $srcdir/before

set -e
mkdir directory
echo x > directory/file
tar cfP archive `pwd`/directory/file
rm -rf directory
echo -----
tar xfP archive `pwd`/directory/file

out="\
-----
"

. $srcdir/after
