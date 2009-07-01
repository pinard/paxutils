/* fmtcpio.c - read and write cpio format.
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

#include "system.h"

#include <assert.h>
#include "extern.h"
#include "cpiohdr.h"
#include "cpio.h"
#include "filetypes.h"

/*==============================================\
| First half of file is reading cpio archives.  |
\==============================================*/

/* Return 16-bit integer I with the bytes swapped.  */
#define swab_short(i) ((((i) << 8) & 0xff00) | (((i) >> 8) & 0x00ff))

/*-------------------------------------------------------------------------.
| Exchange the bytes of each element of the array of COUNT shorts starting |
| at PTR.                                                                  |
`-------------------------------------------------------------------------*/

void
swab_array (char *ptr, int count)
{
  char tmp;

  while (count-- > 0)
    {
      tmp = *ptr;
      *ptr = *(ptr + 1);
      ++ptr;
      *ptr = tmp;
      ++ptr;
    }
}

/*------------------------------------------------------------------------.
| Fill in FILE_HDR by reading an old-format ASCII format cpio header from |
| file descriptor IN_DES, except for the magic number, which is already   |
| filled in.                                                              |
`------------------------------------------------------------------------*/

void
read_in_old_ascii (struct new_cpio_header *file_hdr, int in_des)
{
  char ascii_header[78];
  unsigned long dev;
  unsigned long rdev;

  tape_buffered_read (ascii_header, in_des, 70L);
  ascii_header[70] = '\0';
  sscanf (ascii_header,
	  "%6lo%6lo%6lo%6lo%6lo%6lo%6lo%11lo%6lo%11lo",
	  &dev, &file_hdr->c_ino,
	  &file_hdr->c_mode, &file_hdr->c_uid, &file_hdr->c_gid,
	  &file_hdr->c_nlink, &rdev, &file_hdr->c_mtime,
	  &file_hdr->c_namesize, &file_hdr->c_filesize);
  file_hdr->c_dev_maj = major (dev);
  file_hdr->c_dev_min = minor (dev);
  file_hdr->c_rdev_maj = major (rdev);
  file_hdr->c_rdev_min = minor (rdev);

  /* Read file name from input.  */
  if (file_hdr->c_name != NULL)
    free (file_hdr->c_name);
  file_hdr->c_name = (char *) xmalloc (file_hdr->c_namesize + 1);
  tape_buffered_read (file_hdr->c_name, in_des, (long) file_hdr->c_namesize);
#ifndef __MSDOS__
  /* HP/UX cpio creates archives that look just like ordinary archives, but
     for devices it sets major = 0, minor = 1, and puts the actual major/minor
     number in the filesize field.  See if this is an HP/UX cpio archive, and
     if so fix it.  We have to do this here because process_copy_in() assumes
     filesize is always 0 for devices.  */
  switch (file_hdr->c_mode & CP_IFMT)
    {
      case CP_IFCHR:
      case CP_IFBLK:
#ifdef CP_IFSOCK
      case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
      case CP_IFIFO:
#endif
	if (file_hdr->c_filesize != 0
	    && file_hdr->c_rdev_maj == 0
	    && file_hdr->c_rdev_min == 1)
	  {
	    file_hdr->c_rdev_maj = major (file_hdr->c_filesize);
	    file_hdr->c_rdev_min = minor (file_hdr->c_filesize);
	    file_hdr->c_filesize = 0;
	  }
	break;
      default:
	break;
    }
#endif  /* __MSDOS__ */
}

/*-----------------------------------------------------------------------.
| Fill in FILE_HDR by reading a new-format ASCII format cpio header from |
| file descriptor IN_DES, except for the magic number, which is already  |
| filled in.                                                             |
`-----------------------------------------------------------------------*/

