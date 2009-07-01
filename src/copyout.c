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

#include <assert.h>
#include "filetypes.h"
#include "cpiohdr.h"
#include "extern.h"
#include "defer.h"
#include "rmt.h"

static unsigned long read_for_checksum PARAMS ((int, int, char *));
static void tape_clear_rest_of_block PARAMS ((int));
static int last_link PARAMS ((struct new_cpio_header *));
static int count_defered_links_to_dev_ino PARAMS ((struct new_cpio_header *));
static void add_link_defer PARAMS ((struct new_cpio_header *));
static void writeout_other_defers PARAMS ((struct new_cpio_header *, int));
static void writeout_final_defers PARAMS ((int));
static void writeout_defered_file PARAMS ((struct new_cpio_header *, int));
static void dump_file PARAMS ((int, int, dynamic_string *));
static void copy_out_one_file PARAMS ((int, dynamic_string *, struct stat *));

/*--------------------------------------------------.
| Like copy_out_one_file, but handle all printing.  |
`--------------------------------------------------*/

static void
verbosely_copy_out_file (int out_file_des, dynamic_string *input_name,
			 struct stat *stat_info)
{
  /* NOTE: pax wants to print the file name sans \n before processing, and add
     the \n after processing.  We just do the same thing for cpio (I assume it
     is ok).  */
  if (verbose_flag)
    {
      fprintf (stderr, "%s", input_name->string);
      fflush (stderr);
    }

  copy_out_one_file (out_file_des, input_name, stat_info);

  /* Doesn't really make sense to have dot_flag and verbose_flag set at the
     same time.  Must check for this.  */
#if 0
  assert (!dot_flag || !verbose_flag);
#endif

  if (verbose_flag)
    fputc ('\n', stderr);

  if (dot_flag)
    fputc ('.', stderr);
}

/*----------------------------------------------------------------------------.
| Like copy_out_one_file, but recurses for directories.  TOP_LEVEL is true    |
| if at top level.  OUT_FILE_DES is the file descriptor of archive.           |
| INPUT_NAME is the name of input file, and FILE_STAT holds the stat result.  |
| FIXME: rethink when directories are dumped?                                 |
`----------------------------------------------------------------------------*/

static void
pax_copy_out_one_file (int top_level, int out_file_des,
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
  if (archive_format == arf_tar
      || archive_format == arf_ustar
      || archive_format == arf_oldgnu)
    {
      /* In tar, we dump directories first.  */
      tar_flag = 1;
      verbosely_copy_out_file (out_file_des, input_name, stat_info);
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

	  /* Big enough for directory, plus most files.  FIXME wish I
	     could remember the actual statistic about typical
	     filename lengths.  I recall reading that whoever
	     developed the fast file system did such a study.  */
	  ds_init (&new_name, len + 32);
	  strcpy (new_name.string, input_name->string);
	  if (input_name->string[ds_strlen (input_name) - 1] != '/')
	    {
	      strcat (new_name.string, "/");
	      ++len;
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

	      dump_file (0, out_file_des, &new_name);
	    }

	  closedir (dirp);
	  ds_destroy (&new_name);
	}
    }

  if (!tar_flag)
    {
      /* In cpio formats, we dump directories last.  */
      verbosely_copy_out_file (out_file_des, input_name, stat_info);
    }
}

/*----------------.
| Dump one file.  |
`----------------*/

static void
dump_file (int top_level, int out_file_des, dynamic_string *input_name)
{
  struct stat stat_info;	/* stat record for file */

  /* Check for blank line.  */
  if (input_name->string[0] == 0)
    error (0, 0, _("blank line ignored"));
  else if ((*xstat) (input_name->string, &stat_info) < 0)
    error (0, errno, "%s", input_name->string);
  else if (directory_recurse_flag)
    pax_copy_out_one_file (top_level, out_file_des, input_name,
			   &stat_info);
  else
    verbosely_copy_out_file (out_file_des, input_name, &stat_info);
}

/*-----------------------------------------------------------------------.
| Read a list of file names and write an archive on stdout.  Many global |
| variables are used to control this process.                            |
`-----------------------------------------------------------------------*/

