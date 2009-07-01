#include "system.h"

/*---------------------------------------------------------------------------.
| Prints a file with its bytes swapped if the file has an even number of     |
| bytes.  If it does not have an even number of bytes, swapb prints the file |
| unchanged.                                                                 |
`---------------------------------------------------------------------------*/

/* $ cat foo
   abc
   $ ./swapb foo
   ba
   c$ cat bar
   de
   $ ./swapb bar
   de
   $
*/

main (argc, argv)
     int argc;
     char *argv[];
{
  int length;
  int fd;
  char buf[2];
  char c;

  fd = open (argv[1], 0);
  if (fd < 0)
    exit (1);
  length = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);
  if (length % 2 != 0)
    justcopy (fd);
  while (read (fd, buf, 2) == 2) {
    c = buf[0];
    buf[0] = buf[1];
    buf[1] = c;
    write (1, buf, 2);
  }

  exit (0);
}

justcopy (fd)
     int fd;
{
  char buf[512];
  int bytes_read;

  while (bytes_read = read (fd, buf, 512), bytes_read > 0)
    write (1, buf, bytes_read);
  exit (1);
}
