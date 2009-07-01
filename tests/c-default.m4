#							-*- shell-script -*-

AT_SETUP(if default cpio creation and listing works)
dnl      ------------------------------------------

PREPARE(mix, list-mix)

AT_CHECK(
[cat list-mix | cpio -o > archive || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -t < archive || exit 1
cp list-mix expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -tv < archive | sed -e 's/ -> .*//' -e 's/^.* //'
cp list-mix expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CLEANUP($cleanup archive)

AT_SETUP(if installed cpio can unpack our cpio)
dnl      -------------------------------------

test -n "$BINCPIO" || exit 77

PREPARE(mix, list-mix, cpio-def)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  $BINCPIO -id$DEVFLAG < ../cpio-def || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  $BINCPIO -idum$DEVFLAG < ../cpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(if we can unpack our own archive)
dnl      --------------------------------

PREPARE(mix, list-mix, cpio-def)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id < ../cpio-def || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum < ../cpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)
