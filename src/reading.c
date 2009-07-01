/* Support routines for reading a tar archive.
   Copyright (C) 1988, 92, 93, 94, 96, 97, 98, 99 Free Software Foundation, Inc.
   Written by John Gilmore, on 1985-08-26.

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

#include "system.h"

#include <ctype.h>
#include <time.h>

char *stpncpy ();

#include "common.h"

struct tar_entry current;	/* current tar entry */

/*-------------------------------------------------------------------------.
| Read the next block, which should usually be a header block.  For        |
| "normal" headers, initialise a few fields of ENTRY: the archive format,  |
| header pointer, file size, entry name and linkname.  Identify which kind |
| of header was seen and return this information.                          |
|                                                                          |
| The header checksum is recomputed and validated.  LONGNAME and LONGLINK  |
| entries, which might prefix a normal header block, are automatically     |
| swallowed by this function.                                              |
|                                                                          |
| One should always `set_next_block_after (entry->block)' to skip past the |
| header which this routine reads.  Routine `decode_header' might be used  |
| to later complete the header decoding initiated here.                    |
`-------------------------------------------------------------------------*/

enum read_header
read_header (struct tar_entry *entry)
{
  bool longname_flag = false;	/* if a GNUTAR_LONGNAME has been seen */
  bool longlink_flag = false;	/* if a GNUTAR_LONGLINK has been seen */

  while (true)
    {
      union block *block = find_next_block ();

      entry->block = block;
      if (!block)
	return HEADER_END_OF_FILE;

      /* The standard BSD tar create the checksum by adding up the bytes in
	 the header as type char.  It was unsigned on the PDP-11, but signed
	 on Suns and others.  The sources of BSD tar, after having been
	 imported on Sun, were not adjusted to compute the checksum
	 currectly, so their tar adds the bytes as signed chars.  This works
	 until you get a file with a name containing characters with the
	 high bit set.  So here, we computes two checksums, signed and
	 unsigned, and accept either.  */
      {
	long recorded_sum  = get_header_chksum (block);
	long posix_sum = 0;	/* checksum the POSIX' way :-) */
	long signed_sum = 0;	/* checksum the Sun's way :-( */

	char *cursor;		/* `unsigned' is not available everywhere */

	/* Checksum all bytes except for the checksum itself.  */

	for (cursor = block->buffer; cursor < block->header.chksum; cursor++)
	  {
	    posix_sum += 0xFF & *cursor;
	    signed_sum += *cursor;
	  }
	for (cursor = block->header.chksum + sizeof block->header.chksum;
	     cursor < block->buffer + sizeof *block;
	     cursor++)
	  {
	    posix_sum += 0xFF & *cursor;
	    /* FIXME: The signed checksum computation is broken on machines
	       where char's are unsigned.  */
	    signed_sum += *cursor;
	  }

	/* If we have zero so far, there is a good chance the whole block is
	   zero, just complete the check then.  */

	if (posix_sum == 0)
	  {
	    for (cursor = block->header.chksum;
		 cursor < block->header.chksum + sizeof block->header.chksum;
		 cursor++)
	      if (*cursor)
		break;

	    if (cursor == block->header.chksum + sizeof block->header.chksum)
	      return HEADER_ZERO_BLOCK;
	  }

	/* Adjust checksum to count the CHKSUM field as blanks.  */

	posix_sum += ' ' * sizeof block->header.chksum;
	signed_sum += ' ' * sizeof block->header.chksum;

	if (posix_sum != recorded_sum && signed_sum != recorded_sum)
	  return HEADER_FAILURE;
      }

      /* The block is a good one.  Now decode file size.  Links are zero
	 size on tape, whatever the header says.

	 FIXME: I wonder which tar does archive with random link sizes.  */

      entry->stat.st_size = (block->header.typeflag == LNKTYPE ? 0
			     : get_header_size (block));

      if (block->header.typeflag == GNUTAR_LONGNAME
	  || block->header.typeflag == GNUTAR_LONGLINK)
	{
	  int size = entry->stat.st_size;
	  char *cursor;

	  if (block->header.typeflag == GNUTAR_LONGNAME)
	    {
	      if (longname_flag)
		ERROR ((0, 0, _("Two long file names for a single entry")));
	      else
		longname_flag = true;

	      if (size + 1 > entry->name_allocated)
		{
		  free (entry->name);
		  entry->name = (char *) xmalloc ((size_t) (size + 1));
		  entry->name_allocated = size + 1;
		}
	      cursor = entry->name;
	    }
	  else
	    {
	      if (longlink_flag)
		ERROR ((0, 0, _("Two long link names for a single entry")));
	      else
		longlink_flag = true;

	      if (size + 1 > entry->linkname_allocated)
		{
		  free (entry->linkname);
		  entry->linkname = (char *) xmalloc ((size_t) (size + 1));
		  entry->linkname_allocated = size + 1;
		}
	      cursor = entry->linkname;
	    }

	  set_next_block_after (block);
	  while (size > 0)
	    {
	      union block *data = find_next_block ();
	      size_t size_to_copy;

	      if (data == NULL)
		{
		  ERROR ((0, 0, _("Unexpected end of file in archive")));
		  break;
		}

	      size_to_copy = available_space_after (data);
	      if (size_to_copy > size)
		size_to_copy = size;

	      memcpy (cursor, data->buffer, size_to_copy);
	      cursor += size_to_copy;
	      size -= size_to_copy;

	      set_next_block_after
		((union block *) (data->buffer + size_to_copy - 1));
	    }
	  *cursor = '\0';

	  continue;		/* restart at the beginning */
	}

      /* We now have the genuine entry header.  Decide the archive format.  */

      if (strcmp (block->header.magic, TMAGIC) == 0)
	entry->format = POSIX_FORMAT;
      else if (strcmp (block->header.magic, GNUTAR_MAGIC) == 0)
	entry->format = GNUTAR_FORMAT;
      else
	entry->format = V7_FORMAT;

      /* Save file and link names if not done already.  */

      if (!longname_flag)

	if (entry->format == POSIX_FORMAT)
	  {
	    char *cursor = entry->name;

	    if (*block->header.prefix)
	      {
		cursor = stpncpy (cursor, block->header.prefix,
				  PREFIX_FIELD_SIZE);
		*cursor++ = '/';
	      }
	    cursor = stpncpy (cursor, block->header.name,
			      NAME_FIELD_SIZE);
	    *cursor = '\0';
	  }
	else
	  {
	    char *cursor = stpncpy (entry->name, block->header.name,
				    NAME_FIELD_SIZE);
	    *cursor = '\0';
	  }

      if (!longlink_flag)
	{
	  char *cursor = stpncpy (entry->linkname, block->header.linkname,
				  LINKNAME_FIELD_SIZE);
	  *cursor = '\0';
	}

      return HEADER_SUCCESS;
    }
}