void
read_in_new_ascii (struct new_cpio_header *file_hdr, int in_des)
{
  char ascii_header[112];

  tape_buffered_read (ascii_header, in_des, 104L);
  ascii_header[104] = '\0';
  sscanf (ascii_header,
	  "%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx",
	  &file_hdr->c_ino, &file_hdr->c_mode, &file_hdr->c_uid,
	  &file_hdr->c_gid, &file_hdr->c_nlink, &file_hdr->c_mtime,
	  &file_hdr->c_filesize, &file_hdr->c_dev_maj, &file_hdr->c_dev_min,
	&file_hdr->c_rdev_maj, &file_hdr->c_rdev_min, &file_hdr->c_namesize,
	  &file_hdr->c_chksum);
  /* Read file name from input.  */
  if (file_hdr->c_name != NULL)
    free (file_hdr->c_name);
  file_hdr->c_name = (char *) xmalloc (file_hdr->c_namesize);
  tape_buffered_read (file_hdr->c_name, in_des, (long) file_hdr->c_namesize);

  /* In SVR4 ASCII format, the amount of space allocated for the header is
     rounded up to the next long-word, so we might need to drop 1-3 bytes.  */
  tape_skip_padding (in_des, file_hdr->c_namesize + 110);
}

/*---------------------------------------------------------------------------.
| Fill in FILE_HDR by reading a binary format cpio header from file          |
| descriptor IN_DES, except for the first 6 bytes (the magic number, device, |
| and inode number), which are already filled in.                            |
`---------------------------------------------------------------------------*/

void
read_in_binary (struct new_cpio_header *file_hdr, int in_des)
{
  struct old_cpio_header short_hdr;

  /* Copy the data into the short header, then later transfer it into the
     argument long header.  */
  short_hdr.c_dev = ((struct old_cpio_header *) file_hdr)->c_dev;
  short_hdr.c_ino = ((struct old_cpio_header *) file_hdr)->c_ino;
  tape_buffered_read (((char *) &short_hdr) + 6, in_des, 20L);

  /* If the magic number is byte swapped, fix the header.  */
  if (file_hdr->c_magic == swab_short ((unsigned short) 070707))
    {
      static int warned = 0;

      /* Alert the user that they might have to do byte swapping on
	 the file contents.  */
      if (warned == 0)
	{
	  error (0, 0, _("warning: archive header has reverse byte-order"));
	  warned = 1;
	}
      swab_array ((char *) &short_hdr, 13);
    }

  file_hdr->c_dev_maj = major (short_hdr.c_dev);
  file_hdr->c_dev_min = minor (short_hdr.c_dev);
  file_hdr->c_ino = short_hdr.c_ino;
  file_hdr->c_mode = short_hdr.c_mode;
  file_hdr->c_uid = short_hdr.c_uid;
  file_hdr->c_gid = short_hdr.c_gid;
  file_hdr->c_nlink = short_hdr.c_nlink;
  file_hdr->c_rdev_maj = major (short_hdr.c_rdev);
  file_hdr->c_rdev_min = minor (short_hdr.c_rdev);
  file_hdr->c_mtime = (unsigned long) short_hdr.c_mtimes[0] << 16
    | short_hdr.c_mtimes[1];

  file_hdr->c_namesize = short_hdr.c_namesize;
  file_hdr->c_filesize = (unsigned long) short_hdr.c_filesizes[0] << 16
    | short_hdr.c_filesizes[1];

  /* Read file name from input.  */
  if (file_hdr->c_name != NULL)
    free (file_hdr->c_name);
  file_hdr->c_name = (char *) xmalloc (file_hdr->c_namesize);
  tape_buffered_read (file_hdr->c_name, in_des, (long) file_hdr->c_namesize);

  /* In binary mode, the amount of space allocated in the header for the
     filename is `c_namesize' rounded up to the next short-word, so we might
     need to drop a byte.  */
  if (file_hdr->c_namesize % 2)
    tape_toss_input (in_des, 1L);

#ifndef __MSDOS__
  /* HP/UX cpio creates archives that look just like ordinary archives, but
     for devices it sets major = 0, minor = 1, and puts the actual major/minor
     number in the filesize field.  See if this is an HP/UX cpio archive, and
     if so fix it.  We have to do this here because process_copy_in() assumes
     filesize is always 0 for devices.  */
  switch (file_hdr->c_mode & CP_IFMT)
    {
      case CP_IFCHR:
      case CP_IFBLK:
#ifdef CP_IFSOCK
      case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
      case CP_IFIFO:
#endif
	if (file_hdr->c_filesize != 0
	    && file_hdr->c_rdev_maj == 0
	    && file_hdr->c_rdev_min == 1)
	  {
	    file_hdr->c_rdev_maj = major (file_hdr->c_filesize);
	    file_hdr->c_rdev_min = minor (file_hdr->c_filesize);
	    file_hdr->c_filesize = 0;
	  }
	break;
      default:
	break;
    }
#endif  /* __MSDOS__ */
}


