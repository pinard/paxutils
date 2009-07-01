/* Arithmetic for numbers greater than an unsigned long or int, for tar.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.

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
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"

/* common.h is needed to define FATAL_ERROR.  It also includes arith.h.  */
#include "common.h"

/* tar needs handling numbers exceeding 32 bits, which is the size of unsigned
   long ints for many C compilers.  This module should provide machinery for
   handling at least BITS_PER_TARLONG bits per number.  It uses the first of
   `unsigned int', `unsigned long' or `unsigned long long' types which is
   sufficient for the task.

   If `unsigned long long' is not fully supported, SIZEOF_UNSIGNED_LONG_LONG
   will be set to zero by configure.  In this case, or if an `unsigned long
   long' does not have enough bits, then huge numbers are rather represented
   by an array of `tarcell', with the least significant super-digit at
   position 0.  A `tarcell' is usually an `unsigned long', but if the compiler
   does not support this type, it is an `unsigned int' instead.  For
   multiplications, we arbitrarily decide than the multiplicator should not be
   bigger than a superdigit.  Intermediate results have to fit into a tarcell,
   that is, (BASE-1)^2 should fit into an `tarcell'.  For this to work, a
   superdigit is never more than half the size of a tarcell.  For easing
   decimal output, the BASE of a super-digit is also an exact exponent of 10.

   Currently in tar, we only multiply by 1024 in one place, so one might want
   to define a superdigit as the biggest power of 10 which fits in a tarcell,
   once shortened by 10 bits.  That would be better for tar, but a bit less
   general.  I guess it is not be worth the change.

   Russell Cattelan reports 165 Gb single tapes (digital video D2 tapes on
   Ampex drives), so requiring 38 bits for the tape length in bytes.  He also
   reports breaking the terabyte limit with a single file (using SGI xFS file
   system over 37 28GB disk arrays attached to a Power Challenge XL; check out
   http://www.lcse.umn.edu/ for a picture), so requiring a little more than 40
   bits for the file size in bytes.  The POSIX header structure allows for 12
   octal digits to represent file lengths, that is, considering the required
   NUL or SPC at the end of the field, up to 33 bits *only* for the byte size
   of files.  */

#if SUPERDIGIT

/*-------------------------------.
| Check if ACCUMULATOR is zero.	 |
`-------------------------------*/

bool
zerop_tarlong_helper (tarcell *accumulator)
{
  int counter;

  for (counter = CELLS_PER_TARLONG - 1; counter >= 0; counter--)
    if (accumulator[counter])
      return false;

  return true;
}

/*----------------------------------------------.
| Check if FIRST is strictly less than SECOND.  |
`----------------------------------------------*/

bool
lessp_tarlong_helper (tarcell *first, tarcell *second)
{
  int counter;

  for (counter = CELLS_PER_TARLONG - 1; counter >= 0; counter--)
    if (first[counter] != second[counter])
      return first[counter] < second[counter];

  return false;
}

/*----------------------------.
| Reset ACCUMULATOR to zero.  |
`----------------------------*/

void
clear_tarlong_helper (tarcell *accumulator)
{
  int counter;

  for (counter = 0; counter < CELLS_PER_TARLONG; counter++)
    accumulator[counter] = 0;
}

/*------------------------------------------------------------------.
| To ACCUMULATOR, add VALUE.  VALUE may be bigger than SUPERDIGIT.  |
`------------------------------------------------------------------*/

/* Just suppose for a moment that tarcells are 32 bits, then
   superdigits may not exceed 16 bits, which is much too small in practice.
   As it would be unreasonable to limit VALUE here to less than SUPERDIGIT,
   this routine uses multiplicative operators.  Sigh!  */

void
add_to_tarlong_helper (tarcell *accumulator, tarcell value)
{
  int counter;

  for (counter = 0; counter < CELLS_PER_TARLONG; counter++)
    {
      accumulator[counter] += value;
      if (accumulator[counter] < SUPERDIGIT)
	return;

      value = accumulator[counter] / SUPERDIGIT;
      accumulator[counter] %= SUPERDIGIT;
    }

  FATAL_ERROR ((0, 0, _("Arithmetic overflow")));
}

/*---------------------------------------------------------------------.
| Multiply ACCUMULATOR by VALUE, known to be smaller than SUPERDIGIT.  |
`---------------------------------------------------------------------*/

void
mult_tarlong_helper (tarcell *accumulator, tarcell value)
{
  tarcell carry = 0;
  int counter;

  for (counter = 0; counter < CELLS_PER_TARLONG; counter++)
    {
      carry += accumulator[counter] * value;
      accumulator[counter] = carry % SUPERDIGIT;
      carry /= SUPERDIGIT;
    }
  if (carry)
    FATAL_ERROR ((0, 0, _("Arithmetic overflow")));
}

/*----------------------------------------------------------.
| Print the decimal representation of ACCUMULATOR on FILE.  |
`----------------------------------------------------------*/

void
print_tarlong_helper (tarcell *accumulator, FILE *file)
{
  int counter = CELLS_PER_TARLONG - 1;

  while (counter > 0 && accumulator[counter] == 0)
    counter--;

  fprintf (file, "%lu", accumulator[counter]);
  while (counter > 0)
    fprintf (file, TARLONG_FORMAT, accumulator[--counter]);
}

#endif /* SUPERDIGIT */
