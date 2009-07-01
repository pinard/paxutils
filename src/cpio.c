/* Main program and argument processing for cpio.
   Copyright (C) 1990, 91, 92, 95, 98, 99 Free Software Foundation, Inc.

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

/* Written by Phil Nelson <phil@cs.wwu.edu>,
   David MacKenzie <djm@gnu.ai.mit.edu>,
   and John Oleynick <juo@klinzhai.rutgers.edu>.  */

#include "system.h"
#include "common.h"

#include <getopt.h>
#include "filetypes.h"
#include "rmt.h"

#define BLOCK_SIZE_OPTION    130
#define VERSION_OPTION       131
#define HELP_OPTION          132
#define NO_PRESERVE_OWNER_OPTION    134
#define NO_ABSOLUTE_FILENAMES_OPTION     135
#define ONLY_VERIFY_CRC_OPTION     136
#define RENAME_BATCH_FILE_OPTION   137
#define QUIET_OPTION         138
#define SPARSE_OPTION        139

static struct option long_opts[] =
{
  {"append", 0, 0, 'A'},
  {"block-size", 1, 0, BLOCK_SIZE_OPTION},
  {"create", 0, 0, 'o'},
#ifdef DEBUG_CPIO
  {"debug", 0, (int *) &debug_option, true},
#endif
  {"dereference", 0, 0, 'L'},
  {"dot", 0, 0, 'V'},
  {"extract", 0, 0, 'i'},
  {"file", 1, 0, 'F'},
  {"force-local", 0, (int *) &force_local_option, true},
  {"format", 1, 0, 'H'},
  {"help", 0, 0, HELP_OPTION},
  {"io-size", 1, 0, 'C'},
  {"link", 0, (int *) &link_option, true},
  {"list", 0, (int *) &list_option, true},
  {"make-directories", 0, (int *) &make_directories_option, true},
  {"message", 1, 0, 'M'},
  {"no-absolute-filenames", 0, 0, NO_ABSOLUTE_FILENAMES_OPTION},
  {"no-preserve-owner", 0, 0, NO_PRESERVE_OWNER_OPTION},
  {"nonmatching", 0, (int *) &copy_matching_files, false},
  {"null", 0, 0, '0'},
  {"numeric-uid-gid", 0, (int *) &numeric_uid_gid_option, true},
  {"only-verify-crc", 0, 0, ONLY_VERIFY_CRC_OPTION},
  {"owner", 1, 0, 'R'},
  {"pass-through", 0, 0, 'p'},
  {"pattern-file", 1, 0, 'E'},
  {"preserve-modification-time", 0,
   (int *) &preserve_modification_time_option, true},
  {"quiet", 0, 0, QUIET_OPTION},
  {"rename", 0, (int *) &rename_option, true},
  {"rename-batch-file", 1, 0, RENAME_BATCH_FILE_OPTION},
  {"reset-access-time", 0, (int *) &reset_access_time_option, true},
  {"silent", 0, 0, QUIET_OPTION},
  {"sparse", 0, 0, SPARSE_OPTION},
  {"swap", 0, 0, 'b'},
  {"swap-bytes", 0, 0, 's'},
  {"swap-halfwords", 0, 0, 'S'},
  {"unconditional", 0, (int *) &unconditional_option, true},
  {"verbose", 0, (int *) &verbose_option, true},
  {"version", 0, 0, VERSION_OPTION},
  {0, 0, 0, 0}
};

/*  Print usage message and exit with error.  */

