/* copypass.c - cpio copy pass sub-function.
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
#include "filetypes.h"
#include "cpiohdr.h"
#include "extern.h"

static void copy_pass_one_file PARAMS ((dynamic_string *, dynamic_string *,
					struct stat *));

/*-------------------------------------------------.
| Copy one file, dealing with all printing flags.  |
`-------------------------------------------------*/

static void
verbosely_copy_file (dynamic_string *input_name, dynamic_string *output_name,
		     struct stat *stat_info)
{
  /* FIXME: supposedly pax wants to print the file name sans \n before
     processing, and add the \n after processing.  Verify that this is true.
     */
  if (verbose_flag)
    fprintf (stderr, "%s\n", input_name->string);

  copy_pass_one_file (input_name, output_name, stat_info);

  /* Doesn't really make sense to have dot_flag and verbose_flag set at the
     same time.  Must check for this.  */
#if 0
  assert (!dot_flag || !verbose_flag);
#endif

  if (dot_flag)
    fputc ('.', stderr);
}

/*--------------------------------------------------------------------------.
| Copy one file and maybe recurse.  TOP_LEVEL is true if at top level.      |
| INPUT_NAME is the input file name.  OUTPUT_NAME is the output file name.  |
| DIRNAME_LEN is the length of output dir name.  DIRECTORY_NAME is the      |
| output directory.                                                         |
`--------------------------------------------------------------------------*/

static void
copy_pass_maybe_recurse (int top_level, dynamic_string *input_name,
			 dynamic_string *output_name, int dirname_len,
			 char *directory_name)
{
  struct stat stat_info;	/* Stat record for input file.  */

  /* Check for blank line and ignore it if found.  */
  if (input_name->string[0] == '\0')
    {
      error (0, 0, _("blank line ignored"));
      return;
    }

  /* We must stat the file first, because the stat structure must be filled in
     for the recursion case.  */
  if ((*xstat) (input_name->string, &stat_info) < 0)
    {
      error (0, errno, "%s", input_name->string);
      return;
    }

  if (top_level)
    current_device = stat_info.st_dev;

  /* Check for current directory and ignore it if found.  */
  /* FIXME do this after renaming?  */
  if (! (input_name->string[0] == '.'
	 && (input_name->string[1] == '\0'
	     || (input_name->string[1] == '/'
		 && input_name->string[2] == '\0'))))
    {
      char *new_file;		/* Filename after renaming.  */
      char *slash;		/* For moving past slashes in input name.  */

      /* If we aren't allowed to cross devices, and this file does, then
	 skip it.  */
      if (!cross_device_flag && stat_info.st_dev != current_device)
	return;

      /* Rename file, if required.  Also, consider skipping file.  */
      new_file = possibly_rename_file (input_name->string);
      if (new_file == NULL)
	return;

      /* Make the name of the new file.  */
      for (slash = new_file; *slash == '/'; ++slash)
	;

#ifdef HPUX_CDF
      /* For CDF's we add a 2nd `/' after all "hidden" directories.
	 This kind of a kludge, but it's what we do when creating
	 archives, and it's easier to do this than to separately keep
	 track of which directories in a path are "hidden".  */
      slash = add_cdf_double_slashes (slash);
#endif

      ds_resize (output_name, dirname_len + strlen (slash) + 2);
      strcpy (output_name->string + dirname_len + 1, slash);

      verbosely_copy_file (input_name, output_name, &stat_info);

      free (new_file);
    }

  /* Maybe recurse.  */
  if (directory_recurse_flag && S_ISDIR (stat_info.st_mode))
    {
      DIR *dirp;

      dirp = opendir (input_name->string);
      if (!dirp)
	error (0, errno, _("cannot open directory %s"), input_name->string);
      else
	{
	  int len;
	  struct dirent *d;

	  len = ds_strlen (input_name);
	  /* Big enough for directory, plus most files.  FIXME wish I
	     could remember the actual statistic about typical
	     filename lengths.  I recall reading that whoever
	     developed the fast file system did such a study.  */
	  ds_resize (input_name, len + 32);
	  input_name->string[len] = '/';

	  while ((d = readdir (dirp)))
	    {
	      if (is_dot_or_dotdot (d->d_name))
		continue;

	      ds_resize (input_name, NAMLEN (d) + len + 2);
	      strncpy (input_name->string + len + 1, d->d_name, NAMLEN (d));
	      input_name->string[len + NAMLEN (d) + 1] = '\0';

	      copy_pass_maybe_recurse (0, input_name, output_name,
				       dirname_len, directory_name);
	    }

	  closedir (dirp);
	}
    }
}