/*-------------------------------------------------------------------------.
| Finish decoding the header block for ENTRY, which has already been       |
| started in routine read_header.  Require DECODE_SYMBOLIC for decoding    |
| the user and group information, to spare some time when merely listing.  |
`-------------------------------------------------------------------------*/

/* Calling this routine twice for the same block, using different values for
   DECODE_SYMBOLIC, might end up with different uid/gid for the two calls.
   If anybody wants the uid/gid they should decode it on the first call, and
   other callers should decode it without uid/gid before calling a routine
   (like print_header) which assumes decoded data.

   FIXME: I do not see the point of calling the routine twice, anyway.  */

void
decode_header (struct tar_entry *entry, bool decode_symbolic)
{
  union block *block = entry->block;

  entry->stat.st_mode = get_header_mode (block);
  entry->stat.st_mode &= 07777;
  entry->stat.st_mtime = get_header_mtime (block);

  if (entry->format == GNUTAR_FORMAT && incremental_option)
    {
      entry->stat.st_atime = get_header_atime (block);
      entry->stat.st_ctime = get_header_ctime (block);
    }

  if (entry->format == V7_FORMAT)
    {
      entry->stat.st_uid = get_header_uid (block);
      entry->stat.st_gid = get_header_gid (block);
      entry->stat.st_rdev = 0;
    }
  else
    {
      if (decode_symbolic)
	{
	  /* FIXME: Decide if all this should somewhat depend on -p.  */

	  if (numeric_owner_option || !*block->header.uname)
	    entry->stat.st_uid = get_header_uid (block);
	  else
	    {
	      uid_t *uid = getuidbyname (block->header.uname);

	      entry->stat.st_uid = uid ? *uid : get_header_uid (block);
	    }

	  if (numeric_owner_option || !*block->header.gname)
	    entry->stat.st_gid = get_header_gid (block);
	  else
	    {
	      gid_t *gid = getgidbyname (block->header.gname);

	      entry->stat.st_gid = gid ? *gid : get_header_gid (block);
	    }
	}

      switch (block->header.typeflag)
	{
#ifdef S_IFBLK
	case BLKTYPE:
	  entry->stat.st_rdev = makedev (get_header_devmajor (block),
					 get_header_devminor (block));
	  break;
#endif

#ifdef S_IFCHR
	case CHRTYPE:
	  entry->stat.st_rdev = makedev (get_header_devmajor (block),
					 get_header_devminor (block));
	  break;
#endif

	default:
	  entry->stat.st_rdev = 0;
	}
    }
}

