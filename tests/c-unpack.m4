#							-*- shell-script -*-

AT_SETUP(if installed cpio can unpack our archive)
dnl      ----------------------------------------

test -n "$BINCPIO" || exit 77

PREPARE(structure, struct-list, archive)
mkdir unpacked

AT_CHECK(
[TIME=`echotime`
sleep 1
cd unpacked
  $BINCPIO -id$DEVFLAG < ../archive || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  $BINCPIO -idum$DEVFLAG < ../archive || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unpacked)

AT_SETUP(if we can unpack our own archive)
dnl      --------------------------------

PREPARE(structure, struct-list, archive)
mkdir unpacked

AT_CHECK(
[TIME=`echotime`
sleep 1
cd unpacked
  cpio -id < ../archive || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  cpio -idum < ../archive || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unpacked)
