/* Global variables for paxutils archiving programs.
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#define GLOBAL(Type, Variable, Init) Type Variable
#define GLOBAL_NO_INIT(Type, Variable) Type Variable
#include "common.h"

/* [cpio] 512 bytes of 0; used for various padding operations.  */
char zero_block[BLOCKSIZE];

void
reset_global_variables (void)
{
#undef GLOBAL
#undef GLOBAL_NO_INIT
#define GLOBAL(Type, Variable, Init) Variable = Init
#define GLOBAL_NO_INIT(Type, Variable)
#include "global.h"

  memset (zero_block, 0, BLOCKSIZE);
}
