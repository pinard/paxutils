#							-*- shell-script -*-

### test ownership

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

AT_SETUP(cpio -idumH crc -R $UIDNAME.$GIDNAME < 1.crc.gcpio)
dnl      --------------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc -R $UIDNAME.$GIDNAME < 1.crc.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc -R . < 1.crc.gcpio)
dnl      ----------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc --no-preserve-owner < 1.crc.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc -R $XUID.$GID < 1.crc.gcpio)
dnl      -------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc -R $XUID.$GID < 1.crc.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc --no-preserve-owner < 1.crc.gcpio)
dnl      -------------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc --no-preserve-owner < 1.crc.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc -R $XUID.$GID2 < 1.crc.gcpio)
dnl      --------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc -R $XUID.$GID2 < 1.crc.gcpio
verify -list -match-dir structure -mode-match -uid-match -gid $GID2 -size-match -contents-match -mtime-match -ignore-dot-mtime < struct-list | \
	grep -v '^\. gid [0-9][0-9]* should be [0-9][0-9]*\.$'
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc -R $UIDNAME.$GID2NAME < 1.crc.gcpio)
dnl      ---------------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc -R $UIDNAME.$GID2NAME < 1.crc.gcpio
verify -list -match-dir structure -mode-match -uid-match -gid $GID2 -size-match -contents-match -mtime-match -ignore-dot-mtime < struct-list | \
grep -v '^\. gid [0-9][0-9]* should be [0-9][0-9]*\.$'
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc --owner $UIDNAME.$GID2NAME < 1.crc.gcpio)
dnl      --------------------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc --owner $UIDNAME.$GID2NAME < 1.crc.gcpio
verify -list -match-dir structure -mode-match -uid-match -gid $GID2 -size-match -contents-match -mtime-match -ignore-dot-mtime < struct-list | \
	grep -v '^\. gid [0-9][0-9]* should be [0-9][0-9]*\.$'
AFTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
