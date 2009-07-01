#							-*- shell-script -*-

# Inter-operability checks.

AT_SETUP(interoperability with installed cpio)
dnl      ------------------------------------

test -n "$BINCPIO" || exit 77

PREPARE(structure, struct-list, arch-oldc, arch-inst, arch-c-inst)
mkdir unpacked

AT_CHECK(
[TIME=`echotime`
sleep 1
cd unpacked
  cpio -id < ../arch-inst || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  cpio -idum < ../arch-inst || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
sleep 1
cd unpacked
  $BINCPIO -id$DEVFLAG $BINOLDC < ../arch-oldc || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  $BINCPIO -idum$DEVFLAG $BINOLDC < ../arch-oldc || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
sleep 1
cd unpacked
  cpio -id -c < ../arch-c-inst || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  cpio -idum -c < ../arch-c-inst || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unpacked)

AT_SETUP(cpio interoperability with self)
dnl      -------------------------------

PREPARE(structure, struct-list, arch-oldc)
mkdir unpacked

AT_CHECK(
[TIME=`echotime`
sleep 1
cd unpacked
  cpio -id -c < ../arch-oldc || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unpacked
  cpio -idum -c < ../arch-oldc || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unpacked)
