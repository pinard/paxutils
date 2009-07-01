/* cdf.c - Functions to handle HP CDF files.
   Copyright (C) 1990, 1991, 1992, 1998 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "system.h"

#include "extern.h"
#include "cpiohdr.h"

/*----.
| ??  |
`----*/

void
possibly_munge_cdf_directory_name (char *name, struct new_cpio_header *file_hdr)
{
  /* Strip leading `./' from the filename.  */
  while (*name == '.' && *(name + 1) == '/')
    {
      ++name;
      while (*name == '/')
	++name;
    }

#ifdef HPUX_CDF
  if ((archive_format != arf_tar) && (archive_format != arf_ustar)
      && (archive_format != arf_gnutar))
    {
      /* We mark CDF's in cpio files by adding a 2nd `/' after the
	 "hidden" directory name.  We need to do this so we can
	 properly recreate the directory as hidden (in case the
	 files of a directory go into the archive before the
	 directory itself (e.g from "find ... -depth ... | cpio")).  */
      file_hdr->c_name = add_cdf_double_slashes (name);
      file_hdr->c_namesize = strlen (file_hdr->c_name) + 1;
    }
  else
#endif /* HPUX_CDF */
    {
      /* We don't mark CDF's in tar files.  We assume the "hidden"
	 directory will always go into the archive before any of
	 its files.  */
      file_hdr->c_name = name;
      file_hdr->c_namesize = strlen (name) + 1;
    }
}

#ifdef HPUX_CDF
/* When we create a cpio archive we mark CDF's by putting an extra `/'
   after their component name so we can distinguish the CDF's when we
   extract the archive (in case the "hidden" directory's files appear
   in the archive before the directory itself).  E.g., in the path
   "a/b+/c", if b+ is a CDF, we will write this path as "a/b+//c" in
   the archive so when we extract the archive we will know that b+
   is actually a CDF, and not an ordinary directory whose name happens
   to end in `+'.  We also do the same thing internally in copypass.c.  */

/*---------------------------------------------------------------------------.
| Take an INPUT_NAME and check it for CDF's.  Insert an extra `/' in the     |
| pathname after each "hidden" directory.  If we add any `/'s, return a      |
| malloced string (which it will reuse for later calls so our caller doesn't |
| have to worry about freeing the string) instead of the original input      |
| string.                                                                    |
`---------------------------------------------------------------------------*/

char *
add_cdf_double_slashes (char *input_name)
{
  static char *ret_name = NULL;	/* re-usuable return buffer (malloc'ed) */
  static int ret_size = -1;	/* size of return buffer */
  char *p;
  char *q;
  int n;
  struct stat stat_info;

  /*  Search for a `/' preceeded by a `+'.  */

  for (p = input_name; *p != '\0'; ++p)
    {
      if ( (*p == '+') && (*(p + 1) == '/') )
	break;
    }

  /* If we didn't find a `/' preceeded by a `+' then there are no CDF's in
     this pathname.  Return the original pathname.  */

  if (*p == '\0')
    return input_name;

  /* There was a `/' preceeded by a `+' in the pathname.  If it is a CDF then
     we will need to copy the input pathname to our return buffer so we can
     insert the extra `/'s.  Since we can't tell yet whether or not it is a
     CDF we will just always copy the string to the return buffer.  First we
     have to make sure the buffer is large enough to hold the string and any
     number of extra `/'s we might add.  */

  n = 2 * (strlen (input_name) + 1);
  if (n >= ret_size)
    {
      if (ret_size < 0)
	ret_name = (char *) malloc (n);
      else
	ret_name = (char *)realloc (ret_name, n);
      ret_size = n;
    }

  /* Clear the `/' after this component, so we can stat the pathname up to and
     including this component.  */
  ++p;
  *p = '\0';
  if ((*xstat) (input_name, &stat_info) < 0)
    {
      error (0, errno, "%s", input_name);
      return input_name;
    }

  /* Now put back the `/' after this component and copy the pathname up to and
     including this component and its trailing `/' to the return buffer.  */
  *p++ = '/';
  strncpy (ret_name, input_name, p - input_name);
  q = ret_name + (p - input_name);

  /* If it was a CDF, add another `/'.  */
  if (S_ISDIR (stat_info.st_mode) && (stat_info.st_mode & 04000) )
    *q++ = '/';

  /* Go through the rest of the input pathname, copying it to the return
     buffer, and adding an extra `/' after each CDF.  */
  while (*p != '\0')
    {
      if ( (*p == '+') && (*(p + 1) == '/') )
	{
	  *q++ = *p++;

	  *p = '\0';
	  if ((*xstat) (input_name, &stat_info) < 0)
	    {
	      error (0, errno, "%s", input_name);
	      return input_name;
	    }
	  *p = '/';

	  if (S_ISDIR (stat_info.st_mode) && (stat_info.st_mode & 04000) )
	    *q++ = '/';
	}
      *q++ = *p++;
    }
  *q = '\0';

  return ret_name;
}

/*---------------------------------------------------------------------------.
| Is the last parent directory (e.g., c in a/b/c/d) a CDF?  If the directory |
| name ends in `+' and is followed by 2 `/'s instead of 1 then it is.  This  |
| is only the case for cpio archives, but we don't have to worry about tar   |
| because tar always has the directory before its files (or else we lose).   |
`---------------------------------------------------------------------------*/

int
islastparentcdf (char *path)
{
  char *newpath;
  char *slash;
  int slash_count;

  slash = rindex (path, '/');
  if (slash == 0)
    return 0;
  else
    {
      slash_count = 0;
      while (slash > path && *slash == '/')
	{
	  ++slash_count;
	  --slash;
	}


      if ( (*slash == '+') && (slash_count >= 2) )
	return 1;
    }
  return 0;
}
#endif
