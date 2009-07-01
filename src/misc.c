/* Miscellaneous functions for paxutils.
   Copyright (C) 1988,92,94,95,96,97,98,99 Free Software Foundation, Inc.

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
#include "rmt.h"
#include "common.h"

/* Various integer arithmetic.  */

#if DOSWIN

/*--------------------------------------------------------------------------.
| Compare file modification times.  Returns less than zero, zero or greater |
| than zero if the timestamp of the first argument is respectively older,   |
| equal or later than that of the second.                                   |
`--------------------------------------------------------------------------*/

/* The comparisons are up to a time granularity of 2 seconds, because so are
   DOS file times.  */

int
time_compare (time_t mt1, time_t mt2)
{
  double tdiff;

  /* Optimize the (hopefully) usual case.  */
  if (mt1 == mt2)
    return 0;

  /* MS-DOS or MS-Windows FAT filesystems round file timestamps to the nearest
     multiple of 2 seconds.  Don't report differences unless file times differ
     by more than 1 second.  */
  tdiff = difftime (mt1, mt2);
  return (tdiff > 1) ? 1 : (tdiff < -1) ? -1 : 0;
}

#endif /* DOSWIN */

/* Handling strings, when NOT in context of files names.  */

#define ISPRINT(Char) (ISASCII (Char) && isprint (Char))

/*------------------------------------------------.
| Check if STRING looks like a globbing pattern.  |
`------------------------------------------------*/

/* FIXME: I should better check more closely.  It seems at first glance that
   is_pattern is only used when reading a file, and ignored for all
   command line arguments.  */

bool
is_pattern (const char *string)
{
  return (strchr (string, '*') != 0
	  || strchr (string, '[') != 0
	  || strchr (string, '?') != 0);
}

/*-------------------------------------------------------------------------.
| Assign STRING to a copy of VALUE if not NULL, or to NULL.  If STRING was |
| not NULL, it is freed first.						   |
`-------------------------------------------------------------------------*/

void
assign_string (char **string, const char *value)
{
  if (*string)
    free (*string);
  *string = value ? xstrdup (value) : NULL;
}

/*--------------------------------------------------------------------------.
| Allocate a copy of the string quoted as in C, and returns that.  If the   |
| string does not have to be quoted, it returns the NULL string.  The       |
| allocated copy should normally be freed with free() after the caller is   |
| done with it.                                                             |
|                                                                           |
| This is used while listing a tar file for the --list (-t) option, or when |
| generating the directory file in incremental dumps.                       |
`--------------------------------------------------------------------------*/

char *
quote_copy_string (const char *string)
{
  const char *source = string;
  char *destination = NULL;
  char *buffer = NULL;
  bool copying = false;

  while (*source)
    {
      int character = (unsigned char) *source++;

      if (character == '\\')
	{
	  if (!copying)
	    {
	      size_t length = (source - string) - 1;

	      copying = true;
	      buffer = (char *) xmalloc (length + 5 + strlen (source) * 4);
	      memcpy (buffer, string, length);
	      destination = buffer + length;
	    }
	  *destination++ = '\\';
	  *destination++ = '\\';
	}
      else if (ISPRINT (character))
	{
	  if (copying)
	    *destination++ = character;
	}
      else
	{
	  if (!copying)
	    {
	      size_t length = (source - string) - 1;

	      copying = true;
	      buffer = (char *) xmalloc (length + 5 + strlen (source) * 4);
	      memcpy (buffer, string, length);
	      destination = buffer + length;
	    }
	  *destination++ = '\\';
	  switch (character)
	    {
	    case '\n':
	      *destination++ = 'n';
	      break;

	    case '\t':
	      *destination++ = 't';
	      break;

	    case '\f':
	      *destination++ = 'f';
	      break;

	    case '\b':
	      *destination++ = 'b';
	      break;

	    case '\r':
	      *destination++ = 'r';
	      break;

	    case '\177':
	      *destination++ = '?';
	      break;

	    default:
	      *destination++ = (character >> 6) + '0';
	      *destination++ = ((character >> 3) & 07) + '0';
	      *destination++ = (character & 07) + '0';
	      break;
	    }
	}
    }
  if (copying)
    {
      *destination = '\0';
      return buffer;
    }
  return NULL;
}

