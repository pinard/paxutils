/* Extract files from a tar archive.
   Copyright (C) 1988, 92, 93, 94, 96, 97, 98 Free Software Foundation, Inc.
   Written by John Gilmore, on 1985-11-19.

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
time_t time ();

#include "common.h"

static time_t now;		/* current time */
static bool we_are_root;	/* true if our effective uid == 0 */
static mode_t newdir_umask;	/* umask when creating new directories */
static mode_t current_umask;	/* current umask (which is set to 0 if -p) */

#if 0
/* "Scratch" space to store the information about a sparse file before
   writing the info into the header or extended header.  */
struct sp_array *sparsearray;

/* Number of elts storable in the sparsearray.  */
int sp_array_size = 10;
#endif

struct delayed_set_stat
  {
    struct delayed_set_stat *next;
    char *file_name;
    struct stat stat_info;
  };

static struct delayed_set_stat *delayed_set_stat_head;

/*--------------------------.
| Set up to extract files.  |
`--------------------------*/

void
extr_init (void)
{
  now = time ((time_t *) 0);
  we_are_root = geteuid () == 0;

  /* Option -p clears the kernel umask, so it does not affect proper
     restoration of file permissions.  New intermediate directories will
     comply with umask at start of program.  */

  newdir_umask = umask ((mode_t) 0);
  if (same_permissions_option)
    current_umask = 0;
  else
    {
      umask (newdir_umask);	/* restore the kernel umask */
      current_umask = newdir_umask;
    }

  /* FIXME: Just make sure we can add files in directories we create.  Maybe
     should we later remove permissions we are adding, here?  */
  newdir_umask &= ~0300;
}

/*------------------------------------------------------------------.
| Restore mode for FILE_NAME, from information given in STAT_INFO.  |
`------------------------------------------------------------------*/

static void
set_mode (char *file_name, struct stat *stat_info)
{
  if (no_attributes_option)
    return;

  /* We ought to force permission when -k is not selected, because if the
     file already existed, open or creat would save the permission bits from
     the previously created file, ignoring the ones we specified.

     But with -k selected, we know *we* created this file, so the mode
     bits were set by our open.  If the file has abnormal mode bits, we must
     chmod since writing or chown has probably reset them.  If the file is
     normal, we merely skip the chmod.  This works because we did umask (0)
     when -p, so umask will have left the specified mode alone.  */

  if (!keep_old_files_option
      || (stat_info->st_mode & (S_ISUID | S_ISGID | S_ISVTX)))
    if (chmod (file_name, ~current_umask & stat_info->st_mode) < 0)
      ERROR ((0, errno, _("%s: Cannot change mode to %0.4lo"),
	      file_name, (unsigned long) ~current_umask & stat_info->st_mode));
}

/*----------------------------------------------------------------------.
| Restore stat attributes (owner, group, mode and times) for FILE_NAME, |
| using information given in STAT_INFO.  SYMLINK_FLAG is true for a     |
| freshly restored symbolic link.                                       |
`----------------------------------------------------------------------*/

/* FIXME: About proper restoration of symbolic link attributes, we still do
   not have it right.  Pretesters' reports tell us we need further study and
   probably more configuration.  For now, just use lchown if it exists, and
   punt for the rest.  Sigh!  */

static void
set_stat (char *file_name, struct stat *stat_info, bool symlink_flag)
{
  struct utimbuf utimbuf;

  if (no_attributes_option)
    return;

  if (!symlink_flag)
    {
      /* We do the utime before the chmod because some versions of utime are
	 broken and trash the modes of the file.  */

      if (!touch_option)
	{
	  /* We set the accessed time to `now', which is really the time we
	     started extracting files, unless incremental_option is used, in
	     which case .st_atime is used.  */

	  /* FIXME: incremental_option should set ctime too, but how?  */

	  if (incremental_option)
	    utimbuf.actime = stat_info->st_atime;
	  else
	    utimbuf.actime = now;

	  utimbuf.modtime = stat_info->st_mtime;

	  if (utime (file_name, &utimbuf) < 0)
	    ERROR ((0, errno,
		    _("%s: Could not change access and modification times"),
		    file_name));
	}

      /* Some systems allow non-root users to give files away.  Once this
	 done, it is not possible anymore to change file permissions, so we
	 have to set permissions prior to possibly giving files away.  */

      set_mode (file_name, stat_info);
    }

  /* If we are root, set the owner and group of the extracted file, so we
     extract as the original owner.  Or else, if we are running as a user,
     leave the owner and group as they are, so we extract as that user.  */

  if (we_are_root || same_owner_option)
    {
#if HAVE_LCHOWN

      /* When lchown exists, it should be used to change the attributes of
	 the symbolic link itself.  In this case, a mere chown would change
	 the attributes of the file the symbolic link is pointing to, and
	 should be avoided.  */

      if (symlink_flag)
	{
	  if (lchown (file_name, stat_info->st_uid, stat_info->st_gid) < 0)
	    ERROR ((0, errno, _("%s: Cannot lchown to uid %lu gid %lu"),
		    file_name,
		    (unsigned long) stat_info->st_uid,
		    (unsigned long) stat_info->st_gid));
	}
      else
	{
	  if (chown (file_name, stat_info->st_uid, stat_info->st_gid) < 0)
	    ERROR ((0, errno, _("%s: Cannot chown to uid %lu gid %lu"),
		    file_name,
		    (unsigned long) stat_info->st_uid,
		    (unsigned long) stat_info->st_gid));
	}

#else /* not HAVE_LCHOWN */

      if (!symlink_flag)

	if (chown (file_name, stat_info->st_uid, stat_info->st_gid) < 0)
	  ERROR ((0, errno, _("%s: Cannot chown to uid %lu gid %lu"),
		  file_name,
		  (unsigned long) stat_info->st_uid,
		  (unsigned long) stat_info->st_gid));

#endif/* not HAVE_LCHOWN */

      if (!symlink_flag)

	/* On a few systems, and in particular, those allowing to give files
	   away, changing the owner or group destroys the suid or sgid bits.
	   So, when root, let's attempt setting these bits once more.  */

	if (we_are_root
	    && (stat_info->st_mode & (S_ISUID | S_ISGID | S_ISVTX)))
	  set_mode (file_name, stat_info);
    }
}

