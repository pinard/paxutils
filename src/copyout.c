/* copyout.c - create a cpio archive
   Copyright (C) 1990, 1991, 1992, 1998, 1999 Free Software Foundation, Inc.

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

#include "system.h"
#include "common.h"

#include <assert.h>
#include "filetypes.h"
#include "defer.h"
#include "rmt.h"

static unsigned long read_for_checksum PARAMS ((int, int, char *));
static void tape_clear_rest_of_block PARAMS ((int));
static bool last_link PARAMS ((struct new_cpio_header *));
static int count_defered_links_to_dev_ino PARAMS ((struct new_cpio_header *));
static void add_link_defer PARAMS ((struct new_cpio_header *));
static void writeout_other_defers PARAMS ((struct new_cpio_header *, int));
static void writeout_final_defers PARAMS ((int));
static void writeout_defered_file PARAMS ((struct new_cpio_header *, int));
static void dump_one_file PARAMS ((int, int, dynamic_string *));
static void copy_out_one_file PARAMS ((int, dynamic_string *, struct stat *));

/*--------------------------------------------------.
| Like copy_out_one_file, but handle all printing.  |
`--------------------------------------------------*/

static void
verbosely_copy_out_file (int handle, dynamic_string *input_name,
			 struct stat *stat_info)
{
  /* NOTE: pax wants to print the file name sans \n before processing, and add
     the \n after processing.  We just do the same thing for cpio (I assume it
     is ok).  */
  if (verbose_option)
    {
      fprintf (stderr, "%s", input_name->string);
      fflush (stderr);
    }

  copy_out_one_file (handle, input_name, stat_info);

  /* Doesn't really make sense to have dot_option and verbose_option set at
     the same time.  Must check for this.  */
#if 0
  assert (!dot_option || !verbose_option);
#endif

  if (verbose_option)
    fputc ('\n', stderr);

  if (dot_option)
    fputc ('.', stderr);
}

/*----------------------------------------------------------------------------.
| Like copy_out_one_file, but recurses for directories.  TOP_LEVEL is true    |
| if at top level.  OUT_FILE_DES is the file descriptor of archive.           |
| INPUT_NAME is the name of input file, and FILE_STAT holds the stat result.  |
| FIXME: rethink when directories are dumped?                                 |
`----------------------------------------------------------------------------*/

static void
pax_copy_out_one_file (int top_level, int handle,
		       dynamic_string *input_name, struct stat *stat_info)
{
  int tar_flag = 0;

  if (top_level)
    current_device = stat_info->st_dev;

  /* If we aren't allowed to cross devices, and this file does, then skip it.
     */
  if (!cross_device_flag && stat_info->st_dev != current_device)
    return;

  /* FIXME: other versions of pax always dump directories first, even for cpio
     formats.  We could do this if we were smarter.  For instance when we see
     a directory, we could push its name and mode on a list, and only chmod it
     when we're done reading.  There are caveats to this plan: 1) Deal with
     HPUX CDF somehow.  (The code might need fixing).  2) If the user types
     C-c, we might want to catch it and set the modes correctly (though we
     aren't required to).  */
  if (archive_format == V7_FORMAT
      || archive_format == POSIX_FORMAT
      || archive_format == GNUTAR_FORMAT)
    {
      /* In tar, we dump directories first.  */
      tar_flag = 1;
      verbosely_copy_out_file (handle, input_name, stat_info);
    }

  if (S_ISDIR (stat_info->st_mode))
    {
      /* Recursively do directory.  */
      DIR *dirp;

      dirp = opendir (input_name->string);
      if (!dirp)
	error (0, errno, _("cannot open directory %s"),
	       input_name->string);
      else
	{
	  int len;
	  dynamic_string new_name;
	  struct dirent *d;

	  len = ds_strlen (input_name);

	  /* Big enough for directory, plus most files.  FIXME wish I could
	     remember the actual statistic about typical filename lengths.  I
	     recall reading that whoever developed the fast file system did
	     such a study.  */
	  ds_init (&new_name, len + 32);
	  strcpy (new_name.string, input_name->string);
	  if (input_name->string[ds_strlen (input_name) - 1] != '/')
	    {
	      strcat (new_name.string, "/");
	      len++;
	    }

	  /* Remove "./" from front of all file names.  */
	  if ((len == 2 && new_name.string[0] == '.'
	       && new_name.string[1] == '/')
	      || (len == 1 && new_name.string[0] == '.'))
	    len = 0;

	  while ((d = readdir (dirp)))
	    {
	      if (is_dot_or_dotdot (d->d_name))
		continue;

	      ds_resize (&new_name, NAMLEN (d) + len + 1);
	      strncpy (new_name.string + len, d->d_name, NAMLEN (d));
	      new_name.string[len + NAMLEN (d)] = '\0';

	      dump_one_file (0, handle, &new_name);
	    }

	  closedir (dirp);
	  ds_destroy (&new_name);
	}
    }

  if (!tar_flag)
    /* In cpio formats, we dump directories last.  */
    verbosely_copy_out_file (handle, input_name, stat_info);
}

