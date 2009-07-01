#							-*- shell-script -*-

AT_SETUP(binary format with corruption)
dnl      -----------------------------

PREPARE(structure, struct-list, archive, arch-oldc, arch-newc)
mkdir unpacked

AT_CHECK(
[cd unpacked
  cat minijunk ../archive | cpio -idm || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[cd unpacked
  cat minijunk ../arch-oldc | cpio -idum -c || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[cd unpacked
  cat minijunk ../arch-oldc | cpio -idum || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[cd unpacked
  cat minijunk ../arch-newc | cpio -idum -H newc || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CHECK(
[cd unpacked
  cat minijunk ../arch-oldc | cpio -idum || exit 1
cd ..
VERIFY_EXACT
CPIO_FILTER
], , ,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -H newc < arch-oldc.cor2)
dnl      -----------------------------------

cat maxijunk arch-oldc > arch-oldc.cor2

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -H newc < arch-oldc.cor2
VERIFY_EXACT
CPIO_FILTER
], 0,
[.*warning: skipped 1248 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum < arch-oldc.cor2)
dnl      ---------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum < arch-oldc.cor2
VERIFY_EXACT
CPIO_FILTER
], 0,
[.*warning: skipped 1248 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

### test -H crc with corruption

AT_SETUP(cpio -oH crc)
dnl      ------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -oH crc > arch-crc
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idumH crc < arch-crc-cor)
dnl      ------------------------------

cat minijunk arch-crc > arch-crc-cor

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idumH crc < arch-crc-cor
VERIFY_EXACT
CPIO_FILTER
], 0,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum < arch-crc-cor)
dnl      -------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum < arch-crc-cor
VERIFY_EXACT
CPIO_FILTER
], 0,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

### test -H tar with minor corruption

AT_SETUP(cpio -oH tar)
dnl      ------------

if test "$FIFOS" = yes; then

  AT_CHECK(
  [cd structure
  cat struct-list | cpio -oH tar > arch-tar
  #cat << EOF > expout
  #NN blocks
  #EOF
], 0,
  [.* dev/pipe2 not dumped: not a regular file
  .* pipe not dumped: not a regular file
  .* copy not dumped: not a regular file
  .* diff not dumped: not a regular file
  NN blocks
])

else

  AT_CHECK(
  [cd structure
  cat struct-list | cpio -oH tar > arch-tar
  #cat << EOF > expout
  #NN blocks
  #EOF
], 0,
  [NN blocks
])

fi

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH tar < arch-tar-cor)
dnl      ----------------------------

cat minijunk arch-tar > arch-tar-cor

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -idH tar < arch-tar-cor
cat struct-list | fgrep -v -x -f missing1 | \
verify -list -match-dir unpacked -mode-match -uid-match -gid-match -size-match -contents-match -mtime-lt $TIME | fgrep -v -x -f missing2
CPIO_FILTER
], 0,
[.*invalid header: checksum error
.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id < arch-tar-cor)
dnl      -----------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id < arch-tar-cor
cat struct-list | fgrep -v -x -f missing1 | \
verify -list -match-dir unpacked -mode-match -uid-match -gid-match -size-match -contents-match -mtime-lt $TIME | fgrep -v -x -f $MISSING2
CPIO_FILTER
], 0,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

### test -H ustar with minor corruption

AT_SETUP(cpio -oH ustar)
dnl      --------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -oH ustar > arch-ustar
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH ustar < arch-ustar-cor)
dnl      --------------------------------

cat minijunk arch-ustar > arch-ustar-cor
cat maxijunk arch-ustar > arch-ustar-cor2

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -idH ustar < arch-ustar-cor
VERIFY_NEWER
CPIO_FILTER
], 0,
[.*invalid header: checksum error
.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id < arch-ustar-cor)
dnl      -------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id < arch-ustar-cor
VERIFY_NEWER
CPIO_FILTER
], 0,
[.*warning: skipped 5 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH ustar < arch-ustar-cor2)
dnl      --------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -idH ustar < arch-ustar-cor2
VERIFY_NEWER
CPIO_FILTER
], 0,
[.*invalid header: checksum error
.*warning: skipped 1248 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id < arch-ustar-cor2)
dnl      -------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id < arch-ustar-cor2
VERIFY_NEWER
CPIO_FILTER
], 0,
[.*warning: skipped 1248 bytes of junk
NN blocks
])

AT_CLEANUP($cleanup)
