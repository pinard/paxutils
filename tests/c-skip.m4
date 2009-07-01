#							-*- shell-script -*-

### Test skipping files and -E

AT_SETUP(cpio -idum -H newc foo < 1.c.gcpio)
dnl      -------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -H newc foo < 1.c.gcpio
verify -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match foo
sed -e '/^\.\/foo$/d' -e '/^\.\/*$/d' struct-list | verify -list -non-exist
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -H newc -f foo < 1.c.gcpio)
dnl      ----------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -H newc -f foo < 1.c.gcpio
sed -e '/\.\/foo/d' struct-list | \
  verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match -ignore-dot-mtime | \
  sed -e '/^\.\/foo: missing file\.$/d'
verify -non-exist foo
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumE 1.pat-list -H newc < 1.c.gcpio)
dnl      ------------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
echo foo > 1.pat-list
cpio -idumE 1.pat-list -H newc < 1.c.gcpio
verify -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match foo
sed -e '/^\.\/foo$/d' -e '/^\.\/*$/d' struct-list | verify -list -non-exist
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumfE 1.pat-list -H newc < 1.c.gcpio)
dnl      -------------------------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
echo foo > 1.pat-list
cpio -idumfE 1.pat-list -H newc < 1.c.gcpio
sed -e '/\.\/foo/d' struct-list | \
  verify -list -match-dir structure -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match -ignore-dot-mtime | \
  sed -e '/^\.\/foo: missing file\.$/d'
verify -non-exist foo
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