/*----------------.
| Dump one file.  |
`----------------*/

static void
dump_one_file (int top_level, int handle, dynamic_string *input_name)
{
  struct stat stat_info;	/* stat record for file */

  /* Check for blank line.  */
  if (input_name->string[0] == 0)
    error (0, 0, _("blank line ignored"));
  else if ((*xstat) (input_name->string, &stat_info) < 0)
    error (0, errno, "%s", input_name->string);
  else if (directory_recurse_flag)
    pax_copy_out_one_file (top_level, handle, input_name,
			   &stat_info);
  else
    verbosely_copy_out_file (handle, input_name, &stat_info);
}

/*-----------------------------------------------------------------------.
| Read a list of file names and write an archive on stdout.  Many global |
| variables are used to control this process.                            |
`-----------------------------------------------------------------------*/

void
process_copy_out (void)
{
  int res;			/* result of functions */
  dynamic_string input_name;	/* name of file read from stdin */
  int handle;			/* output file descriptor */
  struct stat stat_info;	/* temporary stat record */

  assert (header_writer != NULL);
  assert (eof_writer != NULL);

  /* Initialize the copy out.  */
  ds_init (&input_name, 128);

#ifdef __MSDOS__
  setmode (archive_des, O_BINARY);
#endif
#if DOSWIN
  if (!isatty (archive_des))
    setmode (archive_des, O_BINARY);
#endif

  /* Check whether the output file might be a tape.  */
  handle = archive_des;
  if (_isrmt (handle))
    {
      output_is_special = 1;
      output_is_seekable = 0;
    }
  else
    {
      if (fstat (handle, &stat_info))
	error (1, errno, _("standard output is closed"));
      output_is_special = (
#ifdef S_ISBLK
			   S_ISBLK (stat_info.st_mode) ||
#endif
			   S_ISCHR (stat_info.st_mode));
      output_is_seekable = S_ISREG (stat_info.st_mode);
    }

#if DOSWIN
  /* We might as well give the user an opportunity to recover from "No space
     on device" even if they are writing to disk files.  It is specifically
     handy with floppies.  */
  output_is_special = !isatty (handle);
#endif

  if (append_option)
    {
      process_copy_in ();
      prepare_append (handle);
    }

  /* Copy files with names read from stdin.  */
  while (get_next_file_name (&input_name))
    dump_one_file (1, handle, &input_name);

  writeout_final_defers (handle);

  /* The collection is complete; append the trailer.  */
  (*eof_writer) (handle);

  /* Fill up the output block.  */
  tape_clear_rest_of_block (handle);
  tape_empty_output_buffer (handle);
  if (dot_option)
    fputc ('\n', stderr);

  if (! quiet_option)
    {
      res = (output_bytes + io_block_size - 1) / io_block_size;
      if (res == 1)
	fprintf (stderr, _("1 block\n"));
      else
	fprintf (stderr, _("%d blocks\n"), res);
    }
}

/*------------------------------------------------------------------------.
| Copy out one file, given its name and some other information.  Fills in |
| header.                                                                 |
`------------------------------------------------------------------------*/

