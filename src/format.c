/* format.c - Deal with format names.
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

#include "system.h"

#include <assert.h>
#include "common.h"

struct format_list
{
  char *name;
  enum archive_format value;
};

/* Canonical names, which must come first so that format_name() will work.  */
static struct format_list the_list[] =
{
  {"crc", CRC_ASCII_FORMAT},
  {"newc", NEW_ASCII_FORMAT},
  {"odc", OLD_ASCII_FORMAT},
  {"bin", BINARY_FORMAT},
  {"ustar", POSIX_FORMAT},
  {"tar", V7_FORMAT},
#ifdef CPIO_USE_GNUTAR
  {"gnutar", GNUTAR_FORMAT},
#endif
  {"hpodc", HPUX_OLD_ASCII_FORMAT},
  {"hpbin", HPUX_BINARY_FORMAT},

  /* Names for compatibility.  */
  /* These are from BSDI pax.  */
  {"cpio", OLD_ASCII_FORMAT},	/* FIXME?  */
  {"bcpio", BINARY_FORMAT},
  {"sv4cpio", NEW_ASCII_FORMAT},
  {"sv4crc", CRC_ASCII_FORMAT},

  {NULL, UNKNOWN_FORMAT}
};

/*-------------------------------------------------------------------------.
| Return format value given name of format.  If format isn't found, return |
| UNKNOWN_FORMAT.                                                          |
`-------------------------------------------------------------------------*/

enum archive_format
find_format (char *format)
{
  struct format_list *fl;

  for (fl = the_list; fl->name != NULL; fl++)
    {
      if (! strcasecmp (format, fl->name))
	break;
    }

  return fl->value;
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
    case BINARY_FORMAT:
    case HPUX_BINARY_FORMAT:
      header_writer = write_out_oldcpio_header;
      eof_writer = write_cpio_eof;
      name_too_long = is_cpio_filename_too_long;
      break;

    case OLD_ASCII_FORMAT:
    case HPUX_OLD_ASCII_FORMAT:
      header_writer = write_out_oldascii_header;
      eof_writer = write_cpio_eof;
      name_too_long = is_cpio_filename_too_long;
      break;

    case NEW_ASCII_FORMAT:
    case CRC_ASCII_FORMAT:
      header_writer = write_out_cpioascii_header;
      eof_writer = write_cpio_eof;
      name_too_long = is_cpio_filename_too_long;
      break;

    case V7_FORMAT:
    case POSIX_FORMAT:
    case GNUTAR_FORMAT:
      header_writer = write_out_tar_header;
      eof_writer = write_tar_eof;
      name_too_long = is_tar_filename_too_long;
      break;

    case UNKNOWN_FORMAT:
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

  for (fl = the_list; fl->name; fl++)
    {
      if (fl->value == fmt)
	return fl->name;
    }

  assert (0);
  return NULL;			/* placate compiler */
}
