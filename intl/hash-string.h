/* hash-string - Implements a string hashing function.
   Copyright (C) 1995 Software Foundation, Inc.

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

#ifdef HAVE_VALUES_H
# include <values.h>
#endif

/* @@ end of prolog @@ */

#ifndef BITSPERBYTE
# define BITSPERBYTE 8
#endif

#ifndef LONGBITS
# define LONGBITS (sizeof (long) * BITSPERBYTE)
#endif	/* LONGBITS  */

/* Defines the so called `hashpjw' function by P.J. Weinberger
   [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
   1986, 1987 Bell Telephone Laboratories, Inc.]  */ 
static inline unsigned long
hash_string (str_param)
     const char *str_param;
{
  unsigned long hval, g;
  unsigned char *str = (unsigned char *) str_param;

  /* Compute the hash value for the given string.  */
  hval = 0;
  while (*str != '\0')
    {
      hval <<= 4;
      hval += *str++;
      g = hval & ((unsigned long) 0xf << (LONGBITS - 4));
      if (g != 0)
	{
	  hval ^= g >> (LONGBITS - 8);
	  hval ^= g;
	}
    }
  return hval;
}
