#							-*- shell-script -*-

AT_SETUP(if append works)
dnl      ---------------

AT_CHECK(
[set -e
touch file1
touch file2
tar cf archive file1
tar rf archive file2
tar tf archive
set +e
], ,
[file1
file2
])

AT_CLEANUP(archive file1 file2)
