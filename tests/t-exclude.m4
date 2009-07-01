#							-*- shell-script -*-

AT_SETUP(if exclude correctly checks the last occurrence of the string)

AT_CHECK(
[mkdir test-directory
touch test-directory/test
tar cvf archive --exclude=test test-directory
], 0,
[test-directory/
])

AT_CLEANUP(archive test-directory)
