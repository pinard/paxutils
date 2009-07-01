#							-*- shell-script -*-

AT_SETUP(if diagnostic is suppressed when directory already exists)
dnl      ---------------------------------------------------------

AT_CHECK(
[set -e
mkdir directory
touch directory/file
tar cf archive directory
tar xf archive
set +e
])

AT_CLEANUP(archive directory)

AT_SETUP(if symlinks can be extracted over existing files)
dnl      ------------------------------------------------

# Skip test when file links are not available, like on DOSish systems.
test -z "[$]COMSPEC" || exit 77
test -z "[$]ComSpec" || exit 77

AT_CHECK(
[set -e
touch file
ln -s file link 2> /dev/null || exit 77
tar cf archive link
rm link
touch link
tar xf archive
set +e
])

AT_CLEANUP(archive file link)

AT_SETUP(if paths going up and down avoid extraction loops)
dnl      -------------------------------------------------

AT_CHECK(
[mkdir directory
tar cfv archive directory/../directory
], 0,
[directory/../directory/
])

AT_CHECK(tar xfv archive, 0,
[directory/../directory/
])

AT_CLEANUP(archive directory)

AT_SETUP(that intermediate directories does not give protection errors)
dnl      -------------------------------------------------------------

# While creating intermediate directories, there was a protection error
# while attemping to create short prefix directories owned by root.

mkdir directory
echo x > directory/file
AT_CHECK(tar cfP archive `pwd`/directory/file, 0)

rm -rf directory
AT_CHECK(tar xfP archive `pwd`/directory/file, 0)

AT_CLEANUP(archive directory)

AT_SETUP(if --recursive-unlink works as expected)
dnl      ---------------------------------------

AT_CHECK(
[set -e
echo OK > contents
tar cf archive contents
rm contents
mkdir contents
touch contents/file
tar xf archive --recursive-unlink
cat contents
set +e
], ,
[OK
])

AT_CLEANUP(archive contents)
