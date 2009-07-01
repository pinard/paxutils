/* Arithmetic for numbers greater than an unsigned long or int, for tar.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define BITS_PER_BYTE 8		/* number of bits in each sizeof unit */
#define BITS_PER_TARLONG 42	/* wanted number of bits in each tarlong */

/* In all cases, tarlong is the proper type for a big number.  tarcell is
   either unsigned long, or unsigned int if the former is unavailable.

   For straight compiler arithmetic, SUPERDIGIT is zero and TARLONG_FORMAT
   is the format to directly print a tarlong (without zero-filling).

   For simulated arithmetic, SUPERDIGIT is the base, TARLONG_FORMAT is the
   format to print a single super-digit filled with zeroes to the left, and
   BITS_PER_SUPERDIGIT is the smallest number of bits required to fully
   represent each super-digit.  CELLS_PER_TARLONG says how many cells are
   required for a full tarlong, and SIZEOF_TARLONG is the size of a tarlong
   in bytes.

   The values of SIZEOF_UNSIGNED_LONG_LONG, SIZEOF_UNSIGNED_LONG and
   SIZEOF_UNSIGNED_INT, below, are obtained through the configuration
   process, and SIZEOF_UNSIGNED_LONG_LONG is reset to zero by the
   configuration process if printf does not support %llu.  If nonzero,
   SUPERDIGIT is the biggest power of 10 which fits in half the bits of an
   tarcell.  See comments at beginning of `arith.c' for more explanations.  */

#if BITS_PER_BYTE * SIZEOF_UNSIGNED_INT >= BITS_PER_TARLONG
# define SUPERDIGIT 0
# define TARLONG_FORMAT "%u"
typedef unsigned int tarlong;
#else
# if BITS_PER_BYTE * SIZEOF_UNSIGNED_LONG >= BITS_PER_TARLONG
#  define SUPERDIGIT 0
#  define TARLONG_FORMAT "%lu"
typedef unsigned long tarlong;
# else
#  if BITS_PER_BYTE * SIZEOF_UNSIGNED_LONG_LONG >= BITS_PER_TARLONG + 1
#   define SUPERDIGIT 0
#   define TARLONG_FORMAT "%llu"
typedef unsigned long long tarlong;
#  else
#   if SIZEOF_UNSIGNED_LONG
typedef unsigned long tarcell;
#    if BITS_PER_BYTE * SIZEOF_UNSIGNED_LONG >= 64
#     define SUPERDIGIT 1000000000L
#     define BITS_PER_SUPERDIGIT 29
#     define TARLONG_FORMAT "%09lu"
#    else
#     if BITS_PER_BYTE * SIZEOF_UNSIGNED_LONG >= 32
#      define SUPERDIGIT 10000L
#      define BITS_PER_SUPERDIGIT 14
#      define TARLONG_FORMAT "%04lu"
#     endif
#    endif
#   else
typedef unsigned int tarcell;
#    if BITS_PER_BYTE * SIZEOF_UNSIGNED_INT >= 64
#     define SUPERDIGIT 1000000000
#     define BITS_PER_SUPERDIGIT 29
#     define TARLONG_FORMAT "%09u"
#    else
#     if BITS_PER_BYTE * SIZEOF_UNSIGNED_INT >= 32
#      define SUPERDIGIT 10000
#      define BITS_PER_SUPERDIGIT 14
#      define TARLONG_FORMAT "%04u"
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#if SUPERDIGIT

# define CELLS_PER_TARLONG \
    ((BITS_PER_TARLONG + BITS_PER_SUPERDIGIT - 1) / BITS_PER_SUPERDIGIT)
# define SIZEOF_TARLONG (CELLS_PER_TARLONG * sizeof (tarcell))

/* The NEC EWS 4.2 C compiler gets confused by a pointer to a typedef that
   is an array.  So we wrap the array into a struct.  (Pouah!)  */

struct tarlong
{
  tarcell digit[CELLS_PER_TARLONG];
};

typedef struct tarlong tarlong;

bool zerop_tarlong_helper PARAMS ((tarcell *));
bool lessp_tarlong_helper PARAMS ((tarcell *, tarcell *));
void clear_tarlong_helper PARAMS ((tarcell *));
void add_to_tarlong_helper PARAMS ((tarcell *, tarcell));
void mult_tarlong_helper PARAMS ((tarcell *, tarcell));
void print_tarlong_helper PARAMS ((tarcell *, FILE *));

# define zerop_tarlong(Accumulator) \
   zerop_tarlong_helper (&(Accumulator).digit[0])

# define lessp_tarlong(First, Second) \
   lessp_tarlong_helper (&(First).digit[0], &(Second).digit[0])

# define clear_tarlong(Accumulator) \
   clear_tarlong_helper (&(Accumulator).digit[0])

# define add_to_tarlong(Accumulator, Value) \
   add_to_tarlong_helper (&(Accumulator).digit[0], (tarcell) (Value))

# define mult_tarlong(Accumulator, Value) \
   mult_tarlong_helper (&(Accumulator).digit[0], (tarcell) (Value))

# define print_tarlong(Accumulator, File) \
   print_tarlong_helper (&(Accumulator).digit[0], (File))

#else /* not SUPERDIGIT */

# define zerop_tarlong(Accumulator) \
   ((Accumulator) == 0)

# define lessp_tarlong(First, Second) \
   ((First) < (Second))

# define clear_tarlong(Accumulator) \
   ((Accumulator) = 0)

# define add_to_tarlong(Accumulator, Value) \
   ((Accumulator) += (Value))

# define mult_tarlong(Accumulator, Value) \
   ((Accumulator) *= (Value))

# define print_tarlong(Accumulator, File) \
   (fprintf ((File), TARLONG_FORMAT, (Accumulator)))

#endif /* not SUPERDIGIT */
