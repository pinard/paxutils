/* extern.h - External declarations for cpio.  Requires system.h.
   Copyright (C) 1990, 1991, 1992, 1998 Free Software Foundation, Inc.

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

#include "dstring.h"

enum archive_format
{
  arf_unknown, arf_binary, arf_oldascii, arf_newascii, arf_crcascii,
  arf_tar, arf_ustar, arf_gnutar, arf_hpoldascii, arf_hpbinary
};
extern enum archive_format archive_format;
extern int reset_time_flag;
extern int io_block_size;
extern int create_dir_flag;
extern int rename_flag;
extern char *rename_batch_file;
extern int table_flag;
extern int unconditional_flag;
extern int verbose_flag;
extern int dot_flag;
extern int link_flag;
extern int retain_time_flag;
extern int crc_i_flag;
extern int append_flag;
extern int swap_bytes_flag;
extern int swap_halfwords_flag;
extern int swapping_bytes;
extern int swapping_halfwords;
extern int set_owner_flag;
extern uid_t set_owner;
extern int set_group_flag;
extern gid_t set_group;
extern int no_chown_flag;
extern int sparse_flag;
extern int quiet_flag;
extern int only_verify_crc_flag;
extern int no_abs_paths_flag;

extern int last_header_start;
extern int copy_matching_files;
extern int numeric_uid;
extern char *pattern_file_name;
extern char *new_media_message;
extern char *new_media_message_with_number;
extern char *new_media_message_after_number;
extern int archive_des;
extern char *archive_name;
extern unsigned long crc;
#ifdef DEBUG_CPIO
extern int debug_flag;
#endif

extern char *input_buffer, *output_buffer;
extern char *in_buff, *out_buff;
extern long input_buffer_size;
extern long input_size, output_size;
#ifdef __GNUC__
extern long long input_bytes, output_bytes;
#else
extern long input_bytes, output_bytes;
#endif
extern char zeros_512[];
extern char *directory_name;
extern char **save_patterns;
extern int num_patterns;
extern char name_end;
extern char input_is_special;
extern char output_is_special;
extern char input_is_seekable;
extern char output_is_seekable;
extern int f_force_local;
extern char *program_name;
extern int (*xstat) ();
extern void (*copy_function) ();
extern char **pax_file_names;
extern int num_pax_file_names;
extern char no_block_message_flag;
extern char directory_recurse_flag;
extern char preserve_mode_flag;
extern char overwrite_existing_files;
extern char cross_device_flag;
extern unsigned long current_device;
extern char pax_rename_flag;

/* ?? */

/* Some globals that need the PARAMS macro.  */
struct new_cpio_header;
extern void (*header_writer) PARAMS ((struct new_cpio_header *, int));
extern void (*eof_writer) PARAMS ((int));
extern int (*name_too_long) PARAMS ((char *));

/* These functions are from the application-dependent file.  IE
   "cpio.c" for cpio, "pax.c" for pax.  */
void process_args PARAMS ((int, char *[]));

/* cdf.c */
void possibly_munge_cdf_directory_name  PARAMS ((char *,
						 struct new_cpio_header *));
#ifdef HPUX_CDF
char *add_cdf_double_slashes PARAMS ((char *));
#endif

/* copyin.c */
struct new_cpio_header;
void read_in_header PARAMS ((struct new_cpio_header *, int));
void process_copy_in PARAMS ((void));
void long_format PARAMS ((struct new_cpio_header *, char *));
void print_name_with_quoting PARAMS ((char *));
void tape_skip_padding PARAMS ((int, int));

/* copyout.c */
void write_out_header PARAMS ((struct new_cpio_header *, int));
void tape_pad_output PARAMS ((int, int));
void process_copy_out PARAMS ((void));

/* copypass.c */
void process_copy_pass PARAMS ((void));
int link_to_maj_min_ino PARAMS ((char *, int, int, int));
int link_to_name PARAMS ((char *, char *));

/* dirname.c */
char *dirname PARAMS ((char *));

/* filemode.c */
void mode_string PARAMS ((unsigned int, char *));

/* format.c */
enum archive_format find_format PARAMS ((char *));
void format_error PARAMS ((char *));
void set_write_pointers_from_format PARAMS ((enum archive_format));
char *format_name PARAMS ((enum archive_format));

/* fmtcpio.c */
void read_in_old_ascii PARAMS ((struct new_cpio_header *, int));
void read_in_new_ascii PARAMS ((struct new_cpio_header *, int));
void read_in_binary PARAMS ((struct new_cpio_header *, int));
void swab_array PARAMS ((char *, int));
void write_out_cpioascii_header PARAMS ((struct new_cpio_header *, int));
void write_out_oldascii_header PARAMS ((struct new_cpio_header *, int));
void write_out_oldcpio_header PARAMS ((struct new_cpio_header *, int));
void write_cpio_eof PARAMS ((int));
int is_cpio_filename_too_long PARAMS ((char *));

/* getfile.c */
int get_next_file_name PARAMS ((dynamic_string *));

/* idcache.c */
#ifndef __MSDOS__
char *getgroup ();
char *getuser ();
uid_t *getuidbyname ();
gid_t *getgidbyname ();
#endif

/* main.c */
void process_args PARAMS ((int, char *[]));
void initialize_buffers PARAMS ((void));

/* makepath.c */
int make_path PARAMS ((char *, int, int, uid_t, gid_t, char *));

/* octal.c */
int otoa PARAMS ((char *, unsigned long *));
void to_oct PARAMS ((long, int, char *));

/* rename.c */
char *possibly_rename_file PARAMS ((char *));
void add_rename_regexp PARAMS ((char *));

/* stripslash.c */
void strip_trailing_slashes PARAMS ((char *));

/* tar.c */
void write_out_tar_header PARAMS ((struct new_cpio_header *, int));
void read_in_tar_header PARAMS ((struct new_cpio_header *, int));
enum archive_format is_tar_header PARAMS ((char *));
int is_tar_filename_too_long PARAMS ((char *));
void write_tar_eof PARAMS ((int));

/* userspec.c */
#ifndef __MSDOS__
char *parse_user_spec PARAMS ((char *, uid_t *, gid_t *, char **, char **));
#endif

/* util.c */
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
int is_dot_or_dotdot PARAMS ((char *));

#define DISK_IO_BLOCK_SIZE	512
