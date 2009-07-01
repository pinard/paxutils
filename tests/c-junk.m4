# Corrupted cpio files.					-*- shell-script -*-

AT_SETUP(cpio usual formats with small corruption)
dnl      ----------------------------------------

PREPARE(mix, list-mix, minijunk, cpio-def, cpio-oldc, cpio-newc)
mkdir unmix

AT_CHECK(
[cd unmix
  cat ../minijunk ../cpio-def | cpio -idm || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[cpio: warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[cd unmix
  cat ../minijunk ../cpio-oldc | cpio -idum -c || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[cpio: warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[cd unmix
  cat ../minijunk ../cpio-oldc | cpio -idum || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[cpio: warning: skipped 5 bytes of junk
NN blocks
])

dnl FIXME!
dnl AT_CHECK(
dnl [cd unmix
dnl   cat ../minijunk ../cpio-newc | cpio -idum -H newc || exit 1
dnl cd ..
dnl VERIFY_EXACT
dnl CPIO_FILTER
dnl ], , ,
dnl [cpio: warning: skipped 5 bytes of junk
dnl NN blocks
dnl ])

AT_CHECK(
[cd unmix
  cat ../minijunk ../cpio-oldc | cpio -idum || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[cpio: warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup unmix)

dnl AT_SETUP(cpio usual formats with bigger corruption)
dnl dnl      -----------------------------------------
dnl
dnl PREPARE(mix, list-mix, maxijunk, cpio-oldc)
dnl mkdir unmix
dnl
dnl FIXME!
dnl AT_CHECK(
dnl [cd unmix
dnl   cat ../maxijunk ../cpio-oldc | cpio -idm -H newc || exit 1
dnl cd ..
dnl VERIFY_EXACT
dnl CPIO_FILTER
dnl ], , ,
dnl [cpio: warning: skipped 1248 bytes of junk
dnl NN blocks
dnl ])
dnl
dnl FIXME!
dnl AT_CHECK(
dnl [cd unmix
dnl   cat ../maxijunk ../cpio-oldc | cpio -idum || exit 1
dnl cd ..
dnl VERIFY_EXACT
dnl CPIO_FILTER
dnl ], , ,
dnl [cpio: warning: skipped 1248 bytes of junk
dnl NN blocks
dnl ])
dnl
dnl AT_CLEANUP($cleanup unmix)

dnl AT_SETUP(cpio CRC format with corruption)
dnl dnl      -------------------------------
dnl
dnl PREPARE(mix, list-mix, cpio-crc)
dnl mkdir unmix
dnl
dnl FIXME!
dnl AT_CHECK(
dnl [cd unmix
dnl   cat ../minijunk ../cpio-crc | cpio -idmH crc || exit 1
dnl cd ..
dnl VERIFY_EXACT
dnl CPIO_FILTER
dnl ], , ,
dnl [cpio: warning: skipped 5 bytes of junk
dnl NN blocks
dnl ])
dnl
dnl FIXME!
dnl AT_CHECK(
dnl [cd unmix
dnl   cat ../minijunk ../cpio-crc | cpio -idum || exit 1
dnl VERIFY_EXACT
dnl CPIO_FILTER
dnl ], , ,
dnl [cpio: warning: skipped 5 bytes of junk
dnl NN blocks
dnl ])
dnl
dnl AT_CLEANUP($cleanup unmix)

AT_SETUP(cpio tar format with corruption)
dnl      -------------------------------

PREPARE(mix, list-mix, cpio-tar)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cat ../minijunk ../cpio-tar | cpio -idH tar || exit 1
cd ..
dnl cat list-mix | fgrep -v -x -f extra1 | \
dnl verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match -contents-match -mtime-le $TIME | fgrep -v -x -f extra2
VERIFY_NEWER_FILTERED
CPIO_FILTER
], , ,
[cpio: invalid header: checksum error
cpio: warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cat ../minijunk ../cpio-tar | cpio -idu || exit 1
cd ..
dnl cat list-mix | fgrep -v -x -f extra1 | \
dnl verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match -contents-match -mtime-le $TIME | fgrep -v -x -f extra2
VERIFY_NEWER_FILTERED
CPIO_FILTER
], , ,
[cpio: warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(cpio ustar format with corruption)
dnl      ---------------------------------

PREPARE(mix, list-mix, minijunk, maxijunk, cpio-ustar)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cat ../minijunk ../cpio-ustar | cpio -idH ustar || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[cpio: invalid header: checksum error
cpio: warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cat ../minijunk ../cpio-ustar | cpio -idu || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[cpio: warning: skipped 5 bytes of junk
NN blocks
])

rm -rf unmix
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cat ../maxijunk ../cpio-ustar | cpio -idH ustar || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[cpio: invalid header: checksum error
cpio: warning: skipped 1248 bytes of junk
NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cat ../maxijunk ../cpio-ustar | cpio -idu || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[cpio: warning: skipped 1248 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup unmix)
