#							-*- shell-script -*-
# Inter-operability of old portable cpio formats.

AT_SETUP(if installed cpio creation works)
dnl      --------------------------------

test -n "$BINCPIO" || exit 77

PREPARE(mix, list-mix)

AT_CHECK(
[cat list-mix | $BINCPIO -o$DEVFLAG > archive || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cat list-mix | $BINCPIO -o$DEVFLAG $BINOLDC > archive || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive)

AT_SETUP(if old portable cpio creation and listing works)
dnl      -----------------------------------------------

PREPARE(mix, list-mix)

AT_CHECK(
[cat list-mix | cpio -o -c > archive || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -t -c < archive || exit 1
cp list-mix expout
CPIO_FILTER
], , expout,
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
[cpio -tv -c < archive | sed -e 's/ -> .*//' -e 's/^.* //'
cp list-mix expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CLEANUP($cleanup archive)

# Do a bit of optimization.
if test -n "$BINCPIO"; then
  PREPARE(icpio-def, icpio-oldc)
fi
PREPARE(cpio-oldc)

AT_SETUP(interoperability with installed cpio)
dnl      ------------------------------------

test -n "$BINCPIO" || exit 77

PREPARE(mix, list-mix, cpio-oldc, icpio-def)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id < ../icpio-def || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum < ../icpio-def || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  $BINCPIO -idum$DEVFLAG $BINOLDC < ../cpio-oldc || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  $BINCPIO -idum$DEVFLAG $BINOLDC < ../cpio-oldc || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(interoperability with installed cpio in portable mode)
dnl      -----------------------------------------------------

PREPARE(mix, list-mix, icpio-oldc)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id -c < ../icpio-oldc || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum -c < ../icpio-oldc || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(cpio interoperability with self)
dnl      -------------------------------

PREPARE(mix, list-mix, cpio-oldc)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id -c < ../cpio-oldc || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum -c < ../cpio-oldc || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

# Optimization cleanup.
if test -n "$BINCPIO"; then
  rm -f icpio-def icpio-oldc
fi
rm -f cpio-oldc