/*---.
| ?  |
`---*/

void
skip_extended_headers (void)
{
  union block *block;

  while (true)
    {
      block = find_next_block ();
      if (!block->sparse_header.isextended)
	{
	  set_next_block_after (block);
	  break;
	}
      set_next_block_after (block);
    }
}

/*--------------------------------------------------------.
| Skip over SIZE bytes of data in blocks in the archive.  |
`--------------------------------------------------------*/

void
skip_file (off_t size)
{
  union block *x;

  if (multi_volume_option)
    {
      save_totsize = size;
      save_sizeleft = size;
    }

  while (size > 0)
    {
      x = find_next_block ();
      if (x == NULL)
	FATAL_ERROR ((0, 0, _("Unexpected end of file in archive")));

      set_next_block_after (x);
      size -= BLOCKSIZE;
      if (multi_volume_option)
	save_sizeleft -= BLOCKSIZE;
    }
}

/*-----------------------------------.
| Main loop for reading an archive.  |
`-----------------------------------*/

void
read_and (void (*do_something) ())
{
  enum read_header status = HEADER_STILL_UNREAD;
  enum read_header previous_status;
  char saved_typeflag;

  name_gather ();
  open_tar_archive (ACCESS_READ);

  while (!quick_option || name_list_unmatched_count)
    {
      previous_status = status;
      status = read_header (&current);
      switch (status)
	{
	case HEADER_STILL_UNREAD:
	  abort ();

	case HEADER_SUCCESS:
	  /* Valid header.  We should decode mode first.  Ensure incoming
	     names are null terminated.  */

	  if ((name_list_head && !find_matching_name (current.name, true))
	      || (newer_mtime_option
		  /* FIXME: This is kludgey to get mtime now, and again
		     later.  Is that fully normal to ignore ctime, here?  */
		  && (current.stat.st_mtime = get_header_mtime (current.block),
		      (time_compare (current.stat.st_mtime, newer_mtime_option)
		       < 0)))
	      || (exclude_option && check_exclude (current.name)))
	    {
	      bool is_extended = false;

	      if (current.block->header.typeflag == GNUTAR_VOLHDR
		  || current.block->header.typeflag == GNUTAR_MULTIVOL
		  || current.block->header.typeflag == GNUTAR_NAMES)
		{
		  (*do_something) ();
		  continue;
		}

	      if (show_omitted_dirs_option
		  && current.block->header.typeflag == DIRTYPE)
		WARN ((0, 0, _("Omitting %s"), current.name));

	      /* Skip past it in the archive.  */

	      if (current.block->gnutar_header.isextended)
		is_extended = true;
	      saved_typeflag = current.block->header.typeflag;
	      set_next_block_after (current.block);
	      if (is_extended)
		skip_extended_headers ();

	      /* Skip to the next header on the archive.  */

	      if (saved_typeflag != DIRTYPE)
		skip_file (current.stat.st_size);
	      continue;
	    }

	  (*do_something) ();
	  continue;

	case HEADER_ZERO_BLOCK:
	  if (block_number_option)
	    {
	      if (checkpoint_option)
		flush_progress_dots ();
	      fprintf (stdlis, _("block %10ul: ** Block of NULs **\n"),
		       (unsigned long) current_block_ordinal ());
	    }

	  set_next_block_after (current.block);
	  status = previous_status;
	  if (ignore_zeros_option)
	    continue;
	  break;

	case HEADER_END_OF_FILE:
	  if (block_number_option)
	    {
	      if (checkpoint_option)
		flush_progress_dots ();
	      fprintf (stdlis, _("block %10ul: ** End of File **\n"),
		       (unsigned long) current_block_ordinal ());
	    }
	  break;

	case HEADER_FAILURE:
	  /* If the previous header was good, tell them that we are
	     skipping bad ones.  */
	  set_next_block_after (current.block);
	  switch (previous_status)
	    {
	    case HEADER_STILL_UNREAD:
	      WARN ((0, 0, _("Hmm, this doesn't look like a tar archive")));
	      /* Fall through.  */

	    case HEADER_ZERO_BLOCK:
	    case HEADER_SUCCESS:
	      WARN ((0, 0, _("Skipping to next file header")));
	      break;

	    case HEADER_END_OF_FILE:
	    case HEADER_FAILURE:
	      /* We are in the middle of a cascade of errors.  */
	      break;
	    }
	  continue;
	}
      break;
    }

  apply_delayed_set_stat ();
  close_tar_archive ();
  report_unprocessed_names ();		/* print names not found */
}