void
process_copy_out (void)
{
  int res;			/* Result of functions.  */
  dynamic_string input_name;	/* Name of file read from stdin.  */
  int out_file_des;		/* Output file descriptor.  */
  struct stat stat_info;	/* Temporary stat record.  */

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
  out_file_des = archive_des;
  if (_isrmt (out_file_des))
    {
      output_is_special = 1;
      output_is_seekable = 0;
    }
  else
    {
      if (fstat (out_file_des, &stat_info))
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
  output_is_special = !isatty (out_file_des);
#endif

  if (append_flag)
    {
      process_copy_in ();
      prepare_append (out_file_des);
    }

  /* Copy files with names read from stdin.  */
  while (get_next_file_name (&input_name))
    dump_file (1, out_file_des, &input_name);

  writeout_final_defers(out_file_des);

  /* The collection is complete; append the trailer.  */
  (*eof_writer) (out_file_des);

  /* Fill up the output block.  */
  tape_clear_rest_of_block (out_file_des);
  tape_empty_output_buffer (out_file_des);
  if (dot_flag)
    fputc ('\n', stderr);

  if (! no_block_message_flag)
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
| file_hdr.                                                               |
`------------------------------------------------------------------------*/

static void
copy_out_one_file (int out_file_des, dynamic_string *input_name,
		   struct stat *stat_info)
{
  struct new_cpio_header file_hdr;
  int in_file_des;		/* source file descriptor */
  char *p;

  /* Set values in output header.  */
  file_hdr.c_magic = 070707;
  file_hdr.c_dev_maj = major (stat_info->st_dev);
  file_hdr.c_dev_min = minor (stat_info->st_dev);
  file_hdr.c_ino = stat_info->st_ino;
#if DOSWIN
  /* DJGPP doesn't support st_rdev.  Repair that.  */
  stat_info->st_rdev = stat_info->st_dev;
#endif
  /* For POSIX systems that don't define the S_IF macros, we can't assume that
     S_ISfoo means the standard Unix S_IFfoo bit(s) are set.  So do it
     manually, with a different name.  Bleah.  */
  file_hdr.c_mode = (stat_info->st_mode & 07777);
  if (S_ISREG (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFREG;
  else if (S_ISDIR (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFDIR;
#ifdef S_ISBLK
  else if (S_ISBLK (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFBLK;
#endif
#ifdef S_ISCHR
  else if (S_ISCHR (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFCHR;
#endif
#ifdef S_ISFIFO
  else if (S_ISFIFO (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFIFO;
#endif
#ifdef S_ISLNK
  else if (S_ISLNK (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFLNK;
#endif
#ifdef S_ISSOCK
  else if (S_ISSOCK (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFSOCK;
#endif
#ifdef S_ISNWK
  else if (S_ISNWK (stat_info->st_mode))
    file_hdr.c_mode |= CP_IFNWK;
#endif
  file_hdr.c_uid = stat_info->st_uid;
  file_hdr.c_gid = stat_info->st_gid;
  file_hdr.c_nlink = stat_info->st_nlink;
  file_hdr.c_rdev_maj = major (stat_info->st_rdev);
  file_hdr.c_rdev_min = minor (stat_info->st_rdev);
  file_hdr.c_mtime = (stat_info->st_mtime < 0) ? 0 :stat_info->st_mtime;
  file_hdr.c_filesize = stat_info->st_size;
  file_hdr.c_chksum = 0;
  file_hdr.c_tar_linkname = NULL;

  /* Handle HPUX CDF files.  */
  possibly_munge_cdf_directory_name (input_name->string, &file_hdr);

  if ((*name_too_long) (file_hdr.c_name))
    {
      error (0, 0, _("%s: file name too long"), file_hdr.c_name);
      return;
    }

  /* FIXME there is a memory leak here, between this and the HPUX stuff above.
     */
  file_hdr.c_name = possibly_rename_file (file_hdr.c_name);

  /* Copy the named file to the output.  */
  switch (file_hdr.c_mode & CP_IFMT)
    {
    case CP_IFREG:
#ifndef __MSDOS__
      if (archive_format == arf_tar || archive_format == arf_ustar
	  || archive_format == arf_oldgnu)
	{
	  char *otherfile;
	  if ((otherfile = find_inode_file (file_hdr.c_ino,
					    file_hdr.c_dev_maj,
					    file_hdr.c_dev_min)))
	    {
	      file_hdr.c_tar_linkname = otherfile;
	      (*header_writer) (&file_hdr, out_file_des);
	      break;
	    }
	}
      if ( (archive_format == arf_newascii || archive_format == arf_crcascii)
	   && (file_hdr.c_nlink > 1) )
	{
	  if (last_link (&file_hdr) )
	    {
	      writeout_other_defers (&file_hdr, out_file_des);
	    }
	  else
	    {
	      add_link_defer (&file_hdr);
	      break;
	    }
	}
#endif
      in_file_des = open (input_name->string,
			  O_RDONLY | O_BINARY, 0);
      if (in_file_des < 0)
	{
	  error (0, errno, "%s", input_name->string);
	  return;
	}

      if (archive_format == arf_crcascii)
	file_hdr.c_chksum = read_for_checksum (in_file_des,
					       file_hdr.c_filesize,
					       input_name->string);

      (*header_writer) (&file_hdr, out_file_des);
      copy_files_disk_to_tape (in_file_des, out_file_des,
			       file_hdr.c_filesize,
			       input_name->string);

#ifndef __MSDOS__
      if (archive_format == arf_tar || archive_format == arf_ustar
	  || archive_format == arf_oldgnu)
	add_inode (file_hdr.c_ino, file_hdr.c_name, file_hdr.c_dev_maj,
		   file_hdr.c_dev_min);
#endif

      tape_pad_output (out_file_des, file_hdr.c_filesize);

      if (close (in_file_des) < 0)
	error (0, errno, "%s", input_name->string);
      if (reset_time_flag)
	{
	  struct utimbuf times;

	  /* Initialize this in case it has members we don't
	     know to set.  */
	  memset (&times, 0, sizeof (struct utimbuf));
	  times.actime = stat_info->st_atime;
	  times.modtime = stat_info->st_mtime;
	  if (utime (file_hdr.c_name, &times) < 0)
	    error (0, errno, _("%s: error resetting file access time"),
		   file_hdr.c_name);
	}
      break;

    case CP_IFDIR:
      file_hdr.c_filesize = 0;
      (*header_writer) (&file_hdr, out_file_des);
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
      if (archive_format == arf_tar)
	{
	  error (0, 0, _("%s not dumped: not a regular file"),
		 file_hdr.c_name);
	  return;
	}
      else if (archive_format == arf_ustar || archive_format == arf_oldgnu)
	{
	  char *otherfile;
	  if ((otherfile = find_inode_file (file_hdr.c_ino,
					    file_hdr.c_dev_maj,
					    file_hdr.c_dev_min)))
	    {
	      /* This file is linked to another file already in the
		 archive, so write it out as a hard link. */
	      file_hdr.c_mode = (stat_info->st_mode & 07777);
	      file_hdr.c_mode |= CP_IFREG;
	      file_hdr.c_tar_linkname = otherfile;
	      (*header_writer) (&file_hdr, out_file_des);
	      break;
	    }
	  add_inode (file_hdr.c_ino, file_hdr.c_name,
		     file_hdr.c_dev_maj, file_hdr.c_dev_min);
	}
      file_hdr.c_filesize = 0;
      (*header_writer) (&file_hdr, out_file_des);
      break;
#endif

#ifdef CP_IFLNK
    case CP_IFLNK:
      {
	char *link_name = (char *) xmalloc (stat_info->st_size + 1);
	int link_size;

	link_size = readlink (input_name->string, link_name,
			      stat_info->st_size);
	if (link_size < 0)
	  {
	    error (0, errno, "%s", input_name->string);
	    free (link_name);
	    return;
	  }
	file_hdr.c_filesize = link_size;
	if (archive_format == arf_tar || archive_format == arf_ustar
	    || archive_format == arf_oldgnu)
	  {
	    /* FIXME: gnu tar can do long symlinks.  */
	    if (link_size + 1 > 100)
	      {
		error (0, 0, _("%s: symbolic link too long"),
		       file_hdr.c_name);
	      }
	    else
	      {
		link_name[link_size] = '\0';
		file_hdr.c_tar_linkname = link_name;
		(*header_writer) (&file_hdr, out_file_des);
	      }
	  }
	else
	  {
	    (*header_writer) (&file_hdr, out_file_des);
	    tape_buffered_write (link_name, out_file_des, link_size);
	    tape_pad_output (out_file_des, link_size);
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
  free (file_hdr.c_name);
  file_hdr.c_name = NULL;
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
      for (i = 0; i < bytes_read; ++i)
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
tape_clear_rest_of_block (int out_file_des)
{
  while (output_size < io_block_size)
    {
      if ((io_block_size - output_size) > 512)
	tape_buffered_write (zeros_512, out_file_des, 512);
      else
	tape_buffered_write (zeros_512, out_file_des, io_block_size - output_size);
    }
}

/*-------------------------------------------------------------------------.
| Write NULs on OUT_FILE_DES to move from OFFSET (the current location) to |
| the end of the header.                                                   |
`-------------------------------------------------------------------------*/

void
tape_pad_output (int out_file_des, int offset)
{
  int pad;

  if (archive_format == arf_newascii || archive_format == arf_crcascii)
    pad = (4 - (offset % 4)) % 4;
  else if (archive_format == arf_tar || archive_format == arf_ustar
	   || archive_format == arf_oldgnu)
    pad = (512 - (offset % 512)) % 512;
  else if (archive_format != arf_oldascii && archive_format != arf_hpoldascii)
    pad = (2 - (offset % 2)) % 2;
  else
    pad = 0;

  if (pad != 0)
    tape_buffered_write (zeros_512, out_file_des, pad);
}

/* When creating newc and crc archives if a file has multiple (hard)
   links, we don't put any of them into the archive until we have seen
   all of them (or until we get to the end of the list of files that
   are going into the archive and know that we have seen all of the links
   to the file that we will see).  We keep these "defered" files on
   this list.   */
static struct deferment *deferouts = NULL;

/*------------------------------------------------------------------------.
| Is this file_hdr the last (hard) link to a file?  I.e., have we already |
| seen and defered all of the other links?                                |
`------------------------------------------------------------------------*/

static int
last_link (struct new_cpio_header *file_hdr)
{
  int other_files_sofar;

  other_files_sofar = count_defered_links_to_dev_ino (file_hdr);
  if (file_hdr->c_nlink == (other_files_sofar + 1) )
    {
      return 1;
    }
  return 0;
}

/* Count the number of other (hard) links to this file that have
   already been defered.  */

static int
count_defered_links_to_dev_ino (file_hdr)
  struct new_cpio_header *file_hdr;
{
  struct deferment *d;
  int ino;
  int maj;
  int min;
  int count;

  ino = file_hdr->c_ino;
  maj = file_hdr->c_dev_maj;
  min = file_hdr->c_dev_min;
  count = 0;
  for (d = deferouts; d != NULL; d = d->next)
    {
      if ( (d->header.c_ino == ino) && (d->header.c_dev_maj == maj)
	  && (d->header.c_dev_min == min) )
	++count;
    }
  return count;
}

/* Add the file header for a link that is being defered to the deferouts
   list.  */

static void
add_link_defer (file_hdr)
  struct new_cpio_header *file_hdr;
{
  struct deferment *d;
  d = create_deferment (file_hdr);
  d->next = deferouts;
  deferouts = d;
}

/* We are about to put a file into a newc or crc archive that is
   multiply linked.  We have already seen and defered all of the
   other links to the file but haven't written them into the archive.
   Write the other links into the archive, and remove them from the
   deferouts list.  */

static void
writeout_other_defers (file_hdr, out_des)
  struct new_cpio_header *file_hdr;
  int out_des;
{
  struct deferment *d;
  struct deferment *d_prev;
  int	ino;
  int 	maj;
  int   min;
  ino = file_hdr->c_ino;
  maj = file_hdr->c_dev_maj;
  min = file_hdr->c_dev_min;
  d_prev = NULL;
  d = deferouts;
  while (d != NULL)
    {
      if ( (d->header.c_ino == ino) && (d->header.c_dev_maj == maj)
	  && (d->header.c_dev_min == min) )
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
  return;
}
/* When writing newc and crc format archives we defer multiply linked
   files until we have seen all of the links to the file.  If a file
   has links to it that aren't going into the archive, then we will
   never see the "last" link to the file, so at the end we just write
   all of the leftover defered files into the archive.  */

static void
writeout_final_defers(out_des)
  int out_des;
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
writeout_defered_file (struct new_cpio_header *header,
		       int out_file_des)
{
  int in_file_des;
  struct new_cpio_header file_hdr;
  struct utimbuf times;		/* For setting file times.  */
  /* Initialize this in case it has members we don't know to set.  */
  memset (&times, 0, sizeof (struct utimbuf));

  file_hdr = *header;


  in_file_des = open (header->c_name,
		      O_RDONLY | O_BINARY, 0);
  if (in_file_des < 0)
    {
      error (0, errno, "%s", header->c_name);
      return;
    }

  if (archive_format == arf_crcascii)
    file_hdr.c_chksum = read_for_checksum (in_file_des,
					   file_hdr.c_filesize,
					   header->c_name);

  (*header_writer) (&file_hdr, out_file_des);
  copy_files_disk_to_tape (in_file_des, out_file_des, file_hdr.c_filesize,
			   header->c_name);

#ifndef __MSDOS__
  if (archive_format == arf_tar || archive_format == arf_ustar
      || archive_format == arf_oldgnu)
    add_inode (file_hdr.c_ino, file_hdr.c_name, file_hdr.c_dev_maj,
	       file_hdr.c_dev_min);
#endif

  tape_pad_output (out_file_des, file_hdr.c_filesize);

  if (close (in_file_des) < 0)
    error (0, errno, "%s", header->c_name);
  if (reset_time_flag)
    {
      times.actime = file_hdr.c_mtime;
      times.modtime = file_hdr.c_mtime;
      if (utime (file_hdr.c_name, &times) < 0)
	error (0, errno, _("%s: error resetting file access time"),
	       file_hdr.c_name);
    }
  return;
}
