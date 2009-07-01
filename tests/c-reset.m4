#							-*- shell-script -*-

AT_SETUP(if access time could be reset)
dnl      -----------------------------

# First copy the directory into unmix so we can make sure that
# the -a doesn't do something weird to the mtime (like it has on a
# weird 88k SysV machine that has a non-standard utimbuf mix).

PREPARE(mix, list-mix, cpio-oldc)
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idm -c < ../cpio-oldc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cat list-mix | cpio -oa -c > cpio-oldc
VERIFY_EXACT
verify -list -ignore-dir-atime -atime-le $TIME < list-mix
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum -c < ../cpio-oldc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)
