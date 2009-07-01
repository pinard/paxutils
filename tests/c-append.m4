#							-*- shell-script -*-

### test -A flag

AT_SETUP(cpio -oO 1.-A.gcpio)
dnl      ----------------------

cd structure
echo append > aptest
echo append2 > tmp/aptest2

AT_CHECK(
[PREPARE(structure, struct-list)
rm -f 1.-A.gcpio
cat struct-list | cpio -oO 1.-A.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oAO 1.-A.gcpio)
dnl      -----------------------

AT_CHECK(
[PREPARE(structure, struct-list)
{ echo aptest; echo tmp/aptest2; } | cpio -oAO 1.-A.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id < 1.-A.gcpio)
dnl      ---------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id < 1.-A.gcpio
VERIFY_NEWER
{ echo aptest; echo tmp/aptest2; } | verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-gt $TIME
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oO 1.c-A.gcpio -H newc)
dnl      -------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
rm -f 1.c-A.gcpio
cat struct-list | cpio -oO 1.c-A.gcpio -H newc
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oAO 1.c-A.gcpio -H newc)
dnl      --------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
{ echo aptest; echo tmp/aptest2; } | cpio -oAO 1.c-A.gcpio -H newc
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id -H newc < 1.c-A.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id -H newc < 1.c-A.gcpio
VERIFY_NEWER
{ echo aptest; echo tmp/aptest2; } | verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-gt $TIME
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumI 1.c-A.gcpio -H newc)
dnl      ----------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumI 1.c-A.gcpio -H newc
verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match < struct-list
{ echo aptest; echo tmp/aptest2; } | verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match
CPIO_FILTER
], 0,
[NN blocks
],
[\./tmp mtime match [0-9]* should be [0-9]*\.
\. mtime match [0-9]* should be [0-9]*\.
])

AT_CLEANUP($cleanup)
