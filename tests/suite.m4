#!/bin/sh
# Validation suite for Free `pax' utilities.		-*- shell-script -*-
# Copyright © 1998, 1999 Progiciels Bourbeau-Pinard inc.
# François Pinard <pinard@iro.umontreal.ca>, 1998.

# Check version with `tar' for the time being, as `pax' is not always built.
# When `pax' will be declared stable, use `pax' instead here.  FIXME!
AT_INIT(tar)

echo
echo 'cpio tests.'
echo

test -n "$check_cpio" || at_skip_mode=1

AT_DEFINE(PREPARE,
[cleanup=
ifelse($#, 0, , $1, , ,
[PREPARE_SUB($@)])])

AT_DEFINE(PREPARE_SUB,
[ifelse($#, 0, , $1, , ,
[if test ! ifelse($1,mix,-d, $1,smix,-d, $1,lmix,-d, $1,mmix,-d, -f) $1; then
ifelse($1, mix,
[  $at_srcdir/make-mix mix],
       $1, list-mix,
[  find mix -depth -print > $1],

       $1, lmix,
[  $at_srcdir/make-mix $1],
       $1, list-lmix,
[  find lmix -depth -print > $1],
       $1, lmix-one,
[  $at_srcdir/make-mix $1],

       $1, mmix,
[  $at_srcdir/make-mix $1],
       $1, list-mmix,
[  find mmix -depth -print > $1],

       $1, smix,
[  $at_srcdir/make-mix smix],
       $1, list-smix,
[  find smix -depth -print > $1],
       $1, listf-smix,
[  find smix -type f -depth -print > $1],

       $1, minijunk,
[  echo junk > $1],
       $1, maxijunk,
[  for loop in 1 2 3 4 5 6 7 8 9 10 11 12; do
    echo junkJUNKjunkJUNKjunkJUNKjunkawholelotofgarbagebecausewewant
    echo tomakesurepeeklooksatmorethan1recordofinput
  done > $1],

       $1, icpio-def,
[  cat list-mix | $BINCPIO -o$DEVFLAG > $1 2> /dev/null],
       $1, icpio-oldc,
[  cat list-mix | $BINCPIO -o$DEVFLAG $BINOLDC > $1 2> /dev/null],
       $1, itar-def,
[  $BINTAR cf $1 mix >/dev/null 2>&1],
       $1, cpio-def,
[  cat list-mix | cpio -o > $1 2> /dev/null],
       $1, cpio-oldc,
[  cat list-mix | cpio -oc > $1 2> /dev/null],
       $1, cpio-newc,
[  cat list-mix | cpio -oH newc > $1 2> /dev/null],
       $1, cpio-crc,
[  cat list-mix | cpio -oH crc > $1 2> /dev/null],
       $1, cpio-tar,
[  cat list-mix | cpio -oH tar > $1 2> /dev/null],
       $1, cpio-ustar,
[  cat list-mix | cpio -oH ustar > $1 2> /dev/null],
       $1, tar-def,
[  tar cf $1 mix >/dev/null 2>&1],

[  echo 2>&1 "ERROR: Do not know how to make \`$1'"
   exit 1])
  cleanup="$cleanup $1"
fi
PREPARE_SUB(AT_SHIFT($@))])])

# We may get many "truncating inode number" diagnostics in case the
# build directory has high-numbered inodes, CPIO_FILTER gets rid of them.
# One might prefer testing on a less used filesystem?  I'm not sure.
AT_DEFINE(CPIO_FILTER,
[grep -v ': truncating inode number' stderr \
| sed 's/^[[1-9][0-9]*] blocks*/NN blocks/' > stderr2
mv stderr2 stderr])

AT_DEFINE(VERIFY_EXACT,
verify -list -match-dir unmix -mode-match -uid-match -gid-match \
  -size-match -contents-match -mtime-match -ignore-dot-mtime < list-mix)

AT_DEFINE(VERIFY_NEWER,
verify -list -match-dir unmix -mode-match -uid-match -gid-match \
  -size-match -contents-match -mtime-le $TIME < list-mix)

AT_DEFINE(VERIFY_NEWER_FILTERED,
cat list-mix \
| fgrep -v -x -f mix/extra1 \
| verify -list -match-dir unmix -mode-match -uid-match -gid-match \
    -size-match -contents-match -mtime-le $TIME \
| fgrep -v -x -f mix/extra2)

rm -rf mix list-mix cpio-def cpio-oldc
PREPARE(mix, list-mix, cpio-def, cpio-oldc)
cleanup=
rm -rf smix list-smix listf-smix

AT_INCLUDE(c-default.m4)
AT_INCLUDE(c-block.m4)
AT_INCLUDE(c-oldc.m4)
AT_INCLUDE(c-pass.m4)
AT_INCLUDE(c-reset.m4)
AT_INCLUDE(c-tar.m4)
AT_INCLUDE(c-newc.m4)
AT_INCLUDE(c-crc.m4)
AT_INCLUDE(c-nopipe.m4)
AT_INCLUDE(c-skip.m4)
AT_INCLUDE(c-append.m4)
AT_INCLUDE(c-swap.m4)
# A few FIXME remain in c-junk.m4.
AT_INCLUDE(c-junk.m4)
# One FIXME remains in c-long.m4.
AT_INCLUDE(c-long.m4)
# A few FIXME remain in c-owner.m4.
AT_INCLUDE(c-owner.m4)

if test -z "$at_stop_on_error" || test -z "$at_failed_list"; then
  rm -rf mix list-mix cpio-def cpio-oldc
fi

at_skip_mode=

echo
echo 'tar tests.'
echo

AT_INCLUDE(t-create.m4)
AT_INCLUDE(t-extract.m4)
AT_INCLUDE(t-exclude.m4)
AT_INCLUDE(t-append.m4)
# One FIXME remains in t-delete.m4.
AT_INCLUDE(t-delete.m4)
AT_INCLUDE(t-incremen.m4)
AT_INCLUDE(t-gzip.m4)
AT_INCLUDE(t-volume.m4)

# End of test suite.
