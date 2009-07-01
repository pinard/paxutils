#							-*- shell-script -*-

AT_SETUP(cpio -idH tar < 1.tar)
dnl      ---------------------

AT_CHECK(
[test -n "$BINTAR" || exit 77
PREPARE(structure, struct-list)
$BINTAR cf 1.tar .
# fixme, should check for bogus errors
TIME=`echotime`
sleep 1
cpio -idH tar < 1.tar
cat struct-list | fgrep -v -x -f missing1 | \
verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-gt $TIME | fgrep -v -x -f missing2
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oH tar)
dnl      ------------

AT_CHECK(
[test "$FIFOS" = yes || exit 77
PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -oH tar > 1.tar.gcpio
CPIO_FILTER
], 0,
[.* dev/pipe2 not dumped: not a regular file
.* pipe not dumped: not a regular file
.* copy not dumped: not a regular file
.* diff not dumped: not a regular file
NN blocks
])

AT_CHECK(
[test "$FIFOS" = yes && exit 77
PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -oH tar > 1.tar.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH tar < 1.tar.gcpio)
dnl      ---------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -idH tar < 1.tar.gcpio
cat struct-list | fgrep -v -x -f missing1 | \
verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-gt $TIME | fgrep -v -x -f missing2
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id < 1.tar.gcpio)
dnl      ----------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id < 1.tar.gcpio
cat struct-list | fgrep -v -x -f missing1 | \
verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-gt $TIME | fgrep -v -x -f missing2
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH ustar < 1.gtar)
dnl      ------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
tar cf 1.gtar .
TIME=`echotime`
sleep 1
cpio -idH ustar < 1.gtar
VERIFY_NEWER
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oH ustar)
dnl      --------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -oH ustar > 1.ustar.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH ustar < 1.ustar.gcpio)
dnl      -------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -idH ustar < 1.ustar.gcpio
VERIFY_NEWER
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id < 1.ustar.gcpio)
dnl      ------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id < 1.ustar.gcpio
VERIFY_NEWER
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
