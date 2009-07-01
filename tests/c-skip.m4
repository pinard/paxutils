# Skipping files while extracting.			-*- shell-script -*-

AT_SETUP(if cpio skips files as per non-options)
dnl      --------------------------------------

PREPARE(mix, list-mix, cpio-newc)
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idm -H newc mix/foo < ../cpio-newc
cd ..
verify -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-match mix/foo
sed -e '/^mix$/d' -e '/^mix\/foo$/d' -e 's,^,unmix/,' list-mix \
| verify -list -non-exist
CPIO_FILTER
], , ,
[NN blocks
])

rm -rf unmix
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idm -H newc -f mix/foo < ../cpio-newc
cd ..
sed -e '/mix\/foo/d' list-mix \
| verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
    -contents-match -mtime-match -ignore-dot-mtime \
| sed -e '/^mix\/foo: extra file$/d'
verify -non-exist unmix/mix/foo
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup unmix)

AT_SETUP(if cpio skips files as per -E file)
dnl      ----------------------------------

PREPARE(mix, list-mix, cpio-newc)
echo mix/foo > pattern
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idmE ../pattern -H newc < ../cpio-newc
cd ..
verify -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-match mix/foo
sed -e '/^mix$/d' -e '/^mix\/foo$/d' -e 's,^,unmix/,' list-mix \
| verify -list -non-exist
CPIO_FILTER
], , ,
[NN blocks
])

rm -rf unmix
mkdir unmix

AT_CHECK(
[cd unmix
  cpio -idmfE ../pattern -H newc < ../cpio-newc
cd ..
sed -e '/^mix\/foo/d' list-mix \
| verify -list -match-dir unmix -mode-match -uid-match -gid-match -size-match \
  -contents-match -mtime-match -ignore-dot-mtime \
| sed -e '/^mix\/foo: extra file$/d'
verify -non-exist unmix/mix/foo
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup pattern unmix)