/*---------------------------------------------------------------------------.
| Takes a quoted string (like those produced by quote_copy_string) and turns |
| it back into the un-quoted original.  This is done in place.  Returns      |
| false only if the string was not properly quoted, but completes the        |
| unquoting anyway (FIXME: the returned result is never checked).            |
|                                                                            |
| This is used for reading the saved directory file in incremental dumps.    |
| It is used for decoding old `N' records (demangling names).  But also, it  |
| is used for decoding file arguments, would they come from the shell or a   |
| -T file, and for decoding the --exclude argument.                          |
`---------------------------------------------------------------------------*/

bool
unquote_string (char *string)
{
  bool result = true;
  char *source = string;
  char *destination = string;

  while (*source)
    if (*source == '\\')
      switch (*++source)
	{
	case '\\':
	  *destination++ = '\\';
	  source++;
	  break;

	case 'n':
	  *destination++ = '\n';
	  source++;
	  break;

	case 't':
	  *destination++ = '\t';
	  source++;
	  break;

	case 'f':
	  *destination++ = '\f';
	  source++;
	  break;

	case 'b':
	  *destination++ = '\b';
	  source++;
	  break;

	case 'r':
	  *destination++ = '\r';
	  source++;
	  break;

	case '?':
	  *destination++ = 0177;
	  source++;
	  break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	  {
	    int value = *source++ - '0';

	    if (*source < '0' || *source > '7')
	      {
		*destination++ = value;
		break;
	      }
	    value = value * 8 + *source++ - '0';
	    if (*source < '0' || *source > '7')
	      {
		*destination++ = value;
		break;
	      }
	    value = value * 8 + *source++ - '0';
	    *destination++ = value;
	    break;
	  }

	default:
	  result = false;
	  *destination++ = '\\';
	  if (*source)
	    *destination++ = *source++;
	  break;
	}
    else if (source != destination)
      *destination++ = *source++;
    else
      source++, destination++;

  if (source != destination)
    *destination = '\0';
  return result;
}

/* Handling strings, when in context of file names.  */

/*---------------------------------------------------------------------.
| Returns true if p is `.' or `..'.  This could be a macro for speed.  |
`---------------------------------------------------------------------*/

/* Early Solaris 2.4 readdir may return d->d_name as `' in NFS-mounted
   directories.  The workaround here skips `' just like `.'.  Without it, tar
   would then treat `' much like `.' and loop endlessly.  */

bool
is_dot_or_dotdot (const char *p)
{
  return (p[0] == '\0'
	  || (p[0] == '.' && (p[1] == '\0'
			      || (p[1] == '.' && p[2] == '\0'))));
}

/*-------------------------------------------------------------------------.
| Allocate a new string, made up of concatenating PATH and NAME.  Abort if |
| result overflows PATH_MAX.                                               |
`-------------------------------------------------------------------------*/

char *
concat_no_slash (const char *path, const char *name)
{
  size_t length = strlen (path) + strlen (name);
  char *buffer;

  if (length > PATH_MAX)
    ERROR ((TAREXIT_FAILURE, 0, _("File name %s%s too long"), path, name));

  buffer = (char *) xmalloc (length + 1);
  strcpy (stpcpy (buffer, path), name);
  return buffer;
}

/*---------------------------------------------------------------------------.
| Allocate a new string, made up of concatenating PATH and NAME with a slash |
| in between.  Abort if result overflows PATH_MAX.                           |
`---------------------------------------------------------------------------*/

