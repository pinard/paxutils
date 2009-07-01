#							-*- shell-script -*-

### test byte swapping, -s, -S, -b

AT_SETUP(cpio -o -H newc)
dnl      ---------------

cd structure
rm aptest tmp/aptest2

AT_CHECK(
[PREPARE(structure, struct-list)
cd ..
mkdir src2 dst2 dst2swap
cd src2
echo 'abc' > abc
echo 'abcdefg' > file1
echo 'abcdefgh' > file3
echo '123456789012345' > file2
echo '1234567890123456' > file4
echo '12345678901234567' > file5
find . -depth -print > 2.find
find . -type f -depth -print > 2.find.files
cat 2.find | cpio -o -H newc > 2.c.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idu -H newc < 2.c.gcpio)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2swap/*
cd dst2swap
cpio -idu -H newc < 2.c.gcpio
verify -list -match-dir src2 -size-match -contents-match < 2.find
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -ids -H newc < 2.c.gcpio)
dnl      -----------------------------

for i in `cat 2.find.files`; do
  swapb $i > $i.swapb
  rm $i
  mv $i.swapb $i
done

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -ids -H newc < 2.c.gcpio
verify -list -match-dir dst2swap -size-match -contents-match -mtime-gt $TIME < 2.find
CPIO_FILTER
], 0,
[.*cannot swap bytes of file3: odd number of bytes
.*cannot swap bytes of file4: odd number of bytes
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idu -H newc < 2.c.gcpio)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2swap/*
cd dst2swap
cpio -idu -H newc < 2.c.gcpio
verify -list -match-dir src2 -size-match -contents-match < 2.find
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idS -H newc < 2.c.gcpio)
dnl      -----------------------------

for i in `cat 2.find.files`; do
  swaphw $i > $i.swaphw
  rm $i
  mv $i.swaphw $i
done

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -idS -H newc < 2.c.gcpio
verify -list -match-dir dst2swap -size-match -contents-match -mtime-gt $TIME < 2.find
CPIO_FILTER
], 0,
[.*cannot swap halfwords of file3: odd number of halfwords
.*cannot swap halfwords of file4: odd number of halfwords
.*cannot swap halfwords of file5: odd number of halfwords
NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idu -H newc < 2.c.gcpio)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2swap/*
cd dst2swap
cpio -idu -H newc < 2.c.gcpio
verify -list -match-dir src2 -size-match -contents-match < 2.find
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idsS -H newc < 2.c.gcpio)
dnl      ------------------------------

for i in `cat 2.find.files`; do
  swapb $i > $i.swapb
  rm $i
  mv $i.swapb $i
done
for i in `cat 2.find.files`; do
  swaphw $i > $i.swaphw
  rm $i
  mv $i.swaphw $i
done

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -idsS -H newc < 2.c.gcpio
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

AT_SETUP(cpio -idb -H newc < 2.c.gcpio)
dnl      -----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst2/*
cd dst2
TIME=`echotime`
sleep 1
cpio -idb -H newc < 2.c.gcpio
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