/*-----------------------------------------------------------------------.
| After a file/link/symlink/directory creation has failed, see if it's   |
| because some required directory was not present, and if so, create all |
| required directories.  Return true if a directory was created.         |
`-----------------------------------------------------------------------*/

static bool
make_directories (char *file_name)
{
  char *cursor;			/* points into path */
  bool did_something = false;	/* did we do anything yet? */
  int saved_errno = errno;	/* remember caller's errno */
  int status;

  for (cursor = strchr (file_name, '/');
       cursor != NULL;
       cursor = strchr (cursor + 1, '/'))
    {
      /* Avoid mkdir of empty string, if leading or double '/'.  */
      if (cursor == file_name || cursor[-1] == '/')
	continue;

#if DOSWIN
      /* Avoid mkdir of a root directory on any drive.  */
      if (cursor == file_name + 2 && cursor[-1] == ':')
	continue;
#endif

      /* Avoid mkdir where last part of path is '.'.  */
      if (cursor[-1] == '.' && (cursor == file_name + 1 || cursor[-2] == '/'))
	continue;

      *cursor = '\0';		/* truncate the path there */
      status = mkdir (file_name, ~newdir_umask & 0777);

      if (status == 0)
	{
	  /* Fix ownership.  */

	  if (we_are_root && !no_attributes_option)
	    if (chown (file_name, current.stat.st_uid, current.stat.st_gid)
		< 0)
	      ERROR ((0, errno,
		      _("%s: Cannot change owner to uid %lu, gid %lu"),
		      file_name,
		      (unsigned long) current.stat.st_uid,
		      (unsigned long) current.stat.st_gid));

	  print_for_mkdir (file_name, cursor - file_name,
			   ~newdir_umask & 0777);
	  did_something = true;

	  *cursor = '/';
	  continue;
	}

      *cursor = '/';

      /* Directory might already exist.  Moreover, when some intermediate
	 directory creation is needed and users are restoring with full paths,
	 it is likely that some prefix directory will be owned by root, and
	 the system will rather say that permission is denied.  DOSWIN can
	 also get an access error when trying to mkdir a directory which is a
	 "cwd" on another drive.  Let's ignore all such cases, hoping that
	 some longer prefix will soon be useful.  */
      if (errno == EEXIST || errno == EACCES)
	continue;

      /* Other errors are unexpected, we then return to the caller now.  */
      break;
    }

  errno = saved_errno;		/* FIXME: errno should be read-only */
  return did_something;		/* tell them to retry if we made one */
}

#if DOSWIN

/*--------------------------------------------------------------------------.
| Attempt to repair file names that are illegal on MS-DOS and MS-Windows by |
| changing illegal characters.                                              |
`--------------------------------------------------------------------------*/

static int msdosify_count = 0;

