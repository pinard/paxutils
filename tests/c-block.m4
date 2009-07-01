#							-*- shell-script -*-

# Test blocking.  It will not do much good on a pipe, but because it will
# cause all reads and writes to use the same blocksize it will help exercise
# `sparse.c'.

AT_SETUP(option -C)
dnl      ---------

PREPARE(mix, list-mix, cpio-def)
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idum -C 1 < ../cpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum -C 256 < ../cpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum --block-size=20 < ../cpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(option -B)
dnl      ---------

PREPARE(mix, list-mix, cpio-def)
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idm -B < ../cpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  # Probably should verify linked message.
  cpio -idumv < ../cpio-def 2>&1 | grep -v 'cpio.*linked to' 1>&2
cd ..
cp list-mix experr
echo 'NN blocks' >> experr
VERIFY_EXACT
CPIO_FILTER
], , , experr)

AT_CLEANUP($cleanup unmix)
