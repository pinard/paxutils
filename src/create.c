/* Create a tar archive.
   Copyright (C) 1985, 92, 93, 94, 96, 97, 98 Free Software Foundation, Inc.
   Written by John Gilmore, on 1985-08-25.

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

#include <pwd.h>
#include <grp.h>

#include "common.h"

#if !LINK_LINEAR_SEARCH
# include "hash.h"
#endif

extern dev_t ar_dev;
extern ino_t ar_ino;

extern struct name *name_list_current;

struct link
  {
#if LINK_LINEAR_SEARCH
    struct link *next;
#endif
    dev_t dev;
    ino_t ino;
    short linkcount;
    char name[1];
  };

#if LINK_LINEAR_SEARCH

struct link *link_list = NULL;	/* points to first link in list */

#else /* not LINK_LINEAR_SEARCH */

struct hash_table *link_table = NULL;

static unsigned int
link_hash (const void *void_link, unsigned int table_size)
{
  struct link const *link = void_link;

  return (link->ino + link->dev) % table_size;
}

static bool
link_compare (const void *void_first, const void *void_second)
{
  struct link const *first = void_first;
  struct link const *second = void_second;

  return first->ino == second->ino && first->dev == second->dev;
}

#endif /* not LINK_LINEAR_SEARCH */

/* Writing routines.  */

/*-----------------------------------------------------------------------.
| Just zeroes out the buffer so we don't confuse ourselves with leftover |
| data.									 |
`-----------------------------------------------------------------------*/

static void
clear_buffer (char *buffer)
{
  memset (buffer, 0, BLOCKSIZE);
}

/*--------------------------------------------------------------------------.
| Write the EOT block(s).  We actually zero at least one block, through the |
| end of the record.  Old tars write garbage after two zeroed blocks.       |
`--------------------------------------------------------------------------*/

void
write_eot (void)
{
  union block *pointer = find_next_block ();

  if (pointer)
    {
      size_t space = available_space_after (pointer);

      memset (pointer->buffer, 0, space);
      set_next_block_after (pointer);
    }
}

/*-----------------------------------------------------.
| Write a OLDGNU_LONGLINK or OLDGNU_LONGNAME block.  |
`-----------------------------------------------------*/

/* FIXME: Cross recursion between start_header and write_long!  */

static union block *start_header PARAMS ((const char *, struct stat *));

static void
write_long (const char *p, char type)
{
  size_t size = strlen (p) + 1;
  size_t bufsize;
  union block *header;
  struct stat stat_info;

  memset (&stat_info, 0, sizeof stat_info);
  stat_info.st_size = size;

  header = start_header ("././@LongLink", &stat_info);
  header->header.typeflag = type;
  finish_header (header);

  header = find_next_block ();

  bufsize = available_space_after (header);

  while (bufsize < size)
    {
      memcpy (header->buffer, p, bufsize);
      p += bufsize;
      size -= bufsize;
      set_next_block_after (header + (bufsize - 1) / BLOCKSIZE);
      header = find_next_block ();
      bufsize = available_space_after (header);
    }
  memcpy (header->buffer, p, size);
  memset (header->buffer + size, 0, bufsize - size);
  set_next_block_after (header + (size - 1) / BLOCKSIZE);
}

/* Header handling.  */

/*---------------------------------------------------------------------------.
| Make a header block for the file name, given its STAT_INFO.  Return header |
| pointer for success, NULL if the name is too long.                         |
`---------------------------------------------------------------------------*/