/*===============================================\
| Second half of file is writing cpio archives.  |
\===============================================*/

/*--------------------------.
| New-style ASCII headers.  |
`--------------------------*/

void
write_out_cpioascii_header (struct new_cpio_header *file_hdr, int out_des)
{
  char ascii_header[112];
  char *magic_string;

  assert (archive_format == arf_newascii || archive_format == arf_crcascii);

  if (archive_format == arf_crcascii)
    magic_string = "070702";
  else
    magic_string = "070701";
  sprintf (ascii_header,
	   "%6s%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx",
	   magic_string,
	   file_hdr->c_ino, file_hdr->c_mode, file_hdr->c_uid,
	   file_hdr->c_gid, file_hdr->c_nlink, file_hdr->c_mtime,
	   file_hdr->c_filesize, file_hdr->c_dev_maj, file_hdr->c_dev_min,
	   file_hdr->c_rdev_maj, file_hdr->c_rdev_min, file_hdr->c_namesize,
	   file_hdr->c_chksum);
  tape_buffered_write (ascii_header, out_des, 110L);

      /* Write file name to output.  */
  tape_buffered_write (file_hdr->c_name, out_des, (long) file_hdr->c_namesize);
  tape_pad_output (out_des, file_hdr->c_namesize + 110);
}

/*--------------------------.
| Old-style ASCII headers.  |
`--------------------------*/

void
write_out_oldascii_header (struct new_cpio_header *file_hdr, int out_des)
{
  char ascii_header[78];
  int magic = 070707;
#ifndef __MSDOS__
  dev_t dev;
  dev_t rdev;
#endif /* __MSDOS__ */

  assert (archive_format == arf_oldascii || archive_format == arf_hpoldascii);

#ifndef __MSDOS__
  if (archive_format == arf_oldascii)
    {
      dev = makedev (file_hdr->c_dev_maj, file_hdr->c_dev_min);
      rdev = makedev (file_hdr->c_rdev_maj, file_hdr->c_rdev_min);
    }
  else /* arf_hpoldascii */
    {
      /* HP/UX cpio creates archives that look just like ordinary archives,
	 but for devices it sets major = 0, minor = 1, and puts the actual
	 major/minor number in the filesize field.  */
      switch (file_hdr->c_mode & CP_IFMT)
	{
	case CP_IFCHR:
	case CP_IFBLK:
#ifdef CP_IFSOCK
	case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
	case CP_IFIFO:
#endif
	  file_hdr->c_filesize = makedev (file_hdr->c_rdev_maj,
					  file_hdr->c_rdev_min);
	  rdev = 1;
	  break;
	default:
	  dev = makedev (file_hdr->c_dev_maj, file_hdr->c_dev_min);
	  rdev = makedev (file_hdr->c_rdev_maj, file_hdr->c_rdev_min);
	  break;
	}
    }
#else
  long dev = 0, rdev = 0;
#endif

  if ((file_hdr->c_ino >> 16) != 0)
    error (0, 0, _("%s: truncating inode number"), file_hdr->c_name);

  sprintf (ascii_header,
	   "%06o%06o%06lo%06lo%06lo%06lo%06lo%06o%011lo%06lo%011lo",
	   magic, ((int) dev) & 0xFFFF,
	   file_hdr->c_ino & 0xFFFF, file_hdr->c_mode & 0xFFFF,
	   file_hdr->c_uid & 0xFFFF, file_hdr->c_gid & 0xFFFF,
	   file_hdr->c_nlink & 0xFFFF, ((int) rdev) & 0xFFFF,
	   file_hdr->c_mtime, file_hdr->c_namesize & 0xFFFF,
	   file_hdr->c_filesize);
  tape_buffered_write (ascii_header, out_des, 76L);

  /* Write file name to output.  */
  tape_buffered_write (file_hdr->c_name, out_des, (long) file_hdr->c_namesize);
}

