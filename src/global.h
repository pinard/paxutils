/* Global variables for paxutils archiving programs.
   Copyright (C) 1999 Free Software Foundation, Inc.

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

/* GLOBAL is defined differently in global.c.  */
#ifndef GLOBAL
# define GLOBAL(Type, Variable, Init) extern Type Variable
# define GLOBAL_NO_INIT(Type, Variable) extern Type Variable
#endif

typedef void (*copy_function_t) PARAMS (());
typedef void (*eof_writer_t) PARAMS ((int));
typedef void (*header_writer_t) PARAMS ((struct new_cpio_header *, int));
typedef bool (*name_too_long_t) PARAMS ((char *));
typedef int (*xstat_t) PARAMS (());

/* Name of this program.  */
GLOBAL (const char *, program_name, NULL);

/* Where we write list messages (not errors, not interactions) to.  Stdout
   unless we're writing a pipe, in which case stderr.  */
GLOBAL (FILE *, stdlis, stdout);

/* [tar] Exit status.  Let's try to keep this list as simple as possible.  -d
   option strongly invites a status different for unequal comparison and other
   errors.  */
GLOBAL (int, exit_status, 0);

/* [tar] Main command option.  */
GLOBAL (enum subcommand, subcommand_option, 0);

/* [tar] Selected format for output archive.  */
GLOBAL (enum archive_format, archive_format, UNKNOWN_FORMAT);
/* [cpio] The header format to recognize and produce.  */
GLOBAL (enum archive_format, archive_format, UNKNOWN_FORMAT);

/* [tar] Size of each record, once in blocks, once in bytes.  Those two
   variables are always related, the second being BLOCKSIZE times the first.
   They do not have _option in their name, even if their values is derived
   from option decoding, as these are especially important in tar.  */
GLOBAL (int, blocking_factor, 0);
GLOBAL (size_t, record_size, 0);

/* [tar] Respect slash prefixes.  */
GLOBAL (bool, absolute_names_option, false);

/* [cpio] Append to end of archive.  (-A) */
GLOBAL (bool, append_option, false);

/* [tar] Restore atime after access.  */
GLOBAL (bool, atime_preserve_option, false);

/* [tar] Backup files before overwriting them.  */
GLOBAL (bool, backup_option, false);

/* [tar] Add block number to verbose messages.  This is sometimes useful to
   debug bad tar archives.  */
GLOBAL (bool, block_number_option, false);

/* [tar] Write a progress message every ten records.  */
GLOBAL (bool, checkpoint_option, false);

/* [tar] Specified name of compression program, or "gzip" as implied by -z.  */
GLOBAL (const char *, use_compress_program_option, NULL);

/* [cpio] Set true if crc_flag is true and we are doing a cpio -i.  Used by
   copy_files so it knows whether to compute the crc.  */
GLOBAL (bool, crc_i_flag, false);

/* [cpio] Create directories as needed.  (-d with -i or -p) */
GLOBAL (bool, make_directories_option, false);

/* [cpio] We can cross devices while making archive.  */
GLOBAL (bool, cross_device_flag, true);

/* [cpio] Current device id.  Used to implement no-device-crossing.  */
GLOBAL_NO_INIT (unsigned long, current_device);

#ifdef DEBUG_CPIO
/* [cpio] Print debugging information.  */
GLOBAL (bool, debug_option, false);
#endif

/* [tar] Do not consider symlinks themselves as objects, but rather only what
   they point to.  In a word, follow symbolic links.  */
GLOBAL (bool, dereference_option, false);

/* [cpio] We recurse into directories when writing.  */
GLOBAL (bool, directory_recurse_flag, false);

/* [cpio] print a . for each file processed.  (-V) */
GLOBAL (bool, dot_option, false);

/* [tar] There were exclude options, so trigger exclude processing.  */
GLOBAL (bool, exclude_option, false);

/* [tar] If files may be obtained in files rather than as command arguments.  */
GLOBAL (bool, files_from_option, false);

/* Do not consider remote file syntax.  That is, do not consider file names
   that contain a `:' to be on remote hosts; all files are local.  */
GLOBAL (bool, force_local_option, false);

/* [tar] Specified value to be put into tar file in place of stat () results,
   or just -1 if such an override should not take place.  */
GLOBAL (gid_t, group_option, 0);

/* [tar] Do not reflect failed reads into the exit status.  */
GLOBAL (bool, ignore_failed_read_option, false);

/* [tar] Merely skip zero blocks, do not consider them as possible end of files.
   This cannot be the default, because Unix tar writes two blocks of zeros,
   then pads out the record with garbage.  */
GLOBAL (bool, ignore_zeros_option, false);

