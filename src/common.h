/* Common declarations for the tar program.
   Copyright (C) 1988, 92, 93, 94, 96, 97, 98 Free Software Foundation, Inc.

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

/* Declare the tar archive format.  */
#include "tar.h"

/* The checksum field is filled with this while the checksum is computed.  */
#define CHKBLANKS	"        "	/* 8 blanks, no null */

/* Some constants from POSIX are given names.  */
#define NAME_FIELD_SIZE 100
#define LINKNAME_FIELD_SIZE 100
#define PREFIX_FIELD_SIZE 155
#define UNAME_FIELD_SIZE 32
#define GNAME_FIELD_SIZE 32

/* POSIX specified symbols currently unused are undefined here.  */
#undef TSUID
#undef TSGID
#undef TSVTX
#undef TUREAD
#undef TUWRITE
#undef TUEXEC
#undef TGREAD
#undef TGWRITE
#undef TGEXEC
#undef TOREAD
#undef TOWRITE
#undef TOEXEC

/* Format of a tar entry in memory.  */

struct tar_entry
{
  enum archive_format format;	/* archive format at this header */
  union block *block;		/* pointer to tar entry header block */
  char *name;			/* name of (file in) entry */
  char *linkname;		/* name of link in entry */
  struct stat stat;		/* attributes of associated file */
#if 0
  char *name_to_free;		/* start of allocated name */
  char *linkname_to_free;	/* start of allocated linkname */
#endif
  size_t name_allocated;	/* buffer size allocated for name */
  size_t linkname_allocated;	/* buffer size allocated for linkname */
};

/* Some various global definitions.  */

#define	STDIN 0			/* file descriptor for standard input */
#define	STDOUT 1		/* file descriptor for standard output */

/* GLOBAL is defined to empty in `tar.c' only, and left alone in other
   modules.  Set it to "extern" if it is not already set.  `tar' depends on
   the system loader to preset all GLOBAL variables to neutral (zero or
   false) values: GLOBAL initialisations are not explicitly done.  */
#ifndef GLOBAL
# define GLOBAL extern
#endif

/* Exit status for tar.  Let's try to keep this list as simple as possible.
   -d option strongly invites a status different for unequal comparison and
   other errors.  */
GLOBAL int exit_status;

#define TAREXIT_SUCCESS 0
#define TAREXIT_DIFFERS 1
#define TAREXIT_FAILURE 2

/* Both WARN and ERROR write a message on stderr and continue processing,
   however ERROR manages so tar will exit unsuccessfully.  FATAL_ERROR
   writes a message on stderr and aborts immediately, with another message
   line telling so.  USAGE_ERROR works like FATAL_ERROR except that the
   other message line suggests trying --help.  All four macros accept a
   single argument of the form ((0, errno, _("FORMAT"), Args...)).  `errno'
   is `0' when the error is not being detected by the system.  */

#define WARN(Args) \
  error Args
#define ERROR(Args) \
  (error Args, exit_status = TAREXIT_FAILURE)
#define FATAL_ERROR(Args) \
  (error Args, error (TAREXIT_FAILURE, 0, \
		      _("Error is not recoverable: exiting now")), 0)
#define USAGE_ERROR(Args) \
  (error Args, usage (TAREXIT_FAILURE), 0)

/* Information gleaned from the command line.  */

#include "arith.h"
#include "backupfile.h"
#include "modechange.h"

/* Name of this program.  */
GLOBAL const char *program_name;

/* Main command option.  */

enum subcommand
{
  UNKNOWN_SUBCOMMAND,		/* none of the following */
  APPEND_SUBCOMMAND,		/* -r */
  CAT_SUBCOMMAND,		/* -A */
  CREATE_SUBCOMMAND,		/* -c */
  DELETE_SUBCOMMAND,		/* -D */
  DIFF_SUBCOMMAND,		/* -d */
  EXTRACT_SUBCOMMAND,		/* -x */
  LIST_SUBCOMMAND,		/* -t */
  UPDATE_SUBCOMMAND		/* -u with -c (implied or explicit) */
};

