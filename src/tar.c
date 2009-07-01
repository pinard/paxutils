/* A tar (tape archiver) program.
   Copyright (C) 1988,92,93,94,95,96,97,98,99 Free Software Foundation, Inc.
   Written by John Gilmore, starting 1985-08-25.

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

#include "system.h"

#include <getopt.h>

/* The following causes "common.h" to produce definitions of all the global
   variables, rather than just "extern" declarations of them.  tar does depend
   on the system loader to preset all GLOBAL variables to neutral (or zero)
   values, explicit initialisation is usually not done.  */
#define GLOBAL
#include "common.h"

time_t get_date ();

const char *parse_user_spec PARAMS((const char *, uid_t *, gid_t *,
				    char **, char **));

#if ENABLE_DALE_CODE
# if !MSDOS
/* Test to see if we are trying to dump the compression work file.  */
dev_t fct_dev;
ino_t fct_ino;
# endif
#endif

/* Local declarations.  */

#ifndef DEFAULT_ARCHIVE
# define DEFAULT_ARCHIVE "tar.out"
#endif

#ifndef DEFAULT_BLOCKING
# define DEFAULT_BLOCKING 20
#endif

static void usage PARAMS ((int));

/* Standard input management.  */

/* Name of option using stdin.  */
static const char *stdin_used_by = NULL;

/* File to use for reading interactive user input, stdin if NULL.  */
static FILE *confirm_file = NULL;

/*----------------------------------------------.
| Doesn't return if stdin already requested.    |
`----------------------------------------------*/

static void
request_stdin (const char *option)
{
  if (stdin_used_by)
    USAGE_ERROR ((0, 0, _("Options `-%s' and `-%s' both want standard input"),
		  stdin_used_by, option));

  stdin_used_by = option;
}

/*-----------------------------------------------------------------------.
| Send MESSAGE, then get an interactive reply in REPLY buffer of a given |
| SIZE, from the user, using fgets.                                      |
`-----------------------------------------------------------------------*/

#if HAVE_TERMIO_H
# include <termio.h>
#endif

#ifndef TCIFLUSH
# define TCIFLUSH 0
#endif

char *
get_reply (const char *message, char *reply, size_t length)
{

  FILE *file;

  /* Select a file to read user replies.  */

  if (!confirm_file && (stdin_used_by || archive == STDIN))
    {
      confirm_file = fopen (CONSOLE, "r");
      if (!confirm_file)
	FATAL_ERROR ((0, 0, _("Cannot read confirmation from user")));
    }

  file = confirm_file ? confirm_file : stdin;
  tcflush (fileno (file), TCIFLUSH);

  /* Write out the prompt.  */

  if (checkpoint_option)
    flush_checkpoint_line ();

  fputs (message, stdlis);
  fflush (stdlis);

  /* Get the reply.  */

  return fgets (reply, length, file);
}

/*------------------------------------------------------------------------.
| Issue message ACTION and NAME, then return true if and only if the user |
| typed 'y' or 'Y'.                                                       |
`------------------------------------------------------------------------*/

bool
confirm (const char *format, const char *argument)
{
  FILE *file;
  int reply;
  int character;

  if (!confirm_file && (stdin_used_by || archive == STDIN))
    {
      confirm_file = fopen (CONSOLE, "r");
      if (!confirm_file)
	FATAL_ERROR ((0, 0, _("Cannot read confirmation from user")));
    }

  if (checkpoint_option)
    flush_checkpoint_line ();

  fprintf (stdlis, format, argument);
  fflush (stdlis);

  file = confirm_file ? confirm_file : stdin;
  reply = getc (file);
  for (character = reply;
       character != '\n' && character != EOF;
       character = getc (file))
    continue;

  return reply == 'y' || reply == 'Y';
}

/*-----------------------------------------------.
| Checkpoint flusher prior to an error message.  |
`-----------------------------------------------*/

static void
flush_checkpoint_print_progname (void)
{
  flush_checkpoint_line ();
  fprintf (stderr, "%s: ", program_name);
}

#if ENABLE_DALE_CODE
/*-------------------------------------------------------------.
| Create and destroy the temporary file for file compression.  |
`-------------------------------------------------------------*/

/* The name of the temporary file.  */
static char *compress_work_file_name;

/* Create the temporary file.  */
static void
init_compress_work_file (void)
{
  struct stat statbuf;

  /* Get the name.  */
  compress_work_file_name = tempnam (NULL, "tfc");
  if (compress_work_file_name == NULL)
    FATAL_ERROR ((0, 0, _("Unable to get temporary file name")));
  /* Open the file.  */
  compress_work_file = open (compress_work_file_name,
			     O_RDWR | O_CREAT | O_BINARY, 0600);
  if (compress_work_file < 0)
    FATAL_ERROR ((0, errno, _("Unable to open temporary file")));
  if (fstat (compress_work_file, &statbuf) < 0)
    FATAL_ERROR ((0, errno, _("Unable to fstat() temporary file")));
  /* Record the device and inode numbers of the temporary file so we do not
     put them into the archive.  */
  fct_dev = statbuf.st_dev;
  fct_ino = statbuf.st_ino;
}

static void
delete_compress_work_file (void)
{
  if (close (compress_work_file) < 0)
    FATAL_ERROR ((0, errno, _("Unable to close temporary file")));
  if (unlink (compress_work_file_name) < 0)
    FATAL_ERROR ((0, errno, _("Unable to delete temporary file")));
}
#endif /* ENABLE_DALE_CODE */

/* Options.  */

/* For long options that unconditionally set a single flag, we have getopt
   do it.  For the others, we share the code for the equivalent short
   named option, the name of which is stored in the otherwise-unused `val'
   field of the `struct option'; for long options that have no equivalent
   short option, we use nongraphic characters as pseudo short option
   characters, starting at 2 and going upwards.  */

