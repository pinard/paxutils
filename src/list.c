/* List a tar archive.
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
   59 Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"
#include <time.h>
#include "common.h"

/* Define to nonzero for forcing old ctime() instead of isotime().  */
#undef USE_OLD_CTIME

/*---------------------------------------------.
| Print a header block, based on tar options.  |
`---------------------------------------------*/

void
list_archive (void)
{
  bool is_extended = false;	/* to remember if current.header is extended */

  /* Print the header block.  */

  if (verbose_option)
    {
      if (verbose_option > 1)
	decode_header (&current, false);
      print_header (&current);
    }

  if (incremental_option && current.block->header.typeflag == OLDGNU_DUMPDIR)
    {
      size_t size;
      size_t size_to_write;
      ssize_t size_written;
      union block *data_block;

      set_next_block_after (current.block);
      if (multi_volume_option)
	{
	  assign_string (&save_name, current.name);
	  save_totsize = current.stat.st_size;
	}
      for (size = current.stat.st_size; size > 0; size -= size_to_write)
	{
	  if (multi_volume_option)
	    save_sizeleft = size;
	  data_block = find_next_block ();
	  if (!data_block)
	    {
	      ERROR ((0, 0, _("Unexpected end of file in archive")));
	      break;		/* FIXME: What happens, then?  */
	    }
	  size_to_write = available_space_after (data_block);
	  if (size_to_write > size)
	    size_to_write = size;
	  errno = 0;		/* FIXME: errno should be read-only */
	  size_written = fwrite (data_block->buffer, 1, size_to_write, stdlis);
	  set_next_block_after ((union block *)
				(data_block->buffer + size_to_write - 1));
	  if (size_written != size_to_write)
	    {
	      /* FIXME: size_written may be negative, here!  */
	      ERROR ((0, errno, _("Only wrote %lu of %lu bytes to file %s"),
		      (unsigned long) size_written,
		      (unsigned long) size_to_write, current.name));
	      skip_file ((off_t) (size - size_to_write));
	      break;
	    }
	}
      if (multi_volume_option)
	assign_string (&save_name, NULL);
      fputc ('\n', stdlis);
      fflush (stdlis);
      return;
    }

  /* Check to see if we have an extended header to skip over also.  */

  if (current.block->oldgnu_header.isextended)
    is_extended = true;

  /* Skip past the header in the archive.  */

  set_next_block_after (current.block);

  /* If we needed to skip any extended headers, do so now, by reading
     extended headers and skipping past them in the archive.  */

  if (is_extended)
    {
#if 0
      union block *exhdr;

      while (true)
	{
	  exhdr = find_next_block ();

	  if (!exhdr->sparse_header.isextended)
	    {
	      set_next_block_after (exhdr);
	      break;
	    }
	  set_next_block_after (exhdr);
	}
#endif
      skip_extended_headers ();
    }

  if (multi_volume_option)
    assign_string (&save_name, current.name);

  /* Skip to the next header on the archive.  */

  skip_file (current.stat.st_size);

  if (multi_volume_option)
    assign_string (&save_name, NULL);
}

#if !USE_OLD_CTIME

/*-------------------------------------------.
| Return the time formatted along ISO 8601.  |
`-------------------------------------------*/

/* Also, see http://www.ft.uni-erlangen.de/~mskuhn/iso-time.html.  */

static char *
isotime (const time_t *value)
{
  static char buffer[21];
  struct tm *tm;

  tm = localtime (value);
  sprintf (buffer, "%4d-%02d-%02d %02d:%02d:%02d\n",
	   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	   tm->tm_hour, tm->tm_min, tm->tm_sec);
  return buffer;
}

#endif /* not USE_OLD_CTIME */

/*-------------------------------------------------------------------------.
| Decode MODE from its binary form in a stat structure, and encode it into |
| a 9 characters string STRING, terminated with a NUL.                     |
`-------------------------------------------------------------------------*/

static void
decode_mode (mode_t mode, char *string)
{
  mode_t mask;
  const char *rwx = "rwxrwxrwx";

  for (mask = 0400; mask; mask >>= 1)
    if (mode & mask)
      *string++ = *rwx++;
    else
      {
	*string++ = '-';
	rwx++;
      }

  if (mode & S_ISUID)
    string[-7] = string[-7] == 'x' ? 's' : 'S';
  if (mode & S_ISGID)
    string[-4] = string[-4] == 'x' ? 's' : 'S';
  if (mode & S_ISVTX)
    string[-1] = string[-1] == 'x' ? 't' : 'T';

  *string = '\0';
}

