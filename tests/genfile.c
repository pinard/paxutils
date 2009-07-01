/* Generate a file containing some preset patterns.
   Copyright � 1995, 1996, 1997 Free Software Foundation, Inc.
   Fran�ois Pinard <pinard@iro.umontreal.ca>, 1995.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "system.h"

#include <getopt.h>

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

enum pattern
{
  DEFAULT,
  ZEROS
};

/* The name this program was run with. */
const char *program_name;

/* If nonzero, display usage information and exit.  */
static int show_help = 0;

/* If nonzero, print the version on standard output and exit.  */
static int show_version = 0;

/* Length of file to generate.  */
static int file_length = 0;

/* Pattern to generate.  */
static enum pattern pattern = DEFAULT;

/*-----------------------------------------------.
| Explain how to use the program, then get out.	 |
`-----------------------------------------------*/

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Generate data files for GNU tar test suite.\n"));
      printf (_("\
\n\
Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
If a long option shows an argument as mandatory, then it is mandatory\n\
for the equivalent short option also.\n\
\n\
  -l, --file-length=LENGTH   LENGTH of generated file\n\
  -p, --pattern=PATTERN      PATTERN is `default' or `zeros'\n\
      --help                 display this help and exit\n\
      --version              output version information and exit\n"),
	     stdout);
    }
  exit (status);
}

/*----------------------------------------------------------------------.
| Main program.  Decode ARGC arguments passed through the ARGV array of |
| strings, then launch execution.				        |
`----------------------------------------------------------------------*/

/* Long options equivalences.  */
static const struct option long_options[] =
{
  {"help", no_argument, &show_help, 1},
  {"length", required_argument, NULL, 'l'},
  {"pattern", required_argument, NULL, 'p'},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0},
};


const char *pattern_strings[] =
{
  "default",			/* 0 */
  "zeros",			/* 1 */
  NULL
};

int
main (int argc, char *const *argv)
{
  int option_char;		/* option character */
  int counter;			/* general purpose counter */

  /* Decode command options.  */

  program_name = argv[0];
  setlocale (LC_ALL, "");

  while (option_char = getopt_long (argc, argv, "l:p:", long_options, NULL),
	 option_char != EOF)
    switch (option_char)
      {
      default:
	usage (EXIT_FAILURE);

      case '\0':
	break;

      case 'l':
	file_length = atoi (optarg);
	break;

      case 'p':
	switch (argmatch (optarg, pattern_strings))
	  {

	  case -2:
	    error (0, 0, _("Ambiguous pattern `%s'"), optarg);
	    usage (EXIT_FAILURE);

	  case -1:
	    error (0, 0, _("Unknown pattern `%s'"), optarg);
	    usage (EXIT_FAILURE);

	  case 0:
	    pattern = DEFAULT;
	    break;

	  case 1:
	    pattern = ZEROS;
	    break;
	  }
	break;
      }

  /* Process trivial options.  */

  if (show_version)
    {
      printf ("genfile (GNU %s) %s\n", PACKAGE, VERSION);
      fputs (_("\
\n\
Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.\n"),
	     stdout);
      fputs (_("\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"),
	     stdout);
      fputs (_("\
\n\
Written by Fran�ois Pinard <pinard@iro.umontreal.ca>.\n"),
	     stdout);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  if (optind < argc)
    usage (EXIT_FAILURE);

  /* Generate file.  */

  switch (pattern)
    {
    case DEFAULT:
      for (counter = 0; counter < file_length; counter++)
	putchar (counter & 255);
      break;

    case ZEROS:
      for (counter = 0; counter < file_length; counter++)
	putchar (0);
      break;
    }

  exit (0);
}
