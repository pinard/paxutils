#if !HAVE_MKNOD

#if MSDOS
typedef int dev_t;
#endif

/*----------------------------.
| Fake mknod by complaining.  |
`----------------------------*/

int
mknod (char *path, unsigned short mode, dev_t dev)
{
  int fd;

  errno = ENXIO;		/* no such device or address */
  return -1;			/* just give an error */
}

/*------------------------.
| Fake links by copying.  |
`------------------------*/

int
link (char *path1, char *path2)
{
  char buf[256];
  int ifd, ofd;
  int nrbytes;
  int nwbytes;

  WARN ((0, 0, _("%s: Cannot link to %s, copying instead"), path1, path2));
  if (ifd = open (path1, O_RDONLY | O_BINARY), ifd < 0)
    return -1;
  if (ofd = creat (path2, 0666), ofd < 0)
    return -1;
#if MSDOS
  setmode (ofd, O_BINARY);
#endif
  while (nrbytes = read (ifd, buf, sizeof (buf)), nrbytes > 0)
    {
      if (nwbytes = write (ofd, buf, nrbytes), nwbytes != nrbytes)
	{
	  nrbytes = -1;
	  break;
	}
    }

  /* Note use of "|" rather than "||" below: we want to close.  */

  if ((nrbytes < 0) | (close (ifd) != 0) | (close (ofd) != 0))
    {
      unlink (path2);
      return -1;
    }
  return 0;
}

/* Everyone owns everything on MS-DOS (or is it no one owns anything?).  */

/*---.
| ?  |
`---*/

static int
chown (char *path, int uid, int gid)
{
  return 0;
}

/*---.
| ?  |
`---*/

int
geteuid (void)
{
  return 0;
}

#endif /* !HAVE_MKNOD */

#if __TURBOC__
#include <time.h>
#include <fcntl.h>
#include <io.h>

struct utimbuf
  {
    time_t actime;		/* access time */
    time_t modtime;		/* modification time */
  };

/*---.
| ?  |
`---*/

int
utime (char *filename, struct utimbuf *utb)
{
  struct tm *tm;
  struct ftime filetime;
  time_t when;
  int fd;
  int status;

  if (utb == 0)
    when = time (0);
  else
    when = utb->modtime;

  fd = _open (filename, O_RDWR);
  if (fd == -1)
    return -1;

  tm = localtime (&when);
  if (tm->tm_year < 80)
    filetime.ft_year = 0;
  else
    filetime.ft_year = tm->tm_year - 80;
  filetime.ft_month = tm->tm_mon + 1;
  filetime.ft_day = tm->tm_mday;
  if (tm->tm_hour < 0)
    filetime.ft_hour = 0;
  else
    filetime.ft_hour = tm->tm_hour;
  filetime.ft_min = tm->tm_min;
  filetime.ft_tsec = tm->tm_sec / 2;

  status = setftime (fd, &filetime);
  _close (fd);
  return status;
}

#endif /* __TURBOC__ */
