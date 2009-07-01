#! /bin/sh
# An interrupted write was not restarted on POSIX systems.

. ./preset

# Skip test when fork is not available, like on DOSish systems.
# I do not know how to do this right, so just exclude DOSish for now.
# But cleanup does not occur, then!  FIXME!
test -z "$COMSPEC" || exit 77
test -z "$ComSpec" || exit 77

. $srcdir/before

genfile -l 100000 > file

( tar cf - file &
  process=$!
  sleep 2
  kill -STOP $process
  sleep 1
  kill -CONT $process
) |
( sleep 5
  cat > /dev/null
)

. $srcdir/after
