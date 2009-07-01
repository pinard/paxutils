/* dstring.h - Dynamic string handling include file.  Requires strings.h.
   Copyright (C) 1990, 1991, 1992, 1997, 1998 Free Software Foundation, Inc.

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

#ifndef PARAMS
# if __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

/* A dynamic string consists of record that records the size of an allocated
   string and the pointer to that string.  The actual string is a normal zero
   byte terminated string that can be used with the usual string functions.
   The major difference is that the dynamic_string routines know how to get
   more space if it is needed by allocating new space and copying the current
   string.  */

struct dynamic_string
{
  size_t length;		/* actual amount of storage allocated */
  char *string;			/* string pointer */
};

typedef struct dynamic_string dynamic_string;

/* Macros that look similar to the original string functions.  WARNING: These
   macros work only on pointers to dynamic string records.  If used with a
   real record, an "&" must be used to get the pointer.  */
#define ds_strlen(String) \
  strlen ((String)->string)
#define ds_strcmp(String1, String2) \
  strcmp ((String1)->string, (String2)->string)
#define ds_strncmp(String1, String2, Size)	\
  strncmp ((String1)->string, (String2)->string, Size)
#define ds_strchr(String, Character) \
  strchr ((String)->string, Character)
#define ds_strrchr(String, Character) \
  strrchr ((String)->string, Character)

void ds_init PARAMS ((dynamic_string *, size_t));
void ds_resize PARAMS ((dynamic_string *, size_t));
void ds_destroy PARAMS ((dynamic_string *));
char *ds_fgetname PARAMS ((FILE *, dynamic_string *));
char *ds_fgets PARAMS ((FILE *, dynamic_string *));
char *ds_fgetstr PARAMS ((FILE *, dynamic_string *, char));
void ds_strcat PARAMS ((dynamic_string *, char *));
void ds_strncat PARAMS ((dynamic_string *, char *, int));
