/* format.c - Deal with format names.
   Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.

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

#include "system.h"

#include <assert.h>
#include "extern.h"

struct format_list
{
  char *name;
  enum archive_format value;
};

static struct format_list the_list[] =
{
  /* Canonical names.  These must come first so that format_name() will work.
     */
  {"crc", arf_crcascii},
  {"newc", arf_newascii},
  {"odc", arf_oldascii},
  {"bin", arf_binary},
  {"ustar", arf_ustar},
  {"tar", arf_tar},
#ifdef CPIO_USE_GNU_TAR
  {"gnutar", arf_gnutar},
#endif /* CPIO_USE_GNU_TAR */
  {"hpodc", arf_hpoldascii},
  {"hpbin", arf_hpbinary},

  /* Names for compatibility.  */
  /* These are from BSDI pax.  */
  {"cpio", arf_oldascii},	/* FIXME?  */
  {"bcpio", arf_binary},
  {"sv4cpio", arf_newascii},
  {"sv4crc", arf_crcascii},

  {NULL, arf_unknown}
};

/*-------------------------------------------------------------------------.
| Return format value given name of format.  If format isn't found, return |
| arf_unknown.                                                             |
`-------------------------------------------------------------------------*/

enum archive_format
find_format (char *format)
{
  struct format_list *fl;

  for (fl = the_list; fl->name != NULL; ++fl)
    {
      if (! strcasecmp (format, fl->name))
	break;
    }

  return (fl->value);
}

/*------------------------------------------------.
| Print standard 'format unknown' error message.  |
`------------------------------------------------*/

void
format_error (char *format)
{
  error (2, 0, _("\
invalid archive format `%s'; valid formats are:\n\
crc, newc, odc, bin, ustar, tar, hpodc, hpbin"),
	 format);
}

/*------------------------------------------.
| Set up hooks for writing a given format.  |
`------------------------------------------*/

void
set_write_pointers_from_format (enum archive_format format)
{
  switch (format)
    {
    case arf_binary:
    case arf_hpbinary:
      header_writer = write_out_oldcpio_header;
      eof_writer = write_cpio_eof;
      name_too_long = is_cpio_filename_too_long;
      break;

    case arf_oldascii:
    case arf_hpoldascii:
      header_writer = write_out_oldascii_header;
      eof_writer = write_cpio_eof;
      name_too_long = is_cpio_filename_too_long;
      break;

    case arf_newascii:
    case arf_crcascii:
      header_writer = write_out_cpioascii_header;
      eof_writer = write_cpio_eof;
      name_too_long = is_cpio_filename_too_long;
      break;

    case arf_tar:
    case arf_ustar:
    case arf_gnutar:
      header_writer = write_out_tar_header;
      eof_writer = write_tar_eof;
      name_too_long = is_tar_filename_too_long;
      break;

    case arf_unknown:
    default:
      assert (0);
      break;
    }
}

/*------------------------.
| Return name of format.  |
`------------------------*/

char *
format_name (enum archive_format fmt)
{
  struct format_list *fl;

  for (fl = the_list; fl->name; ++fl)
    {
      if (fl->value == fmt)
	return (fl->name);
    }

  assert (0);
  return (NULL);		/* Placate compiler.  */
}
