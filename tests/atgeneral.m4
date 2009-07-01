divert(-1)						-*- shell-script -*-
# `m4' macros used in building test suites.
# Copyright © 1998 Progiciels Bourbeau-Pinard inc.
# François Pinard <pinard@iro.umontreal.ca>, 1998.

changequote([, ])

define(AT_DEFINE, defn([define]))
define(AT_EVAL, defn([eval]))
define(AT_FORMAT, defn([format]))
define(AT_INCLUDE, defn([include]))
define(AT_SHIFT, defn([shift]))
define(AT_UNDEFINE, defn([undefine]))

undefine([define])
undefine([eval])
undefine([format])
undefine([include])
undefine([shift])
undefine([undefine])

# AT_INIT(PROGRAM, RELPATH)

# Begin testing suite, using PROGRAM to check version, and RELPATH as a
# relative path (usually `../src') to find executable binaries to test.
# RELPATH may be omitted; `.' is always added in front of the search path.

AT_DEFINE(AT_INIT,
[AT_DEFINE(AT_ordinal, 0)
at_usage="Usage: [$]0 [OPTION]...

  -v  Explain the purpose of each group of tests
  -e  Stop if a test fails, then do not clean up"

while test [$][#] -gt 0; do
  case "[$]1" in
    -v) at_verbose=1; shift ;;
    -e) at_stop_on_error=1 shift ;;
    *) echo 1>&2 "$at_usage"; exit 1 ;;
  esac
done

. ./atconfig

if test -n "`$1 --version | sed -n s/$at_package.*$at_version/OK/p`"; then
  at_banner="Testing suite for Free $at_package, version $at_version"
  at_dashes=`echo $at_banner | sed s/./=/g`
  echo "$at_dashes"
  echo "$at_banner"
  echo "$at_dashes"
else
  echo '======================================================='
  echo 'ERROR: Not using the proper version, no tests performed'
  echo '======================================================='
  exit 1
fi

at_failed_list=
at_test_count=0
divert(1)
# Wrap up the testing suite with summary statistics.

rm -f at-check-line
at_fail_count=0
if test -z "$at_failed_list"; then
  at_banner="All $at_test_count tests were successful"
else
  for at_group in $at_failed_list; do
    at_fail_count=`expr $at_fail_count + 1`
    ( echo '#!/bin/sh'
      sed -n "/^[#] Snippet ((/,/^[#] Snippet ))/p" atconfig
      sed -n "/^[#] Snippet (c$at_group(/,/^[#] Snippet )c$at_group)/p" [$]0
      sed -n "/^[#] Snippet (s$at_group(/,/^[#] Snippet )s$at_group)/p" [$]0
      echo 'exit 0'
    ) | grep -v '^[#] Snippet' > FAIL-$at_group.sh
    chmod +x FAIL-$at_group.sh
    echo "FAIL-$at_group.sh:1: debugging script"
  done
  if test -n "$at_stop_on_error"; then
    at_banner="ERROR: One test failed, inhibiting subsequent tests"
  else
    at_banner="ERROR: $at_fail_count of $at_test_count tests failed"
  fi
fi
at_dashes=`echo $at_banner | sed s/./=/g`
echo "$at_dashes"
echo "$at_banner"
echo "$at_dashes"

test -z "$at_failed_list" || exit 1
exit 0
divert[]dnl
])

# AT_SETUP(DESCRIPTION)

# Start a group of related tests, all to be executed in the same subshell.
# The group is testing what DESCRIPTION says.

AT_DEFINE(AT_SETUP,
[AT_DEFINE([AT_group_line], __file__:__line__)
AT_DEFINE([AT_group_description], [$1])
AT_DEFINE([AT_data_files], )
AT_DEFINE([AT_ordinal], AT_EVAL(AT_ordinal + 1))
if test -z "$at_stop_on_error" || test -z "$at_failed_list"; then
  if test -n "$at_verbose"; then
    echo 'testing AT_group_description'
    echo $at_n "     $at_c"
  fi
  echo $at_n "substr(AT_ordinal. AT_group_line                      , 0, 24)[]$at_c"
  (
[#] Snippet (s[]AT_ordinal[](

[#] The test group starts at `AT_group_line'.  An error occurred while
[#] testing AT_group_description.
])

# AT_CLEANUP(FILES)

# Complete a group of related tests, recursively remove those FILES
# created within the test.  There is no need to list stdout, stderr,
# nor files created with AT_DATA.

AT_DEFINE(AT_CLEANUP,
[[#] Snippet )s[]AT_ordinal[])
  )
  case [$]? in
    0) echo ok
       at_test_count=`expr $at_test_count + 1`
       ;;
    77) echo ignored
	;;
    *) echo "FAILED before line `cat at-check-line`"
       at_test_count=`expr $at_test_count + 1`
       at_failed_list="$at_failed_list AT_ordinal"
       ;;
  esac
  if test -z "$at_stop_on_error" || test -z "$at_failed_list"; then
[#] Snippet (c[]AT_ordinal[](

ifelse([AT_data_files$1], ,
  [rm -f stdout stderr],
  [rm -rf[]AT_data_files[]ifelse($1, , , [ $1]) stdout stderr])
[#] Snippet )c[]AT_ordinal[])
  fi
fi
AT_UNDEFINE([AT_group_description])])

# AT_DATA(FILE, CONTENTS)

# Initialize an input data FILE with given CONTENTS, which should end with
# an end of line.

AT_DEFINE(AT_DATA,
[AT_DEFINE([AT_data_files], AT_data_files[ ]$1)
cat > $1 <<'EOF'
$2'EOF'
])

# AT_CHECK(COMMANDS, STATUS, STDOUT, STDERR)

# Execute a test by performing given shell COMMANDS.  These commands
# should normally exit with STATUS, while producing expected STDOUT and
# STDERR contents.  The special word `expout' for STDOUT means that file
# `expout' contents has been set to the expected stdout.  The special word
# `experr' for STDERR means that file `experr' contents has been set to
# the expected stderr.  STATUS is not checked if it is empty.

AT_DEFINE(AT_CHECK,
[echo __line__ > at-check-line
exec 5>&1 6>&2 1>stdout 2>stderr
$1
ifelse([$2], , , [test $? = $2 || exit 1])
exec 1>&5 2>&6
ifelse([$3], , [test ! -s stdout || exit 1
], [$3], expout, [$at_diff expout stdout || exit 1
], [changequote({, })dnl
echo $at_n "patsubst({$3}, \([\"`$]\), \\\1)$at_c" | $at_diff - stdout || exit 1
changequote([, ])])dnl
ifelse([$4], , [test ! -s stderr || exit 1
], [$4], experr, [$at_diff experr stderr || exit 1
], [changequote({, })dnl
echo $at_n "patsubst({$4}, \([\"`$]\), \\\1)$at_c" | $at_diff - stderr || exit 1
changequote([, ])])dnl
])

divert[]dnl
