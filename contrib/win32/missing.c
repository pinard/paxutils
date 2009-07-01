/* Supporting routines which may sometimes be missing.
   Copyright (C) 1988, 1992 Free Software Foundation

This file is part of GNU Tar.

GNU Tar is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Tar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Tar; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#if WIN32 || _WIN32

#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <sys/types.h>
#include "system.h"
#include "common.h"

uid_t geteuid()
{
	return 0;
}

struct passwd* getpwuid(uid_t uid)
{
	static struct passwd pw;

	pw.pw_name	= "root";
	pw.pw_passwd	= "NONE";
	pw.pw_uid	= 0;
	pw.pw_gid	= 0;
	pw.pw_age	= NULL;
	pw.pw_comment	= NULL;
	pw.pw_gecos	= NULL;
	pw.pw_dir	= NULL;
	pw.pw_shell	= NULL;

	return &pw;
}

struct passwd* getpwnam(const char *name)
{
	static struct passwd pw;

	pw.pw_name	= "root";
	pw.pw_passwd	= "NONE";
	pw.pw_uid	= 0;
	pw.pw_gid	= 0;
	pw.pw_age	= NULL;
	pw.pw_comment	= NULL;
	pw.pw_gecos	= NULL;
	pw.pw_dir	= NULL;
	pw.pw_shell	= NULL;

	return &pw;
}

struct group* getgrnam(const char *grname)
{
	static struct group gr;

	gr.gr_name	= "root";
	gr.gr_passwd	= NULL;
	gr.gr_gid	= 0;
	gr.gr_mem	= NULL;

	return &gr;
}

struct group* getgrgid(gid_t gid)
{
	static struct group gr;

	gr.gr_name	= "root";
	gr.gr_passwd	= NULL;
	gr.gr_gid	= 0;
	gr.gr_mem	= NULL;

	return &gr;
}

void setgrent()
{
	return;
}

int chown(char *file, uid_t uid, gid_t gid)
{
	return 0;
}

/* Fake links by copying */
int link (char *path1, char *path2)
{
	char buf[256];
	int ifd, ofd;
	int nrbytes;
	int nwbytes;

	fprintf (stderr, "%s: %s: cannot link to %s, copying instead\n",
		 program_name, path1, path2);
	if ((ifd = open (path1, O_RDONLY | O_BINARY)) < 0)
		return -1;
	if ((ofd = creat (path2, 0666)) < 0)
		return -1;
	setmode (ofd, O_BINARY);
	while ((nrbytes = read (ifd, buf, sizeof (buf))) > 0)
	{
		if ((nwbytes = write (ofd, buf, nrbytes)) != nrbytes)
		{
			nrbytes = -1;
			break;
		}
	}
	/* Note use of "|" rather than "||" below: we want to close
	 * the files even if an error occurs.
	 */
	if ((nrbytes < 0) | (0 != close (ifd)) | (0 != close (ofd)))
	{
		unlink (path2);
		return -1;
	}
	return 0;
}

/* Fake mknod by complaining */
int mknod (char *path, mode_t mode, dev_t dev)
{
	errno = ENXIO;		/* No such device or address */
	return -1;		/* Just give an error */
}

#endif
