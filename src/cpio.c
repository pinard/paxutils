/* Main program and argument processing for cpio.
   Copyright (C) 1990, 1991, 1992, 1995, 1998 Free Software Foundation, Inc.

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

#include <getopt.h>
#include "filetypes.h"
#include "cpiohdr.h"
#include "extern.h"
#include "rmt.h"

#define OPTION_BLOCK_SIZE    130
#define OPTION_VERSION       131
#define OPTION_HELP          132
#define OPTION_NOPRESERVE    134
#define OPTION_NOABSFILE     135
#define OPTION_CRCVERIFY     136
#define OPTION_BATCHRENAME   137
#define OPTION_QUIET         138
#define OPTION_SPARSE        139

static struct option long_opts[] =
{
  {"null", 0, 0, '0'},
  {"append", 0, 0, 'A'},
  {"block-size", 1, 0, OPTION_BLOCK_SIZE},
  {"create", 0, 0, 'o'},
  {"dereference", 0, 0, 'L'},
  {"dot", 0, 0, 'V'},
  {"extract", 0, 0, 'i'},
  {"file", 1, 0, 'F'},
  {"force-local", 0, &f_force_local, 1},
  {"format", 1, 0, 'H'},
  {"help", 0, 0, OPTION_HELP},
  {"io-size", 1, 0, 'C'},
  {"link", 0, &link_flag, true},
  {"list", 0, &table_flag, true},
  {"make-directories", 0, &create_dir_flag, true},
  {"message", 1, 0, 'M'},
  {"no-absolute-names", 0, 0, OPTION_NOABSFILE},
  {"no-preserve-owner", 0, 0, OPTION_NOPRESERVE},
  {"nonmatching", 0, &copy_matching_files, false},
  {"numeric-uid-gid", 0, &numeric_uid, true},
  {"only-verify-crc", 0, 0, OPTION_CRCVERIFY},
  {"owner", 1, 0, 'R'},
  {"pass-through", 0, 0, 'p'},
  {"pattern-file", 1, 0, 'E'},
  {"preserve-modification-time", 0, &retain_time_flag, true},
  {"rename", 0, &rename_flag, true},
  {"rename-batch-file", 1, 0, OPTION_BATCHRENAME},
  {"quiet", 0, 0, OPTION_QUIET},
  {"silent", 0, 0, OPTION_QUIET},
  {"sparse", 0, 0, OPTION_SPARSE},
  {"swap", 0, 0, 'b'},
  {"swap-bytes", 0, 0, 's'},
  {"swap-halfwords", 0, 0, 'S'},
  {"reset-access-time", 0, &reset_time_flag, true},
  {"unconditional", 0, &unconditional_flag, true},
  {"verbose", 0, &verbose_flag, true},
  {"version", 0, 0, OPTION_VERSION},
#ifdef DEBUG_CPIO
  {"debug", 0, &debug_flag, true},
#endif
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
 --sparse                 write sparse files (copy-out, copy-pass)\n"), fp);
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

  fputs (_("\nReport bugs to <bug-gnu-utils@prep.ai.mit.edu>\n"), fp);

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
	  reset_time_flag = true;
	  break;

	case 'A':		/* Append to the archive.  */
	  append_flag = true;
	  break;

	case 'b':		/* Swap bytes and halfwords.  */
	  swap_bytes_flag = true;
	  swap_halfwords_flag = true;
	  break;

	case 'B':		/* Set block size to 5120.  */
	  io_block_size = 5120;
	  break;

	case OPTION_BLOCK_SIZE:	/* --block-size */
	  io_block_size = atoi (optarg);
	  if (io_block_size < 1)
	    error (2, 0, _("invalid block size"));
	  io_block_size *= 512;
	  break;

	case 'c':		/* Use the old portable ASCII format.  */
	  if (archive_format != arf_unknown)
	    error (2, 0, _("archive format specified twice"));
#ifdef SVR4_COMPAT
	  archive_format = arf_newascii; /* -H newc.  */
#else
	  archive_format = arf_oldascii; /* -H odc.  */
