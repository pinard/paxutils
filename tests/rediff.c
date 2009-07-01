/*----------------------------------------------------------------.
| A simple 'diff'-like program that accepts regular expressions.  |
`----------------------------------------------------------------*/

/* $ cat foo
   abc
   def
   gHi
   $ cat foore
   [aA][bB][cC]
   d.f
   ghi
   $ ./rediff foore foo
   < ghi
   --------------------
   > gHi
*/

/* Regular expressions may only occur in the first file.  It might be nice if
   rediff printed line numbers and could recognize when lines were inserted or
   deleted.  */

#include <stdio.h>
#include <sys/types.h>

#include "regex.h"

char *program_name;	/* if we have to use cpio's alloca(), it calls
			   xmalloc () which calls error() which wants
			   program_name.  */

main (argc, argv)
  int argc;
  char *argv [];
{
  FILE *fp1, *fp2;
  char buf1 [4096], buf2 [4096];
  int rc;
  char *s1, *s2;
  int diffs;

  program_name = argv [0];

  diffs = 0;

  if (strcmp (argv [1], "-") != 0)
    {
      fp1 = fopen (argv [1], "r");
      if (fp1 == NULL)
	{
	  perror (argv [1]);
	  exit (1);
	}
    }
  else
    fp1 = stdin;

  if (strcmp (argv [2], "-") != 0)
    {
      fp2 = fopen (argv [2], "r");
      if (fp2 == NULL)
	{
	  perror (argv [2]);
	  exit (1);
	}
    }
  else
    {
      if (fp1 == stdin)
	{
	  fprintf (stderr, "can only use stdin once.\n");
	  exit (1);
	}
      fp2 = NULL;
    }

  s1 = fgets (buf1, 4096, fp1);
  s2 = fgets (buf2, 4096, fp2);
  while (s1 && s2)
    {
      if (re_comp (buf1) != NULL)
	{
	  fprintf (stderr, "bogus regexp: %s\n", buf1);
	  exit (1);
	}
      rc = re_exec (buf2);
      if (rc < 0)
	{
	  fprintf (stderr, "internal regex error!\n");
	  exit (1);
	}
      if (rc == 0)
	{
	  ++diffs;
	  fprintf (stderr, "< %s--------------------\n> %s", buf1, buf2);
	}
      s1 = fgets (buf1, 4096, fp1);
      s2 = fgets (buf2, 4096, fp2);
    }

  if (s1)
    {
      fprintf (stderr, "< %s", buf1);
      while (s1 = fgets (buf1, 4096, fp1) )
	fprintf (stderr, "< %s", buf1);
    }
  if (s2)
    {
      fprintf (stderr, "> %s", buf2);
      while (s1 = fgets (buf2, 4096, fp2) )
	fprintf (stderr, "> %s", buf2);
    }
  fclose (fp1);
  fclose (fp2);
  if (diffs != 0)
    exit (2);

  exit (0);
}
