#! /bin/sh
# An existing directory was not incrementally emptied at restore time.

. ./preset
. $srcdir/before

set -e

mkdir before
cd before
mkdir dir-a
echo alpha > dir-a/file-1
echo beta > file-2
tar cfg ../archiv-1 ../listed .
echo gamma > file-3
mkdir dir-b
echo delta > dir-b/file-4
rm dir-a/file-1 file-2
rmdir dir-a
tar cfg ../archiv-2 ../listed .
cd ..

mkdir after
cd after
tar xfg ../archiv-1 ../listed
tar xfg ../archiv-2 ../listed
cd ..

diff -r before after

. $srcdir/after
