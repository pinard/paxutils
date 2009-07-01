#							-*- shell-script -*-

### test new -c format

AT_SETUP(cpio -o -H newc)
dnl      ---------------

AT_CHECK(
[PREPARE(structure, struct-list)
cd structure
cat struct-list | cpio -o -H newc > 1.c.gcpio
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -t -H newc < 1.c.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -t -H newc < 1.c.gcpio | sort +8
sed -e '/^\.\/..*$/s/^\.\///' -e 's/^/.*/' -e s'/$/.*/' struct-list | sort > expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)

AT_SETUP(cpio -tv -H newc < 1.c.gcpio)
dnl      -------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -tv -H newc < 1.c.gcpio | sort +8
sed -e '/^\.\/..*$/s/^\.\///' -e 's/^/.*/' -e s'/$/.*/' struct-list | sort > expout
sort expout > ${EXPOUT}2
mv ${EXPOUT}2 expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)

AT_SETUP(cpio -tv < 1.c.gcpio)
dnl      -----------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -tv < 1.c.gcpio | sort +8
sed -e '/^\.\/..*$/s/^\.\///' -e 's/^/.*/' -e s'/$/.*/' struct-list | sort > expout
cat << EOF >> expout
NN blocks
EOF
CPIO_FILTER
], 0)

AT_CLEANUP($cleanup)

AT_SETUP(cpio -id -H newc < 1.c.gcpio)
dnl      ----------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
TIME=`echotime`
sleep 1
cpio -id -H newc < 1.c.gcpio
VERIFY_NEWER
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)

AT_SETUP(cpio -idum -H newc < 1.c.gcpio)
dnl      ------------------------------

AT_CHECK(
[PREPARE(structure, struct-list)
cpio -idum -H newc < 1.c.gcpio
VERIFY_EXACT
CPIO_FILTER
], 0,
[NN blocks
])

AT_CLEANUP($cleanup)
