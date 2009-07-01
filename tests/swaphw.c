/*
 * swaphw.c - prints a file with its halfwords (16 bits) swapped if the
 * file has an even number of halfwords.  If it does not have an
 * even number of halfwords, swaphw prints the file unchanged.
 * $ cat foo
 * abc
 * $ ./swaphw foo
 * c
 * ba$ cat bar
 * de
 * $ ./swaphw bar
 * de
 * $
 */

#include <stdio.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif
#ifndef SEEK_SET
#define SEEK_SET 0
#endif


main (argc, argv)
	int	argc;
	char	*argv [];
{
	int	length;
	int	fd;
	short	buf [2];
	short	c;
	
	fd = open (argv [1], 0);
	if (fd < 0)
		exit (1);
	length = lseek (fd, 0, SEEK_END);
	lseek (fd, 0, SEEK_SET);
	if ( (length % 4) != 0)
		justcopy (fd);
	while (read (fd, buf, 4) == 4) {
		c = buf [0];
		buf [0] = buf [1];
		buf [1] = c;
		write (1, buf, 4);
	}
	return (0);
}
justcopy (fd)
	int	fd;
{
	char	buf [512];
	int	bytes_read;
	while ( (bytes_read = read (fd, buf, 512) ) > 0)
		write (1, buf, bytes_read);
	exit (1);
}