/* [tar] Use incremental mode while dumping or restoring, whether listed or not.
   When making a dump, save directories at the beginning of the archive, and
   include in each directory its contents.  */
GLOBAL (bool, incremental_option, false);

/* [tar] Specified name of script to run at end of each tape change.  */
GLOBAL (const char *, info_script_option, NULL);

/* [tar] Ask confirmation before acting on archive entries.  */
GLOBAL (bool, interactive_option, false);

/* [cpio] Block size value, initially 512.  -B sets to 5120.  */
GLOBAL (int, io_block_size, BLOCKSIZE);

/* [tar] Don't overwrite existing files.  */
GLOBAL (bool, keep_old_files_option, false);

/* [cpio] Link files whenever possible.  Used with -p option.  (-l) */
GLOBAL (bool, link_option, false);

/* [tar] Specified file name for incremental list.  */
GLOBAL (const char *, listed_incremental_option, NULL);

/* [tar] Specified mode change string.  */
GLOBAL (struct mode_change *, mode_option, NULL);

/* [tar] Make multivolume archive.  When we cannot write any more into the
   archive, re-open it, and continue writing.  */
GLOBAL (bool, multi_volume_option, false);

/* [tar] Set if --after-date was given.  See the description for
   time_option_threshold, which this variable qualifies.  */
GLOBAL (bool, newer_ctime_option, false);

/* [tar] Set if either --newer-mtime or --after-date was given.  See the description
   for time_option_threshold, which this variable qualifies.  */
GLOBAL (bool, newer_mtime_option, false);

/* [cpio] Don't use any absolute paths, prefix them by `./'.  */
GLOBAL (bool, no_absolute_filenames_option, false);

/* [tar] Do not restore any file attribute.  */
GLOBAL (bool, no_attributes_option, false);

/* [cpio] Messages like "1 block" should be suppressed.  */
GLOBAL (bool, quiet_option, false);

/* [cpio] Do not chown the files.  */
GLOBAL (bool, no_preserve_owner_option, false);

/* [tar] Do not dump directories contents recursively.  */
GLOBAL (bool, no_recurse_option, false);

/* [tar] Use NUL instead of NL, when reading or producing file lists.  */
GLOBAL (bool, null_option, false);

/* [tar] Use numeric ids directly, do not translate symbolic ids to numbers.  */
GLOBAL (bool, numeric_owner_option, false);

/* [tar] Do not traverse filesystem boundaries while dumping.  */
GLOBAL (bool, one_file_system_option, false);

/* [cpio] Only read the archive and verify the files' CRC's, don't
   actually extract the files.  */
GLOBAL (bool, only_verify_crc_option, false);

/* [cpio] We can overwrite existing files.  */
GLOBAL (bool, overwrite_existing_files, true);

/* [tar] Specified value to be put into tar file in place of stat () results,
   or just -1 if such an override should not take place.  */
GLOBAL (uid_t, owner_option, 0);

/* [cpio] Interactive renames are done pax-style.  This is only
   useful if rename_option is set.  */
GLOBAL (bool, pax_rename_option, false);

#if ENABLE_DALE_CODE
/* [tar] If file compression (-y) has been selected.  */
GLOBAL (bool, per_file_compress_option, false);
#endif

/* [cpio] We should preserve mode of files.  */
GLOBAL (bool, preserve_mode_flag, true);

/* [tar] Assume the archive has no duplicate members.  */
GLOBAL (bool, quick_option, false);

/* [cpio] Don't report number of blocks copied.  */
GLOBAL (bool, quiet_flag, false);

/* [tar] When deleting directories in the way, recursively delete their
   contents.  */
GLOBAL (bool, recursive_unlink_option, false);

/* [tar] Try to reblock input records, like when reading 4.2BSD pipes.  */
GLOBAL (bool, read_full_records_option, false);

/* [cpio] If non-NULL, the name of a file that will be read to
   rename all of the files in the archive.  --rename-batch-file.  */
GLOBAL (char *, rename_batch_file, NULL);

/* [cpio] Interactively rename files.  (-r) */
GLOBAL (bool, rename_option, false);

/* [cpio] Reset access times after reading files (-a). */
GLOBAL (bool, reset_access_time_option, false);

/* [cpio] Retain previous file modification time.  (-m) */
GLOBAL (bool, preserve_modification_time_option, false);

/* [tar] Delete files after having dumped them.  */
GLOBAL (bool, remove_files_option, false);

/* [tar] Specified remote shell command.  */
GLOBAL (const char *, rsh_command_option, NULL);

/* [tar] The list of names to extract are already sorted in the archive order, so
   this list does not have to be swallowed in memory in advance.  */
