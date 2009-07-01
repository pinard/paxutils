#							-*- shell-script -*-
# New portable cpio format.

AT_SETUP(if new portable cpio creation and listing works)
dnl      -----------------------------------------------

PREPARE(mix, list-mix)
sed -e '/^\.\/..*$/s/^\.\///' list-mix | sort > expout

AT_CHECK(
[cat list-mix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -t -H newc < archive | sort
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -tv -H newc < archive | sed -e 's,^.* \(mix[[a-z0-9/]*]\).*,\1,' | sort
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -tv < archive | sed -e 's,^.* \(mix[[a-z0-9/]*]\).*,\1,' | sort
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CLEANUP($cleanup archive)

AT_SETUP(if new portable cpio extraction works)
dnl      -------------------------------------

PREPARE(mix, list-mix, cpio-newc)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id -H newc < ../cpio-newc
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum -H newc < ../cpio-newc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)
