#!/bin/sh
# Validation suite for Free `pax' utilities.
# Copyright © 1998 Progiciels Bourbeau-Pinard inc.
# François Pinard <pinard@iro.umontreal.ca>, 1998.

AT_INIT(pax)

echo
echo 'cpio tests.'
echo

AT_DEFINE(PREPARE,
[cleanup=
ifelse($#, 0, , $1, , ,
[PREPARE_SUB($@)])])

AT_DEFINE(PREPARE_SUB,
[ifelse($#, 0, , $1, , ,
[if test ! ifelse($1, structure, -d, -f) $1; then
ifelse($1, structure,
[  $at_srcdir/make-structure],
       $1, struct-list,
[  find structure -depth -print > $1],
       $1, archive,
[  cat struct-list | cpio -o > $1 2> /dev/null],
       $1, arch-oldc,
[  cat struct-list | cpio -o -c > $1 2> /dev/null],
       $1, arch-newc,
[  cat struct-list | cpio -o -H newc > $1 2> /dev/null],
       $1, arch-crc,
[  cat struct-list | cpio -o -H crc > $1 2> /dev/null],
       $1, arch-tar,
[  cat struct-list | cpio -o -H tar > $1 2> /dev/null],
       $1, arch-ustar,
[  cat struct-list | cpio -o -H ustar > $1 2> /dev/null],
       $1, arch-inst,
[  cat struct-list | $BINCPIO -o$DEVFLAG > $1 2> /dev/null],
       $1, arch-c-inst,
[  cat struct-list | $BINCPIO -o$DEVFLAG $BINOLDC > $1 2> /dev/null],
       $1, minijunk,
[  echo junk > $1],
       $1, maxijunk,
[  for loop in 1 2 3 4 5 6 7 8 9 10 11 12; do
    echo junkJUNKjunkJUNKjunkJUNKjunkawholelotofgarbagebecausewewant
    echo tomakesurepeeklooksatmorethan1recordofinput
  done > $1],
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
| sed 's/^[[1-9][0-9]]* blocks*/NN blocks/' > stderr2
mv stderr2 stderr])

AT_DEFINE(VERIFY_EXACT,
verify -list -match-dir unpacked -mode-match -uid-match -gid-match \
  -size-match -contents-match -mtime-match -ignore-dot-mtime < struct-list)

AT_DEFINE(VERIFY_NEWER,
verify -list -match-dir unpacked -mode-match -uid-match -gid-match \
  -size-match -contents-match -mtime-lt $TIME < struct-list)

# Prior cleanup should ideally not be needed.
rm -rf structure unpacked
rm -f struct-list archive arch-oldc arch-newc
rm -f arch-inst arch-c-inst

# Uncomment to hide our `cpio' by system's one, on the search PATH.
# export PATH=/usr/bin:$PATH

PREPARE(structure, struct-list)
AT_INCLUDE(c-base.m4)

# FIXME: struct-list gets deleted by the above check, while it should not.
PREPARE(structure, struct-list)
PREPARE(archive, arch-oldc)
if test -n "$BINCPIO"; then
  PREPARE(arch-inst, arch-c-inst)
fi
AT_INCLUDE(c-unpack.m4)
AT_INCLUDE(c-block.m4)
AT_INCLUDE(c-interop.m4)
dnl AT_INCLUDE(c-pass.m4)
dnl AT_INCLUDE(c-reset.m4)
dnl AT_INCLUDE(c-tar.m4)
dnl AT_INCLUDE(c-newc.m4)
dnl AT_INCLUDE(c-crc.m4)
dnl AT_INCLUDE(c-nopipe.m4)
dnl AT_INCLUDE(c-skip.m4)
dnl AT_INCLUDE(c-append.m4)
dnl AT_INCLUDE(c-swap.m4)
dnl AT_INCLUDE(c-odd.m4)
dnl AT_INCLUDE(c-junk.m4)
dnl AT_INCLUDE(c-long.m4)
dnl AT_INCLUDE(c-owner.m4)

if test -z "$at_stop_on_error" || test -z "$at_failed_list"; then
  rm -rf structure unpacked
  rm -f struct-list archive arch-oldc arch-newc
  rm -f arch-inst arch-c-inst
fi

echo
echo 'tar tests.'
echo

AT_INCLUDE(create.m4)
AT_INCLUDE(extract.m4)
AT_INCLUDE(exclude.m4)
AT_INCLUDE(append.m4)
AT_INCLUDE(delete.m4)
AT_INCLUDE(incremen.m4)
AT_INCLUDE(gzip.m4)
AT_INCLUDE(volume.m4)
