/* Main program and argument processing for pax.
   Copyright (C) 1995, 1997, 1998, 1999 Free Software Foundation, Inc.

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
#include "common.h"

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "filetypes.h"
#include "rmt.h"

#define HELP_OPTION	130
#define VERSION_OPTION	131

static struct option long_opts[] =
{
  {"null", 0, 0, '0'},
  {"append", no_argument, (int *) &append_option, true},
  {"block-size", required_argument, 0, 'b'},
  {"nonmatching", no_argument, (int *) &copy_matching_files, true},
  {"directories-only", no_argument, 0, 'd'},
  {"file", required_argument, 0, 'f'},
  {"rename", no_argument, (int *) &rename_option, true},
  {"no-overwrite", no_argument, 0, 'k'},
  {"link", no_argument, (int *) &link_option, true},
  {"dereference", 0, 0, 'L'},
  {"first-pattern", no_argument, 0, 'n'},
  {"privileges", required_argument, 0, 'p'},
  {"read", no_argument, 0, 'r'},
  {"replace", required_argument, 0, 's'},
  {"reset-access-time", no_argument, (int *) &reset_access_time_option, true},
  {"keep-old-files", no_argument, (int *) &unconditional_option, false},
  {"verbose", no_argument, (int *) &verbose_option, true},
  {"write", no_argument, 0, 'w'},
  {"format", required_argument, 0, 'x'},
  /* I like this, but maybe tar compatibility is better?  */
  /* {"xdev", no_argument, 0, 'X'}, */
  {"one-file-system", required_argument, 0, 'X'},
#ifdef DEBUG_CPIO
  {"debug", 0, &debug_option, true},
#endif
  {"help", 0, 0, HELP_OPTION},
  {"version", 0, 0, VERSION_OPTION},
  {0, 0, 0, 0}
};

static void
usage (FILE *fp, int status)
{
  fputs (_("Usage: pax [OPTION]... [FILE]... [DIRECTORY]\n"), fp);

  fputs (_("\
Mandatory or optional arguments to long options are mandatory or optional\n\
for short options too.\n"),
	     fp);

  fputs (_("\
\n\
Main operation mode:\n\
 -r, --read               extract files from an archive\n\
 -w, --write              create a new archive\n\
 with neither -r nor -w   list contents of archive\n\
 with both -r and -w      copy files\n"),
	 fp);

  fputs (_("\nOther options:\n"), fp);
  fputs (_("\
 -0, --null               read null-terminated names\n"), fp);
  fputs (_("\
 -a, --append             append to archive\n"), fp);
  fputs (_("\
 -b N, --block-size=N     set block size to N\n"), fp);
  fputs (_("\
 -c, --nonmatching        only copy files that do not match a pattern\n"), fp);
#ifdef DEBUG_CPIO
  fputs (_("\
 --debug                  turn on debugging statements\n"), fp);
#endif
  fputs (_("\
 -d, --directories-only   don't recurse into directories while writing\n"),
	 fp);
  fputs (_("\
 -f FILE, --file=FILE     input or output file\n"), fp);
  fputs (_("\
 --help                   print this help and exit\n"), fp);
  fputs (_("\
 -i, --rename             interactively rename\n"), fp);
  fputs (_("\
 -k, --no-overwrite       don't overwrite existing file on extraction\n"), fp);
  fputs (_("\
 -l, --link               link files instead of copying\n"), fp);
  fputs (_("\
 -L, --dereference        don't dump symlinks; dump the files they point to\n"),
	 fp);
  fputs (_("\
 -n, --first-pattern      allow each pattern to match only once\n"), fp);
  fputs (_("\
 -p, --privileges         preserve permissions on extraction\n"), fp);
  fputs (_("\
 -s REPLACEMENT, --replace=REPLACEMENT\n\
                          rename files according to REPLACEMENT\n"), fp);
  fputs (_("\
 -t, --reset-access-time  reset access time on extraction\n"), fp);
  fputs (_("\
 -u, --keep-old-files     only replace file if replacement is newer\n"), fp);
  fputs (_("\
 -v, --verbose            be verbose\n"), fp);
  fputs (_("\
 --version                print version information and exit\n"), fp);
  fputs (_("\
 -x FORMAT, --format=FORMAT\n\
                          set input or output format\n"), fp);
  fputs (_("\
 -X, --one-file-system    don't cross file systems\n"), fp);

  fputs (_("\nReport bugs to <paxutils-bugs@iro.umontreal.ca>\n"), fp);

  exit (status);
}

