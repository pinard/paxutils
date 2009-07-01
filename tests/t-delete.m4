#							-*- shell-script -*-

AT_SETUP(if deleting a member after a big one leaves a good archive)
dnl      ----------------------------------------------------------

AT_CHECK(
[set -e
genfile -l 50000 > file1
genfile -l 1024 > file2
tar cf archive file1 file2
tar f archive --delete file2
tar tf archive
set +e
], ,
[file1
])

AT_CLEANUP(archive file1 file2)

AT_SETUP(if deleting a member with the archive from stdin works correctly)
dnl      ----------------------------------------------------------------

AT_CHECK(
[set -e
genfile -l 3073 -p zeros > 1
cp 1 2
cp 2 3
tar cf archive 1 2 3
tar tf archive
cat archive | tar f - --delete 2 > archive2
set +e
], ,
[1
2
3
])

dnl FIXME!
dnl AT_CHECK(tar tf archive2, 0,
dnl [1
dnl 3
dnl ])

AT_CLEANUP(archive archive2 1 2 3)
