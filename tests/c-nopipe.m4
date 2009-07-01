#							-*- shell-script -*-

### Test -I,-O and -F

AT_SETUP(cpio -idumcI 1.c.gcpio -H newc)
dnl      ---------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumI 1.c.gcpio -H newc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oO 1.c.gcpio -H newc)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
rm -f 1.c.gcpio
cat struct-list | cpio -oO 1.c.gcpio -H newc
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -H newc < 1.c.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -H newc < 1.c.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumcI 1.c.gcpio -H newc)
dnl      ---------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumI 1.c.gcpio -H newc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumcF 1.c.gcpio -H newc)
dnl      ---------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumF 1.c.gcpio -H newc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -oF 1.c.gcpio -H newc)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
rm -f 1.c.gcpio
cat struct-list | cpio -oF 1.c.gcpio -H newc
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumF 1.c.gcpio -H newc)
dnl      --------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumF 1.c.gcpio -H newc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumcI 1.c.gcpio -H newc)
dnl      ---------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumI 1.c.gcpio -H newc
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
