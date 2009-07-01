# cpio appends.						-*- shell-script -*-

AT_SETUP(if cpio can append using default format)
dnl      ---------------------------------------

PREPARE(mix, list-mix, cpio-def)
echo mix/aptest > aplist
echo mix/tmp/aptest2 >> aplist
cp cpio-def archive
mv mix mix-save
mkdir unmix

AT_CHECK(
[cpio -idm < cpio-def
echo append > mix/aptest
echo append2 > mix/tmp/aptest2
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -oAO archive < aplist
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id < ../archive
cd ..
VERIFY_NEWER
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-le $TIME < aplist
CPIO_FILTER
], , ,
[NN blocks
])

rm -rf mix
mv mix-save mix

AT_CLEANUP($cleanup aplist archive mix-save unmix)

AT_SETUP(if cpio can append using new portable format)
dnl      --------------------------------------------

PREPARE(mix, list-mix, cpio-newc)
echo mix/aptest > aplist
echo mix/tmp/aptest2 >> aplist
cp cpio-newc archive
mv mix mix-save
mkdir unmix

AT_CHECK(
[cpio -idm < cpio-newc
echo append > mix/aptest
echo append2 > mix/tmp/aptest2
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -oAO archive -H newc < aplist
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -id -H newc < ../archive
cd ..
VERIFY_NEWER
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-le $TIME < aplist
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumI ../archive -H newc
cd ..
VERIFY_NEWER
dnl verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match < list-mix
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-match < aplist
CPIO_FILTER
], , ,
[NN blocks
])

rm -rf mix
mv mix-save mix

AT_CLEANUP($cleanup aplist archive mix-save unmix)