static void
copy_out_one_file (int handle, dynamic_string *input_name,
		   struct stat *stat_info)
{
  struct new_cpio_header header;
  int input_handle;		/* source file descriptor */
  char *p;

  /* Set values in output header.  */
  header.c_magic = 070707;
  header.c_dev_maj = major (stat_info->st_dev);
  header.c_dev_min = minor (stat_info->st_dev);
  header.c_ino = stat_info->st_ino;
#if DOSWIN
  /* DJGPP doesn't support st_rdev.  Repair that.  */
  stat_info->st_rdev = stat_info->st_dev;
#endif
  /* For POSIX systems that don't define the S_IF macros, we can't assume that
     S_ISfoo means the standard Unix S_IFfoo bit(s) are set.  So do it
     manually, with a different name.  Bleah.  */
  header.c_mode = (stat_info->st_mode & 07777);
  if (S_ISREG (stat_info->st_mode))
    header.c_mode |= CP_IFREG;
  else if (S_ISDIR (stat_info->st_mode))
    header.c_mode |= CP_IFDIR;
#ifdef S_ISBLK
  else if (S_ISBLK (stat_info->st_mode))
    header.c_mode |= CP_IFBLK;
#endif
#ifdef S_ISCHR
  else if (S_ISCHR (stat_info->st_mode))
    header.c_mode |= CP_IFCHR;
#endif
#ifdef S_ISFIFO
  else if (S_ISFIFO (stat_info->st_mode))
    header.c_mode |= CP_IFIFO;
#endif
#ifdef S_ISLNK
  else if (S_ISLNK (stat_info->st_mode))
    header.c_mode |= CP_IFLNK;
#endif
#ifdef S_ISSOCK
  else if (S_ISSOCK (stat_info->st_mode))
    header.c_mode |= CP_IFSOCK;
#endif
#ifdef S_ISNWK
  else if (S_ISNWK (stat_info->st_mode))
    header.c_mode |= CP_IFNWK;
#endif
  header.c_uid = stat_info->st_uid;
  header.c_gid = stat_info->st_gid;
  header.c_nlink = stat_info->st_nlink;
  header.c_rdev_maj = major (stat_info->st_rdev);
  header.c_rdev_min = minor (stat_info->st_rdev);
  header.c_mtime = (stat_info->st_mtime < 0) ? 0 :stat_info->st_mtime;
  header.c_filesize = stat_info->st_size;
  header.c_chksum = 0;
  header.c_tar_linkname = NULL;

  /* Handle HPUX CDF files.  */
  possibly_munge_cdf_directory_name (input_name->string, &header);

  if ((*name_too_long) (header.c_name))
    {
      error (0, 0, _("%s: file name too long"), header.c_name);
      return;
    }

  /* FIXME: there is a memory leak here, between this and the HPUX stuff
     above.  */
  header.c_name = possibly_rename_file (header.c_name);

