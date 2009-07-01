/* Common declarations for paxutils archiving programs.
   Copyright (C) 1988,90,91,92,93,94,96,97,98,99 Free Software Foundation, Inc.

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
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Declare various archive formats and constants.  */
#include "cpio.h"
#include "tar.h"

/* [tar] Value of the checksum field while the checksum is being computed.  */
#define CHKBLANKS	"        "	/* 8 blanks, no null */

/* [tar] Some constants from POSIX are given names.  */
#define NAME_FIELD_SIZE 100
#define LINKNAME_FIELD_SIZE 100
#define PREFIX_FIELD_SIZE 155
#define UNAME_FIELD_SIZE 32
#define GNAME_FIELD_SIZE 32

/* [tar] POSIX specified symbols currently unused are undefined here.  */
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

/* [tar] Format of a tar entry in memory.  */

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

#define MINIMUM(One, Two) \
  ((One) < (Two) ? (One) : (Two))
#define MAXIMUM(One, Two) \
  ((One) > (Two) ? (One) : (Two))
/* For rounding up, V+D-1-(V+D-1)%D, V>=0, uses only one multiplying operator,
   and lets good compilers do better CSE than with V+D-1-(V-1)%D, V>0.  I like
   it more than either V%D?V+D-V%D:V, (V+D-1)/D*D or V+(D-V%D)%D.  */
#define ROUNDUP(Value, Divisor) \
  ((Value) + (Divisor) - 1 - ((Value) + (Divisor) - 1) % (Divisor))
/* Equivalent of ROUNDUP (Value, Divisor) - Value.  */
#define PADDING(Value, Divisor) \
  (((Divisor) - (Value % Divisor)) % (Divisor))

/* [tar] */

#define	STDIN 0			/* file descriptor for standard input */
#define	STDOUT 1		/* file descriptor for standard output */

#define TAREXIT_SUCCESS 0
#define TAREXIT_DIFFERS 1
#define TAREXIT_FAILURE 2

/* [tar] Both WARN and ERROR write a message on stderr and continue processing,
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

/* [tar] Main command option.  */

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

/* [tar] Structure for keeping track of filenames and lists thereof.  */
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

/* [tar] Pointer to the start of the scratch space.  */
struct sp_array
  {
    off_t offset;
    size_t numbytes;
  };

/* [tar] A file is new enough for backup if its ctime or its mtime is more
   recent than --after-date (-N) value, or else, if the option is just not
   used.  If --newer-mtime is used instead, then ctime is not tested.  Note
   that newer_mtime_option is always implied by newer_ctime_option, yet this
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
    || time_compare ((Stat)->st_mtime, time_option_threshold) >= 0)	\
   && (!newer_ctime_option						\
       || time_compare ((Stat)->st_ctime, time_option_threshold) >= 0))

#if ENABLE_DALE_CODE
/* [tar] Shortest and longest files that will be file-compressed.  */
#define MINIMUM_FILE_COMPRESS_SIZE (BLOCKSIZE + 1)
#define MAXIMUM_FILE_COMPRESS_SIZE 100000000
#endif

/* Declarations for each module.  */

/* [cpio] Some globals that need the PARAMS macro.  */
struct new_cpio_header;
extern void (*header_writer) PARAMS ((struct new_cpio_header *, int));
extern void (*eof_writer) PARAMS ((int));
extern bool (*name_too_long) PARAMS ((char *));

/* [cpio] These functions are from the application-dependent file.  IE
   "cpio.c" for cpio, "pax.c" for pax.  */
void process_args PARAMS ((int, char *[]));

/* [cpio] Module main.c.  */
void process_args PARAMS ((int, char *[]));
void initialize_buffers PARAMS ((void));

/* [tar] FIXME: compare.c should not directly handle the following variable,
   instead, this should be done in buffer.c only.  */

enum access_mode
{
  ACCESS_READ,
  ACCESS_WRITE,
  ACCESS_UPDATE
};
extern enum access_mode access_mode;

/* [tar] Module buffer.c.  */

extern FILE *stdlis;
extern char *save_name;
extern off_t save_sizeleft;
extern off_t save_totsize;
extern bool write_archive_to_stdout;

