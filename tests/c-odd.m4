#							-*- shell-script -*-

### test byte swapping with weird block sizes

AT_SETUP(cpio -idbC 7 -H newc < 2.c.gcpio)
dnl      --------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -idbC 7 -H newc < 2.c.gcpio
verify -list -match-dir dst2swap -size-match -contents-match -mtime-gt $TIME < 2.find
CPIO_FILTER
], 0,
[.*cannot swap halfwords of file3: odd number of halfwords
.*cannot swap bytes of file3: odd number of bytes
.*cannot swap halfwords of file4: odd number of halfwords
.*cannot swap bytes of file4: odd number of bytes
.*cannot swap halfwords of file5: odd number of halfwords
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idbC 3 -H newc < 2.c.gcpio)
dnl      --------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -idbC 3 -H newc < 2.c.gcpio
verify -list -match-dir dst2swap -size-match -contents-match -mtime-gt $TIME < 2.find
CPIO_FILTER
], 0,
[.*cannot swap halfwords of file3: odd number of halfwords
.*cannot swap bytes of file3: odd number of bytes
.*cannot swap halfwords of file4: odd number of halfwords
.*cannot swap bytes of file4: odd number of bytes
.*cannot swap halfwords of file5: odd number of halfwords
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idbC 1 -H newc < 2.c.gcpio)
dnl      --------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -idbC 1 -H newc < 2.c.gcpio
verify -list -match-dir dst2swap -size-match -contents-match -mtime-gt $TIME < 2.find
[.*cannot swap halfwords of file3: odd number of halfwords
.*cannot swap bytes of file3: odd number of bytes
.*cannot swap halfwords of file4: odd number of halfwords
.*cannot swap bytes of file4: odd number of bytes
.*cannot swap halfwords of file5: odd number of halfwords
CPIO_FILTER
], 0,
NN blocks
])

AT_CLEANUP($cleanup)
