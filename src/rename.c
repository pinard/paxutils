/* rename.c - Rename files.

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
#include <ctype.h>
#include <regex.h>

#include "common.h"


/* Interactive file for rename operation.  */
static FILE *tty_in = NULL;

/* Interactive file for rename operation.  */
static FILE *tty_out = NULL;

/* File used for batch renaming.  */
static FILE *rename_in = NULL;

/* An instance of this structure holds a regular expression used for
   renaming files.  */
struct rename_regexp
{
  struct re_pattern_buffer *regex;
  char *replace;		/* replace format */
  char global;			/* true if we should apply until failure  */
  char print;			/* true if we should print transforms */
  struct re_registers regs;	/* registers */
  struct rename_regexp *next;
};

/* List of regexps to use when renaming.  */
static struct rename_regexp *rename_list = NULL;

/*----------------------------------.
| Set up for interactive renaming.  |
`----------------------------------*/

static void
setup_interactive_renaming (void)
{
  /* Set regexp syntax.  */
  /* NOTE: "rx" doesn't include definition for RE_SYNTAX_ED.  */
  re_set_syntax (RE_SYNTAX_POSIX_BASIC);

  /* Open batch rename file for rename operation.  */
  if (rename_batch_file)
    {
      /* Can't have both.  */
      assert (!rename_option);
      rename_in = fopen (rename_batch_file, "r");
      if (rename_in == NULL)
	error (2, errno, "%s", CONSOLE);
    }

  /* Open interactive file pair for rename operation.  */
  if (rename_option)
    {
      tty_in = fopen (CONSOLE, "r");
      if (tty_in == NULL)
	error (2, errno, "%s", CONSOLE);
      tty_out = fopen (CONSOLE, "w");
      if (tty_out == NULL)
	error (2, errno, "%s", CONSOLE);
    }
}

/*------------------------------------------------------------------------.
| Perform a regexp match and substitution.  If GLOBAL is set, keeps going |
| until a failure.                                                        |
`------------------------------------------------------------------------*/

/* Why isn't this in a library somewhere?  It seems like I keep writing code
   like this.  */

static int
match_and_substitute (struct re_pattern_buffer *regex, char *string,
		      char *subst, dynamic_string *outstr,
		      struct re_registers *regs, int global)
{
  int found_one = 0;

  outstr->string[0] = '\0';
  while (*string)
    {
      int sl = strlen (string);
      int match = re_match (regex, string, sl, 0, regs);
      if (match == -1)
	{
	  /* No match.  If we've never had a match, just return
	     failure.  */
	  if (!found_one)
	    return 0;
	  /* Otherwise, we must copy some trailing info.  */
	  break;
	}
      else if (match == -2)
	{
	  /* Some error.  Just bail.  */
	  return -1;
	}
      else
	{
	  char *p;
	  char xx[2];
	  int size;

	  /* Match occurred.  Copy stuff before first match, then do
	     substitution.  */
	  ds_strncat (outstr, string, regs->start[0]);

	  /* Do the hard part.  */
	  size = ds_strlen (outstr) + strlen (subst) + 1;
	  ds_resize (outstr, size);
	  xx[1] = '\0';
	  for (p = subst; *p; p++)
	    {
	      if (*p == '\\')
		{
		  p++;
		  if (isdigit (*p) || *p == '&')
		    {
		      int dig;
		      if (*p == '&')
			dig = 0;
		      else
			dig = *p - '0';
		      size += regs->end[dig] - regs->start[dig];
		      ds_resize (outstr, size);
		      ds_strncat (outstr, string + regs->start[dig],
				  regs->end[dig] - regs->start[dig]);
		    }
		  else
		    {
		      /* Just insert the character.  FIXME
			 inefficiency abounds.  */
		      xx[0] = *p;
		      ds_strcat (outstr, xx);
		    }
		}
	      else
		{
		  /* Ordinary character.  */
		  xx[0] = *p;
		  ds_strcat (outstr, xx);
		}
	    }

	  string += regs->end[0]; /* + 1 */
	}

      /* If we only want one match, then we bail now.  */
      if (!global)
	break;
    }

  /* Copy last part of string into result buffer.  */
  ds_strcat (outstr, string);

  return 1;
}

/*-----------------------------------------------------------------.
| Rename file if desired.  Returns NULL if file is to be skipped.  |
| Otherwise, returns malloc'd copy of new filename.                |
`-----------------------------------------------------------------*/

