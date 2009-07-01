#							-*- shell-script -*-
# CRC cpio format.

AT_SETUP(if CRC cpio creation and listing works)
dnl      --------------------------------------

PREPARE(mix, list-mix)
sed -e '/^\.\/..*$/s/^\.\///' list-mix | sort > expout

AT_CHECK(
[cat list-mix | cpio -oH crc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -tH crc < archive | sort
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -tvH crc < archive | sed -e 's,^.* \(mix[[a-z0-9/]*]\).*,\1,' | sort
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CLEANUP($cleanup archive)

AT_SETUP(if CRC cpio extraction works)
dnl      ----------------------------

PREPARE(mix, list-mix, cpio-crc)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idH crc < ../cpio-crc
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumH crc < ../cpio-crc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum < ../cpio-crc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)