/*-------------------------------------------------------------.
| Read list of file names and copy each file into directory    |
| `directory_name'.  If `link_flag', link instead of copying.  |
`-------------------------------------------------------------*/

void
process_copy_pass (void)
{
  int dirname_len;		/* Length of `directory_name'.  */
  dynamic_string input_name;	/* Name of input file.  */
  dynamic_string output_name;	/* Name of new file.  */

  /* Initialize the copy pass.  */
  dirname_len = strlen (directory_name);
  ds_init (&input_name, 128);
  ds_init (&output_name, dirname_len + 2);
  strcpy (output_name.string, directory_name);
  output_name.string[dirname_len] = '/';
  output_is_seekable = true;

  while (get_next_file_name (&input_name))
    copy_pass_maybe_recurse (1, &input_name, &output_name,
			     dirname_len, directory_name);

  if (dot_flag)
    fputc ('\n', stderr);

  if (! no_block_message_flag)
    {
      int res = (output_bytes + io_block_size - 1) / io_block_size;
      if (res == 1)
	fprintf (stderr, _("1 block\n"));
      else
	fprintf (stderr, _("%d blocks\n"), res);
    }
}

/*--------------------------------------------------------------.
| Copy or link one file, given its name and other information.  |
`--------------------------------------------------------------*/

static void
copy_pass_one_file (dynamic_string *input_name, dynamic_string *output_name,
		    struct stat *stat_info)
{
  int res;			/* result of functions */
  struct utimbuf times;		/* for resetting file times after copy */
  struct stat out_file_stat;	/* stat record for output file */
  int in_file_des;		/* input file descriptor */
  int out_file_des;		/* output file descriptor */
  int existing_dir;		/* true if file is a dir & already exists */
#ifdef HPUX_CDF
  int cdf_flag;
  int cdf_char;
#endif
  int link_res = -1;

  /* Initialize this in case it has members we don't know to set.  */
  memset (&times, 0, sizeof (struct utimbuf));

  existing_dir = false;
  if (lstat (output_name->string, &out_file_stat) == 0)
    {
      if (S_ISDIR (out_file_stat.st_mode)
	  && S_ISDIR (stat_info->st_mode))
	{
	  /* If there is already a directory there that
	     we are trying to create, don't complain about it.  */
	  existing_dir = true;
	}
      else if (!unconditional_flag
	       && stat_info->st_mtime <= out_file_stat.st_mtime)
	{
	  error (0, 0,
		 _("%s not created: newer or same age version exists"),
		 output_name->string);
	  return;		/* Go to the next file.  */
	}
      else if (!overwrite_existing_files)
	{
	  /* Can't overwrite, so just skip.  */
	  error (0, 0, _("%s already exists; not created"));
	  return;		/* Go to the next file.  */
	}
      else if (S_ISDIR (out_file_stat.st_mode)
	       ? rmdir (output_name->string)
	       : unlink (output_name->string))
	{
	  error (0, errno, _("cannot remove current %s"),
		 output_name->string);
	  return;		/* Go to the next file.  */
	}
    }

