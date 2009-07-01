#							-*- shell-script -*-

AT_SETUP(if default cpio creation and listing merely works)
dnl      -------------------------------------------------

PREPARE(structure, struct-list)

AT_CHECK(
[cat struct-list | cpio -o > archive || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -t < archive || exit 1
cp struct-list expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -tv < archive | sed -e 's/ -> .*//' -e 's/^.* //'
cp struct-list expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CLEANUP($cleanup archive expout)

AT_SETUP(if old portable cpio creation and listing merely works)
dnl      ------------------------------------------------------

PREPARE(structure, struct-list)

AT_CHECK(
[cat struct-list | cpio -o -c > arch-oldc || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cpio -t -c < arch-oldc || exit 1
cp struct-list expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -t < arch-oldc || exit 1
cp struct-list expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CHECK(
[cpio -tv -c < arch-oldc | sed -e 's/ -> .*//' -e 's/^.* //'
cp struct-list expout
CPIO_FILTER
], , expout,
[NN blocks
])

AT_CLEANUP($cleanup arch-oldc expout)

AT_SETUP(if installed cpio creation merely works)
dnl      ---------------------------------------

test -n "$BINCPIO" || exit 77

PREPARE(structure, struct-list)

AT_CHECK(
[cat struct-list | $BINCPIO -o$DEVFLAG > arch-inst || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CHECK(
[cat struct-list | $BINCPIO -o$DEVFLAG $BINOLDC > arch-c-inst || exit 1
CPIO_FILTER
], , ,
[NN blocks
])

AT_CLEANUP($cleanup arch-inst arch-c-inst)
