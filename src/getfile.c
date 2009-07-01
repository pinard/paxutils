/* getfile.c - Code to return next filename.
   Copyright (C) 1990, 1991, 1992, 1998, 1999 Free Software Foundation, Inc.

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

/* Written by Tom Tromey <tromey@drip.colorado.edu>.  */

#include <config.h>
#include <sys/types.h>
#include "system.h"
#include "common.h"

/*-----------------------------------------------------------------------.
| Return next file name to process.  If pax_file_names is not NULL, then |
| file names come from the command line.  Otherwise they are read from   |
| stdin.  This function returns 0 when there are no more files.          |
`-----------------------------------------------------------------------*/

bool
get_next_file_name (dynamic_string *result)
{
  if (pax_file_names != NULL)
    {
      if (num_pax_file_names <= 0)
	return false;

      ds_resize (result, strlen (pax_file_names[0]) + 1);
      strcpy (result->string, pax_file_names[0]);

      pax_file_names++;
      num_pax_file_names--;
      return true;
    }
  else
    return ds_fgetstr (stdin, result, name_end) != NULL;
}