  /* Copy the named file to the output.  */
  switch (header.c_mode & CP_IFMT)
    {
    case CP_IFREG:
#ifndef __MSDOS__
      if (archive_format == V7_FORMAT || archive_format == POSIX_FORMAT
	  || archive_format == GNUTAR_FORMAT)
	{
	  char *otherfile;
	  if ((otherfile = find_inode_file (header.c_ino,
					    header.c_dev_maj,
					    header.c_dev_min)))
	    {
	      header.c_tar_linkname = otherfile;
	      (*header_writer) (&header, handle);
	      break;
	    }
	}
      if ((archive_format == NEW_ASCII_FORMAT
	   || archive_format == CRC_ASCII_FORMAT)
	  && header.c_nlink > 1)
	if (last_link (&header) )
	  writeout_other_defers (&header, handle);
	else
	  {
	    add_link_defer (&header);
	    break;
	  }
#endif
      input_handle = open (input_name->string,
			  O_RDONLY | O_BINARY, 0);
      if (input_handle < 0)
	{
	  error (0, errno, "%s", input_name->string);
	  return;
	}

      if (archive_format == CRC_ASCII_FORMAT)
	header.c_chksum = read_for_checksum (input_handle,
					     header.c_filesize,
					     input_name->string);

      (*header_writer) (&header, handle);
      copy_files_disk_to_tape (input_handle, handle,
			       header.c_filesize,
			       input_name->string);

#ifndef __MSDOS__
      if (archive_format == V7_FORMAT
	  || archive_format == POSIX_FORMAT
	  || archive_format == GNUTAR_FORMAT)
	add_inode (header.c_ino, header.c_name,
		   header.c_dev_maj, header.c_dev_min);
#endif

      tape_pad_output (handle, header.c_filesize);

      if (close (input_handle) < 0)
	error (0, errno, "%s", input_name->string);
      if (reset_access_time_option)
	{
	  struct utimbuf times;

	  /* Initialize this in case it has members we don't
	     know to set.  */
	  memset (&times, 0, sizeof (struct utimbuf));
	  times.actime = stat_info->st_atime;
	  times.modtime = stat_info->st_mtime;
	  /* Silently ignore EROFS because reading the file won't have upset
	     its timestamp if it's on a read-only filesystem.  */
	  if (utime (header.c_name, &times) < 0 && errno != EROFS)
	    error (0, errno, _("%s: error resetting file access time"),
		   header.c_name);
	}
      break;

    case CP_IFDIR:
      header.c_filesize = 0;
      (*header_writer) (&header, handle);
      break;

#ifndef __MSDOS__
    case CP_IFCHR:
    case CP_IFBLK:
#ifdef CP_IFSOCK
    case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
    case CP_IFIFO:
#endif
      if (archive_format == V7_FORMAT)
	{
	  error (0, 0, _("%s not dumped: not a regular file"), header.c_name);
	  return;
	}
      else if (archive_format == POSIX_FORMAT
	       || archive_format == GNUTAR_FORMAT)
	{
	  char *otherfile
	    = find_inode_file (header.c_ino,
			       header.c_dev_maj, header.c_dev_min);

	  if (otherfile)
	    {
	      /* This file is linked to another file already in the archive,
		 so write it out as a hard link.  */
	      header.c_mode = (stat_info->st_mode & 07777);
	      header.c_mode |= CP_IFREG;
	      header.c_tar_linkname = otherfile;
	      (*header_writer) (&header, handle);
	      break;
	    }
	  add_inode (header.c_ino, header.c_name,
		     header.c_dev_maj, header.c_dev_min);
	}
      header.c_filesize = 0;
      (*header_writer) (&header, handle);
      break;
#endif

#ifdef CP_IFLNK
    case CP_IFLNK:
      {
	char *link_name = (char *) xmalloc (stat_info->st_size + 1);
	int link_size = readlink (input_name->string, link_name,
				  stat_info->st_size);

	if (link_size < 0)
	  {
	    error (0, errno, "%s", input_name->string);
	    free (link_name);
	    return;
	  }
	header.c_filesize = link_size;
	if (archive_format == V7_FORMAT
	    || archive_format == POSIX_FORMAT
	    || archive_format == GNUTAR_FORMAT)
	  {
	    /* FIXME: tar can do long symlinks.  */
	    if (link_size + 1 > 100)
	      error (0, 0, _("%s: symbolic link too long"),
		     header.c_name);
	    else
	      {
		link_name[link_size] = '\0';
		header.c_tar_linkname = link_name;
		(*header_writer) (&header, handle);
	      }
	  }
	else
	  {
	    (*header_writer) (&header, handle);
	    tape_buffered_write (link_name, handle, link_size);
	    tape_pad_output (handle, link_size);
	  }
	free (link_name);
      }
      break;
#endif

    default:
      error (0, 0, _("%s: unknown file type"), input_name->string);
    }

  /* FIXME shouldn't do this for each file.  Should maintain a dstring
     somewhere.  */
  free (header.c_name);
  header.c_name = NULL;
}

/*--------------------------------------------------------------------------.
| Read FILE_SIZE bytes of FILE_NAME from IN_FILE_DES and compute and return |
| a checksum for them.                                                      |
`--------------------------------------------------------------------------*/

