#							-*- shell-script -*-

AT_SETUP(if directories older than the listed entry are processed)
dnl      --------------------------------------------------------

# Cover for the DOS 2-seconds granularity in file timestamps.

mkdir directory
touch directory/file
sleep 2

AT_CHECK(tar cfv archive --listed=list directory, 0,
[directory/
directory/file
])

sleep 3
touch directory/file

AT_CHECK(tar cfv archive --listed=list directory, 0,
[directory/
directory/file
])

AT_CLEANUP(archive directory)

AT_SETUP(if existing directories are incrementally emptied at restore time)
dnl      -----------------------------------------------------------------

AT_CHECK(
[set -e

mkdir before
cd before
mkdir dir-a
echo alpha > dir-a/file-1
echo beta > file-2
tar cfg ../archiv-1 ../listed .
echo gamma > file-3
mkdir dir-b
echo delta > dir-b/file-4
rm dir-a/file-1 file-2
rmdir dir-a
tar cfg ../archiv-2 ../listed .
cd ..

mkdir after
cd after
tar xfg ../archiv-1 ../listed
tar xfg ../archiv-2 ../listed
cd ..

set +e
diff -r before after
], 0)

AT_CLEANUP(archiv-1 archiv-2 before after)