char *
concat_with_slash (const char *path, const char *name)
{
  size_t length = strlen (path) + strlen (name) + 1;
  char *buffer;
  char *cursor;

  if (length > PATH_MAX)
    ERROR ((TAREXIT_FAILURE, 0, _("File name %s/%s too long"), path, name));

  buffer = (char *) xmalloc (length + 1);
  cursor = stpcpy (buffer, path);
  *cursor++ = '/';
  strcpy (cursor, name);
  return buffer;
}

#if DOSWIN

/*----------------------------------------.
| Turn backslashes into forward slashes.  |
`----------------------------------------*/

void
unixify_file_name (char *file_name)
{
  if (file_name == NULL)
    return;

  for (; *file_name; file_name++)
    if (*file_name == '\\')
      *file_name = '/';
}

/*------------------------------------------------------------------------.
| Return the program base name from the program name, excluding directory |
| and suffix information.  The test suite wants it simple.                |
`------------------------------------------------------------------------*/

/* On DOS and Windows, the parent program have no real control on argv[0] of
   its children.  It is the OS that computes it and puts it in the special
   place where the startup code gets at it.  And DOS and Windows always place
   there a full pathname, unless the system call which invokes the subsidiary
   program already mentions a fully-qualified pathname.  The DJGPP startup
   code does perform some massaging on argv[0] it gets, like mirroring the
   slashes to the Unix forward style, but it cannot go further in adopting the
   Unix behavior by dropping the leading directories because many DOS/Windows
   programs and programmers are used to depend on that (for example, it makes
   looking for files in the same directory as the executable a snap).  */

const char *
get_programe_base_name (const char *name)
{
  /* The test suite needs the program_name to be rather bare.  We need to find
     the basename and drop the .exe suffix, if any; any other solution could
     fail, since there are too many different shells on MS-DOS.  (Does this
     really work with every Unix shell?)  */

  const char *cursor = name;
  const char *base = name;
  const char *extension = name;

  for (; *cursor; cursor++)
    if (*cursor == '/' || *cursor == '\\' || *cursor == ':')
      base = cursor + 1;
    else if (*cursor == '.')
      extension = cursor + 1;

  if (extension > base
      && tolower (extension[0]) == 'e' && tolower (extension[1]) == 'x'
      && tolower (extension[2]) == 'e' && tolower (extension[3]) == '\0')
    {
      char *copy = xmalloc (extension - base + 1);

      memcpy (copy, base, extension - base);
      copy[extension - base] = '\0';
      return copy;
    }

  return base;
}

#endif /* DOSWIN */

/* Sorting lists.  */

/*---.
| ?  |
`---*/

char *
merge_sort (char *list, int length, int offset, int (*compare) (char *, char *))
{
  char *first_list;
  char *second_list;
  int first_length;
  int second_length;
  char *result;
  char **merge_point;
  char *cursor;
  int counter;

#define SUCCESSOR(Pointer) \
  (*((char **) (((char *) (Pointer)) + offset)))

  if (length == 1)
    return list;

  if (length == 2)
    {
      if ((*compare) (list, SUCCESSOR (list)) > 0)
	{
	  result = SUCCESSOR (list);
	  SUCCESSOR (result) = list;
	  SUCCESSOR (list) = NULL;
	  return result;
	}
      return list;
    }

  first_list = list;
  first_length = (length + 1) / 2;
  second_length = length / 2;
  for (cursor = list, counter = first_length - 1;
       counter;
       cursor = SUCCESSOR (cursor), counter--)
    continue;
  second_list = SUCCESSOR (cursor);
  SUCCESSOR (cursor) = NULL;

  first_list = merge_sort (first_list, first_length, offset, compare);
  second_list = merge_sort (second_list, second_length, offset, compare);

  merge_point = &result;
  while (first_list && second_list)
    if ((*compare) (first_list, second_list) < 0)
      {
	cursor = SUCCESSOR (first_list);
	*merge_point = first_list;
	merge_point = &SUCCESSOR (first_list);
	first_list = cursor;
      }
    else
      {
	cursor = SUCCESSOR (second_list);
	*merge_point = second_list;
	merge_point = &SUCCESSOR (second_list);
	second_list = cursor;
      }
  if (first_list)
    *merge_point = first_list;
  else
    *merge_point = second_list;

  return result;

#undef SUCCESSOR
}

