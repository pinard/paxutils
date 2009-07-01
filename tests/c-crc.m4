#							-*- shell-script -*-

### test -H crc format

AT_SETUP(cpio -oH crc)
dnl      ------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -oH crc > 1.crc.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -tH crc < 1.crc.gcpio)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -tH crc < 1.crc.gcpio | sort +8
sed -e '/^\.\/..*$/s/^\.\///' struct-list | sort > expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)

AT_SETUP(cpio -tvH crc < 1.crc.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -tvH crc < 1.crc.gcpio | sort +8
sed -e '/^\.\/..*$/s/^\.\///' -e 's/^/.*/' -e s'/$/.*/' struct-list | sort > expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH crc < 1.crc.gcpio)
dnl      ---------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -idH crc < 1.crc.gcpio
VERIFY_NEWER
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc < 1.crc.gcpio)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc < 1.crc.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum < 1.crc.gcpio)
dnl      ------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum < 1.crc.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