#define BACKUP_OPTION			2
#define DELETE_OPTION			3
#define EXCLUDE_OPTION			4
#define GROUP_OPTION			5
#define MODE_OPTION			6
#define NAME_PREFIX_OPTION		7
#define NEWER_MTIME_OPTION		8
#define NO_ATTRIBUTES_OPTION		9
#define NO_RECURSE_OPTION		10
#define NULL_OPTION			11
#define OWNER_OPTION			12
#define POSIX_OPTION			13
#define PRESERVE_OPTION			14
#define RECORD_SIZE_OPTION		15
#define RSH_COMMAND_OPTION		16
#define SUFFIX_OPTION			17
#define USE_COMPRESS_PROGRAM_OPTION	18
#define VOLNO_FILE_OPTION		19

/* Some cleanup is being made in tar long options.  Using old names is allowed
   for a while, but will also send a warning to stderr.  Take old names out in
   1.14, or in summer 1997, whichever happens last.  We use nongraphic
   characters as pseudo short option characters, starting at 31 and going
   downwards.  */

#define OBSOLETE_ABSOLUTE_NAMES		31
#define OBSOLETE_BLOCK_COMPRESS		30
#define OBSOLETE_BLOCKING_FACTOR	29
#define OBSOLETE_BLOCK_NUMBER		28
#define OBSOLETE_READ_FULL_RECORDS	27
#define OBSOLETE_TOUCH			26
#define OBSOLETE_VERSION_CONTROL	25
#define OBSOLETE_UNCOMPRESS		24
#define OBSOLETE_UNGZIP			23

/* If true, display usage information and exit.  */
static bool show_help = false;

/* If true, print the version on standard output and exit.  */
static bool show_version = false;

/* FIXME: If sizeof (int) != sizeof (bool), we are in trouble!  */

struct option long_options[] =
{
  {"absolute-names", no_argument, NULL, 'P'},
  {"absolute-paths", no_argument, NULL, OBSOLETE_ABSOLUTE_NAMES},
  {"after-date", required_argument, NULL, 'N'},
  {"append", no_argument, NULL, 'r'},
  {"atime-preserve", no_argument, (int *) &atime_preserve_option, true},
  {"backup", optional_argument, NULL, BACKUP_OPTION},
  {"block-compress", no_argument, NULL, OBSOLETE_BLOCK_COMPRESS},
  {"block-number", no_argument, NULL, 'R'},
  {"block-size", required_argument, NULL, OBSOLETE_BLOCKING_FACTOR},
  {"blocking-factor", required_argument, NULL, 'b'},
  {"catenate", no_argument, NULL, 'A'},
  {"checkpoint", no_argument, (int *) &checkpoint_option, true},
  {"compare", no_argument, NULL, 'd'},
  {"compress", no_argument, NULL, 'Z'},
  {"concatenate", no_argument, NULL, 'A'},
  {"confirmation", no_argument, NULL, 'w'},
  {"create", no_argument, NULL, 'c'},
  {"delete", no_argument, NULL, DELETE_OPTION},
  {"dereference", no_argument, NULL, 'h'},
  {"diff", no_argument, NULL, 'd'},
  {"directory", required_argument, NULL, 'C'},
  {"exclude", required_argument, NULL, EXCLUDE_OPTION},
  {"exclude-from", required_argument, NULL, 'X'},
  {"extract", no_argument, NULL, 'x'},
  {"file", required_argument, NULL, 'f'},
  {"files-from", required_argument, NULL, 'T'},
  {"force-local", no_argument, (int *) &force_local_option, true},
  {"get", no_argument, NULL, 'x'},
  {"group", required_argument, NULL, GROUP_OPTION},
  {"gunzip", no_argument, NULL, OBSOLETE_UNGZIP},
  {"gzip", no_argument, NULL, 'z'},
  {"help", no_argument, (int *) &show_help, true},
  {"ignore-failed-read", no_argument, (int *) &ignore_failed_read_option, true},
  {"ignore-zeros", no_argument, NULL, 'i'},
  {"incremental", no_argument, NULL, 'G'},
  {"info-script", required_argument, NULL, 'F'},
  {"interactive", no_argument, NULL, 'w'},
  {"keep-old-files", no_argument, NULL, 'k'},
  {"label", required_argument, NULL, 'V'},
  {"list", no_argument, NULL, 't'},
  {"listed-incremental", required_argument, NULL, 'g'},
  {"mode", required_argument, NULL, MODE_OPTION},
  {"modification-time", no_argument, NULL, OBSOLETE_TOUCH},
  {"multi-volume", no_argument, NULL, 'M'},
  {"name-prefix", required_argument, NULL, NAME_PREFIX_OPTION},
  {"new-volume-script", required_argument, NULL, 'F'},
  {"newer", required_argument, NULL, 'N'},
  {"newer-mtime", required_argument, NULL, NEWER_MTIME_OPTION},
  {"no-attributes", no_argument, NULL, NO_ATTRIBUTES_OPTION},
  {"no-recursion", no_argument, NULL, NO_RECURSE_OPTION},
  {"null", no_argument, NULL, NULL_OPTION},
  {"numeric-owner", no_argument, (int *) &numeric_owner_option, true},
  {"old-archive", no_argument, NULL, 'o'},
  {"one-file-system", no_argument, NULL, 'l'},
  {"owner", required_argument, NULL, OWNER_OPTION},
  {"portability", no_argument, NULL, 'o'},
  {"posix", no_argument, NULL, POSIX_OPTION},
#if ENABLE_DALE_CODE
  {"per-file-compress", no_argument, NULL, 'y'},
#endif
  {"preserve", no_argument, NULL, PRESERVE_OPTION},
  {"preserve-order", no_argument, NULL, 's'},
  {"preserve-permissions", no_argument, NULL, 'p'},
  {"quick", no_argument, NULL, 'q'},
  {"read-full-blocks", no_argument, NULL, OBSOLETE_READ_FULL_RECORDS},
  {"read-full-records", no_argument, NULL, 'B'},
  {"record-number", no_argument, NULL, OBSOLETE_BLOCK_NUMBER},
  {"record-size", required_argument, NULL, RECORD_SIZE_OPTION},
  {"recursive-unlink", no_argument, (int *) &recursive_unlink_option, true},
  {"remove-files", no_argument, (int *) &remove_files_option, true},
  {"rsh-command", required_argument, NULL, RSH_COMMAND_OPTION},
  {"same-order", no_argument, NULL, 's'},
  {"same-owner", no_argument, (int *) &same_owner_option, true},
  {"same-permissions", no_argument, NULL, 'p'},
  {"show-omitted-dirs", no_argument, (int *) &show_omitted_dirs_option, true},
  {"sparse", no_argument, NULL, 'S'},
  {"starting-file", required_argument, NULL, 'K'},
  {"suffix", required_argument, NULL, SUFFIX_OPTION},
  {"tape-length", required_argument, NULL, 'L'},
  {"to-stdout", no_argument, NULL, 'O'},
  {"totals", no_argument, (int *) &totals_option, true},
  {"touch", no_argument, NULL, 'm'},
  {"uncompress", no_argument, NULL, OBSOLETE_UNCOMPRESS},
  {"ungzip", no_argument, NULL, OBSOLETE_UNGZIP},
  {"unlink-first", no_argument, NULL, 'U'},
  {"update", no_argument, NULL, 'u'},
  {"use-compress-program", required_argument, NULL, USE_COMPRESS_PROGRAM_OPTION},
  {"verbose", no_argument, NULL, 'v'},
  {"verify", no_argument, NULL, 'W'},
  {"version", no_argument, (int *) &show_version, true},
  {"version-control", required_argument, NULL, OBSOLETE_VERSION_CONTROL},
  {"volno-file", required_argument, NULL, VOLNO_FILE_OPTION},