char *
possibly_rename_file (char *name)
{
  char *result;
  struct rename_regexp *relist;
  dynamic_string outname;
  int found_one = 0;
  int len = strlen (name);

  ds_init (&outname, len + 1);

  /* Try a batch rename.  Do this first because success here means other
     renames aren't attempted.  */
  if (rename_batch_file != NULL)
    {
      assert (!rename_option);

      if (rename_in == NULL)
	setup_interactive_renaming ();

      result = ds_fgetstr (rename_in, &outname, '\n');
      if (result == NULL || *result == '\0')
	{
	  ds_destroy (&outname);
	  result = NULL;
	}

      /* NOTE: `outname' not unconditionally destroyed here on purpose.  */
      return result;
    }

  /* Apply all renaming regexps, until a success.  */
  for (relist = rename_list; relist != NULL; relist = relist->next)
    {
      int r = match_and_substitute (relist->regex, name, relist->replace,
				    &outname, &relist->regs, relist->global);

      /* Stop on first successful match.  */
      if (r)
	{
	  if (relist->print)
	    fprintf (stderr, "%s >> %s\n", name, outname.string);
	  found_one = 1;
	  break;
	}
    }

  /* If we haven't found it, then use the input name.  */
  if (!found_one)
    strcpy (outname.string, name);

  /* Now interactively rename.  */
  if (rename_option)
    {
      dynamic_string new_name;
      char *str_res;

      /* Initialize here to prevent a premature failure when the tty
	 can't be opened, but no file actually needs to be renamed.  */
      if (tty_in == NULL)
	setup_interactive_renaming ();

      ds_init (&new_name, 128);

      /* FIXME for pax we must print more info here.  */
      fprintf (tty_out, _("rename %s -> "), outname.string);
      fflush (tty_out);
      str_res = ds_fgets (tty_in, &new_name);

      if (pax_rename_option)
	{
	  /* In a pax-style rename, the following hold:
	     1. EOF causes an immediate exit.
	     2. Empty response causes file to be skipped.
	     3. "." means don't rename. */
	  if (str_res == NULL)
	    {
	      /* FIXME use error?  */
	      exit (2);
	    }
	  else if (str_res[0] == '\0')
	    result = NULL;
	  else if (!strcmp (str_res, "."))
	    result = xstrdup (outname.string);
	  else
	    result = xstrdup (new_name.string);
	}
      else
	{
	  /* For cpio-style rename, the rules are different:
	     1. EOF or empty response causes file to be skipped.
	     2. Anything else tries to rename. */
	  if (str_res == NULL || str_res[0] == 0)
	    result = NULL;
	  else
	    result = xstrdup (new_name.string);
	}

      ds_destroy (&new_name);
    }
  else
    result = xstrdup (outname.string);

  ds_destroy (&outname);

  return result;
}

/*---------------------------------------------------------------------------.
| Take a string like "/blah/" and turn it into "blah", making sure that the  |
| first and last characters are the same, and handling quoted separator      |
| characters.  Actually, stops on the occurence of an unquoted separator.    |
| Returns pointer to terminating separator.  Works in place.  terminate name |
| string with NUL.                                                           |
`---------------------------------------------------------------------------*/

static char *
scan_separators (char *name)
{
  char sep = name[0];
  char *copyto = name;
  int quoted = false;

  for (name++; *name != '\0'; name++)
    {
      if (quoted)
	{
	  if (*name == sep)
	    *copyto++ = sep;
	  else
	    {
	      /* Something else is quoted, so preserve the quote. */
	      *copyto++ = '\\';
	      *copyto++ = *name;
	    }
	  quoted = false;
	}
      else if (*name == '\\')
	quoted = true;
      else if (*name == sep)
	break;
      else
	*copyto++ = *name;
    }

  /* Terminate copied string. */
  *copyto = '\0';
  return name;
}

/*----------------------------.
| Add a new renaming regexp.  |
`----------------------------*/

void
add_rename_regexp (char *str)
{
  char *patend, *sub;
  char delimiter = *str;
  struct rename_regexp *renbuf;
  const char *err;
  struct re_pattern_buffer *patbuf;

  if (!delimiter)
    error (2, 0, _("missing regexp"));

  /* Put regexp in STR, and rest in PATEND.  */
  patend = scan_separators (str);
  if (!*str)
    error (2, 0, _("null regexp"));

  /* Put substitution in PATEND, and flags in SUB.  */
  sub = scan_separators (patend);

  patbuf = (struct re_pattern_buffer *)
    xmalloc (sizeof (struct re_pattern_buffer));
  patbuf->buffer = NULL;
  patbuf->allocated = REGS_UNALLOCATED;
  patbuf->fastmap = NULL;
  patbuf->translate = NULL;
  patbuf->no_sub = 1;
  err = re_compile_pattern (str, strlen (str), patbuf);
  if (err != NULL)
    error (2, 0, _("%s while compiling pattern"), err);

  renbuf = (struct rename_regexp *) xmalloc (sizeof (struct rename_regexp));
  renbuf->global = 0;
  renbuf->print = 0;
  renbuf->replace = patend;	/* FIXME consider copying.  */
  renbuf->regex = patbuf;

  /* Scan flag string and set flags.  */
  sub++;
  while (*sub)
    {
      switch (*sub)
	{
	case 'p':
	  renbuf->print = 1;
	  break;
	case 'g':
	  renbuf->global = 1;
	  break;
	default:
	  error (2, 0, _("invalid regexp modifier `%c'"), *sub);
	  break;
	}
      sub++;
    }

  renbuf->next = rename_list;
  rename_list = renbuf;
}