/* Printing progress dots.  */

/* Checkpointing counter; flag to know if an incomplete line is pending.  */
static bool checkpoint_dots = false;
static unsigned checkpoint = 0;
static unsigned checkpoint_frequency;

/*------------------------.
| Checkpoint processing.  |
`------------------------*/

void
init_progress_dots (unsigned frequency)
{
  checkpoint_dots = false;
  checkpoint_frequency = 10;
  checkpoint = 0;
}

void
flush_progress_dots (void)
{
  if (checkpoint_dots)
    {
#if 0
      /* FIXME: Also see the comment in output_dot, below.  It does not mean
	 much to print dot counts unless each dot represents something
	 precise, which experimentation does not grant yet.  */
      fprintf (stderr, " [%d]\n", checkpoint);
#else
      fputc ('\n', stderr);
#endif
      checkpoint_dots = false;
    }
}

void
output_progress_dot (void)
{
  if (++checkpoint % checkpoint_frequency == 0)
    {
      if (!checkpoint_dots)
	{
#if 0
	  /* FIXME: Each dot stands for 10 records, yet apparently not when
	     using -z, even with -B forces reblocking.  Some exploration is
	     needed before it really makes sense to print the record size.  */
	  fprintf (stderr, "[%d] x ", record_size);
#endif
	  checkpoint_dots = true;
	}

      fputc ('.', stderr);
      if (checkpoint % 100 == 0)
	{
	  if (checkpoint % 500 == 0)
	    flush_progress_dots ();
	  else
	    fputc (' ', stderr);
	}

      fflush (stderr);
    }
}

/* File handling.  */

/* Saved names in case backup needs to be undone.  */
enum backup_type backup_type;
static char *before_backup_name = NULL;
static char *after_backup_name = NULL;

/*------------------------------------------------------------------.
| Close file having descriptor FD, and abort if close unsucessful.  |
`------------------------------------------------------------------*/

void
xclose (int fd)
{
  if (close (fd) < 0)
    FATAL_ERROR ((0, errno, _("Cannot close file #%d"), fd));
}

/*-----------------------------------------------------------------------.
| Duplicate file descriptor FROM into becoming INTO, or else, issue	 |
| MESSAGE.  INTO is closed first and has to be the next available slot.	 |
`-----------------------------------------------------------------------*/

void
xdup2 (int from, int into, const char *message)
{
  if (from != into)
    {
      int status = close (into);

      if (status < 0 && errno != EBADF)
	FATAL_ERROR ((0, errno, _("Cannot close descriptor %d"), into));
      status = dup (from);
      if (status != into)
	FATAL_ERROR ((0, errno, "%s", message));
      xclose (from);
    }
}

/*-------------------------------------------------------------------------.
| Delete PATH, whatever it might be.  If RECURSE, first recursively delete |
| the contents of PATH when it is a directory.  Return false on any error, |
| with errno set.  As a special case, if we fail to delete a directory     |
| when not RECURSE, do not set errno (just be tolerant to this error).     |
`-------------------------------------------------------------------------*/