  {0, 0, 0, 0}
};

/*---------------------------------------------.
| Print a usage message and exit with STATUS.  |
`---------------------------------------------*/

static void
usage (int status)
{
  if (status != TAREXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      fputs (_("\
The `tar' tool saves many files together into a single tape or disk archive,\n\
and is able to restore individual files from the archive.\n"),
	     stdout);
      printf (_("\
\n\
Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
\n\
If a long option shows an argument as mandatory, then it is mandatory\n\
for the equivalent short option also.  Similarly for optional arguments.\n"),
	     stdout);
      fputs(_("\
\n\
Main operation mode:\n\
  -t, --list              list the contents of an archive\n\
  -x, --extract, --get    extract files from an archive\n\
  -c, --create            create a new archive\n\
  -d, --diff, --compare   find differences between archive and file system\n\
  -r, --append            append files to the end of an archive\n\
  -A, --catenate          append tar files to an archive\n\
      --concatenate       same as -A\n\
      --delete            delete from the archive (not on mag tapes!)\n"),
	    stdout);
      fputs (_("\
\n\
Operation modifiers:\n\
  -q, --quick                assume the archive has no duplicate members\n\
  -W, --verify               attempt to verify the archive after writing it\n\
      --remove-files         remove files after adding them to the archive\n\
  -k, --keep-old-files       don't overwrite existing files when extracting\n\
  -u, --update               replace entries or extract files only if newer\n\
  -U, --unlink-first         remove each file prior to extracting over it\n\
      --recursive-unlink     empty hierarchies prior to extracting directory\n\
  -S, --sparse               handle sparse files efficiently\n\
  -O, --to-stdout            extract files to standard output\n\
  -G, --incremental          handle old GNU-format incremental backup\n\
  -g, --listed-incremental   handle new GNU-format incremental backup\n\
      --name-prefix=PREFIX   prepend PREFIX to each name in create archive\n\
      --ignore-failed-read   do not exit with nonzero on unreadable files\n"),
	     stdout);
      fputs (_("\
\n\
Handling of file attributes:\n\
      --owner=NAME             force NAME as owner for added files\n\
      --group=NAME             force NAME as group for added files\n\
      --mode=CHANGES           force (symbolic) mode CHANGES for added files\n\
      --atime-preserve         don't change access times on dumped files\n\
  -m, --modification-time      don't extract file modified time\n\
      --same-owner             try extracting files with the same ownership\n\
      --numeric-owner          always use numbers for user/group names\n\
  -p, --same-permissions       extract all protection information\n\
      --preserve-permissions   same as -p\n\
  -s, --same-order             sort names to extract to match archive\n\
      --preserve-order         same as -s\n\
      --no-attributes          do not restore any file attribute\n\
      --preserve               same as both -p and -s\n"),
	     stdout);
      fputs (_("\
\n\
Device selection and switching:\n\
  -f, --file=ARCHIVE             use archive file or device ARCHIVE\n\
      --force-local              archive file is local even if has a colon\n\
      --rsh-command=COMMAND      use remote COMMAND instead of rsh\n\
  -[0-7][lmh]                    specify drive and density\n\
  -M, --multi-volume             create/list/extract multi-volume archive\n\
  -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes\n\
  -F, --info-script=FILE         run script at end of each tape (implies -M)\n\
      --new-volume-script=FILE   same as -F FILE\n\
      --volno-file=FILE          use/update the volume number in FILE\n"),
	     stdout);
      fputs (_("\
\n\
Device blocking:\n\
  -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes per record\n\
      --record-size=SIZE         SIZE bytes per record, multiple of 512\n\
  -i, --ignore-zeros             ignore zeroed blocks in archive (means EOF)\n\
  -B, --read-full-records        reblock as we read (for 4.2BSD pipes)\n"),
	     stdout);
      fputs (_("\
\n\
Archive format selection:\n\
  -V, --label=NAME                   create archive with volume name NAME\n\
              PATTERN                at list/extract time, a globbing PATTERN\n\
  -o, --old-archive, --portability   write a V7 format archive\n\
      --posix                        write a POSIX conformant archive\n\
  -z, --gzip                         filter the archive through `gzip'\n\
  -Z, --compress                     filter the archive through `compress'\n\
      --use-compress-program=PROG    filter through `PROG' (must accept -d)\n"),
	     stdout);
#if ENABLE_DALE_CODE
      fputs (_("\
  -y, --per-file-compress            compress archived files independently\n"),
	       stdout);

#endif
      fputs (_("\
\n\
Local file selection:\n\
  -C, --directory=DIR          change to directory DIR\n\
  -T, --files-from=NAME        get names to extract or create from file NAME\n\
      --null                   names from -T option are NUL terminated\n\
      --exclude=PATTERN        exclude files, given as a globbing PATTERN\n\
  -X, --exclude-from=FILE      exclude globbing patterns listed in FILE\n\
  -P, --absolute-names         don't strip leading `/'s from file names\n\
  -h, --dereference            dump instead the files symlinks point to\n\
      --no-recursion           avoid descending automatically in directories\n\
  -l, --one-file-system        stay in local file system when creating archive\n\
  -K, --starting-file=NAME     begin at file NAME in the archive\n\
  -N, --newer=DATE             only store files newer than DATE\n\
      --newer-mtime            compare date and time when data changed only\n\
      --after-date=DATE        same as -N\n"),
	     stdout);

      fputs (_("\
      --backup[=CONTROL]       backup before removal, choose version control\n\
      --suffix=SUFFIX          backup before removal, override usual suffix\n"),
	     stdout);

      fputs (_("\
\n\
Informative output:\n\
      --help                print this help, then exit\n\
      --version             print tar program version number, then exit\n\
  -v, --verbose             verbosely list files processed\n\
      --checkpoint          write a progress dot every ten records\n\
      --show-omitted-dirs   print directory names while reading the archive\n\
      --totals              print total bytes written while creating archive\n\
  -R, --block-number        insert archive block number within each message\n\
  -w, --interactive         ask for confirmation for every action\n\
      --confirmation        same as -w\n"),
	     stdout);
      fputs (_("\
\n\
The backup suffix is `~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control may be set with --backup or VERSION_CONTROL, values are:\n\
\n\
  t, numbered     make numbered backups\n\
  nil, existing   numbered if numbered backups exist, simple otherwise\n\
  never, simple   always make simple backups\n"),
	     stdout);
      printf (_("\
\n\
This `tar' cannot produce `--posix' archives.  Also, if POSIXLY_CORRECT\n\
is set in the environment, extensions are disallowed with `--posix'.\n\
Support for POSIX is only partially implemented, don't depend on it yet.\n\
ARCHIVE may be FILE, HOST:FILE or USER@HOST:FILE; and FILE may be a file\n\
or a device.  *This* `tar' defaults to `-f%s -b%d'.\n"),
	      DEFAULT_ARCHIVE, DEFAULT_BLOCKING);
      fputs (_("\
\n\
Report bugs to <tar-bugs@iro.umontreal.ca>.\n"),
	       stdout);
    }
  exit (status);
}

/*----------------------------.
| Parse the options for tar.  |
`----------------------------*/

/* Available option letters are DEHIJQY and aejn.  Some are reserved:

   Y  per-block gzip compression */

#if ENABLE_DALE_CODE
# define OPTION_STRING \
   "-01234567ABC:F:GK:L:MN:OPRST:UV:WX:Zb:cdf:g:hiklmopqrstuvwxyz"
#else
# define OPTION_STRING \
   "-01234567ABC:F:GK:L:MN:OPRST:UV:WX:Zb:cdf:g:hiklmopqrstuvwxz"
#endif

static void
set_subcommand_option (enum subcommand subcommand)
{
  if (subcommand_option != UNKNOWN_SUBCOMMAND
      && subcommand_option != subcommand)
    USAGE_ERROR ((0, 0,
		  _("You may not specify more than one `-Acdtrux' option")));

  subcommand_option = subcommand;
}

static void
set_use_compress_program_option (const char *string)
{
  if (use_compress_program_option
      && strcmp (use_compress_program_option, string) != 0)
    USAGE_ERROR ((0, 0, _("Conflicting compression options")));

  use_compress_program_option = string;
}

#if DOSWIN

static char original_directory[PATH_MAX];

static void
restore_original_directory (void)
{
  if (original_directory[0])
    chdir (original_directory);
}

#endif /* DOSWIN */

static unsigned long
decode_number (const char *option_name, const char *option_string)
{
  if (ISASCII (*option_string) && isdigit (*option_string))
    {
      unsigned long value = *option_string - '0';

      option_string++;
      while (ISASCII (*option_string) && isdigit (*option_string))
	{
	  unsigned long value2 = 10 * value + (*option_string - '0');

	  if (value2 < value)
	    /* Overflow occurred.  */
	    break;

	  value = value2;
	  option_string++;
	}

      if (*option_string == '\0')
	return value;
    }

  error (0, 0, _("Invalid numerical value for option `%s'"), option_name);
  usage (TAREXIT_FAILURE);
}

static void
decode_options (int argc, char *const *argv)
{
  int optchar;			/* option letter */
  int input_files;		/* number of input files */
  const char *backup_suffix_string;
  const char *version_control_string;

  /* Set some default option values.  */

  subcommand_option = UNKNOWN_SUBCOMMAND;
  archive_format = DEFAULT_FORMAT;
  blocking_factor = DEFAULT_BLOCKING;
  record_size = DEFAULT_BLOCKING * BLOCKSIZE;

  owner_option = (uid_t) -1;
  group_option = (gid_t) -1;

  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");
  version_control_string = getenv ("VERSION_CONTROL");

#ifdef REMOTE_SHELL
  rsh_command_option = REMOTE_SHELL;
#endif

  /* Convert old-style tar call by exploding option element and rearranging
     options accordingly.  */

  if (argc > 1 && argv[1][0] != '-')
    {
      int new_argc;		/* argc value for rearranged arguments */
      char **new_argv;		/* argv value for rearranged arguments */
      char *const *in;		/* cursor into original argv */
      char **out;		/* cursor into rearranged argv */
      const char *letter;	/* cursor into old option letters */
      char buffer[3];		/* constructed option buffer */
      const char *cursor;	/* cursor in OPTION_STRING */

      /* Initialize a constructed option.  */

      buffer[0] = '-';
      buffer[2] = '\0';

      /* Allocate a new argument array, and copy program name in it.  */

      new_argc = argc - 1 + strlen (argv[1]);
      new_argv = (char **) xmalloc (new_argc * sizeof (char *));
      in = argv;
      out = new_argv;
      *out++ = *in++;

      /* Copy each old letter option as a separate option, and have the
	 corresponding argument moved next to it.  */

      for (letter = *in++; *letter; letter++)
	{
	  buffer[1] = *letter;
	  *out++ = xstrdup (buffer);
	  cursor = strchr (OPTION_STRING, *letter);
	  if (cursor && cursor[1] == ':')
	    if (in < argv + argc)
	      *out++ = *in++;
	    else
	      USAGE_ERROR ((0, 0, _("Old option `%c' requires an argument."),
			    *letter));
	}

      /* Copy all remaining options.  */

      while (in < argv + argc)
	*out++ = *in++;

      /* Replace the old option list by the new one.  */

      argc = new_argc;
      argv = new_argv;
    }

  /* Parse all options and non-options as they appear.  */

  input_files = 0;

  while (optchar = getopt_long (argc, argv, OPTION_STRING, long_options, NULL),
	 optchar != EOF)
    switch (optchar)
      {
      case '?':
	usage (TAREXIT_FAILURE);

      case 0:
	break;

      case 1:
	/* File name or non-parsed option, because of RETURN_IN_ORDER
	   ordering triggerred by the leading dash in OPTION_STRING.  */
	add_name_string (optarg);
	input_files++;
	break;

      case 'A':
	set_subcommand_option (CAT_SUBCOMMAND);
	break;

      case OBSOLETE_BLOCK_COMPRESS:
	WARN ((0, 0, _("Obsolete option, now implied by --blocking-factor")));
	break;

      case OBSOLETE_BLOCKING_FACTOR:
	WARN ((0, 0, _("Obsolete option name replaced by --blocking-factor")));
	/* Fall through.  */

      case 'b':
	blocking_factor = decode_number ("--blocking-factor", optarg);
	record_size = blocking_factor * BLOCKSIZE;
	break;

      case OBSOLETE_READ_FULL_RECORDS:
	WARN ((0, 0,
	       _("Obsolete option name replaced by --read-full-records")));
	/* Fall through.  */

      case 'B':
	/* It would surely make sense to exchange -B and -R, but it seems
	   that -B has been used for a long while in Sun tar ans most
	   BSD-derived systems.  This is a consequence of the block/record
	   terminology confusion.  */
	read_full_records_option = true;
	break;

      case 'c':
	set_subcommand_option (CREATE_SUBCOMMAND);
	break;

      case 'C':
	add_name_string ("-C");
#if DOSWIN
	{
	  char new_arg[PATH_MAX];

	  /* This canonicalizes the file name, mirrors all the backslashes
	     and downcases as appropriate.  It is needed because code that
	     handles the argument to -C doesn't work with backslashes, and
	     because we need the absolute pathname of the directory, so
	     that it is independent of cwd.  */

	  _fixpath (optarg, new_arg);
	  add_name_string (new_arg);
	}

	/* We are going to chdir, and cwd is a global notion on DOSWIN.
	   Remember the original directory, to be restored at exit.

	   Note that this isn't a DJGPP-specific problem, but only DJGPP's
	   `chdir' can change the drive as well, so the code which changes the
	   directory and restores it at exit won't work for others.  */

	if (!original_directory[0])
	  {
	    char *current_directory = getcwd (0, PATH_MAX);

	    if (!current_directory)
	      FATAL_ERROR ((0, 0, _("Could not get current directory")));
	    strcpy (original_directory, current_directory);
	    atexit (restore_original_directory);
	  }
#else
	add_name_string (optarg);
#endif
	break;

      case 'd':
	set_subcommand_option (DIFF_SUBCOMMAND);
	break;

      case 'f':
	if (archive_names == allocated_archive_names)
	  {
	    allocated_archive_names *= 2;
	    archive_name_array = (const char **)
	      xrealloc (archive_name_array,
			sizeof (const char *) * allocated_archive_names);
	  }
#if DOSWIN
	/* Need to mirror the backslashes, some of the rest of the code cannot
	   cope with DOS-style file names.  */
	unixify_file_name (optarg);
#endif
	archive_name_array[archive_names++] = optarg;
	break;

      case 'F':
	/* Since -F is only useful with -M, make it implied.  */
	info_script_option = optarg;
	multi_volume_option = true;
	break;

      case 'g':
	listed_incremental_option = optarg;
	/* Fall through.  */

      case 'G':
	incremental_option = true;
	break;

      case 'h':
	dereference_option = true;
	break;

      case 'i':
	ignore_zeros_option = true;
	break;

      case 'k':
	keep_old_files_option = true;
	break;

      case 'K':
	starting_file_option = true;
	add_name (optarg);
	break;

      case 'l':
	one_file_system_option = true;
	break;

      case 'L':
	clear_tarlong (tape_length_option);
	add_to_tarlong (tape_length_option,
			decode_number ("--tape-length", optarg));
	mult_tarlong (tape_length_option, 1024);
	multi_volume_option = true;
	break;

      case OBSOLETE_TOUCH:
	WARN ((0, 0, _("Obsolete option name replaced by --touch")));
	/* Fall through.  */

      case 'm':
	touch_option = true;
	break;

      case 'M':
	multi_volume_option = true;
	break;

      case 'N':
	newer_ctime_option = true;
	/* Fall through.  */

      case NEWER_MTIME_OPTION:
	if (newer_mtime_option)
	  USAGE_ERROR ((0, 0, _("More than one threshold date")));
	newer_mtime_option = true;

	time_option_threshold = get_date (optarg, (voidstar) 0);
	if (time_option_threshold == (time_t) -1)
	  USAGE_ERROR ((0, 0, _("Invalid date format `%s'"), optarg));
	break;

      case 'o':
	if (archive_format == DEFAULT_FORMAT)
	  archive_format = V7_FORMAT;
	else if (archive_format != V7_FORMAT)
	  USAGE_ERROR ((0, 0, _("Conflicting archive format options")));
	break;

      case 'O':
	to_stdout_option = true;
	break;

      case 'p':
	same_permissions_option = true;
	break;

      case OBSOLETE_ABSOLUTE_NAMES:
	WARN ((0, 0, _("Obsolete option name replaced by --absolute-names")));
	/* Fall through.  */

      case 'P':
	absolute_names_option = true;
	break;

      case 'q':
	quick_option = true;
	break;

      case 'r':
	set_subcommand_option (APPEND_SUBCOMMAND);
	break;

      case OBSOLETE_BLOCK_NUMBER:
	WARN ((0, 0, _("Obsolete option name replaced by --block-number")));
	/* Fall through.  */

      case 'R':
	/* It would surely make sense to exchange -B and -R, but it seems that
	   -B has been used for a long while in Sun tar and most BSD-derived
	   systems.  This is a consequence of the block/record terminology
	   confusion.  */
	block_number_option = true;
	break;

      case 's':
	same_order_option = true;
	break;

      case 'S':
	sparse_option = true;
	break;

      case 't':
	set_subcommand_option (LIST_SUBCOMMAND);
	verbose_option++;
	break;

      case 'T':
	files_from_option = true;

	add_name_string ("-T");
	if (!strcmp (optarg, "-"))
	  request_stdin ("-T");
#if DOSWIN
	{
	  char new_arg[PATH_MAX];

	  /* See comment for case 'C' above.  */
	  _fixpath (optarg, new_arg);
	  add_name_string (new_arg);
	}
#else
	add_name_string (optarg);
#endif
 	break;

      case 'u':
	update_option = true;
	break;

      case 'U':
	unlink_first_option = true;
	break;

      case 'v':
	verbose_option++;
	break;

      case 'V':
	volume_label_option = optarg;
	break;

      case 'w':
	interactive_option = true;
	break;

      case 'W':
	verify_option = true;
	break;

      case 'x':
	set_subcommand_option (EXTRACT_SUBCOMMAND);
	break;

      case 'X':
	exclude_option = true;
	if (!strcmp (optarg, "-"))
	  request_stdin ("-X");
	add_exclude_file (optarg);
	break;

#if ENABLE_DALE_CODE
      case 'y':
	per_file_compress_option = true;
	break;
#endif

      case OBSOLETE_UNGZIP:
	WARN ((0, 0, _("Obsolete option name replaced by --gzip")));
	/* Fall through.  */

      case 'z':
#if ENABLE_DALE_CODE
	compress_whole_archive = true;
#endif
#if DOSWIN
	/* Use explicit .EXE suffix.  First, if the shell is COMMAND.COM, it
	   will *not* be called if gzip is not installed, and thus they get
	   better diagnostics (COMMAND.COM returns 0 status even if it fails
	   to invoke `gzip').  Second, in the test suite, the `gzip.sh' script
	   won't be invoked by DJGPP's too smart `popen' which tries all known
	   executable suffixes, including .sh, in every directory.  */
	set_use_compress_program_option ("gzip.exe");
#else
	set_use_compress_program_option ("gzip");
#endif
	break;

      case OBSOLETE_UNCOMPRESS:
	WARN ((0, 0, _("Obsolete option name replaced by --compress")));
	/* Fall through.  */

      case 'Z':
#if ENABLE_DALE_CODE
	compress_whole_archive = true;
#endif
#if DOSWIN
	set_use_compress_program_option ("compress.exe");
#else
	set_use_compress_program_option ("compress");
#endif
	break;

      case OBSOLETE_VERSION_CONTROL:
	WARN ((0, 0, _("Obsolete option name replaced by --backup")));
	/* Fall through.  */

      case BACKUP_OPTION:
	backup_option = true;
	if (optarg)
	  version_control_string = optarg;
	break;

      case DELETE_OPTION:
	set_subcommand_option (DELETE_SUBCOMMAND);
	break;

      case EXCLUDE_OPTION:
	exclude_option = true;
	add_exclude (optarg);
	break;

      case GROUP_OPTION:
	{
	  gid_t *gid = getgidbyname (optarg);

	  if (gid)
	    group_option = *gid;
	  else
	    group_option = decode_number ("--group", optarg);
	  break;
	}

      case MODE_OPTION:
	mode_option
	  = mode_compile (optarg,
			  MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
	if (mode_option == MODE_INVALID)
	  ERROR ((TAREXIT_FAILURE, 0, _("Invalid mode given on option")));
	if (mode_option == MODE_MEMORY_EXHAUSTED)
	  ERROR ((TAREXIT_FAILURE, 0, _("Memory exhausted")));
	break;

      case NAME_PREFIX_OPTION:
#if DOSWIN
	/* Make sure we have only forward slashes.  */
	unixify_file_name (optarg);
#endif
	name_prefix_option = optarg;
	name_prefix_length = strlen (optarg);
	break;

      case NO_ATTRIBUTES_OPTION:
	no_attributes_option = true;
	touch_option = true;
	break;

      case NO_RECURSE_OPTION:
	no_recurse_option = true;
	break;

      case NULL_OPTION:
	null_option = true;
	break;

      case OWNER_OPTION:
	{
	  const char *diagnostic;
	  char *user;
	  char *group;

	  diagnostic = parse_user_spec (optarg, &owner_option, &group_option,
					&user, &group);
	  if (user)
	    free (user);
	  if (group)
	    free (group);

	  if (diagnostic)
	    ERROR ((TAREXIT_FAILURE, 0, diagnostic));
	  break;
	}

      case POSIX_OPTION:
#if OLDGNU_COMPATIBILITY
	if (archive_format == DEFAULT_FORMAT)
	  archive_format = FREE_FORMAT;
	else if (archive_format != FREE_FORMAT)
	  USAGE_ERROR ((0, 0, _("Conflicting archive format options")));
#else
	if (archive_format == DEFAULT_FORMAT)
	  archive_format = POSIX_FORMAT;
	else if (archive_format != POSIX_FORMAT)
	  USAGE_ERROR ((0, 0, _("Conflicting archive format options")));
#endif
	break;

      case PRESERVE_OPTION:
	same_permissions_option = true;
	same_order_option = true;
	break;

      case RECORD_SIZE_OPTION:
	record_size = decode_number ("--record-size", optarg);
	if (record_size % BLOCKSIZE != 0)
	  USAGE_ERROR ((0, 0, _("Record size must be a multiple of %d."),
			BLOCKSIZE));
	blocking_factor = record_size / BLOCKSIZE;
	break;

      case RSH_COMMAND_OPTION:
	rsh_command_option = optarg;
	break;

      case SUFFIX_OPTION:
	backup_option = true;
	backup_suffix_string = optarg;
	break;

      case VOLNO_FILE_OPTION:
	volno_file_option = optarg;
	break;

      case USE_COMPRESS_PROGRAM_OPTION:
	set_use_compress_program_option (optarg);
	break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':

#ifdef DEVICE_PREFIX
	{
	  int device = optchar - '0';
	  int density;
	  static char buf[sizeof DEVICE_PREFIX + 10];
	  char *cursor;

	  density = getopt_long (argc, argv, "lmh", NULL, NULL);
	  strcpy (buf, DEVICE_PREFIX);
	  cursor = buf + strlen (buf);

#ifdef DENSITY_LETTER

	  sprintf (cursor, "%d%c", device, density);

#else /* not DENSITY_LETTER */

	  switch (density)
	    {
	    case 'l':
#ifdef LOW_NUM
	      device += LOW_NUM;
#endif
	      break;

	    case 'm':
#ifdef MID_NUM
	      device += MID_NUM;
#else
	      device += 8;
#endif
	      break;

	    case 'h':
#ifdef HGH_NUM
	      device += HGH_NUM;
#else
	      device += 16;
#endif
	      break;

	    default:
	      usage (TAREXIT_FAILURE);
	    }
	  sprintf (cursor, "%d", device);

#endif /* not DENSITY_LETTER */

	  if (archive_names == allocated_archive_names)
	    {
	      allocated_archive_names *= 2;
	      archive_name_array = (const char **)
		xrealloc (archive_name_array,
			  sizeof (const char *) * allocated_archive_names);
	    }
	  archive_name_array[archive_names++] = buf;

	  /* FIXME: How comes this works for many archives when buf is
	     not xstrdup'ed?  */
	}
	break;

#else /* not DEVICE_PREFIX */

	USAGE_ERROR ((0, 0,
		      _("Options `-[0-7][lmh]' not supported by *this* tar")));

#endif /* not DEVICE_PREFIX */
      }

  /* Handle any remaining unprocessed arguments as file arguments.  These may
     exist after `--', as we forced REQUIRE_ORDER in option decoding.  */

  while (optind < argc)
    {
      if (quick_option && is_pattern (argv[optind]))
	USAGE_ERROR ((0, 0, _("Wildcards may not be used with `--quick'")));

      add_name_string (argv[optind]);
      input_files++;
      optind++;
    }

  /* Process trivial options.  */

  if (show_version)
    {
      printf ("tar (Free %s) %s\n", PACKAGE, VERSION);
      fputs (_("\
\n\
Copyright (C) 1988, 92, 93, 94, 95, 96, 97 Free Software Foundation, Inc.\n"),
	     stdout);
      fputs (_("\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"),
	     stdout);
      fputs (_("\
\n\
Written by John Gilmore and Jay Fenlason.\n"),
	     stdout);
      exit (TAREXIT_SUCCESS);
    }

  if (show_help)
    usage (TAREXIT_SUCCESS);

  /* Derive option values and check option consistency.  */

  if (archive_format == DEFAULT_FORMAT)
    {
#if OLDGNU_COMPATIBILITY
      archive_format = OLDGNU_FORMAT;
#else
      archive_format = FREE_FORMAT;
#endif
    }

  if (archive_format == FREE_FORMAT && getenv ("POSIXLY_CORRECT"))
    archive_format = POSIX_FORMAT;

  if ((volume_label_option != NULL
       || incremental_option || multi_volume_option || sparse_option)
      && archive_format != OLDGNU_FORMAT && archive_format != FREE_FORMAT)
    USAGE_ERROR ((0, 0,
		  _("Features wanted on incompatible archive format")));

  if (archive_names == 0)
    {
      /* If no archive file name given, try TAPE from the environment, or
	 else, DEFAULT_ARCHIVE from the configuration process.  */

      archive_names = 1;
      archive_name_array[0] = getenv ("TAPE");
      if (archive_name_array[0] == NULL)
	archive_name_array[0] = DEFAULT_ARCHIVE;
    }

  /* Allow multiple archives only with `-M'.  */

  if (archive_names > 1 && !multi_volume_option)
    USAGE_ERROR ((0, 0,
		  _("Multiple archive files requires `-M' option")));

  /* Check if update mode was specified.  If subcommand is --create or is
     not otherwise specified, internally change subcommand to update the
     archive.  Else, the subcommand ought to be --extract.  */

  if (update_option)
    switch (subcommand_option)
      {
      case UNKNOWN_SUBCOMMAND:
      case CREATE_SUBCOMMAND:
	subcommand_option = UPDATE_SUBCOMMAND;
	break;

      case EXTRACT_SUBCOMMAND:
	break;

      default:
	USAGE_ERROR ((0, 0,
		      _("Option `-u' is only meaningful with one of `-cx'")));
      }

  /* If ready to unlink hierarchies, so we are for simpler files.  */
  if (recursive_unlink_option)
    unlink_first_option = true;

  /* Forbid using -c with no input files whatsoever.  Check that `-f -',
     explicit or implied, is used correctly.  */

  switch (subcommand_option)
    {
    case CREATE_SUBCOMMAND:
      if (input_files == 0 && !files_from_option)
	USAGE_ERROR ((0, 0, _("Cravenly refusing to create an empty archive")));
      break;

    case EXTRACT_SUBCOMMAND:
    case LIST_SUBCOMMAND:
    case DIFF_SUBCOMMAND:
      for (archive_name_cursor = archive_name_array;
	   archive_name_cursor < archive_name_array + archive_names;
	   archive_name_cursor++)
	if (!strcmp (*archive_name_cursor, "-"))
	  request_stdin ("-f");
      break;

    case CAT_SUBCOMMAND:
    case UPDATE_SUBCOMMAND:
    case APPEND_SUBCOMMAND:
      for (archive_name_cursor = archive_name_array;
	   archive_name_cursor < archive_name_array + archive_names;
	   archive_name_cursor++)
	if (!strcmp (*archive_name_cursor, "-"))
	  USAGE_ERROR ((0, 0,
			_("Options `-Aru' are incompatible with `-f -'")));

    default:
      break;
    }

  archive_name_cursor = archive_name_array;

  /* Report other conflicting options.  */

  if (no_attributes_option &&
      (owner_option != (uid_t) -1 || group_option != (gid_t) -1
       || numeric_owner_option || same_owner_option
       || mode_option || same_permissions_option || atime_preserve_option))
    USAGE_ERROR ((0, 0, _("Option --no-attributes conflicts with others")));

  /* Prepare for generating backup names.  */

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  if (backup_option)
    {
      backup_type = get_version (version_control_string);
      if (backup_type == none)
	backup_option = false;
    }

#if ENABLE_DALE_CODE
  /* Set up compression options.  */

  if (use_compress_program_option && !per_file_compress_option)
    compress_whole_archive = 1;
  if (compress_whole_archive && per_file_compress_option)
    USAGE_ERROR ((0, 0, _("More than one compression option specified")));
  if (per_file_compress_option && multi_volume_option)
    USAGE_ERROR ((0, 0, _("--file_compress incompatible with --multi-volume")));
  if (use_compress_program_option == NULL)
    /* When there are compressed entries to compare or extract, short of
       knowing better, we assume that `gzip' is the decompressor to use.  */
    set_use_compress_program_option ("gzip");
#endif

  /* Setup chekpoint dots processing.  */

  if (checkpoint_option)
    error_print_progname = flush_checkpoint_print_progname;
}

/* Tar proper.  */

/*-----------------------.
| Main routine for tar.	 |
`-----------------------*/

int
main (int argc, char *const *argv)
{
#if DOSWIN
  program_name = get_program_base_name (argv[0]);
#else
  program_name = argv[0];
#endif
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  exit_status = TAREXIT_SUCCESS;

#if DOSWIN
  /* The test suite needs the program_name to be just "tar".  (FIXME: What is
     the problem?)  We need to find the basename and drop the .EXE suffix, if
     any; any other solution could fail, since there are too many different
     shells on MS-DOS.  Does this (FIXME: what?) really work with every Unix
     shell?  */
  {
    char *p = (char *) program_name, *b = p, *e = p;

    for ( ; *p; p++)
      {
	if (*p == '/' || *p == '\\' || *p == ':')
	  b = p + 1;
	else if (*p == '.')
	  e = p + 1;
      }
    program_name = b;
    if (e > b && tolower (*e) == 'e'
	&& tolower (e[1]) == 'x' && tolower (e[2]) == 'e')
      e[-1] = '\0';
  }
#endif

  /* Pre-allocate a few structures.  */

  allocated_archive_names = 10;
  archive_name_array = (const char **)
    xmalloc (sizeof (const char *) * allocated_archive_names);
  archive_names = 0;

  initialize_name_strings ();

  /* Decode options.  */

  decode_options (argc, argv);

  /* Main command execution.  */

  if (volno_file_option)
    init_volume_number ();

  switch (subcommand_option)
    {
    case UNKNOWN_SUBCOMMAND:
      USAGE_ERROR ((0, 0,
		    _("You must specify one of the `-Acdtrx' options")));

    case CAT_SUBCOMMAND:
    case UPDATE_SUBCOMMAND:
    case APPEND_SUBCOMMAND:
#if ENABLE_DALE_CODE
      if (per_file_compress_option)
	init_compress_work_file();
#endif

      update_archive ();

#if ENABLE_DALE_CODE
      if (per_file_compress_option)
	delete_compress_work_file();
#endif
      break;

    case DELETE_SUBCOMMAND:
      delete_archive_members ();
      break;

    case CREATE_SUBCOMMAND:
      if (totals_option)
	init_total_written ();
#if ENABLE_DALE_CODE
      if (per_file_compress_option)
	init_compress_work_file();
#endif
      create_archive ();

#if ENABLE_DALE_CODE
      if (per_file_compress_option)
	delete_compress_work_file();
#endif
      if (totals_option)
	print_total_written ();
      break;

    case EXTRACT_SUBCOMMAND:
      extr_init ();
      read_and (extract_archive);
      break;

    case LIST_SUBCOMMAND:
      read_and (list_archive);
      break;

    case DIFF_SUBCOMMAND:
      diff_init ();
      read_and (diff_archive);
      break;
    }

  if (volno_file_option)
    closeout_volume_number ();

  /* Dispose of allocated memory, and return.  */

  free (archive_name_array);
  terminate_name_strings ();

  if (exit_status == TAREXIT_FAILURE)
    error (0, 0, _("Processed all files possible, despite earlier errors"));
  exit (exit_status);
}
