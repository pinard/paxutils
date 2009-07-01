#							-*- shell-script -*-

AT_SETUP(copy-pass mode)
dnl      --------------

PREPARE(structure, struct-list)

AT_CHECK(
[TIME=`echotime`
sleep 1
cd structure
  find . -depth -print | cpio -pd ../unpacked || exit 1
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd structure
  find . -depth -print | cpio -pdum ../unpacked || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd structure
  # Probably should verify linked message.
  find . -depth -print | cpio -pdumv ../unpacked 2>&1 | grep -v 'cpio.*linked to' 1>&2
cd ..
VERIFY_EXACT
cp struct-list experr
echo 'NN blocks' >> experr
CPIO_FILTER
], , , experr)

AT_CLEANUP($cleanup unpacked)

AT_SETUP(copy-pass mode through linking)
dnl      -----------------------------

PREPARE(structure, struct-list)

AT_CHECK(
[cd structure
  find . -depth -print | cpio -pdl ../unpacked || exit 1
cd ..
verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match -ino-match -ignore-dir-ino -ignore-link-ino -ignore-link-mtime -ignore-dir-mtime < struct-list
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup unpacked)
