#							-*- shell-script -*-

AT_SETUP(if volume labels are checked on read by fnmatch)
dnl      -----------------------------------------------

# Skip test when /dev/null is not available.
# Hmph!  Seems it was only for DOS, where DJGPP now supports it.
test -r /dev/null || exit 77

AT_CHECK(
[set -e
tar cfVT archive label /dev/null
tar xfV archive label
tar xfV archive 'la?el'
tar xfV archive 'l*l'
set +e
])

AT_CHECK(tar xfV archive lab, 2, ,
[tar: Volume `label' does not match `lab'
tar: Error is not recoverable: exiting now
])

AT_CHECK(tar xfV archive bel, 2, ,
[tar: Volume `label' does not match `bel'
tar: Error is not recoverable: exiting now
])

AT_CHECK(tar xfV archive babel, 2, ,
[tar: Volume `label' does not match `babel'
tar: Error is not recoverable: exiting now
])

AT_CLEANUP(archive)
