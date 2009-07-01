#! /bin/sh
# Check if --recursive-unlink works as expected.

. ./preset
. $srcdir/before

set -e
echo OK > contents
tar cf archive contents
rm contents
mkdir contents
touch contents/file
tar xf archive --recursive-unlink
cat contents

out="\
OK
"

. $srcdir/after
