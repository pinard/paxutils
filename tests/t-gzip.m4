#							-*- shell-script -*-

AT_SETUP(if tar detects that its gzip child failed)
dnl      -----------------------------------------

# Skip test when /dev/null is not available.
# Hmph!  Seems it was only for DOS, where DJGPP now supports it.
#test -r /dev/null && exit 77

AT_CHECK(tar xfvz /dev/null, 2, ,
[
gzip: stdin: unexpected end of file
tar: Child returned status 1
tar: Processed all files possible, despite earlier errors
])

AT_CLEANUP()