static char *
msdosify_file_name (char *file_name)
{
  static char dos_name[PATH_MAX];
  static char illegal_chars_dos[] = ".+, ;=[]|<>\\\":?*";
  static char *illegal_chars_w95 = &illegal_chars_dos[8];
  int idx, dot_idx;
  char *s = file_name, *d = dos_name;
  char *illegal_aliens = illegal_chars_dos;
  size_t len = sizeof (illegal_chars_dos) - 1;
  int save_errno = errno;	/* in case `_use_lfn' below sets errno */

  /* Support for Windows 9X VFAT systems, when available.  Support smaller set
     of illegal characters on MS-Windows 9X.  */

  if (_use_lfn (file_name))
    {
      illegal_aliens = illegal_chars_w95;
      len -= illegal_chars_w95 - illegal_chars_dos;
    }

  /* Get past the drive letter, if any. */
  if (s[0] >= 'A' && s[0] <= 'z' && s[1] == ':')
    {
      *d++ = *s++;
      *d++ = *s++;
    }

  for (idx = 0, dot_idx = -1; *s; s++, d++)
    {
      if (memchr (illegal_aliens, *s, len))
	{
	  /* Dots are special: DOS doesn't allow them as the leading
	     character, and a file name cannot have more than a single dot.
	     We leave the first non-leading dot alone, unless it comes too
	     close to the beginning of the name: we want sh.lex.c to become
	     sh_lex.c, not sh.lex-c.  */
	  if (*s == '.')
	    {
	      if (idx == 0 && (s[1] == '/' || s[1] == '.' && s[2] == '/'))
		{
		  /* Copy "./" and "../" verbatim.  */
		  *d++ = *s++;
		  if (*s == '.')
		    *d++ = *s++;
		  *d = *s;
		}
	      else if (idx == 0)
		*d = '_';
	      else if (dot_idx >= 0)
		{
		  if (dot_idx < 5) /* 5 is a heuristic ad-hoc'ery */
		    {
		      d[dot_idx - idx] = '_'; /* replace previous dot */
		      *d = '.';
		    }
		  else
		    *d = '-';
		}
	      else
		*d = '.';

	      if (*s == '.')
		dot_idx = idx;
	    }
	  else if (*s == '+' && s[1] == '+')
	    {
	      if (idx - 2 == dot_idx) /* .c++, .h++ etc. */
		{
		  *d++ = 'x';
		  *d   = 'x';
		}
	      else
		{
		  /* libg++ etc.  */
		  memcpy (d, "plus", 4);
		  d += 3;
		}
	      s++;
	      idx++;
	    }
	  else
	    *d = '_';
	}
      else
	*d = *s;
      if (*s == '/')
	{
	  idx = 0;
	  dot_idx = -1;
	}
      else
	idx++;
    }

  *d = '\0';
  errno = save_errno;		/* FIXME: errno should be read-only */
  return dos_name;
}

#endif /* DOSWIN */

/*-------------------------------------------------------------------.
| Attempt repairing what went wrong with the extraction.  Delete an  |
| already existing file or create missing intermediate directories.  |
| Return true if we somewhat increased our chances at a successful   |
| extraction.  errno is properly restored on false return.           |
`-------------------------------------------------------------------*/

static bool
maybe_recoverable (char *file_name)
{
  switch (errno)
    {
      /* FIXME: On MS-DOS, we could get EEXIST because names of two files
	 clash in the 8+3 DOS namespace.  (This happens when the archive was
	 brought from Unix.)  We could even be overwriting another file from
	 the same archive.  Tough.  They should use --backup.  */

    case EEXIST:
      /* Attempt deleting an existing file.  However, with -k, just stay
	 quiet.  */
      if (keep_old_files_option)
	return false;

      /* We never have to recursively delete directories, here.  When this is
	 needed in non-incremental cases, --recursive-unlink has surely been
	 set, and --unlink-first is then implied.  Consequently, all possible
	 recursive removal has already been done prior to any extraction
	 attempt.  It is not useful trying it again at recovery time.  */
      return remove_any_file (file_name, false);

    case ENOENT:
#if DOSWIN
    case EACCES:
      /* FILE_NAME may include illegal characters.  Try to replace them, but
	 if it fails again, punt.  */
      if (msdosify_count++ == 0)
	{
	  char *new_name = msdosify_file_name (file_name);
	  if (strcmp (new_name, file_name))
	    {
	      if (verbose_option)
		WARN ((0, 0, _("Renamed %s to %s"), file_name, new_name));
	      strcpy (file_name, new_name);
	      return 1;
	    }
	}
#endif
      /* Attempt creating missing intermediate directories.  */
      return make_directories (file_name);

    default:
      /* Just say we can't do anything about it...  */
      return false;
    }
}

/*---.
| ?  |
`---*/

static void
extract_sparse_file (int fd, off_t *sizeleft, off_t totalsize, char *name)
{
  union block *data_block;
  int sparse_ind = 0;
  size_t size_to_write;
  ssize_t size_written;

  /* FIXME: `data_block' might be used uninitialized in this function.
     Reported by Bruno Haible.  */

  /* assuming sizeleft is initially totalsize */

  while (*sizeleft > 0)
    {
      data_block = find_next_block ();
      if (data_block == NULL)
	{
	  ERROR ((0, 0, _("Unexpected end of file in archive")));
	  return;
	}
      lseek (fd, sparsearray[sparse_ind].offset, SEEK_SET);
      size_to_write = sparsearray[sparse_ind++].numbytes;
      while (size_to_write > BLOCKSIZE)
	{
	  size_written = full_write (fd, data_block->buffer, BLOCKSIZE);
	  if (size_written < 0)
	    ERROR ((0, errno, _("%s: Could not write to file"), name));
	  size_to_write -= size_written;
	  *sizeleft -= size_written;
	  set_next_block_after (data_block);
	  data_block = find_next_block ();
	}

      size_written = full_write (fd, data_block->buffer, size_to_write);

      if (size_written < 0)
	ERROR ((0, errno, _("%s: Could not write to file"), name));
      else if (size_written != size_to_write)
	{
	  ERROR ((0, 0, _("%s: Could only write %lu of %lu bytes"),
		  name, (unsigned long) size_written,
		  (unsigned long) totalsize));
	  skip_file (*sizeleft);
	}

      size_to_write -= size_written;
      *sizeleft -= size_written;
      set_next_block_after (data_block);
    }
  free (sparsearray);
  set_next_block_after (data_block);
}