  /* Do the real copy or link.  */
  if (S_ISREG (stat_info->st_mode))
    {
#ifndef __MSDOS__
      /* Can the current file be linked to a another file?
	 Set link_name to the original file name.  */
      if (link_flag)
	/* User said to link it if possible.  Try and link to
	   the original copy.  If that fails we'll still try
	   and link to a copy we've already made.  */
	link_res = link_to_name (output_name->string,
				 input_name->string);
      if ( (link_res < 0) && (stat_info->st_nlink > 1) )
	link_res = link_to_maj_min_ino (output_name->string,
					major (stat_info->st_dev),
					minor (stat_info->st_dev),
					stat_info->st_ino);
#endif

      /* If the file was not linked, copy contents of file.  */
      if (link_res < 0)
	{
	  in_file_des = open (input_name->string,
			      O_RDONLY | O_BINARY, 0);
	  if (in_file_des < 0)
	    {
	      error (0, errno, "%s", input_name->string);
	      return;
	    }
	  out_file_des = open (output_name->string,
			       O_CREAT | O_WRONLY | O_BINARY, 0600);
	  if (out_file_des < 0 && create_dir_flag)
	    {
	      create_all_directories (output_name->string);
	      out_file_des = open (output_name->string,
				   O_CREAT | O_WRONLY | O_BINARY, 0600);
	    }
	  if (out_file_des < 0)
	    {
	      error (0, errno, "%s", output_name->string);
	      close (in_file_des);
	      return;
	    }

	  copy_files_disk_to_disk (in_file_des, out_file_des,
				   stat_info->st_size,
				   input_name->string);
	  disk_empty_output_buffer (out_file_des);
	  if (close (in_file_des) < 0)
	    error (0, errno, "%s", input_name->string);
	  if (close (out_file_des) < 0)
	    error (0, errno, "%s", output_name->string);

	  /* Set the attributes of the new file.  */
	  if (!no_chown_flag)
	    if ((chown (output_name->string,
			set_owner_flag ? set_owner : stat_info->st_uid,
			set_group_flag ? set_group : stat_info->st_gid) < 0)
		&& errno != EPERM)
	      error (0, errno, "%s", output_name->string);
	  /* chown may have turned off some permissions we wanted. */
	  if (chmod (output_name->string, stat_info->st_mode) < 0)
	    error (0, errno, "%s", output_name->string);
	  if (reset_time_flag)
	    {
	      times.actime = stat_info->st_atime;
	      times.modtime = stat_info->st_mtime;
	      if (utime (input_name->string, &times) < 0)
		error (0, errno, _("%s: error resetting file access time"),
		       input_name->string);
	      if (utime (output_name->string, &times) < 0)
		error (0, errno, _("%s: error resetting file access time"),
		       output_name->string);
	    }
	  if (retain_time_flag)
	    {
	      times.actime = times.modtime = stat_info->st_mtime;
	      if (utime (output_name->string, &times) < 0)
		error (0, errno, "%s", output_name->string);
	    }
	}
    }
  else if (S_ISDIR (stat_info->st_mode))
    {
#ifdef HPUX_CDF
      cdf_flag = 0;
#endif
      if (!existing_dir)
	{
#ifdef HPUX_CDF
	  /* If the directory name ends in a + and is SUID,
	     then it is a CDF.  Strip the trailing + from the name
	     before creating it.  */
	  cdf_char = strlen (output_name->string) - 1;
	  if ( (cdf_char > 0) &&
	       (stat_info->st_mode & 04000) &&
	       (output_name->string [cdf_char] == '+') )
	    {
	      output_name->string [cdf_char] = '\0';
	      cdf_flag = 1;
	    }
#endif
	  res = mkdir (output_name->string, stat_info->st_mode);

	}
      else
	res = 0;
      if (res < 0 && create_dir_flag)
	{
	  create_all_directories (output_name->string);
	  res = mkdir (output_name->string, stat_info->st_mode);
	}
      if (res < 0)
	{
	  /* In some odd cases where the output_name includes `.',
	     the directory may have actually been created by
	     create_all_directories(), so the mkdir will fail
	     because the directory exists.  If that's the case,
	     don't complain about it.  */
	  if ( (errno != EEXIST) ||
	       (lstat (output_name->string, &out_file_stat) != 0) ||
	       !(S_ISDIR (out_file_stat.st_mode) ) )
	    {
	      error (0, errno, "%s", output_name->string);
	      return;
	    }
	}
      if (!no_chown_flag)
	if ((chown (output_name->string,
		    set_owner_flag ? set_owner : stat_info->st_uid,
		    set_group_flag ? set_group : stat_info->st_gid) < 0)
	    && errno != EPERM)
	  error (0, errno, "%s", output_name->string);
      /* chown may have turned off some permissions we wanted. */
      if (chmod (output_name->string, stat_info->st_mode) < 0)
	error (0, errno, "%s", output_name->string);
#ifdef HPUX_CDF
      if (cdf_flag)
	/* Once we "hide" the directory with the chmod(),
	   we have to refer to it using name+ instead of name.  */
	output_name->string [cdf_char] = '+';
#endif
      if (retain_time_flag)
	{
	  times.actime = times.modtime = stat_info->st_mtime;
	  if (utime (output_name->string, &times) < 0)
	    error (0, errno, "%s", output_name->string);
	}
    }
#ifndef __MSDOS__
  else if (S_ISCHR (stat_info->st_mode) ||
	   S_ISBLK (stat_info->st_mode) ||
#ifdef S_ISFIFO
	   S_ISFIFO (stat_info->st_mode) ||
#endif
#ifdef S_ISSOCK
	   S_ISSOCK (stat_info->st_mode) ||
#endif
	   0)
    {
      /* Can the current file be linked to a another file?
	 Set link_name to the original file name.  */
      if (link_flag)
	/* User said to link it if possible.  */
	link_res = link_to_name (output_name->string,
				 input_name->string);
      if ( (link_res < 0) && (stat_info->st_nlink > 1) )
	link_res = link_to_maj_min_ino (output_name->string,
					major (stat_info->st_dev),
					minor (stat_info->st_dev),
					stat_info->st_ino);

      if (link_res < 0)
	{
	  res = mknod (output_name->string, stat_info->st_mode,
		       stat_info->st_rdev);
	  if (res < 0 && create_dir_flag)
	    {
	      create_all_directories (output_name->string);
	      res = mknod (output_name->string, stat_info->st_mode,
			   stat_info->st_rdev);
	    }
	  if (res < 0)
	    {
	      error (0, errno, "%s", output_name->string);
	      return;
	    }
	  if (!no_chown_flag)
	    if ((chown (output_name->string,
			set_owner_flag ? set_owner : stat_info->st_uid,
			set_group_flag ? set_group : stat_info->st_gid) < 0)
		&& errno != EPERM)
	      error (0, errno, "%s", output_name->string);
	  /* chown may have turned off some permissions we wanted. */
	  if (chmod (output_name->string, stat_info->st_mode) < 0)
	    error (0, errno, "%s", output_name->string);
	  if (retain_time_flag)
	    {
	      times.actime = times.modtime = stat_info->st_mtime;
	      if (utime (output_name->string, &times) < 0)
		error (0, errno, "%s", output_name->string);
	    }
	}
    }
#endif

#ifdef S_ISLNK
  else if (S_ISLNK (stat_info->st_mode))
    {
      char *link_name;
      int link_size;
      link_name = (char *) xmalloc ((unsigned int) stat_info->st_size + 1);

      link_size = readlink (input_name->string, link_name,
			    stat_info->st_size);
      if (link_size < 0)
	{
	  error (0, errno, "%s", input_name->string);
	  free (link_name);
	  return;
	}
      link_name[link_size] = '\0';

      res = UMASKED_SYMLINK (link_name, output_name->string,
			     stat_info->st_mode);
      if (res < 0 && create_dir_flag)
	{
	  create_all_directories (output_name->string);
	  res = UMASKED_SYMLINK (link_name, output_name->string,
				 stat_info->st_mode);
	}
      if (res < 0)
	{
	  error (0, errno, "%s", output_name->string);
	  free (link_name);
	  return;
	}

#ifdef HAVE_LCHOWN
      /* Set the attributes of the new link.  */
      if (!no_chown_flag)
	if ((lchown (output_name->string,
		     set_owner_flag ? set_owner : stat_info->st_uid,
		     set_group_flag ? set_group : stat_info->st_gid) < 0)
	    && errno != EPERM)
	  error (0, errno, "%s", output_name->string);
#endif /* HAVE_LCHOWN */
      free (link_name);
    }
#endif /* S_ISLNK */
  else
    {
      error (0, 0, _("%s: unknown file type"), input_name->string);
    }
}

