#							-*- shell-script -*-

AT_SETUP(if tar creation works)
dnl      ---------------------
# This is in preparation for later testing if `cpio' can read `tar' files.

PREPARE(mix)

AT_CHECK([tar cf archive mix], 0)

AT_CLEANUP($cleanup archive)

AT_SETUP(if tar format cpio creation works)
dnl      ---------------------------------

PREPARE(mix, list-mix)

if test "$FIFOS" = yes; then

AT_CHECK(
[cat list-mix | cpio -oH tar > archive
CPIO_FILTER
], , ,
[cpio: mix/dev/pipe2 not dumped: not a regular file
cpio: mix/pipe not dumped: not a regular file
cpio: mix/copy not dumped: not a regular file
cpio: mix/diff not dumped: not a regular file
NN blocks
])

else

AT_CHECK(
[cat list-mix | cpio -oH tar > archive
CPIO_FILTER
], , ,
[NN blocks
])

fi

AT_CLEANUP($cleanup archive)

AT_SETUP(if ustar format cpio creation works)
dnl      -----------------------------------

PREPARE(mix, list-mix)

AT_CHECK(
[cat list-mix | cpio -oH ustar > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive)

AT_SETUP(if cpio can unpack installed tar archives)
dnl      -----------------------------------------

test -n "$BINTAR" || exit 77

PREPARE(mix, list-mix, itar-def)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idH tar < ../itar-def
cd ..
dnl cat list-mix | fgrep -v -x -f extra1 | \
dnl verify -list -match-dir mix -mode-match -uid-match -gid-match -size-match -contents-match -mtime-ge $TIME | fgrep -v -x -f extra2
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(if cpio can unpack tar archives)
dnl      -------------------------------

PREPARE(mix, list-mix, tar-def)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idH ustar < ../tar-def
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(if cpio can unpack tar format)
dnl      -----------------------------

PREPARE(mix, list-mix, cpio-tar)
mkdir unmix

AT_CHECK(
[PREPARE(mix, list-mix, cpio-tar)
TIME=`echotime`
cd unmix
  cpio -idH tar < ../cpio-tar
cd ..
VERIFY_NEWER_FILTERED
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idu < ../cpio-tar
cd ..
VERIFY_NEWER_FILTERED
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(if cpio can unpack ustar format)
dnl      -------------------------------

PREPARE(mix, list-mix, cpio-ustar)
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idH ustar < ../cpio-ustar
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idu < ../cpio-ustar
cd ..
VERIFY_NEWER
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)
