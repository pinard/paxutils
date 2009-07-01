/* popen(3) replacement for systems not having concurrent processes.
   Copyright (C) 1989, 1997, 1998 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include "popen.h"

#if DOSWIN
# include <io.h>
# include <process.h>
#endif

#if 0
static char template[] = "piXXXXXX";
#endif

enum pipemode
{
  UNOPENED = 0,
  READING_MODE,
  WRITING_MODE
};

typedef enum pipemode pipemode;

struct pipe
{
  char *command;
  char *name;
  pipemode mode;
};

#ifdef _NFILE
static struct pipe pipe_array[_NFILE];
#else
static struct pipe *pipe_array;
static int pipes = 0;
#endif

extern void *xmalloc ();
extern void *xrealloc ();
extern char *xstrdup ();

/*-------------------------------------------------------------------------.
| Return a file pointer to either the saved copy of the standard output    |
| collected while executing COMMAND, if DIRECTION is "r".  If DIRECTION is |
| "w", return a file pointer to an accumulation file which will later      |
| become the standard input while executing COMMAND.                       |
`-------------------------------------------------------------------------*/

FILE *
popen (command, direction)
     const char *command;
     const char *direction;
{
  FILE *file;
  char *name;
  struct pipe *pipe;
  pipemode mode;

  /* Decide on mode.   */

  if (strcmp (direction, "r") == 0)
    mode = READING_MODE;
  else if (strcmp (direction, "w") == 0)
    mode = WRITING_MODE;
  else
    return NULL;

  /* Get a name to use.  */

  if (name = tempnam (".", "pip"), name == NULL)
    return NULL;

  /* If we're reading, just call system to get a file filled with output.  */

  if (mode == READING_MODE)
    {
      char *string = (char *)
	xmalloc (4 + strlen (command) + strlen (name));

      sprintf (string, "%s > %s", command, name);
      system (string);
      free (string);

      if (file = fopen (name, "r"), file == NULL)
	return NULL;
    }
  else
    {
      if (file = fopen (name, "w"), file == NULL)
	return NULL;
    }

#ifndef _NFILE
  if (fileno (file) + 1 > pipes)
    {
      pipes = fileno (file) + 1;
      pipe_array = (struct pipe *)
	xrealloc (pipe_array, sizeof (struct pipe) * pipes);
    }
#endif
  pipe = pipe_array + fileno (file);
  if (mode == WRITING_MODE)
    pipe->command = xstrdup (command);
  pipe->name = name;
  pipe->mode = mode;

  return file;
}

/*-------------------------------------------------------------------.
| Complete the processing and unlink the work file associated with a |
| previous call to popen, which returned FILE.                       |
`-------------------------------------------------------------------*/

int
pclose (file)
     FILE *file;
{
  struct pipe *pipe = pipe_array + fileno (file);
  int status;

  switch (pipe->mode)
    {
    case UNOPENED:
      return -1;

    case READING_MODE:
      /* Input pipes are just files we're done with.  */

      status = fclose (file);
      break;

    case WRITING_MODE:
      /* Output pipes are temporary files we have to cram down the throats
	 of programs.  */

      fclose (file);

      {
	char *string = (char *)
	  xmalloc (4 + strlen (pipe->command) + strlen (pipe->name));

	sprintf (string, "%s < %s", pipe->command, pipe->name);
	free (pipe->command);

	status = system (string);
	free (string);
	break;
      }
    }

  /* Clean up current pipe.  */

  unlink (pipe->name);
  free (pipe->name);

  pipe->mode = UNOPENED;

  return status;
}