bool
remove_any_file (const char *path, bool recurse)
{
  struct stat stat_info;

  if (lstat (path, &stat_info) < 0)
    return false;

  if (S_ISDIR (stat_info.st_mode))
    if (recurse)
      {
	DIR *dirp = opendir (path);
	struct dirent *dp;

	if (dirp == NULL)
	  return false;

	while (dp = readdir (dirp), dp)
	  if (!is_dot_or_dotdot (dp->d_name))
	    {
	      char *path_buffer = concat_with_slash (path, dp->d_name);

	      if (!remove_any_file (path_buffer, true))
		{
		  int saved_errno = errno;

		  free (path_buffer);
		  closedir (dirp);
		  errno = saved_errno; /* FIXME: errno should be read-only */
		  return false;
		}
	      free (path_buffer);
	    }
	closedir (dirp);
	return rmdir (path) >= 0;
      }
    else
      {
	/* FIXME: Saving errno might not be needed anymore, now that
	   extract_archive tests for the special case before recovery.  */
	int saved_errno = errno;

	if (rmdir (path) >= 0)
	  return true;
	errno = saved_errno;	/* FIXME: errno should be read-only */
	return false;
      }

  return unlink (path) >= 0;
}

/*-------------------------------------------------------------------------.
| Check if PATH already exists and make a backup of it right now.  Return  |
| true only if the backup in either unneeded, or successful.               |
|                                                                          |
| For now, directories are never considered as needing backup.  If         |
| IS_ARCHIVE is true, this is the archive and so, we do not have to backup |
| block or character devices, nor remote entities.                         |
`-------------------------------------------------------------------------*/

bool
maybe_backup_file (const char *path, bool is_archive)
{
  struct stat file_stat;

  /* Check if we really need to backup the file.  */

  if (is_archive && _remdev (path))
    return true;

  if (stat (path, &file_stat))
    {
      if (errno == ENOENT)
	return true;

      ERROR ((0, errno, "%s", path));
      return false;
    }

  if (S_ISDIR (file_stat.st_mode))
    return true;

#if DOSWIN
  /* Backing up non-regular files will never work on DOSWIN, and we change the
     names of files which are special to DOSWIN anyway.  So just pretend we
     succeeded.  */
  if (!S_ISREG (file_stat.st_mode))
    return true;
#endif

#ifdef S_ISBLK
  if (is_archive && S_ISBLK (file_stat.st_mode))
    return true;
#endif

#ifdef S_ISCHR
  if (is_archive && S_ISCHR (file_stat.st_mode))
    return true;
#endif

  assign_string (&before_backup_name, path);

  /* A run situation may exist between Emacs or other programs trying to make
     a backup for the same file simultaneously.  If theoretically possible,
     real problems are unlikely.  Doing any better would require a convention,
     system-wide, for all programs doing backups.  */

  assign_string (&after_backup_name, NULL);
  after_backup_name = find_backup_file_name (path, backup_type);
  if (after_backup_name == NULL)
    FATAL_ERROR ((0, 0, "Virtual memory exhausted"));

  if (rename (before_backup_name, after_backup_name) == 0)
    {
      if (verbose_option)
	{
	  if (checkpoint_option)
	    flush_progress_dots ();
	  fprintf (stdlis, _("Renaming previous `%s' to `%s'\n"),
		   before_backup_name, after_backup_name);
	}
      return true;
    }

  /* The backup operation failed.  */

  ERROR ((0, errno, _("%s: Cannot rename for backup"), before_backup_name));
  assign_string (&after_backup_name, NULL);
  return false;
}

/*-----------------------------------------------------------------------.
| Try to restore the recently backed up file to its original name.  This |
| is usually only needed after a failed extraction.			 |
`-----------------------------------------------------------------------*/

void
undo_last_backup (void)
{
  if (after_backup_name)
    {
      if (rename (after_backup_name, before_backup_name) != 0)
	ERROR ((0, errno, _("%s: Cannot rename from backup"),
		before_backup_name));
      if (verbose_option)
	{
	  if (checkpoint_option)
	    flush_progress_dots ();
	  fprintf (stdlis, _("Renaming `%s' back to `%s'\n"),
		   after_backup_name, before_backup_name);
	}
      assign_string (&after_backup_name, NULL);
    }
}