GLOBAL (bool, same_order_option, false);

/* [tar] Restore the owner and group attributes as per the archive.  */
GLOBAL (bool, same_owner_option, false);

/* [tar] Restore the permission attributes as per the archive, disobeying the
   current umask.  */
GLOBAL (bool, same_permissions_option, false);

/* [cpio] Set group ownership of all files to GID `set_group'.  */
GLOBAL (bool, set_group_flag, false);
GLOBAL (gid_t, set_group, 0);

/* [cpio] Set ownership of all files to UID `set_owner'.  */
GLOBAL (bool, set_owner_flag, false);
GLOBAL (uid_t, set_owner, 0);

/* [tar] Warn about each skipped directory, while reading an archive.  */
GLOBAL (bool, show_omitted_dirs_option, false);

/* [tar] Attempt to handle files having big zeroed holes in them.  */
/* [cpio] Try to write sparse ("holey") files.  */
GLOBAL (bool, sparse_option, false);

/* [tar] Resume work at a given entry, usually after a previous run which
   failed.  */
GLOBAL (bool, starting_file_option, false);

/* [cpio] Swap bytes of each file during cpio -i.  */
GLOBAL (bool, swap_bytes_option, false);

/* [cpio] Swap halfwords of each file during cpio -i.  */
GLOBAL (bool, swap_halfwords_option, false);

/* [cpio] We are swapping bytes on the current file.  */
GLOBAL (bool, swapping_bytes, false);

/* [cpio] We are swapping halfwords on the current file.  */
GLOBAL (bool, swapping_halfwords, false);

/* [cpio] Print a table of contents of input.  (-t) */
GLOBAL (bool, list_option, 0);

/* [tar] Specified maximum byte length of each tape volume (multiple of 1024).  */
GLOBAL (tarlong, tape_length_option, 0);

/* [tar] Extract entries on standard output, instead of on disk.  */
GLOBAL (bool, to_stdout_option, false);

/* [tar] Echo the total size of the created archive.  */
GLOBAL (bool, totals_option, false);

/* [tar] Do not restore timestamps from the archive, let extracted files be new.  */
GLOBAL (bool, touch_option, false);

/* [cpio] Copy unconditionally (older replaces newer).  (-u) */
GLOBAL (bool, unconditional_option, false);

/* [tar] Delete files before extracting over them, loose previous file nature
   and attributes.  This flag is also set when recursively deleting directory
   hierarchies, as the most implies the least.  */
GLOBAL (bool, unlink_first_option, false);

/* [tar] With create subcommand, rather update entries in preexisting archive
   only from newer files.  With extract subcommand, only replace files if
   archive entry is newer.  */
GLOBAL (bool, update_option, false);

/* [cpio] List the files processed, or ls -l style output with -t.
   (-v) */
/* [tar] Count how many times the option has been set, multiple setting yields
   more verbose behavior.  Value 0 means no verbosity, 1 means file name
   only, 2 means file name and all attributes.  More than 2 is just like 2.  */
GLOBAL (bool, verbose_option, false);
GLOBAL (int, verbose_option_count, 0);

/* [tar] Read after write for media checking purposes.  */
GLOBAL (bool, verify_option, false);

/* [tar] Specified name of file containing the volume number.  */
GLOBAL (const char *, volno_file_option, NULL);

/* [tar] Specified value or pattern.  */
GLOBAL (const char *, volume_label_option, NULL);

/* Other global definitions.  */

/* [tar] File descriptor for archive file.  */
GLOBAL (int, archive, 0);

/* [cpio] File descriptor containing the archive.  */
GLOBAL (int, archive_des, 0);

/* [cpio] Name of file containing the archive, if known; NULL if stdin/out.  */
GLOBAL (char *, archive_name, NULL);

#if ENABLE_DALE_CODE
/* [tar] If archive compression (-z or -Z) has been selected.  */
GLOBAL (bool, compress_whole_archive, false);

/* [tar] Descriptor for compression work file.  */
GLOBAL (int, compress_work_file, 0);
#endif

/* [cpio] With -i; if true, copy only files that match any of the given
   patterns; if false, copy only files that do not match any of the patterns.
   (-f) */
GLOBAL (bool, copy_matching_files, true);

/* [cpio] CRC checksum.  */
GLOBAL_NO_INIT (unsigned long, crc);

/* [tar] True when outputting to /dev/null.  */
GLOBAL (bool, dev_null_output, false);

/* [cpio] Saving of argument values for later reference.  */
GLOBAL (char *, directory_name, NULL);
GLOBAL_NO_INIT (char **, save_patterns);
GLOBAL_NO_INIT (int, num_patterns);

