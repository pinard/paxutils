/*--------------------------.
| Write some sparse files.  |
`--------------------------*/

#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
/* #ifdef HAVE_FCNTL_H */
#include <fcntl.h>
/* #endif */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if STDC_HEADERS || HAVE_STRING_H
# include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# define index strchr
# define rindex strrchr
# define bcopy(s, d, n) memcpy ((d), (s), (n))
# define bcmp(s1, s2, n) memcmp ((s1), (s2), (n))
# define bzero(s, n) memset ((s), 0, (n))
#else
#include <strings.h>
/* memory.h and strings.h conflict on some systems.  */
#endif

#define DISKBLOCKSIZE (512)

main(argc, argv)
     int argc;
     char *argv [];
{
  int fd, rc;
  char buf [DISKBLOCKSIZE];

  fd = write_open ("sparse1");
  write (fd, "hi\n", 3);
  lseek (fd, 4 * DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "bye\n", 4);
  close (fd);

  fd = write_open ("sparse2");
  write (fd, "hi\n", 3);
  lseek (fd, 4 * DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "bye\n", 4);
  lseek (fd, 4, SEEK_CUR);
  buf [0] = '\0';
  write (fd, buf, 1);
  close (fd);

  fd = write_open ("sparse3");
  write (fd, "hi\n", 3);
  lseek (fd, 4 * DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "bye\n", 4);
  lseek (fd, 3 * DISKBLOCKSIZE, SEEK_CUR);
  buf [0] = '\0';
  write (fd, buf, 1);
  close (fd);

  fd = write_open ("sparse4");
  write (fd, "hi\n", 3);
  lseek (fd, DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "yo\n", 3);
  lseek (fd, DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "ai\n", 3);
  lseek (fd, 4 * DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "bye\n", 4);
  lseek (fd, 2 * DISKBLOCKSIZE, SEEK_CUR);
  buf [0] = '\0';
  write (fd, buf, 1);
  close (fd);

  fd = write_open ("sparse5");
  bfill (buf, DISKBLOCKSIZE, 'a');
  write (fd, "hi\n", 3);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  write (fd, buf, DISKBLOCKSIZE);
  lseek (fd, 4 * DISKBLOCKSIZE, SEEK_CUR);
  write (fd, "bye\n", 4);
  close (fd);

  exit (0);
}

bfill (bp, n, value)
     char *bp;
     int n;
     int value;
{
  for (; n > 0; --n)
    *bp++ = value;
}

int
write_open (filename)
     char *filename;
{
  int fd;

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      perror (filename);
      exit (1);
    }
  return fd;
}