static void
usage (fp, status)
  FILE *fp;
  int status;
{
  fputs (_("Usage: cpio [OPTION]... [EXTRA]...\n"), fp);

  fputs (_("\
Mandatory or optional arguments to long options are mandatory or optional\n\
for short options too.\n"),
	     fp);

  fputs (_("\
\n\
Main operation mode:\n\
 -o, --create             create a new archive (copy-out)\n\
                           archive on stdout unless specified\n\
                           list of filenames on stdin\n\
 -i, --extract            extract files from an archive (copy-in)\n\
                           archive on stdin unless specified\n\
                           EXTRA argumentss are list of patterns to match\n\
 -p, --pass-through       copy files (copy-pass)\n\
                           list of filenames on stdin\n\
                           EXTRA argument is destination directory\n"),
	 fp);

  fputs (_("\nOther options:\n"), fp);
  fputs (_("\
 -0, --null               read null-terminated names\n"), fp);
  fputs (_("\
 -a, --reset-access-time  reset access time after reading file\n"), fp);
  fputs (_("\
 -A, --append             append to existing archive (copy-out)\n"), fp);
  fputs (_("\
 -b, --swap               swap both words and bytes of data\n"), fp);
  fputs (_("\
 -B                       set block size to 5120 bytes\n"), fp);
  fputs (_("\
 --block-size=SIZE        set block size to 512 * SIZE bytes\n"), fp);
  fputs (_("\
 -c                       use old portable (ASCII) format\n"), fp);
  fputs (_("\
 -C SIZE, --io-size=SIZE  set block size to SIZE bytes\n"), fp);
  fputs (_("\
 -d, --make-directories   make directories as needed\n"), fp);
  fputs (_("\
 -E FILE, --pattern-file=FILE\n\
                          read filename patterns from FILE\n"), fp);
  fputs (_("\
 -f, --nonmatching        only copy files that do not match a pattern\n"), fp);
  fputs (_("\
 -F FILE, --file=FILE     use FILE for archive\n"), fp);
  fputs (_("\
 --force-local            archive file is local even if has a colon\n"), fp);
  fputs (_("\
 -H FORMAT, --format=FORMAT\n\
                          use archive format FORMAT\n"), fp);
  fputs (_("\
 --help                   print this help and exit\n"), fp);
  fputs (_("\
 -I FILE                  use FILE as input archive (copy-in)\n"), fp);
  fputs (_("\
 -l, --link               link files instead of copying\n"), fp);
  fputs (_("\
 -L, --dereference        don't dump symlinks; dump the files they point to\n"),
	 fp);
  fputs (_("\
 -m, --preserve-modification-time\n\
                          retain modification time when creating files\n"),
	 fp);
  fputs (_("\
 -M MESSAGE, --message=MESSAGE\n\
                          print message when end of volume reached\n"), fp);
  fputs (_("\
 -n, --numeric-uid-gid    show numeric uid and gid\n"), fp);
  fputs (_("\
 --no-absolute-filenames  create all files relative to cwd (copy-in)\n"),
	 fp);
  fputs (_("\
 --no-preserve-owner      do not change ownership of files\n"), fp);
  fputs (_("\
 -O FILE                  use FILE as output archive (copy-out)\n"), fp);
  fputs (_("\
 --only-verify-crc        verify CRCs but do not extract (copy-out)\n"),
	 fp);
  fputs (_("\
 --quiet, --silent        do not print number of blocks copied\n"), fp);
  fputs (_("\
 -r, --rename             interactively rename files\n"), fp);
  fputs (_("\
 -R [USER][:.][GROUP], --owner=[USER][:.][GROUP]\n\
                          set ownership of new files to USER, GROUP\n"), fp);
  fputs (_("\
 -s, --swap-bytes         swap bytes of each halfword in files (copy-in)\n"),
	 fp);
  fputs (_("\
 -S, --swap-halfwords     swap halfwords of each word in files (copy-in)\n"),
	 fp);
  fputs (_("\
 --sparse                 write sparse files (copy-in, copy-pass)\n"), fp);
  fputs (_("\
 -t, --list               print table of contents of input\n"), fp);
  fputs (_("\
 -u, --unconditional      replace all files, even if older\n"), fp);
  fputs (_("\
 -v, --verbose            list files processed\n"), fp);
  fputs (_("\
 -V, --dot                print a `.' for each file processed\n"), fp);
  fputs (_("\
 --version                print version information and exit\n"), fp);

  fputs (_("\nReport bugs to <paxutils-bugs@iro.umontreal.ca>\n"), fp);

  exit (status);
}

/* Process the arguments.  Set all options and set up the copy pass
   directory or the copy in patterns.  */