/* Try and create a hard link from FILE_NAME to another file
   with the given major/minor device number and inode.  If no other
   file with the same major/minor/inode numbers is known, add this file
   to the list of known files and associated major/minor/inode numbers
   and return -1.  If another file with the same major/minor/inode
   numbers is found, try and create another link to it using
   link_to_name, and return 0 for success and -1 for failure.  */

int
link_to_maj_min_ino (file_name, st_dev_maj, st_dev_min, st_ino)
  char *file_name;
  int st_dev_maj;
  int st_dev_min;
  int st_ino;
{
  int	link_res;
  char *link_name;
  link_res = -1;
#ifndef __MSDOS__
  /* Is the file a link to a previously copied file?  */
  link_name = find_inode_file (st_ino,
			       st_dev_maj,
			       st_dev_min);
  if (link_name == NULL)
    add_inode (st_ino, file_name,
	       st_dev_maj,
	       st_dev_min);
  else
    link_res = link_to_name (file_name, link_name);
#endif
  return link_res;
}

/* Try and create a hard link from LINK_NAME to LINK_TARGET.  If
   `create_dir_flag' is set, any non-existent (parent) directories
   needed by LINK_NAME will be created.  If the link is successfully
   created and `verbose_flag' is set, print "LINK_TARGET linked to LINK_NAME\n".
   If the link can not be created and `link_flag' is set, print
   "cannot link LINK_TARGET to LINK_NAME\n".  Return 0 if the link
   is created, -1 otherwise.  */

int
link_to_name (link_name, link_target)
  char *link_name;
  char *link_target;
{
  int res;
#ifdef __MSDOS__
  res = -1;
#else /* not __MSDOS__ */
  res = link (link_target, link_name);
  if (res < 0 && create_dir_flag)
    {
      create_all_directories (link_name);
      res = link (link_target, link_name);
    }
  if (res == 0)
    {
      if (verbose_flag)
	error (0, 0, _("%s linked to %s"),
	       link_target, link_name);
    }
  else if (link_flag)
    {
      error (0, errno, _("cannot link %s to %s"),
	     link_target, link_name);
    }
#endif /* not __MSDOS__ */
  return res;
}