#endif
	  break;

	case 'C':		/* Block size.  */
	  io_block_size = atoi (optarg);
	  if (io_block_size < 1)
	    error (2, 0, _("invalid block size"));
	  break;

	case 'd':		/* Create directories where needed.  */
	  create_dir_flag = true;
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
	  if (archive_format != arf_unknown)
	    error (2, 0, _("archive format specified twice"));

	  archive_format = find_format (optarg);
	  if (archive_format == arf_unknown)
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
	  link_flag = true;
	  break;

	case 'L':		/* Dereference symbolic links.  */
	  xstat = stat;
	  break;

	case 'm':		/* Retain previous file modify times.  */
	  retain_time_flag = true;
	  break;

	case 'M':		/* New media message.  */
	  set_new_media_message (optarg);
	  break;

	case 'n':		/* Long list owner and group as numbers.  */
	  numeric_uid = true;
	  break;

	case OPTION_NOPRESERVE:	/* --no-preserve-owner */
	  if (set_owner_flag || set_group_flag)
	    error (2, 0, _("only one of --no-preserve-owner and -R allowed"));
	  no_chown_flag = true;
	  break;

	case 'o':		/* Copy-out mode.  */
	  if (copy_function != 0)
	    error (2, 0, _("only one of -o, -i, -p allowed"));
	  copy_function = process_copy_out;
	  break;

	case 'O':		/* Output archive file name.  */
	  output_archive_name = optarg;
	  break;

	case OPTION_CRCVERIFY:
	  only_verify_crc_flag = true;
	  break;

	case 'p':		/* Copy-pass mode.  */
	  if (copy_function != 0)
	    error (2, 0, _("only one of -o, -i, -p allowed"));
	  copy_function = process_copy_pass;
	  break;

	case 'r':		/* Interactively rename.  */
	  rename_flag = true;
	  break;

	case OPTION_BATCHRENAME:
	  rename_batch_file = optarg;
	  break;

	case OPTION_QUIET:
	  no_block_message_flag = true;
	  break;

	case 'R':		/* Set the owner.  */
	  if (no_chown_flag)
	    error (2, 0, _("only one of --no-preserve-owner and -R allowed"));
#ifndef __MSDOS__
	  {
	    char *e, *u, *g;

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
	  swap_bytes_flag = true;
	  break;

	case 'S':		/* Swap halfwords.  */
	  swap_halfwords_flag = true;
	  break;

	case 't':		/* Only print a list.  */
	  table_flag = true;
	  break;

	case 'u':		/* Replace all!  Unconditionally!  */
	  unconditional_flag = true;
	  break;

	case 'v':		/* Verbose!  */
	  verbose_flag = true;
	  break;

	case 'V':		/* Print `.' for each file.  */
	  dot_flag = true;
	  break;

	case OPTION_VERSION:
	  printf (_("cpio (Free %s) %s\n"), PACKAGE, VERSION);
	  exit (0);
	  break;

	case OPTION_SPARSE:
	  sparse_flag = true;
	  break;

	case OPTION_HELP:	/* --help */
	  usage (stdout, 0);
	  break;

	default:
	  error (2, 0, _("unrecognized option -- `%c'"), c);
	}
    }

  /* Do error checking and look at other args.  */

  if (copy_function == 0)
    {
      if (table_flag)
	copy_function = process_copy_in;
      else
	error (2, 0, _("one of -o, -i, -p must be specified"));
    }

  if ((!table_flag || !verbose_flag) && numeric_uid)
    error (2, 0, _("-n only makes sense with -t and -v"));

  /* Work around for pcc bug.  */
  copy_in = process_copy_in;
  copy_out = process_copy_out;

  if (copy_function == copy_in)
    {
      archive_des = 0;
      /* FIXME should indicate particular error.  */
      if (link_flag || reset_time_flag || xstat != lstat || append_flag
	  || sparse_flag
	  || output_archive_name
	  || (archive_name && input_archive_name))
	error (2, 0, _("option conflicts with -i"));
      if (archive_format == arf_crcascii)
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
      if (argc != optind || create_dir_flag || rename_flag
	  || table_flag || unconditional_flag || link_flag
	  || retain_time_flag || no_chown_flag || set_owner_flag
	  || set_group_flag || swap_bytes_flag || swap_halfwords_flag
	  || (append_flag && !(archive_name || output_archive_name))
	  || rename_batch_file || no_abs_paths_flag
	  || input_archive_name || (archive_name && output_archive_name))
	error (2, 0, _("option conflicts with -o"));
      if (archive_format == arf_unknown)
	archive_format = arf_binary;
      set_write_pointers_from_format (archive_format);
      if (output_archive_name)
	archive_name = output_archive_name;
    }
  else
    {
      /* Copy pass.  */
      archive_des = -1;
      /* FIXME should indicate particular error.  */
      if (argc - 1 != optind || archive_format != arf_unknown
	  || swap_bytes_flag || swap_halfwords_flag
	  || table_flag || rename_flag || append_flag
	  || rename_batch_file || no_abs_paths_flag)
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
    no_chown_flag = true;
#endif
}

int
main (int argc, char *argv[])
{
  program_name = argv[0];
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