static union block *
start_header (const char *name, struct stat *stat_info)
{
  union block *header;
  char *prefixed_name;

  if (name_prefix_option)
    {
      prefixed_name = concat_no_slash (name_prefix_option, name);
      name = prefixed_name;
    }
  else
    prefixed_name = NULL;

  if (!absolute_names_option)
    {
      static bool warned_once = false;

#if DOSWIN

      if (name[0] >= 'A' && name[0] <= 'z' && name[1] == ':')
	{
	  if (!warned_once)
	    {
	      /* Don't set warned_once if the next character is a slash,
		 so that they will see the message about the slash as well.  */
	      if (name[2] != '/')
		warned_once = true;

	      WARN ((0, 0, _("\
Removing drive spec `%2.2s' from path names in the archive"),
		     name));
	    }
	  name += 2;
	}

#endif /* DOSWIN */

      while (*name == '/')
	{
	  name++;		/* force relative path */
	  if (!warned_once)
	    {
	      warned_once = true;
	      WARN ((0, 0, _("\
Removing leading `/' from absolute path names in the archive")));
	    }
	}
    }

  /* Check the file name and put it in the block.  */

  if (strlen (name) >= (size_t) NAME_FIELD_SIZE)
    write_long (name, OLDGNU_LONGNAME);
  header = find_next_block ();
  memset (header->buffer, 0, sizeof (union block));

  assign_string (&current.name, name);

  strncpy (header->header.name, name, NAME_FIELD_SIZE);
  header->header.name[NAME_FIELD_SIZE - 1] = '\0';

  /* Override some stat fields, if requested to do so.  */

  if (owner_option != (uid_t) -1)
    stat_info->st_uid = owner_option;
  if (group_option != (gid_t) -1)
    stat_info->st_gid = group_option;
  if (mode_option)
    stat_info->st_mode = ((stat_info->st_mode & S_IFMT)
		   | mode_adjust (stat_info->st_mode, mode_option));

  /* Paul Eggert tried the trivial test ($WRITER cf a b; $READER tvf a)
     for a few tars and came up with the following interoperability
     matrix:

	      WRITER
	1 2 3 4 5 6 7 8 9   READER
	. . . . . . . . .   1 = SunOS 4.2 tar
	# . . # # . . # #   2 = NEC SVR4.0.2 tar
	. . . # # . . # .   3 = Solaris 2.1 tar
	. . . . . . . . .   4 = tar 1.11.1
	. . . . . . . . .   5 = HP-UX 8.07 tar
	. . . . . . . . .   6 = Ultrix 4.1
	. . . . . . . . .   7 = AIX 3.2
	. . . . . . . . .   8 = Hitachi HI-UX 1.03
	. . . . . . . . .   9 = Omron UNIOS-B 4.3BSD 1.60Beta

	     . = works
	     # = ``impossible file type''

     The following mask for old archive removes the `#'s in column 4
     above, thus making tar both a universal donor and a universal
     acceptor for Paul's test.  */

  set_header_mode (header, stat_info->st_mode);
  set_header_uid (header, stat_info->st_uid);
  set_header_gid (header, stat_info->st_gid);
  set_header_size (header, stat_info->st_size);
  set_header_mtime (header, stat_info->st_mtime);

  if (incremental_option)
    if (archive_format == OLDGNU_FORMAT)
      {
	set_header_atime (header, stat_info->st_atime);
	set_header_ctime (header, stat_info->st_ctime);
      }

  header->header.typeflag = archive_format == V7_FORMAT ? AREGTYPE : REGTYPE;

  switch (archive_format)
    {
    case DEFAULT_FORMAT:
    case V7_FORMAT:
      break;

    case OLDGNU_FORMAT:
      /* Overwrite header->header.magic and header.version in one blow.  */
      strcpy (header->header.magic, OLDGNU_MAGIC);
      break;

    case POSIX_FORMAT:
    case FREE_FORMAT:
      strncpy (header->header.magic, TMAGIC, TMAGLEN);
      strncpy (header->header.version, TVERSION, TVERSLEN);
      break;
    }

  /* header->header.[ug]name are currently set to the empty string.  See if
     we can do something better.  */

  if (!numeric_owner_option && archive_format != V7_FORMAT)
    {
      char *string;

      if (string = getuser (stat_info->st_uid), string != NULL)
	strncpy (header->header.uname, string, UNAME_FIELD_SIZE);

      if (string = getgroup (stat_info->st_gid), string != NULL)
	strncpy (header->header.gname, string, GNAME_FIELD_SIZE);
    }

  /* Cleanup.  */

  if (prefixed_name)
    free (prefixed_name);

  return header;
}

/*-------------------------------------------------------------------------.
| Finish off a filled-in header block and write it out.  We also print the |
| file name and/or full info if verbose is on.				   |
`-------------------------------------------------------------------------*/

void
finish_header (union block *header)
{
  int sum;
  char *cursor;

  memcpy (header->header.chksum, CHKBLANKS, sizeof (header->header.chksum));

  sum = 0;
  for (cursor = header->buffer;
       cursor < header->buffer + sizeof (*header);
       cursor++)
    /* We can't use unsigned char here because of old compilers, e.g. V7.  */
    sum += 0xFF & *cursor;

  set_header_chksum (header, sum);
  set_next_block_after (header);

  if (verbose_option
      && header->header.typeflag != OLDGNU_LONGLINK
      && header->header.typeflag != OLDGNU_LONGNAME)
    {
      current.block = header;
      current.format = archive_format;
      /* current.stat is already set up.  */

      print_header (&current);
    }
}

/* Sparse file processing.  */

/*-----------------------------------------------------.
| Check if one block of data is fully made of zeroes.  |
`-----------------------------------------------------*/

static bool
is_zero_block (char *buffer)
{
  int counter;

  for (counter = 0; counter < BLOCKSIZE; counter++)
    if (buffer[counter] != '\0')
      return false;
  return true;
}

/*---.
| ?  |
`---*/

static void
init_sparsearray (void)
{
  int counter;

  sp_array_size = 10;

  /* Make room for our scratch space -- initially is 10 elements long.  */

  sparsearray = (struct sp_array *)
    xmalloc (sp_array_size * sizeof (struct sp_array));
  for (counter = 0; counter < sp_array_size; counter++)
    {
      sparsearray[counter].offset = 0;
      sparsearray[counter].numbytes = 0;
    }
}

/*---.
| ?  |
`---*/

static void
find_new_file_size (off_t *size, int highest_index)
{
  int counter;

  *size = 0;
  for (counter = 0;
       sparsearray[counter].numbytes && counter <= highest_index;
       counter++)
    *size += sparsearray[counter].numbytes;
}

/*-----------------------------------------------------------------------.
| Make one pass over the file NAME, studying where any nonzero data is,  |
| that is, how far into the file each instance of data is, and how many  |
| bytes are there.  Save this information in the sparsearray, which will |
| later be translated into header information.                           |
`-----------------------------------------------------------------------*/