/*-------------------------------------------------------------------------.
| Print the given ENTRY, according to verbosity option.                    |
|                                                                          |
| Non-verbose just prints the name, e.g. for "tar t" or "tar x".  This     |
| should just contain file names, so it can be fed back into tar with      |
| xargs or the "-T" option.                                                |
|                                                                          |
| The verbose option can give a bunch of info, one line per file.  I doubt |
| anybody tries to parse its format, or if they do, they shouldn't.  Unix  |
| tar is pretty random here anyway.                                        |
`-------------------------------------------------------------------------*/

/* UGSWIDTH starts with 18, so with user and group names <= 8 chars, the
   columns never shift during the listing.  */
#define UGSWIDTH 18
static int ugswidth = UGSWIDTH;	/* maximum width encountered so far */

/* DATEWIDTH is the number of columns taken by the date and time fields.  */
#if USE_OLD_CDATE
# define DATEWIDTH 19
#else
# define DATEWIDTH 18
#endif

void
print_header (struct tar_entry *entry)
{
  char modes[11];
  char *timestamp;
  char uform[11], gform[11];	/* these hold formatted ints */
  char *user, *group;
  char size[24];		/* holds a formatted long or maj, min */
  time_t longie;		/* to make ctime() call portable */
  int pad;
  char *name;

  if (checkpoint_option)
    flush_checkpoint_line ();

  if (block_number_option)
    fprintf (stdlis, _("block %10lu: "),
	     (unsigned long) current_block_ordinal ());

  if (verbose_option <= 1)
    if (null_option)
      {
	fputs (entry->name, stdlis);
	putc ('\0', stdlis);
      }
    else
      {
	char *quoted_name = quote_copy_string (entry->name);

	if (quoted_name)
	  {
	    fputs (quoted_name, stdlis);
	    free (quoted_name);
	  }
	else
	  fputs (entry->name, stdlis);

	putc ('\n', stdlis);
      }
  else
    {
      /* File type and modes.  */

      modes[0] = '?';
      switch (entry->block->header.typeflag)
	{
	case OLDGNU_VOLHDR:
	  modes[0] = 'V';
	  break;

	case OLDGNU_MULTIVOL:
	  modes[0] = 'M';
	  break;

	case OLDGNU_NAMES:
	  modes[0] = 'N';
	  break;

	case OLDGNU_LONGNAME:
	case OLDGNU_LONGLINK:
	  ERROR ((0, 0, _("Long name has no introducing header")));
	  break;

	case OLDGNU_SPARSE:
	case REGTYPE:
	case AREGTYPE:
	case LNKTYPE:
	  modes[0] = '-';
	  if (entry->name[strlen (entry->name) - 1] == '/')
	    modes[0] = 'd';
	  break;
	case OLDGNU_DUMPDIR:
	  modes[0] = 'd';
	  break;
	case DIRTYPE:
	  modes[0] = 'd';
	  break;
	case SYMTYPE:
	  modes[0] = 'l';
	  break;
	case BLKTYPE:
	  modes[0] = 'b';
	  break;
	case CHRTYPE:
	  modes[0] = 'c';
	  break;
	case FIFOTYPE:
	  modes[0] = 'p';
	  break;
	case CONTTYPE:
	  modes[0] = 'C';
	  break;
	}

      decode_mode (entry->stat.st_mode, modes + 1);

      /* Timestamp.  */

      longie = entry->stat.st_mtime;
#if USE_OLD_CTIME
      timestamp = ctime (&longie);
      timestamp[16] = '\0';
      timestamp[24] = '\0';
#else
      timestamp = isotime (&longie);
      timestamp[16] = '\0';
#endif

      /* User and group names.  */

      if (*entry->block->header.uname && entry->format != V7_FORMAT)
	user = entry->block->header.uname;
      else
	{
	  user = uform;
	  sprintf (uform, "%lu",
		   (unsigned long) get_header_uid (entry->block));
	}

      if (*entry->block->header.gname && entry->format != V7_FORMAT)
	group = entry->block->header.gname;
      else
	{
	  group = gform;
	  sprintf (gform, "%ld",
		   (unsigned long) get_header_gid (entry->block));
	}

      /* Format the file size or major/minor device numbers.  */

      switch (entry->block->header.typeflag)
	{
#if defined(S_IFBLK) || defined(S_IFCHR)
	case CHRTYPE:
	case BLKTYPE:
	  sprintf (size, "%lu,%lu",
		   (unsigned long) major (entry->stat.st_rdev),
		   (unsigned long) minor (entry->stat.st_rdev));
	  break;
#endif

	case OLDGNU_SPARSE:
	  sprintf (size, "%lu",
		   (unsigned long) get_header_realsize (entry->block));
	  break;

	default:
	  sprintf (size, "%lu", (unsigned long) entry->stat.st_size);
	}

      /* Figure out padding and print the whole line.  */

      pad = strlen (user) + strlen (group) + strlen (size) + 1;
      if (pad > ugswidth)
	ugswidth = pad;

#if USE_OLD_CTIME
      fprintf (stdlis, "%s %s/%s %*s%s %s %s",
	       modes, user, group, ugswidth - pad, "",
	       size, timestamp + 4, timestamp + 20);
#else
      fprintf (stdlis, "%s %s/%s %*s%s %s",
	       modes, user, group, ugswidth - pad, "", size, timestamp);
#endif

      name = quote_copy_string (entry->name);
      if (name)
	{
	  fprintf (stdlis, " %s", name);
	  free (name);
	}
      else
	fprintf (stdlis, " %s", entry->name);

      switch (entry->block->header.typeflag)
	{
	case SYMTYPE:
	  name = quote_copy_string (entry->linkname);
	  if (name)
	    {
	      fprintf (stdlis, " -> %s\n", name);
	      free (name);
	    }
	  else
	    fprintf (stdlis, " -> %s\n", entry->linkname);
	  break;

	case LNKTYPE:
	  name = quote_copy_string (entry->linkname);
	  if (name)
	    {
	      fprintf (stdlis, _(" link to %s\n"), name);
	      free (name);
	    }
	  else
	    fprintf (stdlis, _(" link to %s\n"), entry->linkname);
	  break;

	default:
	  fprintf (stdlis, _(" unknown file type `%c'\n"),
		   entry->block->header.typeflag);
	  break;

	case AREGTYPE:
	case REGTYPE:
	case OLDGNU_SPARSE:
	case CHRTYPE:
	case BLKTYPE:
	case DIRTYPE:
	case FIFOTYPE:
	case CONTTYPE:
	case OLDGNU_DUMPDIR:
	  putc ('\n', stdlis);
	  break;

	case OLDGNU_VOLHDR:
	  fprintf (stdlis, _("--Volume Header--\n"));
	  break;

	case OLDGNU_MULTIVOL:
	  fprintf (stdlis, _("--Continued at byte %lu--\n"),
		   (unsigned long) get_header_offset (entry->block));
	  break;

	case OLDGNU_NAMES:
	  fprintf (stdlis, _("--Mangled file names--\n"));
	  break;
	}
    }
  fflush (stdlis);
}

/*--------------------------------------------------------------.
| Print a similar line when we make a directory automatically.  |
`--------------------------------------------------------------*/

void
print_for_mkdir (char *pathname, int length, mode_t mode)
{
  char modes[11];
  char *name;

  if (verbose_option > 1)
    {
      /* File type and modes.  */

      modes[0] = 'd';
      decode_mode (mode, modes + 1);

      if (checkpoint_option)
	flush_checkpoint_line ();

      if (block_number_option)
	fprintf (stdlis, _("block %10lu: "),
		 (unsigned long) current_block_ordinal ());

      if (name = quote_copy_string (pathname), name)
	{
	  fprintf (stdlis, "%s %*s %.*s\n", modes, ugswidth + DATEWIDTH,
		   _("Creating directory:"), length, name);
	  free (name);
	}
      else
	fprintf (stdlis, "%s %*s %.*s\n", modes, ugswidth + DATEWIDTH,
		 _("Creating directory:"), length, pathname);
    }
}