#if DOSWIN

/*------------------------------------------------------------------------.
| Rename a file, for when we have a file in an archive whose name is a    |
| device on DOSWIN.  Trying to extract such a file would fail at best and |
| wedge us at worst.                                                      |
`------------------------------------------------------------------------*/

static char *
rename_if_dos_device_name (char *file_name)
{
  char *base;
  struct stat stat_info;
  char fname[PATH_MAX];
  int i = 0;

  extern char *basename (const char *);	/* FIXME! */

  strcpy (fname, file_name);
  base = basename (fname);
  while (stat (base, &stat_info) == 0 && S_ISCHR (stat_info.st_mode))
    {
      size_t blen = strlen (base);

      /* I don't believe any DOS character device names begin with a
	 `_'.  But in case they invent such a device, let us try twice.  */
      if (++i > 2)
	{
	  ERROR ((0, EACCES, _("%s: Could not create file"), file_name));
	  if (current_header->oldgnu_header.isextended)
	    skip_extended_headers ();
	  skip_file ((long) current_stat.st_size);
	  return file_name;
	}
      /* Prepend a '_'.  */
      memmove (base + 1, base, blen + 1);
      base[0] = '_';
    }
  if (i)
    {
      char *p = 0;

      if (verbose_option)
	WARN ((0, 0, _("Renamed %s to %s"), file_name, fname));
      assign_string (&p, fname);
      free (file_name);
      return p;
    }
  else
    return file_name;
}

#endif /* DOSWIN */

/*----------------------------------.
| Extract a file from the archive.  |
`----------------------------------*/

