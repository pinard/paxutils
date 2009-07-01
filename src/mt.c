/* mt -- control magnetic tape drive operation
   Copyright (C) 1991, 1992, 1995, 1998, 1999 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
   */

/* If -f is not given, the environment variable TAPE is used;
   if that is not set, a default device defined in sys/mtio.h is used.
   The device must be either a character special file or a remote
   tape drive with the form "[user@]system:path".
   The default count is 1.  Some operations ignore it.

   Exit status:
   0	success
   1	invalid operation or device name
   2	operation failed

   Operations (unique abbreviations are accepted):
   eof, weof	Write COUNT EOF marks at current position on tape.
   fsf		Forward space COUNT files.
		Tape is positioned on the first block of the file.
   bsf		Backward space COUNT files.
		Tape is positioned on the first block of the file.
   fsr		Forward space COUNT records.
   bsr		Backward space COUNT records.
   bsfm		Backward space COUNT file marks.
		Tape is positioned on the beginning-of-the-tape side of
		the file mark.
   asf		Absolute space to file number COUNT.
		Equivalent to rewind followed by fsf COUNT.
   eom		Space to the end of the recorded media on the tape
		(for appending files onto tapes).
   rewind	Rewind the tape.
   offline, rewoffl
		Rewind the tape and, if applicable, unload the tape.
   status	Print status information about the tape unit.
   retension	Rewind the tape, then wind it to the end of the reel,
		then rewind it again.
   erase	Erase the tape.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include "system.h"

#include <getopt.h>
#include "rmt.h"

/* The name this program was run with.  */
char *program_name;

/* If false, don't consider file names that contain a `:' to be
   on remote hosts; all files are local.  Always false for mt;
   since when do local device names contain colons?  */
static bool force_local_option = false;

char *opnames[] =
{
  "eof", "weof", "fsf", "bsf", "fsr", "bsr",
  "rewind", "offline", "rewoffl", "eject", "status",
#ifdef MTBSFM
  "bsfm",
#endif
#ifdef MTEOM
  "eom",
#endif
#ifdef MTRETEN
  "retension",
#endif
#ifdef MTERASE
  "erase",
#endif
  "asf",
#ifdef MTFSFM
  "fsfm",
#endif
#ifdef MTSEEK
  "seek",
#endif
  NULL
};

#define MTASF 600		/* Random unused number.  */
short operations[] =
{
  MTWEOF, MTWEOF, MTFSF, MTBSF, MTFSR, MTBSR,
  MTREW, MTOFFL, MTOFFL, MTOFFL, MTNOP,
#ifdef MTBSFM
  MTBSFM,
#endif
#ifdef MTEOM
  MTEOM,
#endif
#ifdef MTRETEN
  MTRETEN,
#endif
#ifdef MTERASE
  MTERASE,
#endif
  MTASF,
#ifdef MTFSFM
  MTFSFM,
#endif
#ifdef MTSEEK
  MTSEEK,
#endif
  0
};

void
check_type (char *dev, int desc)
{
  struct stat stat_info;

  if (_isrmt (desc))
    return;
  if (fstat (desc, &stat_info) == -1)
    error (1, errno, "%s", dev);
  if ((stat_info.st_mode & S_IFMT) != S_IFCHR)
    error (1, 0, _("%s is not a character special file"), dev);
}

void
perform_operation (char *dev, int desc, short op, int count)
{
  struct mtop control;

  control.mt_op = op;
  control.mt_count = count;
  if (rmtioctl (desc, MTIOCTOP, (char *) &control))
    error (2, errno, "%s", dev);
}

void
print_status (char *dev, int desc)
{
  struct mtget status;

  if (rmtioctl (desc, MTIOCGET, (char *) &status))
    error (2, errno, "%s", dev);

  printf (_("drive type = %d\n"), (int) status.mt_type);
#if defined(hpux) || defined(__hpux__)
  printf (_("drive status (high) = %d\n"), (int) status.mt_dsreg1);
  printf (_("drive status (low) = %d\n"), (int) status.mt_dsreg2);
#else
  printf (_("drive status = %d\n"), (int) status.mt_dsreg);
#endif
  printf (_("sense key error = %d\n"), (int) status.mt_erreg);
  printf (_("residue count = %d\n"), (int) status.mt_resid);
#if !defined(ultrix) && !defined(__ultrix__) && !defined(hpux) && !defined(__hppux) && !defined(__osf__)
  printf (_("file number = %d\n"), (int) status.mt_fileno);
  printf (_("block number = %d\n"), (int) status.mt_blkno);
#endif
}

void
usage (fp, status)
  FILE *fp;
  int status;
{
  fprintf (fp, _("\
Usage: %s [-V] [-f device] [--file=device] [--help] [--version] operation [count]\n"),
	   program_name);
  exit (status);
}

struct option longopts[] =
{
  {"file", 1, NULL, 'f'},
  {"version", 0, NULL, 'V'},
  {"help", 0, NULL, 'H'},
  {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
  short operation;
  int count;
  char *tapedev;
  int tapedesc;
  int i;

#if DOSWIN
  program_name = get_program_base_name (argv[0]);
#else
  program_name = argv[0];
#endif

  tapedev = NULL;
  count = 1;

  while ((i = getopt_long (argc, argv, "f:t:V:H", longopts, (int *) 0)) != -1)
    {
      switch (i)
	{
	case 'f':
	case 't':
	  tapedev = optarg;
	  break;

	case 'V':
	  printf ("mt (Free %s) %s\n", PACKAGE, VERSION);
	  exit (0);
	  break;

	case 'H':
	default:
	  usage (stdout, 0);
	}
    }

  if (optind == argc)
    usage (stderr, 1);

  i = argmatch (argv[optind], opnames);
  if (i < 0)
    {
      invalid_arg (_("tape operation"), argv[optind], i);
      exit (1);
    }
  operation = operations[i];

  if (++optind < argc)
    count = atoi (argv[optind]);
  if (++optind < argc)
    usage (stderr, 1);

  if (tapedev == NULL)
    {
      tapedev = getenv ("TAPE");
      if (tapedev == NULL)
#ifdef DEFTAPE			/* From sys/mtio.h.  */
        tapedev = DEFTAPE;
#else
	error (1, 0, _("no tape device specified"));
#endif
    }

  if (operation == MTWEOF
#ifdef MTERASE
      || operation == MTERASE
#endif
      )
    tapedesc = rmtopen (tapedev, O_WRONLY, 0, NULL);
  else
    tapedesc = rmtopen (tapedev, O_RDONLY, 0, NULL);
  if (tapedesc == -1)
    error (1, errno, "%s", tapedev);

  check_type (tapedev, tapedesc);

  if (operation == MTASF)
    {
      perform_operation (tapedev, tapedesc, MTREW, 1);
      operation = MTFSF;
    }
  perform_operation (tapedev, tapedesc, operation, count);
  if (operation == MTNOP)
    print_status (tapedev, tapedesc);

  if (rmtclose (tapedesc) == -1)
    error (2, errno, "%s", tapedev);

  exit (0);
}