size_t available_space_after PARAMS ((union block *));
off_t current_block_ordinal PARAMS ((void));
void close_tar_archive PARAMS ((void));
void closeout_volume_number PARAMS ((void));
union block *find_next_block PARAMS ((void));
void flush_read PARAMS ((void));
void flush_write PARAMS ((void));
void flush_archive PARAMS ((void));
void init_total_written PARAMS ((void));
void init_volume_number PARAMS ((void));
void open_tar_archive PARAMS ((enum access_mode));
void print_total_written PARAMS ((void));
void reset_eof PARAMS ((void));
void set_next_block_after PARAMS ((union block *));

/* [cpio] Module cdf.c.  */
void possibly_munge_cdf_directory_name  PARAMS ((char *,
						 struct new_cpio_header *));
#ifdef HPUX_CDF
char *add_cdf_double_slashes PARAMS ((char *));
#endif

/* [cpio] Module copyin.c.  */
struct new_cpio_header;
void read_in_header PARAMS ((struct new_cpio_header *, int));
void process_copy_in PARAMS ((void));
void long_format PARAMS ((struct new_cpio_header *, char *));
void print_name_with_quoting PARAMS ((char *));
void tape_skip_padding PARAMS ((int, int));

/* [cpio] Module copyout.c.  */
void write_out_header PARAMS ((struct new_cpio_header *, int));
void tape_pad_output PARAMS ((int, int));
void process_copy_out PARAMS ((void));

/* [cpio] Module copypass.c.  */
void process_copy_pass PARAMS ((void));
int link_to_maj_min_ino PARAMS ((char *, int, int, int));
int link_to_name PARAMS ((char *, char *));

/* [tar] Module create.c.  */

void create_archive PARAMS ((void));
void dump_file PARAMS ((char *, dev_t, bool));
void finish_header PARAMS ((union block *));
void write_eot PARAMS ((void));

/* [tar] Module diffarch.c.  */

extern bool now_verifying;

void diff_archive PARAMS ((void));
void diff_init PARAMS ((void));
void verify_volume PARAMS ((void));

/* [tar] Module extract.c.  */

void extr_init PARAMS ((void));
void extract_archive PARAMS ((void));
void apply_delayed_set_stat PARAMS ((void));

/* [tar] Module delete.c.  */

void delete_archive_members PARAMS ((void));

/* [cpio] Module fmtcpio.c.  */
void read_in_old_ascii PARAMS ((struct new_cpio_header *, int));
void read_in_new_ascii PARAMS ((struct new_cpio_header *, int));
void read_in_binary PARAMS ((struct new_cpio_header *, int));
void swab_array PARAMS ((char *, int));
void write_out_cpioascii_header PARAMS ((struct new_cpio_header *, int));
void write_out_oldascii_header PARAMS ((struct new_cpio_header *, int));
void write_out_oldcpio_header PARAMS ((struct new_cpio_header *, int));
void write_cpio_eof PARAMS ((int));
bool is_cpio_filename_too_long PARAMS ((char *));

/* [cpio] Module fmttar.c.  */
void write_out_tar_header PARAMS ((struct new_cpio_header *, int));
void read_in_tar_header PARAMS ((struct new_cpio_header *, int));
enum archive_format is_tar_header PARAMS ((char *));
bool is_tar_filename_too_long PARAMS ((char *));
void write_tar_eof PARAMS ((int));

/* [cpio] Module format.c.  */
enum archive_format find_format PARAMS ((char *));
void format_error PARAMS ((char *));
void set_write_pointers_from_format PARAMS ((enum archive_format));
char *format_name PARAMS ((enum archive_format));

/* [cpio] Module getfile.c.  */
bool get_next_file_name PARAMS ((dynamic_string *));

/* Module global.c.  */

#include "global.h"
extern char zero_block[BLOCKSIZE];

/* [tar] Module header.c.  */

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

/* [tar] Module incremen.c.  */

void collect_and_sort_names PARAMS ((void));
char *get_directory_contents PARAMS ((char *, dev_t));
void write_dir_file PARAMS ((void));
void incremental_restore PARAMS ((int));
void write_directory_file PARAMS ((void));

/* [tar] Module list.c.  */