static unsigned long
read_for_checksum (int in_file_des, int file_size, char *file_name)
{
  unsigned long crc;
  char buf[BUFSIZ];
  int bytes_left;
  int bytes_read;
  int i;

  crc = 0;

  for (bytes_left = file_size; bytes_left > 0; bytes_left -= bytes_read)
    {
      bytes_read = read (in_file_des, buf, BUFSIZ);
      if (bytes_read < 0)
	error (1, errno, _("cannot read checksum for %s"), file_name);
      if (bytes_read == 0)
	break;
      for (i = 0; i < bytes_read; i++)
	crc += buf[i] & 0xff;
    }
  if (lseek (in_file_des, 0L, SEEK_SET))
    error (1, errno, _("cannot read checksum for %s"), file_name);

  return crc;
}

/*---------------------------------------------------------------------------.
| Write out NULs to fill out the rest of the current block on OUT_FILE_DES.  |
`---------------------------------------------------------------------------*/

static void
tape_clear_rest_of_block (int handle)
{
  while (output_buffer_remaining_size < io_block_size)
    {
      if ((io_block_size - output_buffer_remaining_size) > BLOCKSIZE)
	tape_buffered_write (zero_block, handle, BLOCKSIZE);
      else
	tape_buffered_write (zero_block, handle,
			     io_block_size - output_buffer_remaining_size);
    }
}

/*-------------------------------------------------------------------------.
| Write NULs on OUT_FILE_DES to move from OFFSET (the current location) to |
| the end of the header.                                                   |
`-------------------------------------------------------------------------*/

void
tape_pad_output (int handle, int offset)
{
  int pad;

  switch (archive_format)
    {
    case HPUX_OLD_ASCII_FORMAT:
    case OLD_ASCII_FORMAT:
      pad = 0;
      break;

    default:
      pad = PADDING (offset, 2);
      break;

    case CRC_ASCII_FORMAT:
    case NEW_ASCII_FORMAT:
      pad = PADDING (offset, 4);
      break;

    case GNUTAR_FORMAT:
    case POSIX_FORMAT:
    case V7_FORMAT:
      pad = PADDING (offset, BLOCKSIZE);
      break;
    }

  if (pad != 0)
    tape_buffered_write (zero_block, handle, pad);
}

/* When creating newc and crc archives if a file has multiple (hard)
   links, we don't put any of them into the archive until we have seen
   all of them (or until we get to the end of the list of files that
   are going into the archive and know that we have seen all of the links
   to the file that we will see).  We keep these "defered" files on
   this list.   */
static struct deferment *deferouts = NULL;

/*------------------------------------------------------------------------.
| Is this header the last (hard) link to a file?  I.e., have we already |
| seen and defered all of the other links?                                |
`------------------------------------------------------------------------*/

static bool
last_link (struct new_cpio_header *header)
{
  int other_files_sofar = count_defered_links_to_dev_ino (header);

  return header->c_nlink == other_files_sofar + 1;
}

/*---------------------------------------------------------------------------.
| Count the number of other (hard) links to this file that have already been |
| defered.                                                                   |
`---------------------------------------------------------------------------*/

static int
count_defered_links_to_dev_ino (struct new_cpio_header *header)
{
  struct deferment *d;
  int ino;
  int maj;
  int min;
  int count;

  ino = header->c_ino;
  maj = header->c_dev_maj;
  min = header->c_dev_min;
  count = 0;
  for (d = deferouts; d != NULL; d = d->next)
    {
      if (d->header.c_ino == ino
	  && d->header.c_dev_maj == maj
	  && d->header.c_dev_min == min)
	count++;
    }
  return count;
}

/*----------------------------------------------------------------------.
| Add the file header for a link that is being defered to the deferouts |
| list.                                                                 |
`----------------------------------------------------------------------*/

static void
add_link_defer (struct new_cpio_header *header)
{
  struct deferment *d;
  d = create_deferment (header);
  d->next = deferouts;
  deferouts = d;
}