GLOBAL enum subcommand subcommand_option;

/* Selected format for output archive.  */
GLOBAL enum archive_format archive_format;

/* Either NL or NUL, as decided by the --null option.  */
GLOBAL char filename_terminator;

/* Size of each record, once in blocks, once in bytes.  Those two variables
   are always related, the second being BLOCKSIZE times the first.  They do
   not have _option in their name, even if their values is derived from
   option decoding, as these are especially important in tar.  */
GLOBAL int blocking_factor;
GLOBAL size_t record_size;

/* Respect slash prefixes.  */
GLOBAL bool absolute_names_option;

/* Restore atime after access.  */
GLOBAL bool atime_preserve_option;

/* Backup files before overwriting them.  */
GLOBAL bool backup_option;

/* Add block number to verbose messages.  This is sometimes useful to debug
   bad tar archives.  */
GLOBAL bool block_number_option;

/* Write a progress message every ten records.  */
GLOBAL bool checkpoint_option;

/* Specified name of compression program, or "gzip" as implied by -z.  */
GLOBAL const char *use_compress_program_option;

/* Do not consider symlinks themselves as objects, but rather only what they
   point to.  In a word, follow symbolic links.  */
GLOBAL bool dereference_option;

/* There were exclude options, so trigger exclude processing.  */
GLOBAL bool exclude_option;

/* Specified file containing names to work on.  */
GLOBAL const char *files_from_option;

/* Do not consider remote file syntax.  */
GLOBAL bool force_local_option;

/* Specified value to be put into tar file in place of stat () results, or
   just -1 if such an override should not take place.  */
GLOBAL gid_t group_option;

/* Do not reflect failed reads into the exit status.  */
GLOBAL bool ignore_failed_read_option;

/* Merely skip zero blocks, do not consider them as possible end of files.
   This cannot be the default, because Unix tar writes two blocks of zeros,
   then pads out the record with garbage.  */
GLOBAL bool ignore_zeros_option;

/* Use incremental mode while dumping or restoring, whether listed or not.
   When making a dump, save directories at the beginning of the archive, and
   include in each directory its contents.  */
GLOBAL bool incremental_option;

/* Specified name of script to run at end of each tape change.  */
GLOBAL const char *info_script_option;

/* Ask confirmation before acting on archive entries.  */
GLOBAL bool interactive_option;

/* Don't overwrite existing files.  */
GLOBAL bool keep_old_files_option;

/* Specified file name for incremental list.  */
GLOBAL const char *listed_incremental_option;

/* Specified mode change string.  */
GLOBAL struct mode_change *mode_option;

/* Make multivolume archive.  When we cannot write any more into the
   archive, re-open it, and continue writing.  */
GLOBAL bool multi_volume_option;

/* Set if --after-date was given.  See the description for
   time_option_threshold, which this variable qualifies.  */
GLOBAL bool newer_ctime_option;

/* Set if either --newer-mtime or --after-date was given.  See the description
   for time_option_threshold, which this variable qualifies.  */
GLOBAL bool newer_mtime_option;

/* Do not restore any file attribute.  */
GLOBAL bool no_attributes_option;

/* Do not dump directories contents recursively.  */
GLOBAL bool no_recurse_option;

/* Use numeric ids directly, do not translate symbolic ids to numbers.  */
GLOBAL bool numeric_owner_option;

/* Do not traverse filesystem boundaries while dumping.  */
GLOBAL bool one_file_system_option;

/* Specified value to be put into tar file in place of stat () results, or
   just -1 if such an override should not take place.  */
GLOBAL uid_t owner_option;

/* When deleting directories in the way, recursively delete their contents.  */
GLOBAL bool recursive_unlink_option;

/* Try to reblock input records, like when reading 4.2BSD pipes.  */
GLOBAL bool read_full_records_option;

/* Delete files after having dumped them.  */
GLOBAL bool remove_files_option;

/* Specified remote shell command.  */
GLOBAL const char *rsh_command_option;

/* The list of names to extract are already sorted in the archive order, so
   this list does not have to be swallowed in memory in advance.  */
