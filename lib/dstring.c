/* dstring.c - The dynamic string handling routines used by cpio.
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# if HAVE_STRING_H
#  include <string.h>
# else
#  include <strings.h>
# endif
#endif

#include <stdio.h>
#include "dstring.h"

char *xmalloc PARAMS ((size_t));
char *xrealloc PARAMS ((char *, size_t));

/* Initialiaze dynamic string STRING with space for SIZE characters.  */

void
ds_init (dynamic_string *string, size_t size)
{
  string->length = size;
  string->string = (char *) xmalloc (size);
}

/* Expand dynamic string STRING, if necessary, to hold SIZE characters.  */

void
ds_resize (dynamic_string *string, size_t size)
{
  if (size > string->length)
    {
      string->length = size;
      string->string = (char *)
	xrealloc ((char *) string->string, size);
    }
}

/* Delete dynamic string.  */

void
ds_destroy (dynamic_string *string)
{
  free (string->string);
  string->string = NULL;
}

/* Dynamic string S gets a string terminated by the EOS character (which is
   removed) from file F.  S will increase in size during the function if the
   string from F is longer than the current size of S.  Return NULL if end of
   file is detected.  Otherwise, return a pointer to the null-terminated
   string in S.  */

char *
/* We need a __STDC__ case here because the ANSI prototype in dstring.h
   might have different promotion rules for type "char" in the ANSI vs
   non-ANSI function definition rules.  */
#if __STDC__
ds_fgetstr (FILE *f, dynamic_string *s, char eos)
#else
ds_fgetstr (f, s, eos)
     FILE *f;
     dynamic_string *s;
     char eos;
#endif
{
  int insize;			/* Amount needed for line.  */
  int strsize;			/* Amount allocated for S.  */
  int next_ch;

  /* Initialize.  */
  insize = 0;
  strsize = s->length;

  /* Read the input string.  */
  next_ch = getc (f);
  while (next_ch != eos && next_ch != EOF)
    {
      if (insize >= strsize - 1)
	{
	  ds_resize (s, strsize * 2 + 2);
	  strsize = s->length;
	}
      s->string[insize++] = next_ch;
      next_ch = getc (f);
    }
  s->string[insize++] = '\0';

  if (insize == 1 && next_ch == EOF)
    return NULL;
  else
    return s->string;
}

char *
ds_fgets (FILE *f, dynamic_string *s)
{
  return ds_fgetstr (f, s, '\n');
}

char *
ds_fgetname (FILE *f, dynamic_string *s)
{
  return ds_fgetstr (f, s, '\0');
}

void
ds_strcat (dynamic_string *s, char *t)
{
  ds_resize (s, ds_strlen (s) + strlen (t) + 1);
  strcat (s->string, t);
}

void
ds_strncat (dynamic_string *s, char *t, int n)
{
  size_t length = ds_strlen (s);

  ds_resize (s, length + n + 1);
  strncat (s->string, t, n);
  s->string[length + n] = '\0';
}