/*------------------------------------------------------------------------.
| We are about to put a file into a newc or crc archive that is multiply  |
| linked.  We have already seen and defered all of the other links to the |
| file but haven't written them into the archive.  Write the other links  |
| into the archive, and remove them from the deferouts list.              |
`------------------------------------------------------------------------*/

static void
writeout_other_defers (struct new_cpio_header *header, int out_des)
{
  struct deferment *d;
  struct deferment *d_prev;
  int	ino;
  int 	maj;
  int   min;
  ino = header->c_ino;
  maj = header->c_dev_maj;
  min = header->c_dev_min;
  d_prev = NULL;
  d = deferouts;
  while (d != NULL)
    {
      if (d->header.c_ino == ino
	  && d->header.c_dev_maj == maj
	  && d->header.c_dev_min == min)
	{
	  struct deferment *d_free;
	  d->header.c_filesize = 0;
	  (*header_writer) (&d->header, out_des);
	  if (d_prev != NULL)
	    d_prev->next = d->next;
	  else
	    deferouts = d->next;
	  d_free = d;
	  d = d->next;
	  free_deferment (d_free);
	}
      else
	{
	  d_prev = d;
	  d = d->next;
	}
    }
}

/*--------------------------------------------------------------------------.
| When writing newc and crc format archives we defer multiply linked files  |
| until we have seen all of the links to the file.  If a file has links to  |
| it that aren't going into the archive, then we will never see the "last"  |
| link to the file, so at the end we just write all of the leftover defered |
| files into the archive.                                                   |
`--------------------------------------------------------------------------*/

static void
writeout_final_defers (int out_des)
{
  struct deferment *d;
  int other_count;
  while (deferouts != NULL)
    {
      other_count = count_defered_links_to_dev_ino (&deferouts->header);
      if (other_count == 1)
	writeout_defered_file (&deferouts->header, out_des);
      else
	{
	  unsigned long save = deferouts->header.c_filesize;
	  deferouts->header.c_filesize = 0;
	  (*header_writer) (&deferouts->header, out_des);
	  deferouts->header.c_filesize = save;
	}
      d = deferouts->next;
      free_deferment (deferouts);
      deferouts = d;
    }
}

/*---------------------------------------------------------------------------.
| Write a file into the archive.  This code is the same as the code in       |
| process_copy_out(), but we need it here too for writeout_final_defers() to |
| call.                                                                      |
`---------------------------------------------------------------------------*/

static void
writeout_defered_file (struct new_cpio_header *header_param,
		       int handle)
{
  int input_handle;
  struct new_cpio_header header;
  struct utimbuf times;		/* for setting file times */

  /* Initialize this in case it has members we don't know to set.  */
  memset (&times, 0, sizeof (struct utimbuf));

  header = *header_param;


  input_handle = open (header_param->c_name, O_RDONLY | O_BINARY, 0);
  if (input_handle < 0)
    {
      error (0, errno, "%s", header_param->c_name);
      return;
    }

  if (archive_format == CRC_ASCII_FORMAT)
    header.c_chksum = read_for_checksum (input_handle,
					   header.c_filesize,
					   header_param->c_name);

  (*header_writer) (&header, handle);
  copy_files_disk_to_tape (input_handle, handle, header.c_filesize,
			   header_param->c_name);

#ifndef __MSDOS__
  if (archive_format == V7_FORMAT || archive_format == POSIX_FORMAT
      || archive_format == GNUTAR_FORMAT)
    add_inode (header.c_ino, header.c_name, header.c_dev_maj,
	       header.c_dev_min);
#endif

  tape_pad_output (handle, header.c_filesize);

  if (close (input_handle) < 0)
    error (0, errno, "%s", header_param->c_name);
  if (reset_access_time_option)
    {
      times.actime = header.c_mtime;
      times.modtime = header.c_mtime;
      /* Silently ignore EROFS because reading the file won't have upset its
	 timestamp if it's on a read-only filesystem.  */
      if (utime (header.c_name, &times) < 0 && errno != EROFS)
	error (0, errno, _("%s: error resetting file access time"),
	       header.c_name);
    }
}