GLOBAL bool same_order_option;

/* Restore the owner and group attributes as per the archive.  */
GLOBAL bool same_owner_option;

/* Restore the permission attributes as per the archive, disobeying the
   current umask.  */
GLOBAL bool same_permissions_option;

/* Warn about each skipped directory, while reading an archive.  */
GLOBAL bool show_omitted_dirs_option;

/* Attempt to handle files having big zeroed holes in them.  */
GLOBAL bool sparse_option;

/* Resume work at a given entry, usually after a previous run which failed.  */
GLOBAL bool starting_file_option;

/* Specified maximum byte length of each tape volume (multiple of 1024).  */
GLOBAL tarlong tape_length_option;

/* Extract entries on standard output, instead of on disk.  */
GLOBAL bool to_stdout_option;

/* Echo the total size of the created archive.  */
GLOBAL bool totals_option;

/* Do not restore timestamps from the archive, let extracted files be new.  */
GLOBAL bool touch_option;

/* Delete files before extracting over them, loose previous file nature and
   attributes.  This flag is also set when recursively deleting directory
   hierarchies, as the most implies the least.  */
GLOBAL bool unlink_first_option;

/* With create subcommand, rather update entries in preexisting archive only
   from newer files.  With extract subcommand, only replace files if archive
   entry is newer.  */
GLOBAL bool update_option;

/* Count how many times the option has been set, multiple setting yields
   more verbose behavior.  Value 0 means no verbosity, 1 means file name
   only, 2 means file name and all attributes.  More than 2 is just like 2.  */
GLOBAL int verbose_option;

/* Read after write for media checking purposes.  */
GLOBAL bool verify_option;

/* Specified name of file containing the volume number.  */
GLOBAL const char *volno_file_option;

/* Specified value or pattern.  */
GLOBAL const char *volume_label_option;

/* Other global definitions.  */

/* File descriptor for archive file.  */
GLOBAL int archive;

/* True when outputting to /dev/null.  */
GLOBAL bool dev_null_output;

/* Name prefix to apply to all files in created archive, and to require for
   all files before extracting them.  */
GLOBAL const char *name_prefix_option;
GLOBAL size_t name_prefix_length;

/* List of tape drive names, number of such tape drives, allocated number,
   and current cursor in list.  */
GLOBAL const char **archive_name_array;
GLOBAL int archive_names;
GLOBAL int allocated_archive_names;
GLOBAL const char **archive_name_cursor;

/* Structure for keeping track of filenames and lists thereof.  */
struct name
  {
    struct name *next;		/* for chaining of name entries */
    bool is_chdir;		/* entry for introducing a -C value */
    bool first_is_literal;	/* first character is literally matched */
    bool is_wildcard;		/* this name is a wildcard, not a literal */
    bool match_found;		/* a matching file has been found */
    char *change_dir;		/* set with the -C option */
    const char *dir_contents;	/* for incremental_option */
    size_t length;		/* cached strlen (name) */
    char name[1];		/* varylength string holding the name */
  };

/* Pointer to the start of the scratch space.  */
struct sp_array
  {
    off_t offset;
    size_t numbytes;
  };
GLOBAL struct sp_array *sparsearray;

/* Initial size of the sparsearray.  */
GLOBAL int sp_array_size;

/* If both newer_ctime_option and newer_mtime_option are false, then
   time_option_threshold is irrelevant.  If newer_ctime_option is false and
   newer_mtime_option is true, then files get archived if their mtime is
   greater than time_option_threshold.  If newer_ctime_option is true, then
   newer_mtime_option always gets selected, so files get archived if *either*
   their ctime or mtime is greater than time_option_threshold.  */
GLOBAL time_t time_option_threshold;

