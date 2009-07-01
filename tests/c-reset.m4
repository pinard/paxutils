#							-*- shell-script -*-

### test -a

# first copy the directory into unpacked so we can make sure that the -a doesn't
# do something weird to the mtime (like it has on a weird 88k SysV machine
# that has a non-standard utimbuf structure).

AT_SETUP(cpio -o -c)
dnl      ----------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -o -c > arch-oldc
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -c < arch-oldc)
dnl      ---------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -c < arch-oldc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oa -c)
dnl      -----------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
TIME=`echotime`
sleep 1
cat struct-list | cpio -oa -c > arch-oldc
VERIFY_EXACT
verify -list -ignore-dir-atime -atime-lt $TIME < struct-list
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -c < arch-oldc)
dnl      ---------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -c < arch-oldc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