void
process_args (int argc, char *argv[])
{
  void (*copy_in) ();		/* Work around for pcc bug.  */
  void (*copy_out) ();
  int c;
  char *input_archive_name = 0;
  char *output_archive_name = 0;

  if (argc < 2)
    usage (stderr, 2);

  xstat = lstat;

  while ((c = getopt_long (argc, argv,
			   "0aAbBcC:dfE:F:H:iI:lLmM:noO:prR:sStuvVz",
			   long_opts, (int *) 0)) != -1)
    {
      switch (c)
	{
	case 0:			/* A long option that just sets a flag.  */
	  break;

	case '0':		/* Read null-terminated filenames.  */
	  name_end = '\0';
	  break;

	case 'a':		/* Reset access times.  */
	  reset_access_time_option = true;
	  break;

	case 'A':		/* Append to the archive.  */
	  append_option = true;
	  break;

	case 'b':		/* Swap bytes and halfwords.  */
	  swap_bytes_option = true;
	  swap_halfwords_option = true;
	  break;

	case 'B':		/* Set block size to 5120.  */
	  io_block_size = 10 * BLOCKSIZE;
	  break;

	case BLOCK_SIZE_OPTION:
	  io_block_size = atoi (optarg);
	  if (io_block_size < 1)
	    error (2, 0, _("invalid block size"));
	  io_block_size *= BLOCKSIZE;
	  break;

	case 'c':		/* Use the old portable ASCII format.  */
	  if (archive_format != UNKNOWN_FORMAT)
	    error (2, 0, _("archive format specified twice"));
#ifdef SVR4_COMPAT
	  archive_format = NEW_ASCII_FORMAT; /* -H newc.  */
#else
	  archive_format = OLD_ASCII_FORMAT; /* -H odc.  */
#endif
	  break;

	case 'C':		/* Block size.  */
	  io_block_size = atoi (optarg);
	  if (io_block_size < 1)
	    error (2, 0, _("invalid block size"));
	  break;

	case 'd':		/* Create directories where needed.  */
	  make_directories_option = true;
	  break;

	case 'f':		/* Only copy files not matching patterns.  */
	  copy_matching_files = false;
	  break;

	case 'E':		/* Pattern file name.  */
	  pattern_file_name = optarg;
	  break;

	case 'F':		/* Archive file name.  */
	  archive_name = optarg;
	  break;

	case 'H':		/* Header format name.  */
	  if (archive_format != UNKNOWN_FORMAT)
	    error (2, 0, _("archive format specified twice"));

	  archive_format = find_format (optarg);
	  if (archive_format == UNKNOWN_FORMAT)
	    format_error (optarg);
	  break;

	case 'i':		/* Copy-in mode.  */
	  if (copy_function != 0)
	    error (2, 0, _("only one of -o, -i, -p allowed"));
	  copy_function = process_copy_in;
	  break;

	case 'I':		/* Input archive file name.  */
	  input_archive_name = optarg;
	  break;

	case 'k':		/* Handle corrupted archives.  We always handle
				   corrupted archives, but recognize this
				   option for compatability.  */
	  break;

	case 'l':		/* Link files when possible.  */
	  link_option = true;
	  break;

	case 'L':		/* Dereference symbolic links.  */
	  xstat = stat;
	  break;

	case 'm':		/* Retain previous file modify times.  */
	  preserve_modification_time_option = true;
	  break;

	case 'M':		/* New media message.  */
	  set_new_media_message (optarg);
	  break;

	case 'n':		/* Long list owner and group as numbers.  */
	  numeric_uid_gid_option = true;
	  break;

	case NO_ABSOLUTE_FILENAMES_OPTION: /* --no-absolute-filenames */
	  no_absolute_filenames_option = true;
	  break;

	case NO_PRESERVE_OWNER_OPTION:	/* --no-preserve-owner */
	  if (set_owner_flag || set_group_flag)
	    error (2, 0, _("only one of --no-preserve-owner and -R allowed"));
	  no_preserve_owner_option = true;
	  break;

	case 'o':		/* Copy-out mode.  */
	  if (copy_function != 0)
	    error (2, 0, _("only one of -o, -i, -p allowed"));
	  copy_function = process_copy_out;
	  break;

	case 'O':		/* Output archive file name.  */
	  output_archive_name = optarg;
	  break;

	case ONLY_VERIFY_CRC_OPTION:
	  only_verify_crc_option = true;
	  break;

	case 'p':		/* Copy-pass mode.  */
	  if (copy_function != 0)
	    error (2, 0, _("only one of -o, -i, -p allowed"));
	  copy_function = process_copy_pass;
	  break;

	case 'r':		/* Interactively rename.  */
	  rename_option = true;
	  break;

	case RENAME_BATCH_FILE_OPTION:
	  rename_batch_file = optarg;
	  break;

	case QUIET_OPTION:
	  quiet_option = true;
	  break;

	case 'R':		/* Set the owner.  */
	  if (no_preserve_owner_option)
	    error (2, 0, _("only one of --no-preserve-owner and -R allowed"));
#ifndef __MSDOS__
	  {
	    const char *e;
	    char *u;
	    char *g;

	    e = parse_user_spec (optarg, &set_owner, &set_group, &u, &g);
	    if (e)
	      error (2, 0, "%s: %s", optarg, e);
	    if (u)
	      {
		free (u);
		set_owner_flag = true;
	      }
	    if (g)
	      {
		free (g);
		set_group_flag = true;
	      }
	  }
#endif
	  break;

	case 's':		/* Swap bytes.  */
	  swap_bytes_option = true;
	  break;

	case 'S':		/* Swap halfwords.  */
	  swap_halfwords_option = true;
	  break;

	case 't':		/* Only print a list.  */
	  list_option = true;
	  break;

	case 'u':		/* Replace all!  Unconditionally!  */
	  unconditional_option = true;
	  break;

	case 'v':		/* Verbose!  */
	  verbose_option = true;
	  break;

	case 'V':		/* Print `.' for each file.  */
	  dot_option = true;
	  break;

	case VERSION_OPTION:
	  printf (_("cpio (Free %s) %s\n"), PACKAGE, VERSION);
	  exit (0);
	  break;

	case SPARSE_OPTION:
	  sparse_option = true;
	  break;

	case HELP_OPTION:	/* --help */
	  usage (stdout, 0);
	  break;

	case '?':
	default:
	  fprintf (stderr, _("Try `%s --help' for more information.\n"),
		   program_name);
	  exit (2);
	}
    }

  /* Do error checking and look at other args.  */

  if (copy_function == 0)
    {
      if (list_option)
	copy_function = process_copy_in;
      else
	error (2, 0, _("one of -o, -i, -p must be specified"));
    }

  if (!(list_option && verbose_option) && numeric_uid_gid_option)
    error (2, 0, _("-n only makes sense with -t and -v"));

  /* Work around for pcc bug.  */
  copy_in = process_copy_in;
  copy_out = process_copy_out;

  if (copy_function == copy_in)
    {
      archive_des = 0;
      /* FIXME should indicate particular error.  */
      if (link_option || reset_access_time_option || xstat != lstat || append_option
	  || output_archive_name
	  || (archive_name && input_archive_name))
	error (2, 0, _("option conflicts with -i"));
      if (archive_format == CRC_ASCII_FORMAT)
	crc_i_flag = true;
      num_patterns = argc - optind;
      save_patterns = &argv[optind];
      if (input_archive_name)
	archive_name = input_archive_name;
    }
  else if (copy_function == copy_out)
    {
      archive_des = 1;
      /* FIXME should indicate particular error.  */
      if (argc != optind || make_directories_option || rename_option
	  || sparse_option || list_option || unconditional_option || link_option
	  || preserve_modification_time_option || no_preserve_owner_option || set_owner_flag
	  || set_group_flag || swap_bytes_option || swap_halfwords_option
	  || (append_option && !(archive_name || output_archive_name))
	  || rename_batch_file || no_absolute_filenames_option
	  || input_archive_name || (archive_name && output_archive_name))
	error (2, 0, _("option conflicts with -o"));
      if (archive_format == UNKNOWN_FORMAT)
	archive_format = BINARY_FORMAT;
      set_write_pointers_from_format (archive_format);
      if (output_archive_name)
	archive_name = output_archive_name;
    }
  else
    {
      /* Copy pass.  */
      archive_des = -1;
      /* FIXME should indicate particular error.  */
      if (argc - 1 != optind || archive_format != UNKNOWN_FORMAT
	  || swap_bytes_option || swap_halfwords_option
	  || list_option || rename_option || append_option
	  || rename_batch_file || no_absolute_filenames_option)
	error (2, 0, _("option conflicts with -p"));
      directory_name = argv[optind];
    }

  if (archive_name)
    {
      if (copy_function != copy_in && copy_function != copy_out)
	error (2, 0, _("can't specify archive name with -p"));
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
# if 0
  program_name = get_program_base_name (argv[0]);
# else
  /* Drop the full path and .exe suffix.  */
  {
    char *p = program_name + strlen (program_name) - 4;

    if (p > program_name
	&& (strcmp (p, ".exe") == 0 || strcmp (p, ".EXE") == 0))
      *p-- = '\0';
    else
      p += 3;

    for (; p > program_name; p--)
      if (*p == '/' || *p == '\\' || *p == ':')
	{
	  program_name = p + 1;
	  break;
	}
  }
# endif
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
