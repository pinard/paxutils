#							-*- shell-script -*-

# Test blocking.  It will not do much good on a pipe, but because it will
# cause all reads and writes to use the same blocksize it will help exercise
# `sparse.c'.

AT_SETUP(option -C)
dnl      ---------

PREPARE(structure, struct-list, archive)
mkdir unpacked

AT_CHECK(
[cd unpacked
  cpio -idum -C 1 < ../archive || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  cpio -idum -C 256 < ../archive || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  cpio -idum --block-size=20 < ../archive || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unpacked)

AT_SETUP(option -B)
dnl      ---------

PREPARE(structure, struct-list, archive)
mkdir unpacked

AT_CHECK(
[cd unpacked
  cpio -idm -B < ../archive || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  # Probably should verify linked message.
  cpio -idumv < ../archive 2>&1 | grep -v 'cpio.*linked to' 1>&2
cd ..
cp struct-list experr
echo 'NN blocks' >> experr
VERIFY_EXACT
CPIO_FILTER
], , , experr)

AT_CLEANUP($cleanup unpacked)