/* [cpio] Input and output buffers.   */
GLOBAL_NO_INIT (char *, input_buffer);
GLOBAL_NO_INIT (char *, output_buffer);

/* [cpio] Current locations in buffers.  */
GLOBAL (char *, input_buffer_cursor, NULL);
GLOBAL (char *, output_buffer_cursor, NULL);

/* [cpio] Current number of bytes available at buffer cursors.  */
GLOBAL_NO_INIT (long, input_buffer_remaining_size);
GLOBAL_NO_INIT (long, output_buffer_remaining_size);

/* [cpio] The size of the input buffer.  */
GLOBAL_NO_INIT (long, input_buffer_size);

/* [cpio] Total number of bytes read and written for all files.  Now that many
   tape drives hold more than 4Gb we need more than 32 bits to hold
   input_bytes and output_bytes.  But it's not worth the trouble of adding
   special multi-precision arithmetic if the compiler doesn't support 64 bit
   ints since input_bytes and output_bytes are only used to print the number
   of blocks copied.  */
#ifdef __GNUC__
GLOBAL_NO_INIT (long long, input_bytes);
GLOBAL_NO_INIT (long long, output_bytes);
#else
GLOBAL_NO_INIT (long, input_bytes);
GLOBAL_NO_INIT (long, output_bytes);
#endif

/* [cpio] lseek works on the input.  */
GLOBAL (bool, input_is_seekable, false);

/* [cpio] Input (cpio -i) or output (cpio -o) is a device node.  */
GLOBAL (char, input_is_special, false);
GLOBAL (char, output_is_special, false);

/* [cpio] File position of last header read.  Only used during -A to determine
   where the old TRAILER!!! record started.  */
GLOBAL (int, last_header_start, 0);

/* [cpio] Character that terminates file names read from stdin.  */
GLOBAL (char, name_end, '\n');

/* [cpio] Message to print when end of medium is reached (-M).  */
GLOBAL (char *, new_media_message, NULL);

/* [cpio] With -M with %d, message to print when end of medium is reached.  */
GLOBAL (char *, new_media_message_with_number, NULL);
GLOBAL (char *, new_media_message_after_number, NULL);

/* [cpio] With -itv; if true, list numeric uid and gid instead of translating
   them into names.  */
GLOBAL (bool, numeric_uid_gid_option, false);

/* [cpio] lseek works on the output.  */
GLOBAL (bool, output_is_seekable, false);

/* [cpio] Name of file containing additional patterns (-E).  */
GLOBAL (char *, pattern_file_name, NULL);

/* [cpio] Array of filenames to use in 'pax' mode.  These files are either
   written to an archive or copied.  */
GLOBAL_NO_INIT (char **, pax_file_names);
GLOBAL (int, num_pax_file_names, 0);

/* [tar] Name prefix to apply to all files in created archive, and to require
   for all files before extracting them.  */
GLOBAL (const char *, name_prefix_option, NULL);
GLOBAL (size_t, name_prefix_length, 0);

/* [tar] List of tape drive names, number of such tape drives, allocated number,
   and current cursor in list.  */
GLOBAL (const char **, archive_name_array, NULL);
GLOBAL (int, archive_names, 0);
GLOBAL (int, allocated_archive_names, 0);
GLOBAL (const char **, archive_name_cursor, NULL);

/* [tar] Pointer to the start of the scratch space.  */
GLOBAL (struct sp_array *, sparsearray, NULL);

/* [tar] Initial size of the sparsearray.  */
GLOBAL (int, sp_array_size, 0);

/* [tar] If both newer_ctime_option and newer_mtime_option are false, then
   time_option_threshold is irrelevant.  If newer_ctime_option is false and
   newer_mtime_option is true, then files get archived if their mtime is
   greater than time_option_threshold.  If newer_ctime_option is true, then
   newer_mtime_option always gets selected, so files get archived if *either*
   their ctime or mtime is greater than time_option_threshold.  */
GLOBAL (time_t, time_option_threshold, 0);

/* [cpio] A pointer to either lstat or stat, depending on whether
   dereferencing of symlinks is done for input files.  */
GLOBAL_NO_INIT (xstat_t, xstat);

/* [cpio] Which copy operation to perform.  (-i, -o, -p) */
GLOBAL (copy_function_t, copy_function, NULL);

/* Returns true if name is too long for archive format.  Only used when
   writing.  */
GLOBAL_NO_INIT (name_too_long_t, name_too_long);

/* Function to use to write a header.  */
GLOBAL_NO_INIT (header_writer_t, header_writer);

/* Function to use to indicate that archive is finished.  */
GLOBAL_NO_INIT (eof_writer_t, eof_writer);
