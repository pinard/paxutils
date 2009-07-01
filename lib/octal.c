/* octal.c - Functions to convert to/from octal.
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


/* Convert the string of octal digits S into a number and store
   it in *N.  Return nonzero if the whole string was converted,
   zero if there was something after the number.
   Skip leading and trailing spaces.  */

int
otoa (s, n)
     char *s;
     unsigned long *n;
{
  unsigned long val = 0;

  while (*s == ' ')
    ++s;
  while (*s >= '0' && *s <= '7')
    val = 8 * val + *s++ - '0';
  while (*s == ' ')
    ++s;
  *n = val;
  return *s == '\0';
}

/* Convert a number into a string of octal digits.
   Convert long VALUE into a DIGITS-digit field at WHERE,
   including a trailing space and room for a NUL.  DIGITS==3 means
   1 digit, a space, and room for a NUL.

   We assume the trailing NUL is already there and don't fill it in.
   This fact is used by start_header and finish_header, so don't change it!

   This is be equivalent to:
   sprintf (where, "%*lo ", digits - 2, value);
   except that sprintf fills in the trailing NUL and we don't.  */

void
to_oct (value, digits, where)
     register long value;
     register int digits;
     register char *where;
{
  --digits;			/* Leave the trailing NUL slot alone.  */
  where[--digits] = ' ';	/* Put in the space, though.  */

  /* Produce the digits -- at least one.  */
  do
    {
      where[--digits] = '0' + (char) (value & 7); /* One octal digit.  */
      value >>= 3;
    }
  while (digits > 0 && value != 0);

  /* Add leading spaces, if necessary.  */
  while (digits > 0)
    where[--digits] = ' ';
}