/*---------------------------.
| Old-style binary headers.  |
`---------------------------*/

void
write_out_oldcpio_header (struct new_cpio_header *file_hdr, int out_des)
{
  struct old_cpio_header short_hdr;

  assert (archive_format == arf_binary || archive_format == arf_hpbinary);

  short_hdr.c_magic = 070707;
  short_hdr.c_dev = makedev (file_hdr->c_dev_maj, file_hdr->c_dev_min);

  if ((file_hdr->c_ino >> 16) != 0)
    error (0, 0, _("%s: truncating inode number"), file_hdr->c_name);

  short_hdr.c_ino = file_hdr->c_ino & 0xFFFF;
  short_hdr.c_mode = file_hdr->c_mode & 0xFFFF;
  short_hdr.c_uid = file_hdr->c_uid & 0xFFFF;
  short_hdr.c_gid = file_hdr->c_gid & 0xFFFF;
  short_hdr.c_nlink = file_hdr->c_nlink & 0xFFFF;
  if (archive_format != arf_hpbinary)
    short_hdr.c_rdev = makedev (file_hdr->c_rdev_maj, file_hdr->c_rdev_min);
  else
    {
      switch (file_hdr->c_mode & CP_IFMT)
	{
	  /* HP/UX cpio creates archives that look just like ordinary
	     archives, but for devices it sets major = 0, minor = 1, and
	     puts the actual major/minor number in the filesize field.  */
	case CP_IFCHR:
	case CP_IFBLK:
#ifdef CP_IFSOCK
	case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
	case CP_IFIFO:
#endif
	  file_hdr->c_filesize = makedev (file_hdr->c_rdev_maj,
					  file_hdr->c_rdev_min);
	  short_hdr.c_rdev = makedev (0, 1);
	  break;
	default:
	  short_hdr.c_rdev = makedev (file_hdr->c_rdev_maj,
				      file_hdr->c_rdev_min);
	  break;
	}
    }
  short_hdr.c_mtimes[0] = file_hdr->c_mtime >> 16;
  short_hdr.c_mtimes[1] = file_hdr->c_mtime & 0xFFFF;

  short_hdr.c_namesize = file_hdr->c_namesize & 0xFFFF;

  short_hdr.c_filesizes[0] = file_hdr->c_filesize >> 16;
  short_hdr.c_filesizes[1] = file_hdr->c_filesize & 0xFFFF;

  /* Output the file header.  */
  tape_buffered_write ((char *) &short_hdr, out_des, 26L);

  /* Write file name to output.  */
  tape_buffered_write (file_hdr->c_name, out_des, (long) file_hdr->c_namesize);

  tape_pad_output (out_des, file_hdr->c_namesize + 26);
}

/*---------------------------------.
| Write trailer for CPIO archive.  |
`---------------------------------*/

void
write_cpio_eof (int out_file_des)
{
  struct new_cpio_header file_hdr; /* output header information */

  file_hdr.c_ino = 0;
  file_hdr.c_mode = 0;
  file_hdr.c_uid = 0;
  file_hdr.c_gid = 0;
  file_hdr.c_nlink = 1;		/* must be 1 for crc format */
  file_hdr.c_dev_maj = 0;
  file_hdr.c_dev_min = 0;
  file_hdr.c_rdev_maj = 0;
  file_hdr.c_rdev_min = 0;
  file_hdr.c_mtime = 0;
  file_hdr.c_chksum = 0;

  file_hdr.c_filesize = 0;
  file_hdr.c_namesize = 11;
  file_hdr.c_name = "TRAILER!!!";

  (*header_writer) (&file_hdr, out_file_des);
}

/*--------------------------------------------------------.
| Return `true' if FILENAME is too long for cpio format.  |
`--------------------------------------------------------*/

int
is_cpio_filename_too_long (char *filename)
{
  /* cpio can handle any length of name.  */
  return (0);
}
