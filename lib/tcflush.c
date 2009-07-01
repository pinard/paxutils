/* Flush accumulated queues for a given file.
   François Pinard <pinard@iro.umontreal.ca>, 1999.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_TERMIO_H

/* SysV variants.  */

# include <sys/termio.h>

int
tcflush (int fd, int selector)
{
  return ioctl (fd, TCFLSH, selector);
}

#else /* not HAVE_TERMIO_H */

/* BSD variants.  */

# define TCIFLUSH 0
# define TCOFLUSH 2
# define TCIOFLUSH 2

# include <sys/ioctl.h>
# include <sys/file.h>

int
tcflush (int fd, int selector)
{
  int selection = 0;

  switch (selector)
    {
    case TCIFLUSH:
      selection = FREAD;
      break;

    case TCOFLUSH:
      selection = FWRITE;
      break;

    case TCIOFLUSH:
      selection = FREAD | FWRITE;
      break;
    }

  return ioctl (fd, TIOCFLUSH, &selection);
}

#endif /* not HAVE_TERMIO_H */