/* A file is new enough for backup if its ctime or its mtime is more recent
   than --after-date (-N) value, or else, if the option is just not used.  If
   --newer-mtime is used instead, then ctime is not tested.  Note that
   newer_mtime_option is always implied by newer_ctime_option, yet this
   implication is not used in the definition for FILE_IS_NEW_ENOUGH, below.

   Dave Coffin reports that his CMOS clock once got set to the year 2097
   instead of 1997 before files were created, and that Linux read the DOS
   year 2097 as 1961, because of the peculiar way UNIX stores dates:

	0x00000000 is 1970-01-01 00:00:00,
	0x7fffffff is 2038-01-19 03:14:07,
	0x80000000 is 1901-12-13 20:45:52 and
	0xffffffff is 1969-12-31 23:59:59.

   To break the 2038 barrier, Unix should change time_t from 32-bit long to
   32-bit unsigned long, or maybe go to 64 bits.

   When tar collapsed newer_mtime_option and time_option_threshold into a
   single variable, using zero value as *also* meaning a very old file, it
   broke on negative dates, rejecting the files when --newer flag was not
   used.  Dave also suggested presetting the single variable to MININT (or
   0x80000000), or using (unsigned) casts, so that 1961 becomes 2097.  But
   these solutions would require even more configuration trickery.  */

#define FILE_IS_NEW_ENOUGH(Stat) \
  ((!newer_mtime_option							\
    || time_compare ((Stat)->st_mtime, time_option_threshold) > 0)	\
   && (!newer_ctime_option						\
       || time_compare ((Stat)->st_ctime, time_option_threshold)) > 0)

/* Declarations for each module.  */

/* FIXME: compare.c should not directly handle the following variable,
   instead, this should be done in buffer.c only.  */

enum access_mode
{
  ACCESS_READ,
  ACCESS_WRITE,
  ACCESS_UPDATE
};
extern enum access_mode access_mode;

/* Module buffer.c.  */

extern FILE *stdlis;
extern char *save_name;
extern off_t save_sizeleft;
extern off_t save_totsize;
extern bool write_archive_to_stdout;

size_t available_space_after PARAMS ((union block *));
off_t current_block_ordinal PARAMS ((void));
void close_archive PARAMS ((void));
void closeout_volume_number PARAMS ((void));
union block *find_next_block PARAMS ((void));
void flush_checkpoint_line PARAMS ((void));
void flush_read PARAMS ((void));
void flush_write PARAMS ((void));
void flush_archive PARAMS ((void));
void init_total_written PARAMS ((void));
void init_volume_number PARAMS ((void));
void open_archive PARAMS ((enum access_mode));
void print_total_written PARAMS ((void));
void reset_eof PARAMS ((void));
void set_next_block_after PARAMS ((union block *));

/* Module create.c.  */

void create_archive PARAMS ((void));
void dump_file PARAMS ((char *, dev_t, bool));
void finish_header PARAMS ((union block *));
void write_eot PARAMS ((void));

/* Module diffarch.c.  */

extern bool now_verifying;

void diff_archive PARAMS ((void));
void diff_init PARAMS ((void));
void verify_volume PARAMS ((void));

/* Module extract.c.  */

void extr_init PARAMS ((void));
void extract_archive PARAMS ((void));
void apply_delayed_set_stat PARAMS ((void));

/* Module delete.c.  */

void delete_archive_members PARAMS ((void));

/* Module header.c.  */

size_t get_extended_header_numbytes PARAMS ((union block *, int));
off_t get_extended_header_offset PARAMS ((union block *, int));
time_t get_header_atime PARAMS ((union block *));
int get_header_chksum PARAMS ((union block *));
time_t get_header_ctime PARAMS ((union block *));
major_t get_header_devmajor PARAMS ((union block *));
minor_t get_header_devminor PARAMS ((union block *));
gid_t get_header_gid PARAMS ((union block *));
mode_t get_header_mode PARAMS ((union block *));
time_t get_header_mtime PARAMS ((union block *));
off_t get_header_offset PARAMS ((union block *));
off_t get_header_realsize PARAMS ((union block *));
off_t get_header_size PARAMS ((union block *));
uid_t get_header_uid PARAMS ((union block *));
size_t get_initial_header_numbytes PARAMS ((union block *, int));
off_t get_initial_header_offset PARAMS ((union block *, int));
void set_extended_header_numbytes PARAMS ((union block *, int, size_t));
void set_extended_header_offset PARAMS ((union block *, int, off_t));
void set_header_atime PARAMS ((union block *, time_t));
void set_header_chksum PARAMS ((union block *, int));
void set_header_ctime PARAMS ((union block *, time_t));
void set_header_devmajor PARAMS ((union block *, major_t));
void set_header_devminor PARAMS ((union block *, minor_t));
void set_header_gid PARAMS ((union block *, gid_t));
void set_header_mode PARAMS ((union block *, mode_t));
void set_header_mtime PARAMS ((union block *, time_t));
void set_header_offset PARAMS ((union block *, off_t));
void set_header_realsize PARAMS ((union block *, off_t));
void set_header_size PARAMS ((union block *, off_t));
void set_header_uid PARAMS ((union block *, uid_t));
void set_initial_header_numbytes PARAMS ((union block *, int, size_t));
void set_initial_header_offset PARAMS ((union block *, int, off_t));