/* There is little point in trimming small amounts of null data at the head
   and tail of blocks, only avoid dumping full null blocks.  */

/* FIXME: this routine might accept bits of algorithmic cleanup, it is
   too kludgey for my taste...  Also document the return value.  */

static int
deal_with_sparse (char *name, union block *header)
{
  size_t numbytes = 0;
  off_t offset = 0;
  int file;
  int sparse_index = 0;
  ssize_t size_read;
  char buffer[BLOCKSIZE];

  if (archive_format == OLDGNU_FORMAT)
    header->oldgnu_header.isextended = 0;

  if (file = open (name, O_RDONLY), file < 0)
    /* This problem will be caught later on, so just return.  */
    return 0;

  init_sparsearray ();
  clear_buffer (buffer);

  while (size_read = full_read (file, buffer, sizeof buffer), size_read != 0)
    {
      /* Realloc the scratch area as necessary.  FIXME: should reallocate
	 only at beginning of a new instance of nonzero data.  */

      if (sparse_index > sp_array_size - 1)
	{

	  sparsearray = (struct sp_array *)
	    xrealloc (sparsearray,
		      2 * sp_array_size * sizeof (struct sp_array));
	  sp_array_size *= 2;
	}

      /* Process one block.  */

      if (size_read == sizeof buffer)

	if (is_zero_block (buffer))
	  {
	    if (numbytes)
	      {
		sparsearray[sparse_index++].numbytes = numbytes;
		numbytes = 0;
	      }
	  }
	else
	  {
	    if (!numbytes)
	      sparsearray[sparse_index].offset = offset;
	    numbytes += size_read;
	  }

      else

	/* Since size_read < sizeof buffer, we have the last bit of the
	   file.  */

	if (!is_zero_block (buffer))
	  {
	    if (!numbytes)
	      sparsearray[sparse_index].offset = offset;
	    numbytes += size_read;
	  }
	else
	  /* The next two lines are suggested by Andreas Degert, who says
	     they are required for trailing full blocks to be written to the
	     archive, when all zeroed.  Yet, it seems to me that the case
	     does not apply.  Further, at restore time, the file is not as
	     sparse as it should.  So, some serious cleanup is *also* needed
	     in this area.  Just one more... :-(.  FIXME.  */
	  if (numbytes)
	    numbytes += size_read;

      /* Prepare for next block.  */

      offset += size_read;
      /* FIXME: do not clear unless necessary.  */
      clear_buffer (buffer);
    }

  if (numbytes)
    sparsearray[sparse_index++].numbytes = numbytes;
  else
    {
      sparsearray[sparse_index].offset = offset - 1;
      sparsearray[sparse_index++].numbytes = 1;
    }

  close (file);
  return sparse_index - 1;
}

/*---.
| ?  |
`---*/

static bool
finish_sparse_file (int file, off_t *sizeleft, off_t fullsize, char *name)
{
  union block *start;
  size_t bufsize;
  int sparse_index = 0;
  ssize_t size_read;
#if 0
  long nwritten = 0;
#endif

  while (*sizeleft > 0)
    {
      start = find_next_block ();
      memset (start->buffer, 0, BLOCKSIZE);
      bufsize = sparsearray[sparse_index].numbytes;
      if (!bufsize)
	{
	  /* We blew it, maybe.  */

	  ERROR ((0, 0, _("Wrote %lu of %lu bytes to file %s"),
		  (unsigned long) (fullsize - *sizeleft),
		  (unsigned long) fullsize,
		  name));
	  break;
	}
      lseek (file, sparsearray[sparse_index++].offset, SEEK_SET);

      /* If the number of bytes to be written here exceeds the size of
	 the temporary buffer, do it in steps.  */

      while (bufsize > BLOCKSIZE)
	{
#if 0
	  if (amount_read)
	    {
	      size_read = full_read (file, start->buffer + amount_read,
			    BLOCKSIZE - amount_read);
	      bufsize -= BLOCKSIZE - amount_read;
	      amount_read = 0;
	      set_next_block_after (start);
	      start = find_next_block ();
	      memset (start->buffer, 0, BLOCKSIZE);
	    }
#endif
	  /* Store the data.  */

	  size_read = full_read (file, start->buffer, BLOCKSIZE);
	  if (size_read < 0)
	    {
	      ERROR ((0, errno, _("\
Read error at byte %lu, reading %lu bytes, in file %s"),
		      (unsigned long) fullsize - *sizeleft,
		      (unsigned long) bufsize,
		      name));
	      return true;
	    }
	  bufsize -= size_read;
	  *sizeleft -= size_read;
	  set_next_block_after (start);
#if 0
	  nwritten += BLOCKSIZE;
#endif
	  start = find_next_block ();
	  memset (start->buffer, 0, BLOCKSIZE);
	}

      {
	char buffer[BLOCKSIZE];

	clear_buffer (buffer);
	size_read = full_read (file, buffer, bufsize);
	memcpy (start->buffer, buffer, BLOCKSIZE);
      }

      if (size_read < 0)
	{
	  ERROR ((0, errno,
		  _("Read error at byte %lu, reading %lu bytes, in file %s"),
		  (unsigned long) (fullsize - *sizeleft),
		  (unsigned long) bufsize,
		  name));
	  return true;
	}
#if 0
      if (amount_read >= BLOCKSIZE)
	{
	  amount_read = 0;
	  set_next_block_after (start + (size_read - 1) / BLOCKSIZE);
	  if (size_read != bufsize)
	    {
	      ERROR ((0, 0,
		      _("File %s shrunk by %lu bytes, padding with zeros"),
		      name, (unsigned long) *sizeleft));
	      /* FIXME: Handle previous message for huge files.  */
	      return true;
	    }
	  start = find_next_block ();
	}
      else
	amount_read += bufsize;
#endif
#if 0
      nwritten += size_read;
#endif
      *sizeleft -= size_read;
      set_next_block_after (start);

    }
  free (sparsearray);
#if 0
  printf (_("Amount actually written is (I hope) %d.\n"), nwritten);
  set_next_block_after (start + (size_read - 1) / BLOCKSIZE);
#endif
  return false;
}