void
extract_archive (void)
{
  union block *data_block;
  int fd;
  int status;
  size_t name_length;
  size_t size_to_write;
  ssize_t size_written;
  int open_flags;
  long size;
  int skipcrud;
  int counter;
#if 0
  int sparse_ind = 0;
#endif
  union block *exhdr;
  struct delayed_set_stat *data;

#define CURRENT_FILE_NAME (skipcrud + current.name)

  set_next_block_after (current.block);
  decode_header (&current, true);

  if (interactive_option && !confirm (_("extract %s?"), current.name))
    {
      if (current.block->oldgnu_header.isextended)
	skip_extended_headers ();
      skip_file (current.stat.st_size);
      return;
    }

  /* Print the block from `current.block' and `current.stat'.  */

  if (verbose_option)
    print_header (&current);

  /* Check for fully specified file names and other atrocities.  */

  skipcrud = 0;

#if DOSWIN
  msdosify_count = 0;
  if (!to_stdout_option)
    current_file_name = rename_if_dos_device_name (current_file_name);

  /* We could have a filename like "9:30:45", and we don't want to treat it as
     if it were an absolute file name with a drive letter.  */
  if (!absolute_names_option
      && CURRENT_FILE_NAME[0] >= 'A' && CURRENT_FILE_NAME[0] <= 'z'
      && CURRENT_FILE_NAME[1] == ':' && CURRENT_FILE_NAME[2] == '/')
    skipcrud += 2;
#endif

  while (!absolute_names_option && CURRENT_FILE_NAME[0] == '/')
    {
      static bool warned_once = false;

      skipcrud++;		/* force relative path */
      if (!warned_once && !to_stdout_option)
	{
	  warned_once = true;
	  WARN ((0, 0, _("\
Removing leading `/' from absolute path names in the archive")));
	}
    }

  if (name_prefix_option)
    if (strncmp (name_prefix_option, CURRENT_FILE_NAME, name_prefix_length)
	== 0)
      skipcrud += name_prefix_length;
    else
      {
	if (current.block->oldgnu_header.isextended)
	  skip_extended_headers ();
	skip_file (current.stat.st_size);
	return;
      }

  /* Take a safety backup of a previously existing file.  */

  if (backup_option && !to_stdout_option)
    if (!maybe_backup_file (CURRENT_FILE_NAME, false))
      {
	ERROR ((0, errno, _("%s: Was unable to backup this file"),
		CURRENT_FILE_NAME));
	if (current.block->oldgnu_header.isextended)
	  skip_extended_headers ();
	skip_file (current.stat.st_size);
	return;
      }

  /* Extract the archive entry according to its type.  */

  switch (current.block->header.typeflag)
    {
      /* JK - What we want to do if the file is sparse is loop through
	 the array of sparse structures in the header and read in and
	 translate the character strings representing 1) the offset at
	 which to write and 2) how many bytes to write into numbers,
	 which we store into the scratch array, "sparsearray".  This
	 array makes our life easier the same way it did in creating the
	 tar file that had to deal with a sparse file.

	 After we read in the first five (at most) sparse structures, we
	 check to see if the file has an extended header, i.e., if more
	 sparse structures are needed to describe the contents of the new
	 file.  If so, we read in the extended headers and continue to
	 store their contents into the sparsearray.  */

    case OLDGNU_SPARSE:
      sp_array_size = 10;
      sparsearray = (struct sp_array *)
	xmalloc (sp_array_size * sizeof (struct sp_array));

      for (counter = 0; counter < SPARSES_IN_OLDGNU_HEADER; counter++)
	{
	  sparsearray[counter].offset =
	    get_initial_header_offset (current.block, counter);
	  sparsearray[counter].numbytes =
	    get_initial_header_numbytes (current.block, counter);

	  if (!sparsearray[counter].numbytes)
	    break;
	}

      if (current.block->oldgnu_header.isextended)
	{
	  /* Read in the list of extended headers and translate them into
	     the sparsearray as before.  */

	  /* static */ int ind = SPARSES_IN_OLDGNU_HEADER;

	  while (true)
	    {
	      exhdr = find_next_block ();
	      for (counter = 0; counter < SPARSES_IN_SPARSE_HEADER; counter++)
		{
		  if (counter + ind > sp_array_size - 1)
		    {
		      /* Realloc the scratch area since we've run out of
			 room.  */

		      sp_array_size *= 2;
		      sparsearray = (struct sp_array *)
			xrealloc (sparsearray,
				  sp_array_size * (sizeof (struct sp_array)));
		    }

		  /* Compare to 0, or use !(int) EXPR, for Pyramid's dumb
		     compiler.  */
		  if (exhdr->sparse_header.sp[counter].numbytes == 0)
		    break;

		  sparsearray[counter + ind].offset =
		    get_extended_header_offset (exhdr, counter);
		  sparsearray[counter + ind].numbytes =
		    get_extended_header_numbytes (exhdr, counter);
		}
	      if (!exhdr->sparse_header.isextended)
		break;
	      else
		{
		  ind += SPARSES_IN_SPARSE_HEADER;
		  set_next_block_after (exhdr);
		}
	    }
	  set_next_block_after (exhdr);
	}
      /* Fall through.  */

    case AREGTYPE:
    case REGTYPE:
    case CONTTYPE:

      /* Appears to be a file.  But BSD tar uses the convention that a slash
	 suffix means a directory.  */

      name_length = strlen (CURRENT_FILE_NAME) - 1;
      if (CURRENT_FILE_NAME[name_length] == '/')
	goto really_dir;

      /* FIXME: deal with protection issues.  */

    again_file:
      open_flags = (keep_old_files_option ?
		  O_BINARY | O_NDELAY | O_WRONLY | O_CREAT | O_EXCL :
		  O_BINARY | O_NDELAY | O_WRONLY | O_CREAT | O_TRUNC)
	| ((current.block->header.typeflag == OLDGNU_SPARSE) ? 0 : O_APPEND);

      /* JK - The last | is a kludge to solve the problem the O_APPEND
	 flag causes with files we are trying to make sparse: when a file
	 is opened with O_APPEND, it writes to the last place that
	 something was written, thereby ignoring any lseeks that we have
	 done.  We add this extra condition to make it able to lseek when
	 a file is sparse, i.e., we don't open the new file with this
	 flag.  (Grump -- this bug caused me to waste a good deal of
	 time, I might add)  */

      if (to_stdout_option)
	{
	  fd = 1;
#if DOSWIN
	  /* If stdout is a pipe or was redirected to a file, we should write
	     it in binary mode!  Otherwise, non-text files will end up
	     corrupted.  (But if they want binary garbage on their screen, let
	     them have fun, since switching the console to binary mode has
	     some really nasty side-effects).  */

	  if (!isatty (1))
	    setmode (1, O_BINARY);
#endif
	  goto extract_file;
	}

      if (unlink_first_option)
	remove_any_file (CURRENT_FILE_NAME, recursive_unlink_option);

#if O_CTG
      /* Contiguous files (on the Masscomp) have to specify the size in
	 the open call that creates them.  */

      if (current.block->header.typeflag == CONTTYPE)
	fd = open (CURRENT_FILE_NAME, open_flags | O_CTG,
		   current.stat.st_mode, current.stat.st_size);
      else
	fd = open (CURRENT_FILE_NAME, open_flags, current.stat.st_mode);

#else /* not O_CTG */
      if (current.block->header.typeflag == CONTTYPE)
	{
	  static bool conttype_diagnosed = false;

	  if (!conttype_diagnosed)
	    {
	      conttype_diagnosed = true;
	      WARN ((0, 0, _("Extracting contiguous files as regular files")));
	    }
	}
      fd = open (CURRENT_FILE_NAME, open_flags, current.stat.st_mode);

#endif /* not O_CTG */

      if (fd < 0)
	{
	  if (maybe_recoverable (CURRENT_FILE_NAME))
	    goto again_file;

	  ERROR ((0, errno, _("%s: Could not create file"),
		  CURRENT_FILE_NAME));
	  if (current.block->oldgnu_header.isextended)
	    skip_extended_headers ();
	  skip_file (current.stat.st_size);
	  if (backup_option)
	    undo_last_backup ();
	  break;
	}

    extract_file:
      if (current.block->header.typeflag == OLDGNU_SPARSE)
	{
	  size_t name_length_bis = strlen (CURRENT_FILE_NAME) + 1;
	  char *name = (char *) xmalloc (name_length_bis);

	  /* Kludge alert.  NAME is assigned to header.name because
	     during the extraction, the space that contains the header
	     will get scribbled on, and the name will get munged, so any
	     error messages that happen to contain the filename will look
	     REAL interesting unless we do this.  */

	  memcpy (name, CURRENT_FILE_NAME, name_length_bis);
	  size = current.stat.st_size;
	  extract_sparse_file (fd, &size, current.stat.st_size, name);
	}
      else
	for (size = current.stat.st_size; size > 0; size -= size_to_write)
	  {
#if 0
	    long offset, numbytes;
#endif
	    if (multi_volume_option)
	      {
		assign_string (&save_name, current.name);
		save_totsize = current.stat.st_size;
		save_sizeleft = size;
	      }

	    /* Locate data, determine max length writeable, write it,
	       block that we have used the data, then check if the write
	       worked.  */

	    data_block = find_next_block ();
	    if (data_block == NULL)
	      {
		ERROR ((0, 0, _("Unexpected end of file in archive")));
		break;		/* FIXME: What happens, then?  */
	      }

	    /* If the file is sparse, use the sparsearray that we created
	       before to lseek into the new file the proper amount, and to
	       see how many bytes we want to write at that position.  */

#if 0
	    if (current.block->header.typeflag == OLDGNU_SPARSE)
	      {
		off_t pos;

		pos = lseek (fd, (off_t) sparsearray[sparse_ind].offset, SEEK_SET);
		fprintf (msg_file, _("%d at %d\n"), (int) pos, sparse_ind);
		size_to_write = sparsearray[sparse_ind++].numbytes;
	      }
	    else
#endif
	      size_to_write = available_space_after (data_block);

	    if (size_to_write > size)
	      size_to_write = size;
	    errno = 0;		/* FIXME: errno should be read-only */
	    size_written = full_write (fd, data_block->buffer, size_to_write);

	    set_next_block_after ((union block *)
				  (data_block->buffer + size_to_write - 1));
	    if (size_written == size_to_write)
	      continue;

	    /* Error in writing to file.  Print it, skip to next file in
	       archive.  */

	    if (size_written < 0)
	      ERROR ((0, errno, _("%s: Could not write to file"),
		      CURRENT_FILE_NAME));
	    else
	      ERROR ((0, 0, _("%s: Could only write %lu of %lu bytes"),
		      CURRENT_FILE_NAME,
		      (unsigned long) size_written,
		      (unsigned long) size_to_write));
	    skip_file ((off_t) (size - size_to_write));
	    break;		/* still do the close, mod time, chmod, etc */
	  }

      if (multi_volume_option)
	assign_string (&save_name, NULL);

      /* If writing to stdout, don't try to do anything to the filename;
	 it doesn't exist, or we don't want to touch it anyway.  */

      if (to_stdout_option)
	break;

#if 0
      if (current.block->header.isextended)
	{
	  union block *exhdr;
	  int counter;

	  for (counter = 0; counter < SPARSES_IN_SPARSE_HEADER; counter++)
	    {
	      off_t offset;

	      if (!exhdr->sparse_header.sp[counter].numbytes)
		break;
	      offset = get_extended_header_offset (exhdr, counter);
	      size_to_write = get_extended_header_numbytes (exhdr, counter);
	      lseek (fd, offset, SEEK_SET);
	      size_written
		= full_write (fd, data_block->buffer, size_to_write);
	      if (size_written == size_to_write)
		continue;
	    }
	}
#endif
      status = close (fd);
      if (status < 0)
	{
	  ERROR ((0, errno, _("%s: Error while closing"), CURRENT_FILE_NAME));
	  if (backup_option)
	    undo_last_backup ();
	}

      set_stat (CURRENT_FILE_NAME, &current.stat, false);
      break;

    case SYMTYPE:
      if (to_stdout_option)
	break;

#ifdef S_ISLNK
      if (unlink_first_option)
	remove_any_file (CURRENT_FILE_NAME, recursive_unlink_option);

      while (status = symlink (current.linkname, CURRENT_FILE_NAME),
	     status != 0)
	if (!maybe_recoverable (CURRENT_FILE_NAME))
	  break;

      if (status == 0)

	/* Setting the attributes of symbolic links might, on some systems,
	   change the pointed to file, instead of the symbolic link itself.
	   At least some of these systems have a lchown call, and the
	   set_stat routine knows about this.    */

	set_stat (CURRENT_FILE_NAME, &current.stat, true);

      else
	{
	  ERROR ((0, errno, _("%s: Could not create symlink to `%s'"),
		  CURRENT_FILE_NAME, current.linkname));
	  if (backup_option)
	    undo_last_backup ();
	}
      break;

#else /* not S_ISLNK */
      {
	static bool warned_once = false;

	if (!warned_once)
	  {
	    warned_once = true;
	    WARN ((0, 0, _("\
Attempting extraction of symbolic links as hard links")));
	  }
      }
      /* Fall through.  */

#endif /* not S_ISLNK */

    case LNKTYPE:
      if (to_stdout_option)
	break;

      if (unlink_first_option)
	remove_any_file (CURRENT_FILE_NAME, recursive_unlink_option);

    again_link:
      {
	struct stat stat_info_1, stat_info_2;

#if DOSWIN
	/* Rename current_link_name if it might get us in trouble.  */
	current_link_name = rename_if_dos_device_name (current_link_name);
#endif

	/* DOSWIN does not implement links.  However, link() actually copies
	   the file.  */
	status = link (current.linkname, CURRENT_FILE_NAME);

	if (status == 0)
	  break;
	if (maybe_recoverable (CURRENT_FILE_NAME))
	  goto again_link;

	if (incremental_option && errno == EEXIST)
	  break;
	if (stat (current.linkname, &stat_info_1) == 0
	    && stat (CURRENT_FILE_NAME, &stat_info_2) == 0
	    && stat_info_1.st_dev == stat_info_2.st_dev
#if !DOSWIN
	    /* Both files already exist.  On DOSWIN, they have different
	       inodes numbers.  So just don't check inodes on DOSWIN.  */
	    && stat_info_1.st_ino == stat_info_2.st_ino
#endif
	    )
	  break;

	ERROR ((0, errno, _("%s: Could not link to `%s'"),
		CURRENT_FILE_NAME, current.linkname));
	if (backup_option)
	  undo_last_backup ();
      }
      break;

#if S_IFCHR
    case CHRTYPE:
      current.stat.st_mode |= S_IFCHR;
      goto make_node;
#endif

#if S_IFBLK
    case BLKTYPE:
      current.stat.st_mode |= S_IFBLK;
#endif

#if defined(S_IFCHR) || defined(S_IFBLK)
    make_node:
      if (to_stdout_option)
	break;

      if (unlink_first_option)
	remove_any_file (CURRENT_FILE_NAME, recursive_unlink_option);

      status = mknod (CURRENT_FILE_NAME, current.stat.st_mode,
		      current.stat.st_rdev);
      if (status != 0)
	{
	  if (maybe_recoverable (CURRENT_FILE_NAME))
	    goto make_node;

	  ERROR ((0, errno, _("%s: Could not make node"), CURRENT_FILE_NAME));
	  if (backup_option)
	    undo_last_backup ();
	  break;
	};
      set_stat (CURRENT_FILE_NAME, &current.stat, false);
      break;
#endif

#ifdef S_ISFIFO
    case FIFOTYPE:
      if (to_stdout_option)
	break;

      if (unlink_first_option)
	remove_any_file (CURRENT_FILE_NAME, recursive_unlink_option);

      while (status = mkfifo (CURRENT_FILE_NAME, current.stat.st_mode),
	     status != 0)
	if (!maybe_recoverable (CURRENT_FILE_NAME))
	  break;

      if (status == 0)
	set_stat (CURRENT_FILE_NAME, &current.stat, false);
      else
	{
	  ERROR ((0, errno, _("%s: Could not make fifo"), CURRENT_FILE_NAME));
	  if (backup_option)
	    undo_last_backup ();
	}
      break;
#endif

    case DIRTYPE:
    case OLDGNU_DUMPDIR:
      name_length = strlen (CURRENT_FILE_NAME) - 1;

    really_dir:
#if DOSWIN
      /* Under -P, we could have "d:/".  Leave that trailing slash alone.  */
      if (skipcrud != 0 || name_length != 2 || CURRENT_FILE_NAME[1] != ':')
#endif
	/* Check for trailing /, and zap as many as we find.  */
	while (name_length && CURRENT_FILE_NAME[name_length] == '/')
	  CURRENT_FILE_NAME[name_length--] = '\0';

      if (incremental_option)
	{
	  /* Read the entry and delete files that aren't listed in the
	     archive.  */

	  incremental_restore (skipcrud);
	}
      else if (current.block->header.typeflag == OLDGNU_DUMPDIR)
	skip_file (current.stat.st_size);

      if (to_stdout_option)
	break;

      if (unlink_first_option)
	remove_any_file (CURRENT_FILE_NAME, recursive_unlink_option);

    again_dir:
      status = mkdir (CURRENT_FILE_NAME,
		     (we_are_root ? 0 : 0300) | current.stat.st_mode);
      if (status != 0)
	{
	  /* If the directory creation fails, let's consider immediately the
	     case where the directory already exists.  We have three good
	     reasons for clearing out this case before attempting recovery.

	     1) It would not be efficient recovering the error by deleting
	     the directory in maybe_recoverable, then recreating it right
	     away.  We only hope we will be able to adjust its permissions
	     adequately, later.

	     2) Removing the directory might fail if it is not empty.  By
	     exception, this real error is traditionally not reported.

	     3) Let's suppose `DIR' already exists and we are about to
	     extract `DIR/../DIR'.  This would fail because the directory
	     already exists, and maybe_recoverable would react by removing
	     `DIR'.  This then would fail again because there are missing
	     intermediate directories, and maybe_recoverable would react by
	     creating `DIR'.  We would then have an extraction loop.  */

	  if (errno == EEXIST)
	    {
	      struct stat stat_info;
	      int saved_errno = errno;

	      if (stat (CURRENT_FILE_NAME, &stat_info) == 0
		  && S_ISDIR (stat_info.st_mode))
		goto check_perms;

	      errno = saved_errno; /* FIXME: errno should be read-only */
	    }

#if DOSWIN
	  /* The ubiquitous DOS "Access denied" lossage.  If that disaster
	     strikes us, and the directory already exists, avoid printing a
	     message and/or failing.  */

	  if (errno == EACCES)
	    {
	      struct stat stat_info;
	      int saved_errno = errno;

	      if (stat (CURRENT_FILE_NAME, &stat_info) == 0
		  && S_ISDIR (stat_info.st_mode))
		break;

	      errno = saved_errno; /* FIXME: errno should be read-only */
	    }
#endif

	  if (maybe_recoverable (CURRENT_FILE_NAME))
	    goto again_dir;

	  /* If we're trying to create '.', let it be.  */

	  /* FIXME: Strange style...  */

	  if (CURRENT_FILE_NAME[name_length] == '.'
	      && (name_length == 0
		  || CURRENT_FILE_NAME[name_length - 1] == '/'))
	    goto check_perms;

	  ERROR ((0, errno, _("%s: Could not create directory"),
		  CURRENT_FILE_NAME));
	  if (backup_option)
	    undo_last_backup ();
	  break;
	}

    check_perms:
      if (!we_are_root && 0300 != (0300 & current.stat.st_mode))
	{
	  current.stat.st_mode |= 0300;
	  WARN ((0, 0, _("Added write and execute permission to directory %s"),
		 CURRENT_FILE_NAME));
	}

#if !DOSWIN
      /* DOSWIN does not allow to change timestamps of directories.
         In this case, no need to try delaying their restoration.  */

      if (touch_option)

	/* FIXME: I do not believe in this.  Ignoring time stamps does not
	   alleviate the need of delaying the restoration of directories'
	   mode.  Let's ponder this for a little while.  */

	set_mode (CURRENT_FILE_NAME, &current.stat);

      else
	{
	  data = ((struct delayed_set_stat *)
		      xmalloc (sizeof (struct delayed_set_stat)));
	  data->file_name = xstrdup (CURRENT_FILE_NAME);
	  data->stat_info = current.stat;
	  data->next = delayed_set_stat_head;
	  delayed_set_stat_head = data;
	}
#endif /* !DOSWIN */
      break;

    case OLDGNU_VOLHDR:
      if (verbose_option)
	{
	  if (checkpoint_option)
	    flush_checkpoint_line ();
	  fprintf (stdlis, _("Reading %s\n"), current.name);
	}
      break;

    case OLDGNU_NAMES:
      extract_mangle ();
      break;

    case OLDGNU_MULTIVOL:
      ERROR ((0, 0, _("\
Cannot extract `%s' -- file is continued from another volume"),
	      current.name));
      skip_file (current.stat.st_size);
      if (backup_option)
	undo_last_backup ();
      break;

    case OLDGNU_LONGNAME:
    case OLDGNU_LONGLINK:
      ERROR ((0, 0, _("Long name has no introducing header")));
      skip_file (current.stat.st_size);
      if (backup_option)
	undo_last_backup ();
      break;

    default:
      WARN ((0, 0,
	     _("Unknown file type `%c' for %s, extracted as normal file"),
	     current.block->header.typeflag, CURRENT_FILE_NAME));
      goto again_file;
    }

#undef CURRENT_FILE_NAME
}

/*----------------------------------------------------------------.
| Set back the utime and mode for all the extracted directories.  |
`----------------------------------------------------------------*/

void
apply_delayed_set_stat (void)
{
  struct delayed_set_stat *data;

  while (delayed_set_stat_head != NULL)
    {
      data = delayed_set_stat_head;
      delayed_set_stat_head = delayed_set_stat_head->next;
      set_stat (data->file_name, &data->stat_info, false);
      free (data->file_name);
      free (data);
    }
}
