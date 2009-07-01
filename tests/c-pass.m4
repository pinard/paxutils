#							-*- shell-script -*-

AT_SETUP(copy-pass mode)
dnl      --------------

PREPARE(mix, list-mix)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
find mix -depth -print | cpio -pd unmix || exit 1
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[find mix -depth -print | cpio -pdum unmix || exit 1
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[# Probably should verify linked message.
find mix -depth -print | cpio -pdumv unmix 2>&1 | grep -v 'cpio.*linked to' 1>&2
VERIFY_EXACT
cp list-mix experr
echo 'NN blocks' >> experr
CPIO_FILTER
], , , experr)

AT_CLEANUP($cleanup unmix)

AT_SETUP(copy-pass mode through linking)
dnl      -----------------------------

PREPARE(mix, list-mix)
mkdir unmix

AT_CHECK(
[find mix -depth -print | cpio -pdl unmix || exit 1
verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match -ino-match -ignore-dir-ino -ignore-link-ino -ignore-link-mtime -ignore-dir-mtime < list-mix
CPIO_FILTER
], , ,
[0 blocks
])

AT_CLEANUP($cleanup unmix)
