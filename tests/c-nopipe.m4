#							-*- shell-script -*-
# Options -I, -O and -F.

AT_SETUP(if cpio can read without pipe)
dnl      -----------------------------

PREPARE(mix, list-mix, cpio-newc)
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idmI ../cpio-newc -H newc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumF ../cpio-newc -H newc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(if cpio can write without pipe using -O)
dnl      ---------------------------------------

PREPARE(mix, list-mix)
mkdir unmix

AT_CHECK(
[cat list-mix | cpio -oO archive -H newc
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idm -H newc < ../archive
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumI ../archive -H newc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix)

AT_SETUP(if cpio can write without pipe using -F)
dnl      ---------------------------------------

PREPARE(mix, list-mix)
mkdir unmix

AT_CHECK(
[cat list-mix | cpio -oF archive -H newc
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idmF ../archive -H newc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumI ../archive -H newc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix)
