/* Header management for tar.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
   Written by John Gilmore, on 1985-08-25.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"

#define	ISODIGIT(Char) \
  ((unsigned char) (Char) >= '0' && (unsigned char) (Char) <= '7')
#define ISSPACE(Char) (ISASCII (Char) && isspace (Char))

#include "common.h"

/* Octal conversions.  */

/*------------------------------------------------------------------------.
| Return value of number expressed in octal, starting at BUFFER for WIDTH |
| characters.  Result is -1 if the field is all blank, or not terminated  |
| by a space or a null.                                                   |
`------------------------------------------------------------------------*/

static unsigned long
octal_to_ulong (const char *buffer, int width)
{
  unsigned long value;

  while (ISSPACE (*buffer) && width > 0)
    {
      buffer++;
      width--;
    }
  if (width == 0)
    return -1;

  value = 0;
  while (width > 0 && ISODIGIT (*buffer))
    {
      value = (value << 3) | (*buffer++ - '0');
      width--;
    }
  if (width > 0 && *buffer && !ISSPACE (*buffer))
    return -1;

  return value;
}

/*------------------------------------------------------------------------.
| Converts VALUE into a WIDTH-digit field at BUFFER, including one        |
| trailing space.  For example, 2 for WIDTH means one digit and a space.  |
|                                                                         |
`------------------------------------------------------------------------*/

/* This is like: sprintf (BUFFER, "%*lo ", WIDTH - 1, VALUE); except that no
   trailing NUL is produced.  We assume the trailing NUL is already there,
   this fact is used by start_header and finish_header.  */

static void
ulong_to_octal (char *buffer, int width, unsigned long value)
{
  buffer[--width] = ' ';
  buffer[--width] = '0' + (char) (value & 7);
  value >>= 3;

  while (width > 0 && value != 0)
    {
      buffer[--width] = '0' + (char) (value & 7);
      value >>= 3;
    }

  while (width > 0)
    buffer[--width] = ' ';
}

/* Simple getting and setting of header fields.  */

#define FROM_OCTAL(Buffer) \
  octal_to_ulong ((Buffer), sizeof (Buffer))

#define TO_OCTAL(Buffer, Value) \
  ulong_to_octal ((Buffer), sizeof (Buffer), (unsigned long) (Value))

/* For when a NUL has to be preserved at Buffer[sizeof Buffer - 1].  */
#define TO_OCTAL_NUL(Buffer, Value) \
  ulong_to_octal ((Buffer), sizeof (Buffer) - 1, (unsigned long) (Value))

/*----------------------.
| POSIX header fields.  |
`----------------------*/

/* mode.  */
/* ------ */

mode_t
get_header_mode (union block *block)
{
  unsigned long value;

  if (block->header.mode[0] == '\0'
      && block->header.name[NAME_FIELD_SIZE - 1] != '\0'
      && block->header.prefix[0] == '\0')

    /* The mode field starts with a NUL, and the file name seems like having
       exactly 100 characters.  It looks like a buggy archive created by
       Solaris tar, where the NUL terminating the name field overflowed into
       the mode field, shifting the mode after it.  Properly get the mode.  */
    value = octal_to_ulong (block->header.mode + 1,
			    sizeof (block->header.mode) - 1);

  else

    /* This is the normal case.  */
    value = FROM_OCTAL (block->header.mode);

  return value;
}

void
set_header_mode (union block *block, mode_t value)
{
  if (archive_format == V7_FORMAT)
    /* FIXME: probably worth a diagnostic if bits are getting lost.  */
    value &= 07777;

  TO_OCTAL_NUL (block->header.mode, value);
}

/* uid.  */
/* ----- */

uid_t
get_header_uid (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.uid);

  return value;
}

void
set_header_uid (union block *block, uid_t value)
{
  TO_OCTAL_NUL (block->header.uid, value);
}

/* gid.  */
/* ----- */

gid_t
get_header_gid (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.gid);

  return value;
}

void
set_header_gid (union block *block, gid_t value)
{
  TO_OCTAL_NUL (block->header.gid, value);
}

/* size.  */
/* ------ */

off_t
get_header_size (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.size);

  return value;
}

void
set_header_size (union block *block, off_t value)
{
  TO_OCTAL (block->header.size, value);
}

/* mtime.  */
/* ------- */

time_t
get_header_mtime (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.mtime);

  return value;
}

