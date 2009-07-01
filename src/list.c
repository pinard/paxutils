/* List a tar archive, with support routines for reading a tar archive.
   Copyright (C) 1988, 92, 93, 94, 96, 97 Free Software Foundation, Inc.
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

/* Define to non-zero for forcing old ctime() instead of isotime().  */
#undef USE_OLD_CTIME

#include "system.h"

#include <ctype.h>
#include <time.h>

#define	ISODIGIT(Char) \
  ((unsigned char) (Char) >= '0' && (unsigned char) (Char) <= '7')
#define ISSPACE(Char) (ISASCII (Char) && isspace (Char))

#include "common.h"

union block *current_header;	/* points to current archive header */
struct stat current_stat;	/* stat struct corresponding */
enum archive_format current_format; /* recognized format */

/*-----------------------------------.
| Main loop for reading an archive.  |
`-----------------------------------*/

void
read_and (void (*do_something) ())
{
  enum read_header status = HEADER_STILL_UNREAD;
  enum read_header prev_status;
  char save_typeflag;

  name_gather ();
  open_archive (ACCESS_READ);

  while (1)
    {
      prev_status = status;
      status = read_header ();
      switch (status)
	{
	case HEADER_STILL_UNREAD:
	  abort ();

	case HEADER_SUCCESS:

	  /* Valid header.  We should decode next field (mode) first.
	     Ensure incoming names are null terminated.  */

	  /* FIXME: This is a quick kludge before 1.12 goes out.  */
	  current_stat.st_mtime
	    = from_oct (1 + 12, current_header->header.mtime);

	  if (!name_match (current_file_name)
	      || current_stat.st_mtime < newer_mtime_option
	      || (exclude_option && check_exclude (current_file_name)))
	    {
	      int isextended = 0;

	      if (current_header->header.typeflag == GNUTYPE_VOLHDR
		  || current_header->header.typeflag == GNUTYPE_MULTIVOL
		  || current_header->header.typeflag == GNUTYPE_NAMES)
		{
		  (*do_something) ();
		  continue;
		}
	      if (show_omitted_dirs_option
		  && current_header->header.typeflag == DIRTYPE)
		WARN ((0, 0, _("Omitting %s"), current_file_name));

	      /* Skip past it in the archive.  */

	      if (current_header->oldgnu_header.isextended)
		isextended = 1;
	      save_typeflag = current_header->header.typeflag;
	      set_next_block_after (current_header);
	      if (isextended)
		{
#if 0
		  union block *exhdr;

		  while (1)
		    {
		      exhdr = find_next_block ();
		      if (!exhdr->sparse_header.isextended)
			{
			  set_next_block_after (exhdr);
			  break;
			}
		    }
		  set_next_block_after (exhdr);
#endif
		  skip_extended_headers ();
		}

	      /* Skip to the next header on the archive.  */

	      if (save_typeflag != DIRTYPE)
		skip_file ((long) current_stat.st_size);
	      continue;
	    }

	  (*do_something) ();
	  continue;

	case HEADER_ZERO_BLOCK:
	  if (block_number_option)
	    fprintf (stdlis, _("block %10ld: ** Block of NULs **\n"),
		     current_block_ordinal ());

	  set_next_block_after (current_header);
	  status = prev_status;
	  if (ignore_zeros_option)
	    continue;
	  break;

	case HEADER_END_OF_FILE:
	  if (block_number_option)
	    fprintf (stdlis, _("block %10ld: ** End of File **\n"),
		     current_block_ordinal ());
	  break;

	case HEADER_FAILURE:
	  /* If the previous header was good, tell them that we are
	     skipping bad ones.  */
	  set_next_block_after (current_header);
	  switch (prev_status)
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
  close_archive ();
  names_notfound ();		/* print names not found */
}

/*---------------------------------------------.
| Print a header block, based on tar options.  |
`---------------------------------------------*/

void
list_archive (void)
{
  int isextended = 0;		/* to remember if current_header is extended */

  /* Print the header block.  */

  if (verbose_option)
    {
      if (verbose_option > 1)
	decode_header (current_header, &current_stat, &current_format, 0);
      print_header ();
    }

  if (incremental_option && current_header->header.typeflag == GNUTYPE_DUMPDIR)
    {
      size_t size, written, check;
      union block *data_block;

      set_next_block_after (current_header);
      if (multi_volume_option)
	{
	  assign_string (&save_name, current_file_name);
	  save_totsize = current_stat.st_size;
	}
      for (size = current_stat.st_size; size > 0; size -= written)
	{
	  if (multi_volume_option)
	    save_sizeleft = size;
	  data_block = find_next_block ();
	  if (!data_block)
	    {
	      ERROR ((0, 0, _("EOF in archive file")));
	      break;		/* FIXME: What happens, then?  */
	    }
	  written = available_space_after (data_block);
	  if (written > size)
	    written = size;
	  errno = 0;		/* FIXME: errno should be read-only */
	  check = fwrite (data_block->buffer, sizeof (char), written, stdlis);
	  set_next_block_after ((union block *)
				(data_block->buffer + written - 1));
	  if (check != written)
	    {
	      ERROR ((0, errno, _("Only wrote %ld of %ld bytes to file %s"),
		      check, written, current_file_name));
	      skip_file ((long) (size - written));
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

  if (current_header->oldgnu_header.isextended)
    isextended = 1;

  /* Skip past the header in the archive.  */

  set_next_block_after (current_header);

  /* If we needed to skip any extended headers, do so now, by reading
     extended headers and skipping past them in the archive.  */

  if (isextended)
    {
#if 0
      union block *exhdr;

      while (1)
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
    assign_string (&save_name, current_file_name);

  /* Skip to the next header on the archive.  */

  skip_file ((long) current_stat.st_size);

  if (multi_volume_option)
    assign_string (&save_name, NULL);
}

/*-----------------------------------------------------------------------.
| Read a block that's supposed to be a header block.  Return its address |
| in "current_header", and if it is good, the file's size in             |
| current_stat.st_size.                                                  |
|                                                                        |
| Return 1 for success, 0 if the checksum is bad, EOF on eof, 2 for a    |
| block full of zeros (EOF marker).                                      |
|                                                                        |
| You must always set_next_block_after(current_header) to skip past the  |
| header which this routine reads.                                       |
`-----------------------------------------------------------------------*/

/* The standard BSD tar sources create the checksum by adding up the
   bytes in the header as type char.  I think the type char was unsigned
   on the PDP-11, but it's signed on the Next and Sun.  It looks like the
   sources to BSD tar were never changed to compute the checksum
   currectly, so both the Sun and Next add the bytes of the header as
   signed chars.  This doesn't cause a problem until you get a file with
   a name containing characters with the high bit set.  So read_header
   computes two checksums -- signed and unsigned.  */

/* FIXME: The signed checksum computation is broken on machines where char's
   are unsigned.  It's uneasy to handle all cases correctly...  */

enum read_header
read_header (void)
{
  int i;
  long unsigned_sum;		/* the POSIX one :-) */
  long signed_sum;		/* the Sun one :-( */
  long recorded_sum;
  char *p;
  union block *header;
  char **longp;
  char *bp;
  union block *data_block;
  int size, written;
  static char *next_long_name, *next_long_link;

  while (1)
    {
      header = find_next_block ();
      current_header = header;
      if (!header)
	return HEADER_END_OF_FILE;

      recorded_sum
	= from_oct (sizeof header->header.chksum, header->header.chksum);

      unsigned_sum = 0;
      signed_sum = 0;
      p = header->buffer;
      for (i = sizeof (*header); --i >= 0;)
	{
	  /* We can't use unsigned char here because of old compilers,
	     e.g. V7.  */

	  unsigned_sum += 0xFF & *p;
	  signed_sum += *p++;
	}

      /* Adjust checksum to count the "chksum" field as blanks.  */

      for (i = sizeof (header->header.chksum); --i >= 0;)
	{
	  unsigned_sum -= 0xFF & header->header.chksum[i];
	  signed_sum -= header->header.chksum[i];
	}
      unsigned_sum += ' ' * sizeof header->header.chksum;
      signed_sum += ' ' * sizeof header->header.chksum;

      if (unsigned_sum == sizeof header->header.chksum * ' ')
	{
	  /* This is a zeroed block...whole block is 0's except for the
	     blanks we faked for the checksum field.  */

	  return HEADER_ZERO_BLOCK;
	}

      if (unsigned_sum != recorded_sum && signed_sum != recorded_sum)
	return HEADER_FAILURE;

      /* Good block.  Decode file size and return.  */

      if (header->header.typeflag == LNKTYPE)
	current_stat.st_size = 0;	/* links 0 size on tape */
      else
	current_stat.st_size = from_oct (1 + 12, header->header.size);

      header->header.name[NAME_FIELD_SIZE - 1] = '\0';
      if (header->header.typeflag == GNUTYPE_LONGNAME
	  || header->header.typeflag == GNUTYPE_LONGLINK)
	{
	  longp = ((header->header.typeflag == GNUTYPE_LONGNAME)
		   ? &next_long_name
		   : &next_long_link);

	  set_next_block_after (header);
	  if (*longp)
	    free (*longp);
	  bp = *longp = (char *) xmalloc ((size_t) current_stat.st_size);

	  for (size = current_stat.st_size; size > 0; size -= written)
	    {
	      data_block = find_next_block ();
	      if (data_block == NULL)
		{
		  ERROR ((0, 0, _("Unexpected EOF on archive file")));
		  break;
		}
	      written = available_space_after (data_block);
	      if (written > size)
		written = size;

	      memcpy (bp, data_block->buffer, (size_t) written);
	      bp += written;
	      set_next_block_after ((union block *)
				    (data_block->buffer + written - 1));
	    }

	  /* Loop!  */

	}
      else
	{
	  assign_string (&current_file_name,
			 (next_long_name ? next_long_name
			  : current_header->header.name));
	  assign_string (&current_link_name,
			 (next_long_link ? next_long_link
			  : current_header->header.linkname));
	  next_long_link = next_long_name = 0;
	  return HEADER_SUCCESS;
	}
    }
}

/*-------------------------------------------------------------------------.
| Decode things from a file HEADER block into STAT_INFO, also setting	   |
| *FORMAT_POINTER depending on the header block format.  If DO_USER_GROUP, |
| decode the user/group information (this is useful for extraction, but	   |
| waste time when merely listing).					   |
| 									   |
| read_header() has already decoded the checksum and length, so we don't.  |
| 									   |
| This routine should *not* be called twice for the same block, since the  |
| two calls might use different DO_USER_GROUP values and thus might end up |
| with different uid/gid for the two calls.  If anybody wants the uid/gid  |
| they should decode it first, and other callers should decode it without  |
| uid/gid before calling a routine, e.g. print_header, that assumes	   |
| decoded data.								   |
`-------------------------------------------------------------------------*/

void
decode_header (union block *header, struct stat *stat_info,
	       enum archive_format *format_pointer, int do_user_group)
{
  enum archive_format format;

  if (strcmp (header->header.magic, TMAGIC) == 0)
    format = POSIX_FORMAT;
  else if (strcmp (header->header.magic, OLDGNU_MAGIC) == 0)
    format = OLDGNU_FORMAT;
  else
    format = V7_FORMAT;
  *format_pointer = format;

  stat_info->st_mode = from_oct (8, header->header.mode);
  stat_info->st_mode &= 07777;
  stat_info->st_mtime = from_oct (1 + 12, header->header.mtime);

  if (format == OLDGNU_FORMAT && incremental_option)
    {
      stat_info->st_atime = from_oct (1 + 12, header->oldgnu_header.atime);
      stat_info->st_ctime = from_oct (1 + 12, header->oldgnu_header.ctime);
    }

  if (format == V7_FORMAT)
    {
      stat_info->st_uid = from_oct (8, header->header.uid);
      stat_info->st_gid = from_oct (8, header->header.gid);
      stat_info->st_rdev = 0;
    }
  else
    {
      if (do_user_group)
	{
	  /* FIXME: Decide if this should somewhat depend on -p.  */

	  if (numeric_owner_option
	      || !*header->header.uname
	      || !uname_to_uid (header->header.uname, &stat_info->st_uid))
	    stat_info->st_uid = from_oct (8, header->header.uid);

	  if (numeric_owner_option
	      || !*header->header.gname
	      || !gname_to_gid (header->header.gname, &stat_info->st_gid))
	    stat_info->st_gid = from_oct (8, header->header.gid);
	}
      switch (header->header.typeflag)
	{
#ifdef S_IFBLK
	case BLKTYPE:
	  stat_info->st_rdev = makedev (from_oct (8, header->header.devmajor),
					from_oct (8, header->header.devminor));
	  break;
#endif

#ifdef S_IFCHR
	case CHRTYPE:
	  stat_info->st_rdev = makedev (from_oct (8, header->header.devmajor),
					from_oct (8, header->header.devminor));
	  break;
#endif

	default:
	  stat_info->st_rdev = 0;
	}
    }
}

/*------------------------------------------------------------------------.
| Quick and dirty octal conversion.  Result is -1 if the field is invalid |
| (all blank, or nonoctal).						  |
`------------------------------------------------------------------------*/

long
from_oct (int digs, char *where)
{
  long value;

  while (ISSPACE (*where))
    {				/* skip spaces */
      where++;
      if (--digs <= 0)
	return -1;		/* all blank field */
    }
  value = 0;
  while (digs > 0 && ISODIGIT (*where))
    {
      /* Scan til nonoctal.  */

      value = (value << 3) | (*where++ - '0');
      --digs;
    }

  if (digs > 0 && *where && !ISSPACE (*where))
    return -1;			/* ended on non-space/nul */

  return value;
}

#if !USE_OLD_CTIME

/*-------------------------------------------.
| Return the time formatted along ISO 8601.  |
`-------------------------------------------*/

/* Also, see http://www.ft.uni-erlangen.de/~mskuhn/iso-time.html.  */

static char *
isotime (const time_t *time)
{
  static char buffer[21];
  struct tm *tm;

  tm = localtime (time);
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
decode_mode (unsigned mode, char *string)
{
  unsigned mask;
  const char *rwx = "rwxrwxrwx";

  for (mask = 0400; mask != 0; mask >>= 1)
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
| Actually print it.							   |
| 									   |
| Plain and fancy file header block logging.  Non-verbose just prints the  |
| name, e.g. for "tar t" or "tar x".  This should just contain file names, |
| so it can be fed back into tar with xargs or the "-T" option.  The	   |
| verbose option can give a bunch of info, one line per file.  I doubt	   |
| anybody tries to parse its format, or if they do, they shouldn't.  Unix  |
| tar is pretty random here anyway.					   |
`-------------------------------------------------------------------------*/

/* FIXME: Note that print_header uses the globals HEAD, HSTAT, and
   HEAD_STANDARD, which must be set up in advance.  Not very clean...  */

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
print_header (void)
{
  char modes[11];
  char *timestamp;
  char uform[11], gform[11];	/* these hold formatted ints */
  char *user, *group;
  char size[24];		/* holds a formatted long or maj, min */
  time_t longie;		/* to make ctime() call portable */
  int pad;
  char *name;

  if (block_number_option)
    fprintf (stdlis, _("block %10ld: "), current_block_ordinal ());

  if (verbose_option <= 1)
    {
      /* Just the fax, mam.  */

      char *quoted_name = quote_copy_string (current_file_name);

      if (quoted_name)
	{
	  fprintf (stdlis, "%s\n", quoted_name);
	  free (quoted_name);
	}
      else
	fprintf (stdlis, "%s\n", current_file_name);
    }
  else
    {
      /* File type and modes.  */

      modes[0] = '?';
      switch (current_header->header.typeflag)
	{
	case GNUTYPE_VOLHDR:
	  modes[0] = 'V';
	  break;

	case GNUTYPE_MULTIVOL:
	  modes[0] = 'M';
	  break;

	case GNUTYPE_NAMES:
	  modes[0] = 'N';
	  break;

	case GNUTYPE_LONGNAME:
	case GNUTYPE_LONGLINK:
	  ERROR ((0, 0, _("Visible longname error")));
	  break;

	case GNUTYPE_SPARSE:
	case REGTYPE:
	case AREGTYPE:
	case LNKTYPE:
	  modes[0] = '-';
	  if (current_file_name[strlen (current_file_name) - 1] == '/')
	    modes[0] = 'd';
	  break;
	case GNUTYPE_DUMPDIR:
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

      decode_mode ((unsigned) current_stat.st_mode, modes + 1);

      /* Timestamp.  */

      longie = current_stat.st_mtime;
#if USE_OLD_CTIME
      timestamp = ctime (&longie);
      timestamp[16] = '\0';
      timestamp[24] = '\0';
#else
      timestamp = isotime (&longie);
      timestamp[16] = '\0';
#endif

      /* User and group names.  */

      if (*current_header->header.uname && current_format != V7_FORMAT)
	user = current_header->header.uname;
      else
	{
	  user = uform;
	  sprintf (uform, "%ld", from_oct (8, current_header->header.uid));
	}

      if (*current_header->header.gname && current_format != V7_FORMAT)
	group = current_header->header.gname;
      else
	{
	  group = gform;
	  sprintf (gform, "%ld", from_oct (8, current_header->header.gid));
	}

      /* Format the file size or major/minor device numbers.  */

      switch (current_header->header.typeflag)
	{
#if defined(S_IFBLK) || defined(S_IFCHR)
	case CHRTYPE:
	case BLKTYPE:
	  sprintf (size, "%d,%d",
		   major (current_stat.st_rdev), minor (current_stat.st_rdev));
	  break;
#endif
	case GNUTYPE_SPARSE:
	  sprintf (size, "%ld",
		   from_oct (1 + 12, current_header->oldgnu_header.realsize));
	  break;
	default:
	  sprintf (size, "%ld", (long) current_stat.st_size);
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

      name = quote_copy_string (current_file_name);
      if (name)
	{
	  fprintf (stdlis, " %s", name);
	  free (name);
	}
      else
	fprintf (stdlis, " %s", current_file_name);

      switch (current_header->header.typeflag)
	{
	case SYMTYPE:
	  name = quote_copy_string (current_link_name);
	  if (name)
	    {
	      fprintf (stdlis, " -> %s\n", name);
	      free (name);
	    }
	  else
	    fprintf (stdlis, " -> %s\n", current_link_name);
	  break;

	case LNKTYPE:
	  name = quote_copy_string (current_link_name);
	  if (name)
	    {
	      fprintf (stdlis, _(" link to %s\n"), name);
	      free (name);
	    }
	  else
	    fprintf (stdlis, _(" link to %s\n"), current_link_name);
	  break;

	default:
	  fprintf (stdlis, _(" unknown file type `%c'\n"),
		   current_header->header.typeflag);
	  break;

	case AREGTYPE:
	case REGTYPE:
	case GNUTYPE_SPARSE:
	case CHRTYPE:
	case BLKTYPE:
	case DIRTYPE:
	case FIFOTYPE:
	case CONTTYPE:
	case GNUTYPE_DUMPDIR:
	  putc ('\n', stdlis);
	  break;

	case GNUTYPE_VOLHDR:
	  fprintf (stdlis, _("--Volume Header--\n"));
	  break;

	case GNUTYPE_MULTIVOL:
	  fprintf (stdlis, _("--Continued at byte %ld--\n"),
		   from_oct (1 + 12, current_header->oldgnu_header.offset));
	  break;

	case GNUTYPE_NAMES:
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
print_for_mkdir (char *pathname, int length, int mode)
{
  char modes[11];
  char *name;

  if (verbose_option > 1)
    {
      /* File type and modes.  */

      modes[0] = 'd';
      decode_mode ((unsigned) mode, modes + 1);

      if (block_number_option)
	fprintf (stdlis, _("block %10ld: "), current_block_ordinal ());
      name = quote_copy_string (pathname);
      if (name)
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

/*--------------------------------------------------------.
| Skip over SIZE bytes of data in blocks in the archive.  |
`--------------------------------------------------------*/

void
skip_file (long size)
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
	FATAL_ERROR ((0, 0, _("Unexpected EOF on archive file")));

      set_next_block_after (x);
      size -= BLOCKSIZE;
      if (multi_volume_option)
	save_sizeleft -= BLOCKSIZE;
    }
}

/*---.
| ?  |
`---*/

void
skip_extended_headers (void)
{
  union block *exhdr;

  while (1)
    {
      exhdr = find_next_block ();
      if (!exhdr->sparse_header.isextended)
	{
	  set_next_block_after (exhdr);
	  break;
	}
      set_next_block_after (exhdr);
    }
}