/* Main functions of this module.  */

/*---.
| ?  |
`---*/

void
create_archive (void)
{
  char *p;

  open_archive (ACCESS_WRITE);

  if (incremental_option)
    {
      char *buffer = xmalloc (PATH_MAX);
      const char *q;
      char *bufp;

      collect_and_sort_names ();

      while (p = name_from_list (), p)
	dump_file (p, (dev_t) -1, true);

      blank_name_list ();
      while (p = name_from_list (), p)
	{
	  strcpy (buffer, p);
	  if (p[strlen (p) - 1] != '/')
	    strcat (buffer, "/");
	  bufp = buffer + strlen (buffer);
	  for (q = name_list_current->dir_contents;
	       q && *q;
	       q += strlen (q) + 1)
	    {
	      if (*q == 'Y')
		{
		  strcpy (bufp, q + 1);
		  dump_file (buffer, (dev_t) -1, true);
		}
	    }
	}
      free (buffer);
    }
  else
    {
      while (p = name_next (true), p)
	dump_file (p, (dev_t) -1, true);
    }

  write_eot ();
  close_archive ();

  if (listed_incremental_option && !dev_null_output)
    write_dir_file ();
}

/*--------------------------------------------------------------------------.
| Dump a single file.  Recurse on directories.                              |
|                                                                           |
| P is file name to dump.  PARENT_DEVICE is device our parent directory was |
| on.  TOP_LEVEL tells wether we are a toplevel call.  On return, leaves    |
| global CURRENT_STAT to stat output for this file.                         |
`--------------------------------------------------------------------------*/

/* FIXME: One should make sure that for *every* path leading to setting
   exit_status to failure, a clear diagnostic has been issued.  */