void
set_header_mtime (union block *block, time_t value)
{
  TO_OCTAL (block->header.mtime, value);
}

/* chksum.  */
/* -------- */

int
get_header_chksum (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.chksum);

  return value;
}

void
set_header_chksum (union block *block, int value)
{
  /* It's formatted differently from the other fields: it has [6] digits, a
     null, then a space -- rather than digits, a space, then a null.  We use
     ulong_to_octal then write the null in over ulong_to_octal's space.  The
     final space is already there, from checksumming, and ulong_to_octal
     doesn't modify it.  This is a fast way to do:

     sprintf(block->header.chksum, "%6o", sum); */

  TO_OCTAL_NUL (block->header.chksum, value);
  block->header.chksum[6] = '\0';
}

/* devmajor.  */
/* ---------- */

major_t
get_header_devmajor (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.devmajor);

  return value;
}

void
set_header_devmajor (union block *block, major_t value)
{
  TO_OCTAL_NUL (block->header.devmajor, value);
}

/* devminor.  */
/* ---------- */

minor_t
get_header_devminor (union block *block)
{
  unsigned long value = FROM_OCTAL (block->header.devminor);

  return value;
}

void
set_header_devminor (union block *block, minor_t value)
{
  TO_OCTAL_NUL (block->header.devminor, value);
}

/*------------------------.
| Old GNU header fields.  |
`------------------------*/

/* atime.  */
/* ------- */

time_t
get_header_atime (union block *block)
{
  unsigned long value = FROM_OCTAL (block->oldgnu_header.atime);

  return value;
}

void
set_header_atime (union block *block, time_t value)
{
  TO_OCTAL (block->oldgnu_header.atime, value);
}

/* ctime.  */
/* ------- */

time_t
get_header_ctime (union block *block)
{
  unsigned long value = FROM_OCTAL (block->oldgnu_header.ctime);

  return value;
}

void
set_header_ctime (union block *block, time_t value)
{
  TO_OCTAL (block->oldgnu_header.ctime, value);
}

/* offset.  */
/* -------- */

off_t
get_header_offset (union block *block)
{
  unsigned long value = FROM_OCTAL (block->oldgnu_header.offset);

  return value;
}

void
set_header_offset (union block *block, off_t value)
{
  TO_OCTAL (block->oldgnu_header.offset, value);
}

/* realsize.  */
/* ---------- */

off_t
get_header_realsize (union block *block)
{
  unsigned long value = FROM_OCTAL (block->oldgnu_header.realsize);

  return value;
}

void
set_header_realsize (union block *block, off_t value)
{
  TO_OCTAL (block->oldgnu_header.realsize, value);
}

/*---------------------.
| Old sparse headers.  |
`---------------------*/

/* offset in old initial header.  */
/* ------------------------------ */

off_t
get_initial_header_offset (union block *block, int ordinal)
{
  unsigned long value = FROM_OCTAL (block->oldgnu_header.sp[ordinal].offset);

  return value;
}

void
set_initial_header_offset (union block *block, int ordinal, off_t value)
{
  TO_OCTAL (block->oldgnu_header.sp[ordinal].offset, value);
}

/* numbytes in old initial header.  */
/* -------------------------------- */

size_t
get_initial_header_numbytes (union block *block, int ordinal)
{
  unsigned long value = FROM_OCTAL (block->oldgnu_header.sp[ordinal].numbytes);

  return value;
}

void
set_initial_header_numbytes (union block *block, int ordinal, size_t value)
{
  TO_OCTAL (block->oldgnu_header.sp[ordinal].numbytes, value);
}

/* offset in old extended headers.  */
/* -------------------------------- */

off_t
get_extended_header_offset (union block *block, int ordinal)
{
  unsigned long value = FROM_OCTAL (block->sparse_header.sp[ordinal].offset);

  return value;
}

void
set_extended_header_offset (union block *block, int ordinal, off_t value)
{
  TO_OCTAL (block->sparse_header.sp[ordinal].offset, value);
}

/* numbytes in old extended headers.  */
/* ---------------------------------- */

size_t
get_extended_header_numbytes (union block *block, int ordinal)
{
  unsigned long value = FROM_OCTAL (block->sparse_header.sp[ordinal].numbytes);

  return value;
}

void
set_extended_header_numbytes (union block *block, int ordinal, size_t value)
{
  TO_OCTAL (block->sparse_header.sp[ordinal].numbytes, value);
}
