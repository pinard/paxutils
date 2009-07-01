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

#ifndef _PORT_H
#define _PORT_H

#if WIN32 || _WIN32

#define F_OK		0
#define W_OK		2
#define R_OK		4

typedef unsigned short	ushort_t;
typedef unsigned long	ulong_t;

typedef long		uid_t;
typedef uid_t		gid_t;

typedef ulong_t		mode_t;

struct passwd {
        char    *pw_name;
        char    *pw_passwd;
        uid_t   pw_uid;
        gid_t   pw_gid;
        char    *pw_age;
        char    *pw_comment;
        char    *pw_gecos;
        char    *pw_dir;
        char    *pw_shell;
};

struct group {
	char	*gr_name;
	char	*gr_passwd;
	gid_t	gr_gid;
	char	**gr_mem;
};

/* Define function prototypes for "missing" functions */
struct passwd* getpwuid(uid_t);
struct passwd* getpwnam(const char*);
struct group* getgrnam(const char*);
struct group* getgrgid(gid_t);
void setgrent();
uid_t geteuid();
int chown(char *, uid_t, gid_t);
int link(char *, char *);
int mknod(char *, mode_t, dev_t);

#endif  /* WIN32 */

#endif  /* _PORT_H_ */