void
dump_file (char *p, dev_t parent_device, bool top_level)
{
  union block *header;
  char type;
  union block *exhdr;
  char save_typeflag;
  struct utimbuf restore_times;

  /* FIXME: `header' and `upperbound' might be used uninitialized in this
     function.  Reported by Bruno Haible.  */

  if (interactive_option && !confirm (_("add %s?"), p))
    return;

  /* Use stat if following (rather than dumping) 4.2BSD's symbolic links.
     Otherwise, use lstat (which falls back to stat if no symbolic links).  */

  if (dereference_option
#ifdef STX_HIDDEN		/* AIX */
      ? statx (p, &current.stat, STATSIZE, STX_HIDDEN)
      : statx (p, &current.stat, STATSIZE, STX_HIDDEN | STX_LINK)
#else
      ? stat (p, &current.stat)
      : lstat (p, &current.stat)
#endif
      )
    {
      WARN ((0, errno, _("Cannot add file %s"), p));
      if (!ignore_failed_read_option)
	exit_status = TAREXIT_FAILURE;
      return;
    }

  restore_times.actime = current.stat.st_atime;
  restore_times.modtime = current.stat.st_mtime;

#ifdef S_ISHIDDEN
  if (S_ISHIDDEN (current.stat.st_mode))
    {
      char *new = (char *) alloca (strlen (p) + 2);

      if (new)
	{
	  strcpy (new, p);
	  strcat (new, "@");
	  p = new;
	}
    }
#endif

  /* See if we only want new files, and check if this one is too old to
     put in the archive.  */

  if (!incremental_option && !S_ISDIR (current.stat.st_mode)
      && !FILE_IS_NEW_ENOUGH (&current.stat))
    {
      if (parent_device == (dev_t) -1)
	WARN ((0, 0, _("%s: is unchanged; not dumped"), p));
      /* FIXME: recheck this return.  */
      return;
    }

  /* See if we are trying to dump the archive.  */

  if (ar_dev && current.stat.st_dev == ar_dev && current.stat.st_ino == ar_ino)
    {
      WARN ((0, 0, _("%s is the archive; not dumped"), p));
      return;
    }

  /* Check for multiple links.

     We maintain a list of all such files that we've written so far.  Any
     time we see another, we check the list and avoid dumping the data
     again if we've done it once already.  */

  if (current.stat.st_nlink > 1
      && (S_ISREG (current.stat.st_mode)
#ifdef S_ISCTG
	  || S_ISCTG (current.stat.st_mode)
#endif
#ifdef S_ISCHR
	  || S_ISCHR (current.stat.st_mode)
#endif
#ifdef S_ISBLK
	  || S_ISBLK (current.stat.st_mode)
#endif
#ifdef S_ISFIFO
	  || S_ISFIFO (current.stat.st_mode)
#endif
      ))

    {
      struct link *lp;

#if LINK_LINEAR_SEARCH
      for (lp = link_list; lp; lp = lp->next)
	if (lp->ino == current.stat.st_ino && lp->dev == current.stat.st_dev)
	  break;
#else
      if (!link_table)
	{
	  link_table = hash_initialize (0, NULL, link_hash, link_compare, NULL);
	  if (!link_table)
	    FATAL_ERROR ((0, 0, _("Memory exhausted")));
	}

      {
	struct link link;

	link.ino = current.stat.st_ino;
	link.dev = current.stat.st_dev;
	lp = hash_lookup (link_table, &link);
      }
#endif

      if (lp)
	{
	  char *linkname = lp->name;

	  /* We found a link.  */

	  while (!absolute_names_option && *linkname == '/')
	    {
	      static bool warned_once = false;

	      if (!warned_once)
		{
		  warned_once = true;
		  WARN ((0, 0, _("\
Removing leading `/' from absolute links")));
		}
	      linkname++;
	    }
	  if (strlen (linkname) >= NAME_FIELD_SIZE)
	    write_long (linkname, OLDGNU_LONGLINK);
	  assign_string (&current.linkname, linkname);

	  current.stat.st_size = 0;
	  header = start_header (p, &current.stat);
	  if (header == NULL)
	    {
	      exit_status = TAREXIT_FAILURE;
	      return;
	    }
	  strncpy (header->header.linkname,
		   linkname, NAME_FIELD_SIZE);

	  /* Force null truncated.  */

	  header->header.linkname[NAME_FIELD_SIZE - 1] = 0;

	  header->header.typeflag = LNKTYPE;
	  finish_header (header);

	  /* FIXME: Maybe remove from list after all links found?  */

	  if (remove_files_option)
	    if (unlink (p) == -1)
	      ERROR ((0, errno, _("Cannot remove %s"), p));

	  /* We dumped it.  */
	  return;
	}

      /* Not found.  Add it to the list of possible links.  */

      lp = (struct link *) xmalloc (sizeof (struct link) + strlen (p));
      lp->ino = current.stat.st_ino;
      lp->dev = current.stat.st_dev;
      strcpy (lp->name, p);

#if LINK_LINEAR_SEARCH
      lp->next = link_list;
      link_list = lp;
#else
      {
	bool done;

	hash_insert (link_table, lp, &done);
	if (!done)
	  FATAL_ERROR ((0, 0, _("Memory exhausted")));
      }
#endif
    }

  /* This is not a link to a previously dumped file, so dump it.  */

  if (S_ISREG (current.stat.st_mode)
#ifdef S_ISCTG
      || S_ISCTG (current.stat.st_mode)
#endif
      )
    {
      int handle;		/* file descriptor */
      size_t bufsize;
      ssize_t size_read;
      off_t sizeleft;
      union block *start;
      bool header_moved = false;
      bool is_extended = false;
      int upperbound;
      long padding_count = 0L;
      bool complain = false;

      if (sparse_option)
	{
	  /* Check the size of the file against the number of blocks
	     allocated for it, counting both data and indirect blocks.
	     If there is a smaller number of blocks that would be
	     necessary to accommodate a file of this size, this is safe
	     to say that we have a sparse file: at least one of those
	     blocks in the file is just a useless hole.  For sparse
	     files not having more hole blocks than indirect blocks, the
	     sparseness will go undetected.  */

	  /* tar.h defines ST_NBLOCKS in term of 512 byte sectors, even
	     for HP-UX's which count in 1024 byte units and AIX's which
	     count in 4096 byte units.  So this should work...  */

	  /* Bruno Haible sent me these statistics for Linux.  It seems
	     that some filesystems count indirect blocks in st_blocks,
	     while others do not seem to:

	     minix-fs   tar: size=7205, st_blocks=18 and ST_NBLOCKS=18
	     extfs      tar: size=7205, st_blocks=18 and ST_NBLOCKS=18
	     ext2fs     tar: size=7205, st_blocks=16 and ST_NBLOCKS=16
	     msdos-fs   tar: size=7205, st_blocks=16 and ST_NBLOCKS=16

	     Dick Streefland reports the previous numbers as misleading,
	     because ext2fs use 12 direct blocks, while minix-fs uses only
	     6 direct blocks.  Dick gets:

	     ext2	size=20480	ls listed blocks=21
	     minix	size=20480	ls listed blocks=21
	     msdos	size=20480	ls listed blocks=20

	     It seems that indirect blocks *are* included in st_blocks.
	     The minix filesystem does not account for phantom blocks in
	     st_blocks, so `du' and `ls -s' give wrong results.  So, the
	     --sparse option would not work on a minix filesystem.  */

	  if (current.stat.st_size > ST_NBLOCKS (current.stat) * BLOCKSIZE)
	    {
	      off_t file_size = current.stat.st_size;
	      int counter;

	      header = start_header (p, &current.stat);
	      if (header == NULL)
		{
		  exit_status = TAREXIT_FAILURE;
		  return;
		}
	      header->header.typeflag = OLDGNU_SPARSE;
	      header_moved = true;

	      /* Call the routine that figures out the layout of the
		 sparse file in question.  UPPERBOUND is the index of the
		 last element of the "sparsearray," i.e., the number of
		 elements it needed to describe the file.  */

	      upperbound = deal_with_sparse (p, header);

	      /* See if we'll need an extended header later.  */

	      if (upperbound > SPARSES_IN_OLDGNU_HEADER - 1)
		header->oldgnu_header.isextended = 1;

	      /* We store the "real" file size so we can show that in
		 case someone wants to list the archive, i.e., tar tvf
		 <file>.  It might be kind of disconcerting if the
		 shrunken file size was the one that showed up.  */

	      set_header_realsize (header, current.stat.st_size);

	      /* This will be the new "size" of the file, i.e., the size
		 of the file minus the blocks of holes that we're
		 skipping over.  */

	      find_new_file_size (&file_size, upperbound);
	      current.stat.st_size = file_size;
	      set_header_size (header, file_size);

	      for (counter = 0; counter < SPARSES_IN_OLDGNU_HEADER; counter++)
		{
		  if (!sparsearray[counter].numbytes)
		    break;

		  set_initial_header_offset
		    (header, counter, sparsearray[counter].offset);
		  set_initial_header_numbytes
		    (header, counter, sparsearray[counter].numbytes);
		}
	    }
	}
      else
	upperbound = SPARSES_IN_OLDGNU_HEADER - 1;

      sizeleft = current.stat.st_size;

      /* Don't bother opening empty, world readable files.  Also do not open
	 files when archive is meant for /dev/null.  */

      if (dev_null_output
	  || (sizeleft == 0 && 0444 == (0444 & current.stat.st_mode)))
	handle = -1;
      else
	{
	  handle = open (p, O_RDONLY | O_BINARY);
	  if (handle < 0)
	    {
	      WARN ((0, errno, _("Cannot add file %s"), p));
	      if (!ignore_failed_read_option)
		exit_status = TAREXIT_FAILURE;
	      return;
	    }
	}

      /* If the file is sparse, we've already taken care of this.  */

      if (!header_moved)
	{
	  header = start_header (p, &current.stat);
	  if (header == NULL)
	    {
	      if (handle >= 0)
		close (handle);
	      exit_status = TAREXIT_FAILURE;
	      return;
	    }
	}

#ifdef S_ISCTG
      /* Mark contiguous files, if we support them.  */

      if (archive_format != V7_FORMAT && S_ISCTG (current.stat.st_mode))
	header->header.typeflag = CONTTYPE;
#endif
      is_extended = header->oldgnu_header.isextended != 0;
      save_typeflag = header->header.typeflag;
      finish_header (header);
      if (is_extended)
	{
	  int counter;
#if 0
	  union block *exhdr;
	  int arraybound = SPARSES_IN_SPARSE_HEADER;
#endif
	  /* static */ int index_offset = SPARSES_IN_OLDGNU_HEADER;

	  while (true)
	    {
	      exhdr = find_next_block ();

	      if (exhdr == NULL)
		{
		  exit_status = TAREXIT_FAILURE;
		  return;
		}
	      memset (exhdr->buffer, 0, BLOCKSIZE);
	      for (counter = 0; counter < SPARSES_IN_SPARSE_HEADER; counter++)
		{
		  if (counter + index_offset > upperbound)
		    break;

		  set_extended_header_numbytes
		    (exhdr, counter,
		     sparsearray[counter + index_offset].numbytes);
		  set_extended_header_offset
		    (exhdr, counter,
		     sparsearray[counter + index_offset].offset);
		}
	      if (counter + index_offset <= upperbound)
		set_next_block_after (exhdr);
	      if (index_offset + counter > upperbound)
		break;

	      index_offset += counter;
	      exhdr->sparse_header.isextended = 1;
	    }
	}

      if (save_typeflag == OLDGNU_SPARSE)
	{
	  if (handle < 0 || finish_sparse_file (handle, &sizeleft,
						current.stat.st_size, p))
	    {
	      /* File shrunk or gave error, pad out tape to match the size we
		 specified in the header. */

	      while (sizeleft > 0)
		{
		  save_sizeleft = sizeleft;
		  start = find_next_block ();
		  memset (start->buffer, 0, BLOCKSIZE);
		  set_next_block_after (start);
		  sizeleft -= BLOCKSIZE;
		}
	    }
	}
      else
	while (sizeleft > 0)
	  {
	    if (multi_volume_option)
	      {
		assign_string (&save_name, p);
		save_sizeleft = sizeleft;
		save_totsize = current.stat.st_size;
	      }
	    start = find_next_block ();
	    bufsize = available_space_after (start);
	    if (sizeleft < bufsize)
	      {
		size_t count;

		/* Last read -- zero out area beyond.  */

		bufsize = sizeleft;
		count = bufsize % BLOCKSIZE;
		if (count)
		  memset (start->buffer + sizeleft, 0, BLOCKSIZE - count);
	      }

	    if (handle < 0)
	      size_read = bufsize;
	    else if (padding_count)
	      size_read = 0;
	    else
	      size_read = full_read (handle, start->buffer, bufsize);

	    if (size_read < 0)
	      {
		if (!complain)
		  ERROR ((0, errno, _("\
Read error at byte %lu, reading %lu bytes, in file %s"),
			  (unsigned long) (current.stat.st_size - sizeleft),
			  (unsigned long) bufsize,
			  p));
		complain = true;
		size_read = 0;
	      }
	    sizeleft -= size_read;

	    if (size_read < bufsize)
	      {
		/* Pad out to match the specified size.  */
		memset (start->buffer + size_read, 0, bufsize - size_read);
		padding_count += bufsize - size_read;
		sizeleft -= bufsize - size_read;
		size_read = bufsize;
	      }

	    /* FIXME: check if a cast is needed for the argument.  */
	    set_next_block_after (start + (size_read - 1) / BLOCKSIZE);
	  }

      if (padding_count && !complain)
	ERROR ((0, 0, _("File %s shrunk by %ld bytes, padding with zeros"),
		p, padding_count));

      if (multi_volume_option)
	assign_string (&save_name, NULL);

      if (handle >= 0)
	{
	  close (handle);
	  if (atime_preserve_option)
	    utime (p, &restore_times);
	}

      if (remove_files_option)
	{
	  /* FIXME: If an error occurred (for example, if a file shrunk), it
	     might be removed nevertheless.  Is that really OK?  */
	  if (unlink (p) == -1)
	    ERROR ((0, errno, _("Cannot remove %s"), p));
	}

      return;
    }

#ifdef S_ISLNK
  if (S_ISLNK (current.stat.st_mode))
    {
      int size;
      char *buffer = (char *) alloca (PATH_MAX + 1);

      size = readlink (p, buffer, PATH_MAX + 1);
      if (size < 0)
	{
	  WARN ((0, errno, _("Cannot add file %s"), p));
	  if (!ignore_failed_read_option)
	    exit_status = TAREXIT_FAILURE;
	  return;
	}
      buffer[size] = '\0';
      if (size >= NAME_FIELD_SIZE)
	write_long (buffer, OLDGNU_LONGLINK);
      assign_string (&current.linkname, buffer);

      current.stat.st_size = 0;	/* force zero size on symlink */
      header = start_header (p, &current.stat);
      if (header == NULL)
	{
	  exit_status = TAREXIT_FAILURE;
	  return;
	}
      strncpy (header->header.linkname, buffer, NAME_FIELD_SIZE);
      header->header.linkname[NAME_FIELD_SIZE - 1] = '\0';
      header->header.typeflag = SYMTYPE;
      finish_header (header);
      if (remove_files_option)
	{
	  if (unlink (p) == -1)
	    ERROR ((0, errno, _("Cannot remove %s"), p));
	}

      return;
    }
#endif /* S_ISLNK */

  if (S_ISDIR (current.stat.st_mode))
    {
      DIR *directory;
      struct dirent *entry;
      char *namebuf;
      size_t buflen;
      size_t len;
      dev_t our_device = current.stat.st_dev;

      /* If this tar program is installed suid root, like for Amanda, the
	 access might look like denied, while it is not really.

	 FIXME: I have the feeling this test is done too early.  Couldn't it
	 just be bundled in later actions?  I guess that the proper support
	 of --ignore-failed-read is the key of the current writing.  */

      if (access (p, R_OK) == -1 && geteuid () != 0)
	{
	  WARN ((0, errno, _("Cannot add directory %s"), p));
	  if (!ignore_failed_read_option)
	    exit_status = TAREXIT_FAILURE;
	  return;
	}

      /* Build new prototype name.  Ensure exactly one trailing slash.  */

      len = strlen (p);
      buflen = len + NAME_FIELD_SIZE;
      namebuf = xmalloc (buflen + 1);
      strncpy (namebuf, p, buflen);
      while (len >= 1 && namebuf[len - 1] == '/')
	len--;
      namebuf[len++] = '/';
      namebuf[len] = '\0';

      if (true)
	{
	  /* The test above used to be "archive_format != V7_FORMAT", tar
	     was just not writing directory blocks at all.  Daniel Trinkle
	     writes: ``All old versions of tar I have ever seen have
	     correctly archived an empty directory.  The really old ones I
	     checked included HP-UX 7 and Mt. Xinu More/BSD.  There may be
	     some subtle reason for the exclusion that I don't know, but the
	     current behavior is broken.''  I do not know those subtle
	     reasons either, so until these are reported (anew?), just allow
	     directory blocks to be written even with old archives.  */

	  current.stat.st_size = 0; /* force zero size on dir */

	  /* FIXME: If people could really read standard archives, this
	     should be:

	     header
	       = start_header (standard_option ? p : namebuf, &current.stat);

	     but since they'd interpret DIRTYPE blocks as regular
	     files, we'd better put the / on the name.  */

	  header = start_header (namebuf, &current.stat);
	  if (header == NULL)
	    {
	      exit_status = TAREXIT_FAILURE;
	      return;	/* eg name too long */
	    }

	  if (incremental_option)
	    header->header.typeflag = OLDGNU_DUMPDIR;
	  else /* if (standard_option) */
	    header->header.typeflag = DIRTYPE;

	  /* If we're gnudumping, we aren't done yet so don't close it.  */

	  if (!incremental_option)
	    finish_header (header);	/* done with directory header */
	}

      if (incremental_option && name_list_current->dir_contents)
	{
	  const char *buffer = name_list_current->dir_contents;
	  off_t totsize = 0;

	  off_t sizeleft;
	  size_t bufsize;
	  union block *start;
	  const char *p_buffer;

	  for (p_buffer = buffer; p_buffer && *p_buffer;)
	    {
	      size_t tmp = strlen (p_buffer) + 1;

	      totsize += tmp;
	      p_buffer += tmp;
	    }
	  totsize++;
	  set_header_size (header, totsize);
	  finish_header (header);
	  p_buffer = buffer;
	  sizeleft = totsize;
	  while (sizeleft > 0)
	    {
	      if (multi_volume_option)
		{
		  assign_string (&save_name, p);
		  save_sizeleft = sizeleft;
		  save_totsize = totsize;
		}
	      start = find_next_block ();
	      bufsize = available_space_after (start);
	      if (sizeleft < bufsize)
		{
		  size_t count;

		  bufsize = sizeleft;
		  count = bufsize % BLOCKSIZE;
		  if (count)
		    memset (start->buffer + sizeleft, 0, BLOCKSIZE - count);
		}
	      memcpy (start->buffer, p_buffer, bufsize);
	      sizeleft -= bufsize;
	      p_buffer += bufsize;
	      set_next_block_after (start + (bufsize - 1) / BLOCKSIZE);
	    }
	  if (multi_volume_option)
	    assign_string (&save_name, NULL);
	  if (atime_preserve_option)
	    utime (p, &restore_times);
	  return;
	}

      /* See if we are about to recurse into a directory, and avoid doing
	 so if the user wants that we do not descend into directories.  */

      if (no_recurse_option)
	return;

      /* See if we are crossing from one file system to another, and
	 avoid doing so if the user only wants to dump one file system.  */

      if (one_file_system_option && !top_level
	  && parent_device != current.stat.st_dev)
	{
	  if (verbose_option)
	    WARN ((0, 0, _("%s: is on a different filesystem; not dumped"),
		   p));
	  return;
	}

      /* Now output all the files in the directory.  */

      errno = 0;		/* FIXME: errno should be read-only */

      directory = opendir (p);
      if (!directory)
	{
	  ERROR ((0, errno, _("Cannot open directory %s"), p));
	  return;
	}

      /* Hack to remove "./" from the front of all the file names.  */

      if (len == 2 && namebuf[0] == '.' && namebuf[1] == '/')
	len = 0;

      /* FIXME: Should speed this up by cd-ing into the dir.  */

      while (entry = readdir (directory), entry)
	{
	  /* Skip `.' and `..'.  */

	  if (is_dot_or_dotdot (entry->d_name))
	    continue;

	  if (NAMLEN (entry) + len >= buflen)
	    {
	      buflen = len + NAMLEN (entry);
	      namebuf = (char *) xrealloc (namebuf, buflen + 1);
#if 0
	      namebuf[len] = '\0';
	      ERROR ((0, 0, _("File name %s%s too long"),
		      namebuf, entry->d_name));
	      continue;
#endif
	    }
	  strcpy (namebuf + len, entry->d_name);
	  if (exclude_option && check_exclude (namebuf))
	    continue;
	  dump_file (namebuf, our_device, false);
	}

      closedir (directory);
      free (namebuf);
      if (atime_preserve_option)
	utime (p, &restore_times);

      return;
    }

  /* Here, the only cases left are for either a special device of a fifo.  */

  if (archive_format == V7_FORMAT)
    {
      ERROR ((0, 0, _("%s: Unknown file type; file ignored"), p));
      return;
    }

#ifdef S_ISCHR
  if (S_ISCHR (current.stat.st_mode))
    type = CHRTYPE;
#endif
#ifdef S_ISBLK
  else if (S_ISBLK (current.stat.st_mode))
    type = BLKTYPE;
#endif
#if (_ISP__M68K == 0) && (_ISP__A88K == 0) && defined(S_ISFIFO)
  /* Avoid screwy Apollo lossage where S_IFIFO == S_IFSOCK.  */
  else if (S_ISFIFO (current.stat.st_mode))
    type = FIFOTYPE;
#endif
#ifdef S_ISSOCK
  else if (S_ISSOCK (current.stat.st_mode))
    type = FIFOTYPE;
#endif
  else
    {
      ERROR ((0, 0, _("%s: Unknown file type; file ignored"), p));
      return;
    }

  current.stat.st_size = 0;	/* force zero size */
  header = start_header (p, &current.stat);
  if (header == NULL)
    {
      /* Something wrong happened while building the header.  For example, the
	 file name was too long.  */
      exit_status = TAREXIT_FAILURE;
      return;
    }

  header->header.typeflag = type;

#if defined(S_IFBLK) || defined(S_IFCHR)
  if (type != FIFOTYPE)
    {
      set_header_devmajor (header, major (current.stat.st_rdev));
      set_header_devminor (header, minor (current.stat.st_rdev));
    }
#endif

  finish_header (header);
  if (remove_files_option)
    {
      if (unlink (p) == -1)
	ERROR ((0, errno, _("Cannot remove %s"), p));
    }
}
