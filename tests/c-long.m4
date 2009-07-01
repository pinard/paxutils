#							-*- shell-script -*-

### test new -c format with pretty long names

AT_SETUP(cpio -o -H newc > 3.c.gcpio)
dnl      ------------------------------

### Test some long filenames

mkdir src3
cd src3

mkdir -p 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper
echo undeuxtrois >> 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/123
echo quatrecinqsixsepthuit >> 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/45678

mkdir -p 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/8910/almostdeeper
echo probablyspelledwrong >>  12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/8910/almostdeeper/onetwothree

mkdir -p 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/notquiteasdeep/butstillrather/fardownthere
echo twistypassagesallalike >> 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/notquiteasdeep/butstillrather/fardownthere/twistypassages
echo twistypassagesalldifferent >> 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/notquiteasdeep/butstillrather/fardownthere/twistypassage2

find 12345678901234 -depth -print > 3.find

# we will use this 1 file to verify that tar format won't do files
# longer than 100 chars
echo 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/notquiteasdeep/butstillrather/fardownthere/twistypassage2 > 3.find.only1

# Create a destination directory

mkdir dst3
cd src3

AT_CHECK(
[PREPARE(structure, struct-list)
cd src3
cat 3.find | cpio -o -H newc > 3.c.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -H newc < 3.c.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst3
mkdir dst3
cd dst3
cpio -idum -H newc < 3.c.gcpio
verify -list -match-dir src3 -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match -ignore-dot-mtime < 3.find
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

### test -H ustar format with pretty long names

AT_SETUP(cpio -oH ustar > 3.ustar.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd src3
cat 3.find | cpio -oH ustar > 3.ustar.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idH ustar < 3.ustar.gcpio)
dnl      -------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst3
mkdir dst3
cd dst3
TIME=`echotime`
sleep 1
cpio -idH ustar < 3.ustar.gcpio
verify -list -match-dir src3 -mode-match -uid-match -gid-match -size-match -contents-match -mtime-gt $TIME < 3.find
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

### test -H tar format with pretty long names

AT_SETUP(cpio -oH tar > 3.tar.gcpio)
dnl      --------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd src3
cat 3.find.only1 | cpio -oH tar > 3.tar.gcpio
sed -e 's%^%.*: %' -e 's%$%: file name too long%' < 3.find.only1 > expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)

### test new -c format with really long names

AT_SETUP(cpio -o -H newc > 4.c.gcpio)
dnl      ------------------------------

### Test some really long filenames

mkdir src4
cd src4

mkdir -p 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper
echo foo > 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/FOO

# Do some versions of find lose on really long names?  Just to be safe...
echo 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/FOO > 4.find
echo 12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper/12345678901234/abcdefghijklmn/43210987654321/ABCDEFGHIJKLMN/foobarbazwhizz/onetwothreefou/rfivesixsevene/ightnineten/deeper >> 4.find

# Create a destination directory

mkdir dst4
cd src4

AT_CHECK(
[PREPARE(structure, struct-list)
cd src4
cat 4.find | cpio -o -H newc > 4.c.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -H newc < 4.c.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
rm -rf dst4
mkdir dst4
cd dst4
cpio -idum -H newc < 4.c.gcpio
verify -list -match-dir src4 -mode-match -uid-match -gid-match -size-match -contents-match -mtime-match -ignore-dot-mtime < 4.find
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

### test -H ustar format with really long names

AT_SETUP(cpio -oH ustar > 4.ustar.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd src4
cat 4.find | cpio -oH ustar > 4.ustar.gcpio
sed -e 's%^%.*: %' -e 's%$%: file name too long%' < 4.find > expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)