/* Module incremen.c.  */

void collect_and_sort_names PARAMS ((void));
char *get_directory_contents PARAMS ((char *, dev_t));
void write_dir_file PARAMS ((void));
void incremental_restore PARAMS ((int));
void write_directory_file PARAMS ((void));

/* Module list.c.  */

void list_archive PARAMS ((void));
void print_for_mkdir PARAMS ((char *, int, mode_t));
void print_header PARAMS ((struct tar_entry *));

/* Module mangle.c.  */

void extract_mangle PARAMS ((void));

/* Module misc.c.  */

extern enum backup_type backup_type;

#if DOSWIN
int time_compare (time_t, time_t);
#else
# define time_compare(Arg1, Arg2) \
   ((Arg1) - (Arg2))
#endif

void assign_string PARAMS ((char **, const char *));
char *quote_copy_string PARAMS ((const char *));
bool unquote_string PARAMS ((char *));
char *concat_no_slash PARAMS ((const char *, const char *));
char *concat_with_slash PARAMS ((const char *, const char *));

char *merge_sort PARAMS ((char *, int, int, int (*) (char *, char *)));

bool is_dot_or_dotdot PARAMS ((const char *));
bool remove_any_file PARAMS ((const char *, bool));
bool maybe_backup_file PARAMS ((const char *, bool));
void undo_last_backup PARAMS ((void));

/* Module names.c.  */

extern struct name *name_list_head;
extern struct name *name_list_tail;
extern struct name *name_list_current;

void init_names PARAMS ((void));
void name_add PARAMS ((const char *));
void name_init PARAMS ((int, char *const *));
void name_term PARAMS ((void));
char *name_next PARAMS ((bool));
void name_close PARAMS ((void));
void name_gather PARAMS ((void));
void add_name PARAMS ((const char *));
bool name_match PARAMS ((const char *));
void names_notfound PARAMS ((void));
void name_expand PARAMS ((void));
struct name *name_scan PARAMS ((const char *));
char *name_from_list PARAMS ((void));
void blank_name_list PARAMS ((void));

void add_exclude PARAMS ((char *));
void add_exclude_file PARAMS ((const char *));
bool check_exclude PARAMS ((const char *));

/* Module reading.c.  */

extern struct tar_entry current;

enum read_header
{
  HEADER_STILL_UNREAD,		/* for when read_header has not been called */
  HEADER_SUCCESS,		/* header successfully read and checksummed */
  HEADER_ZERO_BLOCK,		/* zero block where header expected */
  HEADER_END_OF_FILE,		/* true end of file while header expected */
  HEADER_FAILURE		/* ill-formed header, or bad checksum */
};

void decode_header PARAMS ((struct tar_entry *, bool));
void read_and PARAMS ((void (*do_) ()));
enum read_header read_header PARAMS ((struct tar_entry *));
void skip_extended_headers PARAMS ((void));
void skip_file PARAMS ((off_t));

/* Module tar.c.  */

bool confirm PARAMS ((const char *, const char *));
char *get_reply PARAMS ((const char *, char *, size_t));

/* Module update.c.  */

extern char *output_start;
extern bool time_to_start_writing;

void update_archive PARAMS ((void));
