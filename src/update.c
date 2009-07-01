/* Update a tar archive.
   Copyright (C) 1988, 92, 94, 96, 97, 98 Free Software Foundation, Inc.

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

/* Implement the 'r', 'u' and 'A' options for tar.  'A' means that the
   file names are tar files, and they should simply be appended to the end
   of the archive.  No attempt is made to record the reads from the args; if
   they're on raw tape or something like that, it'll probably lose...  */

#include "system.h"
#include "common.h"

/* FIXME: This module should not directly handle the following variable,
   instead, this should be done in buffer.c only.  */
extern union block *current_block;

/* We're reading, but we just read the last block and its time to update.
   That is, we have hit the end of the old stuff, and its now time to start
   writing new stuff to the tape.  This involves seeking back one record and
   re-writing the current record (which has been changed).  */
bool time_to_start_writing = false;

/* Pointer to where we started to write in the first record we write out.
   This is used if we can't backspace the output and have to null out the
   first part of the record.  */
char *output_start;

/*------------------------------------------------------------------------.
| Catenate file PATH to the archive without creating a header for it.  It |
| had better be a tar file or the archive is screwed.			  |
`------------------------------------------------------------------------*/

static void
append_file (char *path)
{
  int handle;
  struct stat stat_info;
  off_t bytes_left;

  if (stat (path, &stat_info) != 0
      || (handle = open (path, O_RDONLY | O_BINARY), handle < 0))
    {
      ERROR ((0, errno, _("Cannot open file %s"), path));
      return;
    }

  bytes_left = stat_info.st_size;

  while (bytes_left > 0)
    {
      union block *start = find_next_block ();
      size_t size_to_read = available_space_after (start);
      ssize_t size_read;

      if (bytes_left < size_to_read)
	{
	  size_t count;

	  size_to_read = bytes_left;
	  count = size_to_read % BLOCKSIZE;
	  if (count)
	    memset (start->buffer + bytes_left, 0, BLOCKSIZE - count);
	}

      size_read = full_read (handle, start->buffer, size_to_read);
      if (size_read < 0)
	FATAL_ERROR ((0, errno,
		      _("Read error at byte %ld reading %lu bytes in file %s"),
		      stat_info.st_size - bytes_left,
		      (unsigned long) size_to_read, path));
      bytes_left -= size_read;

      set_next_block_after (start + (size_read - 1) / BLOCKSIZE);

      if (size_read != size_to_read)
	FATAL_ERROR ((0, 0, _("%s: File shrunk by %lu bytes, (yark!)"),
		      path, (unsigned long) bytes_left));
    }

  close (handle);
}

/*-----------------------------------------------------------------------.
| Implement the 'r' (add files to end of archive), and 'u' (add files to |
| end of archive if they arent there, or are more up to date than the	 |
| version in the archive.) commands.					 |
`-----------------------------------------------------------------------*/

void
update_archive (void)
{
  enum read_header previous_status = HEADER_STILL_UNREAD;
  bool found_end = false;

  name_gather ();
  if (subcommand_option == UPDATE_SUBCOMMAND)
    name_expand ();
  open_archive (ACCESS_UPDATE);

  while (!found_end)
    {
      enum read_header status = read_header (&current);

      switch (status)
	{
	case HEADER_STILL_UNREAD:
	  abort ();

	case HEADER_SUCCESS:
	  {
	    struct name *name;

	    if (subcommand_option == UPDATE_SUBCOMMAND
		&& (name = name_scan (current.name), name))
	      {
		struct stat stat_info;

		decode_header (&current, false);
		if (stat (current.name, &stat_info) < 0)
		  ERROR ((0, errno, _("Cannot stat %s"), current.name));
		else if (time_compare (current.stat.st_mtime,
				       stat_info.st_mtime)
			 >= 0)
		  name->match_found = true;
	      }
	    set_next_block_after (current.block);
	    if (current.block->oldgnu_header.isextended)
	      skip_extended_headers ();
	    skip_file (current.stat.st_size);
	    break;
	  }

	case HEADER_ZERO_BLOCK:
	  current_block = current.block;
	  found_end = true;
	  break;

	case HEADER_END_OF_FILE:
	  found_end = true;
	  break;

	case HEADER_FAILURE:
	  set_next_block_after (current.block);
	  switch (previous_status)
	    {
	    case HEADER_STILL_UNREAD:
	      WARN ((0, 0, _("This does not look like a tar archive")));
	      /* Fall through.  */

	    case HEADER_SUCCESS:
	    case HEADER_ZERO_BLOCK:
	      ERROR ((0, 0, _("Skipping to next header")));
	      /* Fall through.  */

	    case HEADER_FAILURE:
	      break;

	    case HEADER_END_OF_FILE:
	      abort ();
	    }
	  break;
	}

      previous_status = status;
    }

  reset_eof ();
  time_to_start_writing = true;
  output_start = current_block->buffer;

  {
    char *path;

    while (path = name_from_list (), path)
      {
	if (interactive_option && !confirm (_("add %s?"), path))
	  continue;
	if (subcommand_option == CAT_SUBCOMMAND)
	  append_file (path);
	else
	  dump_file (path, (dev_t) -1, true);
      }
  }

  write_eot ();
  close_archive ();
  names_notfound ();
}
