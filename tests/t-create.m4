#							-*- shell-script -*-

AT_SETUP(that -- does not stop option decoding)
dnl      -------------------------------------
AT_CHECK([
touch file1 file2
tar cvf archive file1 -- file2
], 0,
[file1
file2
])
AT_CLEANUP(archive file1 file2)

AT_SETUP(if an interrupted write gets restarted on POSIX systems)
dnl      -------------------------------------------------------

# Skip test when fork is not available, like on DOSish systems.
# I do not know how to do this right, so just exclude DOSish for now.
# But cleanup does not occur, then!  FIXME!
test -z "[$]COMSPEC" || exit 77
test -z "[$]ComSpec" || exit 77

genfile -l 100000 > file

AT_CHECK(
[( tar cf - file &
  process=[$]!
  sleep 2
  kill -STOP [$]process
  sleep 1
  kill -CONT [$]process
) |
( sleep 5
  cat > /dev/null
)
], 0)

AT_CLEANUP(file)

AT_SETUP(if old archives receive directories)
dnl      -----------------------------------

AT_CHECK(
[set -e
mkdir directory
tar cfvo archive directory
tar tf archive
set +e
], ,
[directory/
directory/
])

AT_CLEANUP(archive directory)

AT_SETUP(if --from-files and --directory may be used together)
dnl      ----------------------------------------------------

AT_CHECK(
[set -e
> file
mkdir directory
cd directory
tar cfv archive -C .. file
tar cfCv archive .. file
echo file | tar cfCTv archive .. -
cd ..
], ,
[file
file
file
])

AT_CLEANUP(directory file)

AT_SETUP(if --ignore-failed-read handles unreadable directories)
dnl      ------------------------------------------------------

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

touch file
mkdir directory
touch directory/file

AT_CHECK(
[chmod 000 file
tar cf archive file
status=$?
chmod 600 file
test $status = 2 || exit 1
sed 's/denied (.*)$/denied/' stderr > stderr2
mv stderr2 stderr
], , ,
[tar: Cannot add file file: Permission denied
tar: Processed all files possible, despite earlier errors
])

AT_CHECK(
[chmod 000 file
tar cf archive --ignore-failed-read file || exit 1
status=$?
chmod 600 file
test $status = 0 || exit 1
sed 's/denied (.*)$/denied/' stderr > stderr2
mv stderr2 stderr
], , ,
[tar: Cannot add file file: Permission denied
])

AT_CHECK(
[chmod 000 directory
tar cf archive directory
status=$?
chmod 700 directory
test $status = 2 || exit 1
sed 's/denied (.*)$/denied/' stderr > stderr2
mv stderr2 stderr
], , ,
[tar: Cannot add directory directory: Permission denied
tar: Processed all files possible, despite earlier errors
])

AT_CHECK(
[chmod 000 directory
tar cf archive --ignore-failed-read directory || exit 1
status=$?
chmod 700 directory
test $status = 0 || exit 1
sed 's/denied (.*)$/denied/' stderr > stderr2
mv stderr2 stderr
], , ,
[tar: Cannot add directory directory: Permission denied
])

AT_CLEANUP(archive directory file)
