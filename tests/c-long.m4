# Long files names in cpio.				-*- shell-script -*-

AT_SETUP(cpio usual formats with long file names)
dnl      ---------------------------------------

PREPARE(lmix, list-lmix)
mkdir unmix

AT_CHECK(
[cat list-lmix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idm -H newc < ../archive
cd ..
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-match -ignore-dot-mtime < list-lmix
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix)

AT_SETUP(cpio ustar format with long file names)
dnl      --------------------------------------

PREPARE(lmix, list-lmix)
mkdir unmix

AT_CHECK(
[cat list-lmix | cpio -oH ustar > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idH ustar < ../archive
cd ..
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-le $TIME < list-lmix
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix)

AT_SETUP(cpio tar format with long file names)
dnl      -------------------------------------

PREPARE(lmix, lmix-one)

AT_CHECK(
[cat lmix-one | cpio -oH tar > archive
sed -e 's,^,cpio: ,' -e 's,$,: file name too long,' < lmix-one > experr
echo 'NN blocks' >> experr
CPIO_FILTER
], , , experr)

AT_CLEANUP($cleanup archive)

AT_SETUP(cpio new portable format with longer file names)
dnl      -----------------------------------------------

PREPARE(mmix, list-mmix)
mkdir unmix

AT_CHECK(
[cat list-mmix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idum -H newc < ../archive
cd ..
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-match -ignore-dot-mtime < list-mmix
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix)

dnl FIXME!
dnl AT_SETUP(cpio ustar format with longer file names)
dnl dnl      ----------------------------------------
dnl
dnl PREPARE(mmix, list-mmix)
dnl
dnl AT_CHECK(
dnl [cat list-mmix | cpio -oH ustar > archive
dnl sed -e 's,^,cpio: ,' -e 's,$,: file name too long,' < list-mmix > experr
dnl echo 'NN blocks' >> experr
dnl CPIO_FILTER
dnl ], , , experr)
dnl
dnl AT_CLEANUP($cleanup archive)