void list_archive PARAMS ((void));
void print_for_mkdir PARAMS ((char *, int, mode_t));
void print_header PARAMS ((struct tar_entry *));

/* [tar] Module mangle.c.  */

void extract_mangle PARAMS ((void));

/* [tar] Module misc.c.  */

extern enum backup_type backup_type;

#if DOSWIN
int time_compare (time_t, time_t);
#else
# define time_compare(Arg1, Arg2) \
   ((Arg1) - (Arg2))
#endif

bool is_pattern PARAMS ((const char *));
#if DOSWIN
const char *get_program_base_name PARAMS ((const char *));
void unixify_file_name PARAMS ((char *));
#endif
void assign_string PARAMS ((char **, const char *));
char *quote_copy_string PARAMS ((const char *));
bool unquote_string PARAMS ((char *));
char *concat_no_slash PARAMS ((const char *, const char *));
char *concat_with_slash PARAMS ((const char *, const char *));

char *merge_sort PARAMS ((char *, int, int, int (*) (char *, char *)));

bool is_dot_or_dotdot PARAMS ((const char *));
void xclose PARAMS ((int));
void xdup2 PARAMS ((int, int, const char *));
bool remove_any_file PARAMS ((const char *, bool));
bool maybe_backup_file PARAMS ((const char *, bool));
void undo_last_backup PARAMS ((void));

void init_progress_dots PARAMS ((unsigned));
void flush_progress_dots PARAMS ((void));
void output_progress_dot PARAMS ((void));

/* [tar] Module names.c.  */

extern struct name *name_list_head;
extern struct name *name_list_current;
extern unsigned name_list_unmatched_count;

void add_name_string PARAMS ((const char *));
void initialize_name_strings PARAMS (());
char *next_name_string PARAMS ((bool));
void terminate_name_strings PARAMS ((void));

void add_name PARAMS ((const char *));
void name_gather PARAMS ((void));
struct name *find_matching_name PARAMS ((const char *, bool));
char *next_unprocessed_name PARAMS ((void));
void prepare_to_reprocess_names PARAMS ((void));
void report_unprocessed_names PARAMS ((void));

void add_exclude PARAMS ((char *));
void add_exclude_file PARAMS ((const char *));
bool check_exclude PARAMS ((const char *));

/* [tar] Module reading.c.  */

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

/* [cpio] Module rename.c.  */
char *possibly_rename_file PARAMS ((char *));
void add_rename_regexp PARAMS ((char *));

/* [tar] Module tar.c.  */

bool confirm PARAMS ((const char *, const char *));
char *get_reply PARAMS ((const char *, char *, size_t));

/* [tar] Module update.c.  */

extern char *output_start;
extern bool time_to_start_writing;

void update_archive PARAMS ((void));

/* [cpio] Module util.c.  */
void initialize_buffers PARAMS ((void));
void tape_empty_output_buffer PARAMS ((int));
void disk_empty_output_buffer PARAMS ((int));
void swahw_array PARAMS ((char *, int));
void tape_buffered_write PARAMS ((char *, int, long));
void tape_buffered_read PARAMS ((char *, int, long));
int tape_buffered_peek PARAMS ((char *, int, int));
void tape_toss_input PARAMS ((int, long));
void copy_files_tape_to_disk PARAMS ((int, int, long));
void copy_files_disk_to_tape PARAMS ((int, int, long, char *));
void copy_files_disk_to_disk PARAMS ((int, int, long, char *));
void create_all_directories PARAMS ((char *));
void prepare_append PARAMS ((int));
char *find_inode_file PARAMS ((unsigned long, unsigned long, unsigned long));
void add_inode PARAMS ((unsigned long, char *, unsigned long, unsigned long));
int open_archive PARAMS ((char *));
void tape_offline PARAMS ((int));
void get_next_reel PARAMS ((int));
void set_new_media_message PARAMS ((char *));
#if defined(__MSDOS__) && !defined(__GNUC__)
int chown PARAMS ((char *, int, int));
#endif
#ifdef __TURBOC__
int utime PARAMS ((char *, struct utimbuf *));
#endif

/* [cpio] ?? */
#define DISK_IO_BLOCK_SIZE 512
