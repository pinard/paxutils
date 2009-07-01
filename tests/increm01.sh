#! /bin/sh
# A directory older than the listed entry was skipped completely.

. ./preset
. $srcdir/before

# Cover for the DOS 2-seconds granularity in file timestamps.

set -e
mkdir structure
touch structure/file
sleep 2
tar cfv archive --listed=list structure
echo -----
tar cfv archive --listed=list structure
sleep 3
touch structure/file
echo -----
tar cfv archive --listed=list structure

out="\
structure/
structure/file
-----
structure/
-----
structure/
structure/file
"

. $srcdir/after
