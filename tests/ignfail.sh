#! /bin/sh
# Unreadable directories yielded error despite --ignore-failed-read.

. ./preset

## Skip test when root.  These few lines (which warranted a few emails! :-)
## are kept here, but commented, as a reference for possible later use.
#
#touch file
#set - x`ls -l file`
#if test $3 = root; then
#  rm file
#  exit 77
#else
#  rm file
#fi

# Skip test when directory permissions are not enforced, like when
# on AFS filesystems, or DOSish systems.  It also takes care of
# the root case, so the check above does not need to be done anymore.

mkdir directory
chmod 000 directory
if touch 2>/dev/null directory/file; then
  rm -rf directory
  exit 77
else
  chmod 700 directory
  rm -rf directory
fi

. $srcdir/before

touch file
mkdir directory
touch directory/file

echo 1>&2 -----
chmod 000 file
tar cf archive file
status=$?
chmod 600 file
test $status = 2 || exit 1

echo 1>&2 -----
chmod 000 file
tar cf archive --ignore-failed-read file || exit 1
status=$?
chmod 600 file
test $status = 0 || exit 1

echo 1>&2 -----
chmod 000 directory
tar cf archive directory
status=$?
chmod 700 directory
test $status = 2 || exit 1

echo 1>&2 -----
chmod 000 directory
tar cf archive --ignore-failed-read directory || exit 1
status=$?
chmod 700 directory
test $status = 0 || exit 1

err="\
-----
tar: Cannot add file file: Permission denied
tar: Processed all files possible, despite earlier errors
-----
tar: Cannot add file file: Permission denied
-----
tar: Cannot add directory directory: Permission denied
tar: Processed all files possible, despite earlier errors
-----
tar: Cannot add directory directory: Permission denied
"

. $srcdir/after
