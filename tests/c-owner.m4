# Ownership tests in cpio.				-*- shell-script -*-

AT_SETUP(if cpio can handle current owner/group)
dnl      --------------------------------------

# FIXME: Why? :-(
rm -f cpio-crc

PREPARE(mix, list-mix, cpio-crc)
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idumH crc -R $UIDNAME.$GIDNAME < ../cpio-crc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumH crc --no-preserve-owner < ../cpio-crc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumH crc -R $XUID.$GID < ../cpio-crc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idumH crc --no-preserve-owner < ../cpio-crc
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

dnl FIXME!
dnl AT_SETUP(if cpio can handle different group)
dnl dnl      ----------------------------------
dnl
dnl PREPARE(mix, list-mix, cpio-crc)
dnl mkdir unmix
dnl
dnl AT_CHECK(
dnl [cd unmix
dnl   cpio -idmH crc -R $XUID.$GID2 < ../cpio-crc
dnl cd ..
dnl verify -list -match-dir unmix -mode-match -uid-match -gid $GID2 -size-match \
dnl   -contents-match -mtime-match -ignore-dot-mtime < list-mix \
dnl | grep -v '^\. gid [0-9][0-9]* should be [0-9][0-9]*\.$'
dnl CPIO_FILTER
dnl ], , ,
dnl [NN blocks
dnl ])
dnl
dnl AT_CHECK(
dnl [cd unmix
dnl   cpio -idumH crc -R $UIDNAME.$GID2NAME < ../cpio-crc
dnl cd ..
dnl verify -list -match-dir unmix -mode-match -uid-match -gid $GID2 -size-match \
dnl    -contents-match -mtime-match -ignore-dot-mtime < list-mix \
dnl | grep -v '^\. gid [0-9][0-9]* should be [0-9][0-9]*\.$'
dnl CPIO_FILTER
dnl ], , ,
dnl [NN blocks
dnl ])
dnl
dnl AT_CHECK(
dnl [cd unmix
dnl   cpio -idumH crc --owner $UIDNAME.$GID2NAME < ../cpio-crc
dnl cd ..
dnl verify -list -match-dir unmix -mode-match -uid-match -gid $GID2 -size-match \
dnl    -contents-match -mtime-match -ignore-dot-mtime < list-mix \
dnl | grep -v '^\. gid [0-9][0-9]* should be [0-9][0-9]*\.$'
dnl AFTER
dnl ], , ,
dnl [NN blocks
dnl ])
dnl
dnl AT_CLEANUP($cleanup unmix)