/*--------------------------------------------------------------------------.
| Parse a buffer size string.  These strings look like so: NUM{xNUM}.  NUM  |
| looks like [0-9]+[kb]?.  We extend this to allow any number of numbers to |
| be multiplied together.                                                   |
`--------------------------------------------------------------------------*/

static int
parse_buffer_size (char *str)
{
  char *num = str;
  char save;
  int product = 1;
  int value;

  while (*num)
    {
      str = num;

      /* Extract one number.  */
      while (true)
	{
	  if (!isascii (*num))
	    error (2, 0, _("parse error in blocksize"));

	  if (isdigit (*num))
	    num++;
	  else if (!*num || *num == 'k' || *num == 'b' || *num == 'x')
	    break;
	  else
	    error (2, 0, _("parse error in blocksize"));
	}

      save = *num;
      *num = '\0';

      value = atoi (str);
      if (!value)
	error (2, 0, _("block size cannot be 0"));

      product *= value;
      if (save == 'k')
	{
	  product *= 1024;
	  num++;
	}
      else if (save == 'b')
	{
	  product *= 512;	/* FIXME: use a symbol */
	  num++;
	}
      else if (save == 'x')
	num++;
    }

  return product;
}

void
process_args (int argc, char *argv[])
{
  void (*copy_in) ();		/* Work around for pcc bug.  */
  void (*copy_out) ();
  void (*copy_pass) ();
  int c;

  /* Set up pax defaults.  */
  xstat = lstat;
  unconditional_option = true;
  quiet_option = true;
  directory_recurse_flag = true;
  reset_access_time_option = true;
  preserve_modification_time_option = true;
  preserve_mode_flag = false;
  pax_rename_option = true;

  /* Work around for pcc bug.  */
  copy_in = process_copy_in;
  copy_out = process_copy_out;
  copy_pass = process_copy_pass;

  while ((c = getopt_long (argc, argv,
			   "ab:cdf:iklLno:p:rs:tuvVwx:X",
			   long_opts, (int *) 0)) != -1)
    {
      switch (c)
	{
	case 0:
	  /* Already processed by getopt_long.  */
	  break;

	case '0':
	  name_end = '\0';
	  break;

	case 'a':		/* Append to the archive.  */
	  append_option = true;
	  break;

	case 'b':		/* Block size.  */
	  io_block_size = parse_buffer_size (optarg);
	  if (io_block_size < 1)
	    error (2, 0, _("invalid block size"));
	  break;

	case 'c':
	  copy_matching_files = false;
	  break;

	case 'd':
	  directory_recurse_flag = false;
	  break;

	case 'f':
	  archive_name = optarg;
	  break;

	case 'i':
	  rename_option = true;
	  break;

	case 'k':
	  overwrite_existing_files = false;
	  break;

	case 'l':
	  link_option = true;
	  break;

	case 'L':
	  xstat = stat;
	  break;

	case 'n':
	  /* FIXME */
	  break;

	case 'o':
	  /* We don't understand any extra options.  So we just ignore
	     this.  */
	  break;

	case 'p':
	  /* Process 1-char flags.  */
	  for (; *optarg; optarg++)
	    {
	      switch (*optarg)
		{
		case 'a':
		  reset_access_time_option = false;
		  break;

		case 'e':
		  reset_access_time_option = true;
		  preserve_modification_time_option = true;
		  set_owner_flag = false;
		  set_group_flag = false;
		  no_preserve_owner_option = false;
		  preserve_mode_flag = true;
		  break;

		case 'm':
		  preserve_modification_time_option = false;
		  break;

		case 'o':
		  set_owner_flag = false;
		  set_group_flag = false;
		  no_preserve_owner_option = false;
		  break;

		case 'p':
		  preserve_mode_flag = true;
		  break;

		default:
		  error (2, 0, _("\
unrecognized flag `%c' for -p; recognized flags are `aemop'"),
			 *optarg);
		  break;
		}
	    }
	  break;

	case 'r':
	  if (copy_function == copy_out)
	    copy_function = copy_pass;
	  else if (copy_function == 0)
	    copy_function = copy_in;
	  else
	    usage (stderr, 2);
	  break;

	case 's':
	  add_rename_regexp (optarg);
	  break;

	case 't':
	  reset_access_time_option = true;
	  break;

	case 'u':
	  unconditional_option = false;
	  break;

	case 'v':
	  verbose_option = true;
	  break;

	case 'w':
	  if (copy_function == copy_in)
	    copy_function = copy_pass;
	  else if (copy_function == 0)
	    copy_function = copy_out;
	  else
	    usage (stderr, 2);
	  break;

	case 'x':
	  if (archive_format != UNKNOWN_FORMAT)
	    usage (stderr, 2);

	  archive_format = find_format (optarg);
	  if (archive_format == UNKNOWN_FORMAT)
	    format_error (optarg);
	  break;

	case 'X':
	  cross_device_flag = false;
	  break;

	case HELP_OPTION:
	  usage (stdout, 0);
	  break;

	case VERSION_OPTION:
	  printf ("pax (Free %s) %s\n", PACKAGE, VERSION);
	  exit (0);
	  break;

	default:
	  error (2, 0, _("unrecognized option -- `%c'"), c);
	}
    }

  /* Sanity checks and cleanups.  */
  if (copy_function == 0)
    {
      /* FIXME we don't check for all illegal options here.  */
      if (name_end == '\0' || append_option || rename_option || link_option)
	usage (stderr, 2);

      list_option = true;
      copy_function = copy_in;
      num_patterns = argc - optind;
      save_patterns = &argv[optind];
      if (archive_format == CRC_ASCII_FORMAT)
	crc_i_flag = true;
    }
  else if (copy_function == copy_in)
    {
      /* -r */
      archive_des = 0;
      if (link_option || xstat != lstat || append_option)
	usage (stderr, 2);
      num_patterns = argc - optind;
      save_patterns = &argv[optind];
      if (archive_format == CRC_ASCII_FORMAT)
	crc_i_flag = true;
    }
  else if (copy_function == copy_out)
    {
      /* -w */
      archive_des = 1;
      if (0)
	usage (stderr, 2);
      if (archive_format == UNKNOWN_FORMAT)
	archive_format = POSIX_FORMAT;
      if (argc > optind)
	{
	  num_pax_file_names = argc - optind;
	  pax_file_names = &argv[optind];
	}
      set_write_pointers_from_format (archive_format);
    }
  else
    {
      /* Copy pass.  */
      assert (copy_function == copy_pass);

      archive_des = -1;
      if (optind == argc || archive_format != UNKNOWN_FORMAT || append_option)
	usage (stderr, 2);
      directory_name = argv[argc - 1];
      if (argc > optind + 1)
	{
	  num_pax_file_names = argc - optind - 1;
	  pax_file_names = &argv[optind];
	}
    }

  if (archive_name)
    {
      if (copy_function != copy_in && copy_function != copy_out)
	usage (stderr, 2);
      archive_des = open_archive (archive_name);
      if (archive_des < 0)
	error (1, errno, "%s", archive_name);
    }

#ifndef __MSDOS__
  /* Prevent SysV non-root users from giving away files inadvertantly.  This
     happens automatically on BSD, where only root can give away files.  */
  if (set_owner_flag == false && set_group_flag == false && geteuid ())
    no_preserve_owner_option = true;
#endif
}

int
main (int argc, char *argv[])
{
  reset_global_variables ();
#if DOSWIN
  program_name = get_program_base_name (argv[0]);
#else
  program_name = argv[0];
#endif

  umask (0);

  setlocale (LC_ALL, "");
  textdomain (PACKAGE);

#ifdef __TURBOC__
  _fmode = O_BINARY;		/* Put stdin and stdout in binary mode.  */
#endif
#ifdef __EMX__			/* gcc on OS/2.  */
  _response (&argc, &argv);
  _wildcard (&argc, &argv);
#endif

  process_args (argc, argv);

  initialize_buffers ();

  (*copy_function) ();

  if (archive_des >= 0 && rmtclose (archive_des) == -1)
    error (1, errno, _("error closing archive"));

  exit (0);
}
