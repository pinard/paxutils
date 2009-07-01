# Swapping options.					-*- shell-script -*-

AT_SETUP(if cpio byte swapping works)
dnl      ---------------------------

PREPARE(smix, list-smix, listf-smix)
mkdir unmix xmix

AT_CHECK(
[cat list-smix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd xmix
  cpio -id -H newc < ../archive
cd ..
verify -list -match-dir xmix -size-match -contents-match < list-smix
CPIO_FILTER
], , ,
[NN blocks
])

cd xmix
  while read file; do
    swapb $file > temp
    rm $file
    mv temp $file
  done < ../listf-smix
cd ..

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -ids -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../unmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap bytes of smix/file3: odd number of bytes
cpio: cannot swap bytes of smix/file4: odd number of bytes
NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idu -H newc < ../archive
cd ..
verify -list -match-dir unmix -size-match -contents-match < list-smix
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix xmix)

AT_SETUP(if cpio halfword swapping works)
dnl      -------------------------------

PREPARE(smix, list-smix, listf-smix)
mkdir unmix xmix

AT_CHECK(
[cat list-smix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd xmix
  cpio -id -H newc < ../archive
cd ..
verify -list -match-dir xmix -size-match -contents-match < list-smix
CPIO_FILTER
], , ,
[NN blocks
])

cd xmix
  while read file; do
    swaphw $file > temp
    rm $file
    mv temp $file
  done < ../listf-smix
cd ..

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idS -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../unmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap halfwords of smix/file3: odd number of halfwords
cpio: cannot swap halfwords of smix/file4: odd number of halfwords
cpio: cannot swap halfwords of smix/file5: odd number of halfwords
NN blocks
])

AT_CHECK(
[cd unmix
  cpio -idu -H newc < ../archive
cd ..
verify -list -match-dir unmix -size-match -contents-match < list-smix
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup archive unmix xmix)

AT_SETUP(if cpio combined swapping works)
dnl      -------------------------------

PREPARE(smix, list-smix, listf-smix)
mkdir unmix xmix

AT_CHECK(
[cat list-smix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd xmix
  cpio -id -H newc < ../archive
cd ..
verify -list -match-dir xmix -size-match -contents-match < list-smix
CPIO_FILTER
], , ,
[NN blocks
])

cd xmix
  while read file; do
    swapb $file > temp
    rm $file
    swaphw temp > $file
    rm temp
  done < ../listf-smix
cd ..

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idsS -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../unmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap halfwords of smix/file3: odd number of halfwords
cpio: cannot swap bytes of smix/file3: odd number of bytes
cpio: cannot swap halfwords of smix/file4: odd number of halfwords
cpio: cannot swap bytes of smix/file4: odd number of bytes
cpio: cannot swap halfwords of smix/file5: odd number of halfwords
NN blocks
])

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idub -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../unmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap halfwords of smix/file3: odd number of halfwords
cpio: cannot swap bytes of smix/file3: odd number of bytes
cpio: cannot swap halfwords of smix/file4: odd number of halfwords
cpio: cannot swap bytes of smix/file4: odd number of bytes
cpio: cannot swap halfwords of smix/file5: odd number of halfwords
NN blocks
])

AT_CLEANUP($cleanup archive unmix xmix)

AT_SETUP(cpio swapping with weird block sizes)
dnl      ------------------------------------

PREPARE(smix, list-smix)
mkdir unmix xmix

AT_CHECK(
[cat list-smix | cpio -o -H newc > archive
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cd xmix
  cpio -id -H newc < ../archive
cd ..
verify -list -match-dir xmix -size-match -contents-match < list-smix
CPIO_FILTER
], , ,
[NN blocks
])

cd xmix
  while read file; do
    swapb $file > temp
    rm $file
    swaphw temp > $file
    rm temp
  done < ../listf-smix
cd ..

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idbC 7 -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../unmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap halfwords of smix/file3: odd number of halfwords
cpio: cannot swap bytes of smix/file3: odd number of bytes
cpio: cannot swap halfwords of smix/file4: odd number of halfwords
cpio: cannot swap bytes of smix/file4: odd number of bytes
cpio: cannot swap halfwords of smix/file5: odd number of halfwords
NN blocks
])

rm -rf unmix
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idbC 3 -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../unmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap halfwords of smix/file3: odd number of halfwords
cpio: cannot swap bytes of smix/file3: odd number of bytes
cpio: cannot swap halfwords of smix/file4: odd number of halfwords
cpio: cannot swap bytes of smix/file4: odd number of bytes
cpio: cannot swap halfwords of smix/file5: odd number of halfwords
NN blocks
])

rm -rf unmix
mkdir unmix

AT_CHECK(
[TIME=`echotime`
cd unmix
  cpio -idbC 1 -H newc < ../archive
cd ..
cd xmix
  verify -list -match-dir ../xmix -size-match -contents-match -mtime-le $TIME \
    < ../list-smix
cd ..
CPIO_FILTER
], , ,
[cpio: cannot swap halfwords of smix/file3: odd number of halfwords
cpio: cannot swap bytes of smix/file3: odd number of bytes
cpio: cannot swap halfwords of smix/file4: odd number of halfwords
cpio: cannot swap bytes of smix/file4: odd number of bytes
cpio: cannot swap halfwords of smix/file5: odd number of halfwords
NN blocks
])

AT_CLEANUP($cleanup archive unmix xmix)
