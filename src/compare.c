/* Diff files from a tar archive.
   Copyright (C) 1988, 92, 93, 94, 96, 97, 98, 99 Free Software Foundation, Inc.
   Written by John Gilmore, on 1987-04-30.

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

#include <signal.h>
#if HAVE_LINUX_FD_H
# include <linux/fd.h>
#endif

#include "common.h"
#include "rmt.h"

/* Spare space for messages, hopefully safe even after gettext.  */
#define MESSAGE_BUFFER_SIZE 100

/* True if we are verifying at the moment.  */
bool now_verifying = false;

/* File descriptor for the file we are diffing.  */
static int diff_handle;

/* Area for reading file contents into.  */
static char *diff_buffer = NULL;

/*--------------------------------.
| Initialize for a diff operation |
`--------------------------------*/

void
diff_init (void)
{
  diff_buffer = (char *) valloc (record_size);
  if (!diff_buffer)
    FATAL_ERROR ((0, 0,
		  _("Could not allocate memory for diff buffer of %lu bytes"),
		  record_size));
}

/*------------------------------------------------------------------------.
| Sigh about something that differs by writing a MESSAGE to stdlis, given |
| MESSAGE is not NULL.  Also set the exit status if not already.          |
`------------------------------------------------------------------------*/

static void
report_difference (const char *message)
{
  if (message)
    {
      if (checkpoint_option)
	flush_checkpoint_line ();
      fprintf (stdlis, "%s: %s\n", current.name, message);
    }

  if (exit_status == TAREXIT_SUCCESS)
    exit_status = TAREXIT_DIFFERS;
}

/*-----------------------------------------------------------------------.
| Takes a buffer returned by read_and_process and does nothing with it.	 |
`-----------------------------------------------------------------------*/

/* Yes, I know.  SIZE and DATA are unused in this function.  Some compilers
   may even report it.  If you see how to silence them in reasonable ways,
   with no execution overhead, please tell me.  */

static bool
process_noop (size_t size, char *data)
{
  return true;
}

/*---.
| ?  |
`---*/

static bool
process_rawdata (size_t size, char *buffer)
{
  ssize_t read_size = full_read (diff_handle, diff_buffer, size);
  char message[MESSAGE_BUFFER_SIZE];

  if (read_size != size)
    {
      if (read_size < 0)
	{
	  WARN ((0, errno, _("Cannot read %s"), current.name));
	  report_difference (NULL);
	}
      else
	{
	  sprintf (message, _("Could only read %lu of %lu bytes"),
		   (unsigned long) read_size, (unsigned long) size);
	  report_difference (message);
	}
      return false;
    }

  if (memcmp (buffer, diff_buffer, size))
    {
      report_difference (_("Data differs"));
      return false;
    }

  return true;
}

/*---.
| ?  |
`---*/

/* Directory contents, only for OLDGNU_DUMPDIR.  */

static char *dumpdir_cursor;

static bool
process_dumpdir (size_t size, char *buffer)
{
  if (memcmp (buffer, dumpdir_cursor, size))
    {
      report_difference (_("Data differs"));
      return false;
    }

  dumpdir_cursor += size;
  return true;
}

/*------------------------------------------------------------------------.
| Some other routine wants SIZE bytes in the archive.  For each chunk of  |
| the archive, call PROCESSOR with the size of the chunk, and the address |
| of the chunk it can work with.  The PROCESSOR should return true for    |
| success.  It it return error once, continue skipping without calling    |
| PROCESSOR anymore.                                                      |
`------------------------------------------------------------------------*/

static void
read_and_process (size_t size, bool (*processor) (size_t, char *))
{
  union block *data_block;
  size_t data_size;

  if (multi_volume_option)
    save_sizeleft = size;
  while (size)
    {
      data_block = find_next_block ();
      if (data_block == NULL)
	{
	  ERROR ((0, 0, _("Unexpected end of file in archive")));
	  return;
	}

      data_size = available_space_after (data_block);
      if (data_size > size)
	data_size = size;
      if (!(*processor) (data_size, data_block->buffer))
	processor = process_noop;
      set_next_block_after
	((union block *) (data_block->buffer + data_size - 1));
      size -= data_size;
      if (multi_volume_option)
	save_sizeleft -= data_size;
    }
}

/*---.
| ?  |
`---*/

/* JK This routine should be used more often than it is ... look into
   that.  Anyhow, what it does is translate the sparse information on the
   header, and in any subsequent extended headers, into an array of
   structures with true numbers, as opposed to character strings.  It
   simply makes our life much easier, doing so many comparisong and such.
   */

static void
fill_in_sparse_array (void)
{
  int counter;

  /* Allocate space for our scratch space; it's initially 10 elements long,
     but can change in this routine if necessary.  */

  sp_array_size = 10;
  sparsearray = (struct sp_array *)
    xmalloc (sp_array_size * sizeof (struct sp_array));

  /* There are at most five of these structures in the header itself; read
     these in first.  */

  for (counter = 0; counter < SPARSES_IN_OLDGNU_HEADER; counter++)
    {
      /* Compare to 0, or use !(int) EXPR, for Pyramid's dumb compiler.  */
      if (current.block->oldgnu_header.sp[counter].numbytes == 0)
	break;

      sparsearray[counter].offset =
	get_initial_header_offset (current.block, counter);
      sparsearray[counter].numbytes =
	get_initial_header_numbytes (current.block, counter);
    }

  /* If the header's extended, we gotta read in exhdr's till we're done.  */

  if (current.block->oldgnu_header.isextended)
    {
      /* How far into the sparsearray we are `so far'.  */
      static int so_far_ind = SPARSES_IN_OLDGNU_HEADER;
      union block *exhdr;

      while (true)
	{
	  exhdr = find_next_block ();
	  for (counter = 0; counter < SPARSES_IN_SPARSE_HEADER; counter++)
	    {
	      if (counter + so_far_ind > sp_array_size - 1)
		{
		  /* We just ran out of room in our scratch area -
		     realloc it.  */

		  sp_array_size *= 2;
		  sparsearray = (struct sp_array *)
		    xrealloc (sparsearray,
			      sp_array_size * sizeof (struct sp_array));
		}

	      sparsearray[counter + so_far_ind].offset =
		get_extended_header_offset (exhdr, counter);
	      sparsearray[counter + so_far_ind].numbytes =
		get_extended_header_numbytes (exhdr, counter);
	    }

	  /* If this is the last extended header for this file, we can
	     stop.  */

	  if (!exhdr->sparse_header.isextended)
	    break;

	  so_far_ind += SPARSES_IN_SPARSE_HEADER;
	  set_next_block_after (exhdr);
	}

      /* Be sure to skip past the last one.  */

      set_next_block_after (exhdr);
    }
}

/*---.
| ?  |
`---*/

/* JK Diff'ing a sparse file with its counterpart on the tar file is a
   bit of a different story than a normal file.  First, we must know what
   areas of the file to skip through, i.e., we need to contruct a
   sparsearray, which will hold all the information we need.  We must
   compare small amounts of data at a time as we find it.  */

/* FIXME: This does not look very solid to me, at first glance.  Zero areas
   are not checked, spurious sparse entries seemingly goes undetected, and
   I'm not sure overall identical sparsity is verified.  */

static void
diff_sparse_files (off_t size_of_file)
{
  off_t remaining_file_size = size_of_file;
  char *chunk_buffer = (char *) xmalloc (BLOCKSIZE);
  size_t chunk_buffer_allocated_size = BLOCKSIZE;
  union block *data_block = NULL;
  int counter = 0;
  bool different = false;

  fill_in_sparse_array ();

  while (remaining_file_size > 0)
    {
      ssize_t size_read;
      size_t chunk_size;
#if 0
      off_t amount_read = 0;
#endif

      data_block = find_next_block ();
      chunk_size = sparsearray[counter].numbytes;
      if (!chunk_size)
	break;

      lseek (diff_handle, sparsearray[counter].offset, SEEK_SET);

      /* Take care to not run out of room in our buffer.  */

      while (chunk_buffer_allocated_size < chunk_size)
	{
	  chunk_buffer_allocated_size *= 2;
	  chunk_buffer = (char *)
	    xrealloc (chunk_buffer, chunk_buffer_allocated_size);
	}

      while (chunk_size > BLOCKSIZE)
	{
	  if (size_read = full_read (diff_handle, chunk_buffer, BLOCKSIZE),
	      size_read != BLOCKSIZE)
	    {
	      if (size_read < 0)
		{
		  WARN ((0, errno, _("Cannot read %s"), current.name));
		  report_difference (NULL);
		}
	      else
		{
		  char message[MESSAGE_BUFFER_SIZE];

		  sprintf (message, _("Could only read %lu of %lu bytes"),
			   (unsigned long) size_read,
			   (unsigned long) chunk_size);
		  report_difference (message);
		}
	      break;
	    }

	  if (memcmp (chunk_buffer, data_block->buffer, BLOCKSIZE))
	    {
	      different = true;
	      break;
	    }

	  chunk_size -= size_read;
	  remaining_file_size -= size_read;
	  set_next_block_after (data_block);
	  data_block = find_next_block ();
	}

      if (size_read = full_read (diff_handle, chunk_buffer, chunk_size),
	  size_read != chunk_size)
	{
	  if (size_read < 0)
	    {
	      WARN ((0, errno, _("Cannot read %s"), current.name));
	      report_difference (NULL);
	    }
	  else
	    {
	      char message[MESSAGE_BUFFER_SIZE];

	      sprintf (message, _("Could only read %lu of %lu bytes"),
		       (unsigned long) size_read, (unsigned long) chunk_size);
	      report_difference (message);
	    }
	  break;
	}

      if (memcmp (chunk_buffer, data_block->buffer, chunk_size))
	{
	  different = true;
	  break;
	}
#if 0
      amount_read += chunk_size;
      if (amount_read >= BLOCKSIZE)
	{
	  amount_read = 0;
	  set_next_block_after (data_block);
	  data_block = find_next_block ();
	}
#endif
      set_next_block_after (data_block);
      counter++;
      remaining_file_size -= chunk_size;
    }

#if 0
  /* If the number of bytes read isn't the number of bytes supposedly in the
     file, they're different.  */

  if (amount_read != size_of_file)
    different = true;
#endif

  set_next_block_after (data_block);
  free (sparsearray);

  if (different)
    report_difference (_("Data differs"));
}

#if ENABLE_DALE_CODE

/*---.
| ?  |
`---*/

/* Diff'ing a compressed file with its counterpart on the tar file is a bit of
   a different story than a normal file.  We need to spawn a process to
   uncompress the data.  (We need to uncompress the data for comparison rather
   than compressing the real file, because we don't necessarily know what
   compression format the tar data is in, and many uncompress programs will
   handle multiple formats.  */

static void
diff_compressed_file (size_t size_of_file, char *name)
{
  union block *data_block;
  int written, count, child, status;
  int child_process_1 = 0, child_process_2 = 0;
  size_t sizeleft;
  WAIT_T wait_status;
  /* For sending data from the parent to child process 1.  */
  int pipe1[2];
# define pipe1r	(pipe1[0])
# define pipe1w	(pipe1[1])
  /* For sending data from child process 1 to child process 2.  */
  int pipe2[2];
# define pipe2r	(pipe2[0])
# define pipe2w	(pipe2[1])

  /* Flush the listing file, so we don't double-write its data.  */
  fflush (stdlis);

  /* Create pipes.  */
  if (pipe (pipe1) < 0)
    FATAL_ERROR ((0, errno, _("Cannot create pipe")));
  if (pipe (pipe2) < 0)
    FATAL_ERROR ((0, errno, _("Cannot create pipe")));

  /* Fork child 1, which will uncompress the data.  */
  child_process_1 = fork ();
  if (child_process_1 < 0)
    {
      /* An error was found attempting to fork.  */
      ERROR ((0, errno, _("\
%s: Error in fork() attempting to decompress file - file skipped"), name));
      /* Skip the file.  */
      skip_file ((long) size_of_file);
      close (pipe1r);
      close (pipe1w);
      close (pipe2r);
      close (pipe2w);
      goto finish_children;
    }
  else if (child_process_1 == 0)
    {
      /* Child process 1.  */

      /* Close pipes we will not use.  */
      close (pipe1w);
      close (pipe2r);
      /* Close standard input.  Dup pipe 1 as standard input.  */
      xdup2 (pipe1r, STDIN,
	     _("archive data pipe to stdin for file decompression"));
      /* Close standard output.  Dup pipe 2 as standard output.  */
      xdup2 (pipe2w, STDOUT,
	     _("file compare pipe to stdout for file decompression"));

      /* Execute the uncompression program.  */
      execlp (use_compress_program_option, use_compress_program_option,
	      "-d", (char *) 0);
      /* Should not return.  */
      FATAL_ERROR ((0, errno, _("Cannot exec %s -d"),
		    use_compress_program_option));
    }

  /* Fork child 2, which will compare the uncompressed data to the file.  */
  child_process_2 = fork ();
  if (child_process_2 < 0)
    {
      /* An error was found attempting to fork.  */
      ERROR ((0, errno, _("\
%s: Error in fork() attempting to decompress file - file skipped"), name));
      /* Skip the file.  */
      skip_file ((long) size_of_file);
      /* Closing the pipes will cause the child processes to die.  */
      close (pipe1r);
      close (pipe1w);
      close (pipe2r);
      close (pipe2w);
      goto finish_children;
    }
  else if (child_process_2 == 0)
    {
      int exit_status = TAREXIT_SUCCESS;
      /* Child process 2.  */

      /* Close pipes we will not use.  */
      close (pipe1r);
      close (pipe1w);
      close (pipe2w);

      /* Do the comparison. */
      while (1)
	{
	  /* Read data from pipe 2 in chunks and compare them.  */
	  int bytes_read, i;
	  char pipe_buffer[sizeof (diff_buffer)];

	  /* Read however many bytes we can.  */
	  status = read (pipe2r, pipe_buffer, sizeof (pipe_buffer));
	  if (status == 0)
	    /* Zero bytes read indicates EOF.  */
	    /* We do not have to check for EOF on the disk file, since
	       we have previously verified that it has the right length.  */
	    break;
	  else if (status < 0)
	    {
	      /* An error was detected.  */
	      ERROR ((0, errno, _("%s: Could not read from pipe"),
		      name));
	      /* Signal parent process that something was wrong.  */
	      exit (TAREXIT_FAILURE);
	    }
	  bytes_read = status;

	  /* Now read the same number of bytes from the disk file.  */
	  status = read (diff_handle, diff_buffer, bytes_read);
	  if (status < 0)
	    {
	      /* An error was detected.  */
	      ERROR ((0, errno, _("%s: Could not read from file"),
		      name));
	      /* Signal parent process that something was wrong.  */
	      exit (TAREXIT_FAILURE);
	    }
	  else if (status != bytes_read)
	    {
	      /* Incomplete read indicates disk file has different length.
	         This is not expected, because we checked the length
		 earlier.  */
	      ERROR ((0, 0, _("%s: Could only read %d of %d bytes"),
		      name, status, bytes_read));
	      /* Signal parent process that something was wrong.  */
	      exit (TAREXIT_FAILURE);
	    }
	  /* Compare the data.  */
	  if (memcmp(diff_buffer, pipe_buffer, bytes_read) != 0)
	    {
	      /* Remember to signal parent process that data differs.  But
		 don't exit now, so parent process does not get SIGPIPE trying
		 to send the rest of the data to us.  */
	      exit_status = TAREXIT_DIFFERS;
	    }
	}
      /* Return the exit status to the parent.  */
      exit (exit_status);
    }

  /* Continue in the parent process to extract the data from the archive and
     feed it to child process 1.  Close pipes we will not use.  */
  close (pipe1r);
  close (pipe2r);
  close (pipe2w);

  for (sizeleft = size_of_file; sizeleft > 0; sizeleft -= written)
    {
      data_block = find_next_block ();
      if (data_block == NULL)
	{
	  ERROR ((0, 0, _("Unexpected EOF on archive file")));
	  /* Closing the pipes will cause the child processes to die.  */
	  close (pipe1w);
	  goto finish_children;
	}

      /* Determine the number of bytes available in this data block to send to
	 the uncompress process.  */
      written = available_space_after (data_block);
      if (written > sizeleft)
	written = sizeleft;

      /* Write to child process 1.  */
      status = write (pipe1w, data_block->buffer, (size_t) written);
      set_next_block_after ((union block *)
			    (data_block->buffer + written - 1));
      if (status == written)
	continue;

      /* Error in writing pipe.  Print message, then skip to next file in
	 archive.  */
      if (status < 0)
	ERROR ((0, errno, _("%s: Could not write to pipe"), name));
      else
	ERROR ((0, 0, _("%s: Could only write %d of %d bytes"),
		name, status, written));
      skip_file ((long) (sizeleft - written));
      goto finish_children;
    }

  /* Close the pipe to child process 1, letting it knows that we have reached
     EOF.  */
  close (pipe1w);

  /* Wait for the child processes to finish so we can catch errors and ensure
     the system isn't filled with zombie processes.  */
 finish_children:
  /* Wait for child processes to finish.  */
  while (1)
    {
      /* Wait for a child to finish, or for no more children.  */
      while (child = wait (&wait_status),
             (child != child_process_1
	      && child != child_process_2
	      && child != -1))
	continue;

      /* If no more children, exit this loop.  */
      if (child == -1)
	break;

      if (WIFSIGNALED (wait_status)
#if 0
	  && !WIFSTOPPED (wait_status)
#endif
	  )
	{
	  /* SIGPIPE is OK, everything else is a problem.  */

	  if (WTERMSIG (wait_status) != SIGPIPE) {
	    ERROR ((0, 0, _("Child died with signal %d%s"),
		    WTERMSIG (wait_status),
		    WCOREDUMP (wait_status) ? _(" (core dumped)") : ""));
	  }
	}
      else
	{
	  /* Child voluntarily terminated -- but why?  /bin/sh returns SIGPIPE
	     + 128 if its child, then do nothing.  */

	  if (WEXITSTATUS (wait_status) != (SIGPIPE + 128)
	      && WEXITSTATUS (wait_status))
	    /* We have to capture exit status 1 from child 2, which indicates
	       a difference (or error) was discovered.  But child 2 will then
	       have written an error message, so we shouldn't report one as
	       well.  But we should set the exit status.  */
	    if (child == child_process_2)
	      switch (WEXITSTATUS (wait_status))
		{
		case TAREXIT_SUCCESS:
		  break;
		case TAREXIT_DIFFERS:
		  /* Report difference.  (This sets the exit code.)  */
		  report_difference (_("Data differs"));
		  break;
		default:
		  exit_status = WEXITSTATUS (wait_status);
		  break;
		}
	    else
	      ERROR ((0, 0, _("Child returned status %d"),
		      WEXITSTATUS (wait_status)));
	}
    }
}

#endif /* ENABLE_DALE_CODE */

/*---------------------------------------------------------------------.
| Call either stat or lstat over STAT_DATA, depending on --dereference |
| (-h), for a file which should exist.  Diagnose any problem.  Return  |
| true for success, false otherwise.				       |
`---------------------------------------------------------------------*/

static bool
get_stat_info (struct stat *stat_info)
{
  int status = (dereference_option
		? stat (current.name, stat_info)
		: lstat (current.name, stat_info));

  if (status < 0)
    {
      if (errno == ENOENT)
	report_difference (_("File does not exist"));
      else
	{
	  ERROR ((0, errno, _("Cannot stat file %s"), current.name));
	  report_difference (NULL);
	}
#if 0
      skip_file (current.stat.st_size);
#endif
      return false;
    }

#if DOSWIN
  /* DJGPP doesn't set st_rdev.  Fix this.  */
  if (stat_info->st_rdev == 0)
    stat_info->st_rdev = stat_info->st_dev;
#endif

  return true;
}

#if DOSWIN

/*------------------------------------------------------------------.
| Diff two files.  Used on MS-DOS to compare with simulated links.  |
`------------------------------------------------------------------*/

static void
diff_two_files (const char *file_name1, const char *file_name2)
{
  int diff_handle1, diff_handle2, r1, r2;
  static unsigned char *buf;
  char message[MESSAGE_BUFFER_SIZE];

  if (!buf)
    buf = (unsigned char *) xmalloc (record_size);

  /* No need to check `open' succeeded, since the two files were already
     verified to exist.  */
  diff_handle1 = open (file_name1, O_RDONLY | O_BINARY);
  diff_handle2 = open (file_name1, O_RDONLY | O_BINARY);

  while ((r1 = read (diff_handle1, diff_buffer, record_size)) > 0
	 && (r2 = read (diff_handle2, buf, record_size)) > 0)
    {
      if (r1 != r2)
	{
	  if (r1 < 0)
	    {
	      WARN ((0, errno, _("Cannot read %s"), file_name1));
	      report_difference (NULL);
	    }
	  else if (r2 < 0)
	    {
	      WARN ((0, errno, _("Cannot read %s"), file_name2));
	      report_difference (NULL);
	    }
	  else
	    {
	      sprintf (message, _("Could only read %d of %ld bytes"),
		       r1 < r2 ? r1 : r2, r1 > r2 ? r1 : r2);
	      report_difference (message);
	    }
	  return;
	}
      if (memcmp (buf, diff_buffer, (size_t) r1))
	{
	  report_difference (_("Data differs"));
	  return;
	}
    }
}

#endif

/*--------------------------.
| Comparing uids and gids.  |
`--------------------------*/

#if DOSWIN

/* Ignore arguments and return false.  */

# define uid_differs(Arg1, Arg2) \
   (false)
# define gid_differs(Arg1, Arg2) \
   (false)

#else

# define uid_differs(Arg1, Arg2) \
   ((Arg1) != (Arg2))
# define gid_differs(Arg1, Arg2) \
   ((Arg1) != (Arg2))

#endif

/*--------------------.
| Compare mode bits.  |
`--------------------*/

#if DOSWIN

/* Only compares bits that are stored by the filesystem.  */

static bool
mode_differs (mode_t mode1, mode_t mode2)
{
  /* Quickly handle the (hopefully) usual case.  */
  if (mode1 == mode2)
    return false;

  if ((mode1 & S_IFMT) != (mode2 & S_IFMT))
    return true;

  /* MS-DOS files are always readable, only writable by owner, and the execute
     bit is invented, not really stored in the filesystem.  This could cause
     some false alarms.  So only report a difference when the user's write bit
     is different.  */
  if (!S_ISDIR (mode1) && (mode1 & S_IWUSR) == (mode2 & S_IWUSR))
    return false;

  /* For directories and non-files, check both write and execute bits.  */
  if ((mode1 & (S_IWUSR | S_IXUSR)) == (mode2 & (S_IWUSR | S_IXUSR)))
    return false;

  return true;
}

#else /* not DOSWIN */

# define mode_differs(Arg1, Arg2) \
   ((Arg1) != (Arg2))

#endif /* not DOSWIN */

/*----------------------------------.
| Diff a file against the archive.  |
`----------------------------------*/

void
diff_archive (void)
{
  struct stat stat_info;
  size_t name_length;
  int status;
  struct utimbuf restore_times;

  errno = EPIPE;		/* FIXME: errno should be read-only */
				/* FIXME: remove perrors */

  set_next_block_after (current.block);
  decode_header (&current, true);

  /* Print the block from `current.block' and `current.stat'.  */

  if (verbose_option)
    {
      if (now_verifying)
	{
	  if (checkpoint_option)
	    flush_checkpoint_line ();
	  fprintf (stdlis, _("Verify "));
	}
      print_header (&current);
    }

  switch (current.block->header.typeflag)
    {
    default:
      WARN ((0, 0, _("Unknown file type `%c' for %s, diffed as normal file"),
		 current.block->header.typeflag, current.name));
      /* Fall through.  */

    case AREGTYPE:
    case REGTYPE:
    case OLDGNU_SPARSE:
    case CONTTYPE:
    case OLDGNU_REGULAR_COMPRESSED:
    case OLDGNU_SPARSE_COMPRESSED:
    case OLDGNU_CONTIG_COMPRESSED:

      /* Appears to be a file.  See if it's really a directory.  */

      name_length = strlen (current.name) - 1;
      if (current.name[name_length] == '/')
	goto really_dir;

      if (!get_stat_info (&stat_info))
	{
	  if (current.block->oldgnu_header.isextended)
	    skip_extended_headers ();
	  skip_file (current.stat.st_size);
	  goto quit;
	}

      if (!S_ISREG (stat_info.st_mode))
	{
	  report_difference (_("Not a regular file"));
	  skip_file (current.stat.st_size);
	  goto quit;
	}

      stat_info.st_mode &= 07777;
      if (mode_differs (stat_info.st_mode, current.stat.st_mode))
	report_difference (_("Mode differs"));

      if (uid_differs (stat_info.st_uid, current.stat.st_uid))
	report_difference (_("Uid differs"));
      if (gid_differs (stat_info.st_gid, current.stat.st_gid))
	report_difference (_("Gid differs"));

      if (time_compare (stat_info.st_mtime, current.stat.st_mtime) != 0)
	report_difference (_("Mod time differs"));
#if ENABLE_DALE_CODE
      /* Compare the recorded size of the file with the current size.  This is
	 made difficult because several file formats store a compressed size
	 in the header size field, so we have to then compare with a "real
	 size" file in the Gnu extensions part of the header.  */
      if (stat_info.st_size
	  != ((current.block->header.typeflag == OLDGNU_SPARSE
	       || current.block->header.typeflag == OLDGNU_REGULAR_COMPRESSED
	       || current.block->header.typeflag == OLDGNU_SPARSE_COMPRESSED
	       || current.block->header.typeflag == OLDGNU_CONTIG_COMPRESSED)
	      ? strtol (current.block->oldgnu_header.realsize, NULL, 8)
	      : current.stat.st_size))
 	{
	  report_difference (_("Size differs"));
	  skip_file (current.stat.st_size);
	  goto quit;
	}
#else /* not ENABLE_DALE_CODE */
      if (current.block->header.typeflag != OLDGNU_SPARSE
	  && stat_info.st_size != current.stat.st_size)
	{
	  report_difference (_("Size differs"));
	  skip_file (current.stat.st_size);
	  goto quit;
	}
#endif /* not ENABLE_DALE_CODE */

      diff_handle = open (current.name, O_NDELAY | O_RDONLY | O_BINARY);

      if (diff_handle < 0 && !absolute_names_option)
	{
	  char *tmpbuf = xmalloc (strlen (current.name) + 2);

	  *tmpbuf = '/';
	  strcpy (tmpbuf + 1, current.name);
	  diff_handle = open (tmpbuf, O_NDELAY | O_RDONLY | O_BINARY);
	  free (tmpbuf);
	}
      if (diff_handle < 0)
	{
	  ERROR ((0, errno, _("Cannot open %s"), current.name));
	  if (current.block->oldgnu_header.isextended)
	    skip_extended_headers ();
	  skip_file (current.stat.st_size);
	  report_difference (NULL);
	  goto quit;
	}

      /* Record the access and modification times.  They will be restored
	 afterward, if --atime-preserve was specified. */
      restore_times.actime = stat_info.st_atime;
      restore_times.modtime = stat_info.st_mtime;

      /* Need to treat sparse files completely differently here.  Similarly,
	 compressed files need to be handle specially.  */

      if (current.block->header.typeflag == OLDGNU_SPARSE)
	diff_sparse_files (current.stat.st_size);
#if ENABLE_DALE_CODE
      else if (current.block->header.typeflag == OLDGNU_REGULAR_COMPRESSED
	       || current.block->header.typeflag == OLDGNU_SPARSE_COMPRESSED
	       || current.block->header.typeflag == OLDGNU_CONTIG_COMPRESSED)
	diff_compressed_file (current.stat.st_size, current.name);
#endif
      else
	{
	  if (multi_volume_option)
	    {
	      assign_string (&save_name, current.name);
	      save_totsize = current.stat.st_size;
	      /* save_sizeleft is set in read_and_process.  */
	    }

	  read_and_process (current.stat.st_size, process_rawdata);

	  if (multi_volume_option)
	    assign_string (&save_name, NULL);
	}

      status = close (diff_handle);
      if (status < 0)
	ERROR ((0, errno, _("Error while closing %s"), current.name));

      /* Restore access and modification times if --atime-preserve was
	 specified. */
      if (atime_preserve_option)
	utime (current.name, &restore_times);

    quit:
      break;

    case LNKTYPE:
      {
	dev_t dev;
	ino_t ino;

	if (!get_stat_info (&stat_info))
	  break;

	dev = stat_info.st_dev;
	ino = stat_info.st_ino;
	status = stat (current.linkname, &stat_info);
	if (status < 0)
	  {
	    if (errno == ENOENT)
#if DOSWIN
	      report_difference (_("Target of link does not exist"));
#else
 	      report_difference (_("Does not exist"));
#endif
	    else
	      {
		WARN ((0, errno, _("Cannot stat file %s"), current.linkname));
		report_difference (NULL);
	      }
	    break;
	  }

	if (stat_info.st_dev != dev
#if !DOSWIN
	    /* DOSWIN doesn't support hard links, so only compare inodes when
	       not DOSWIN.  */
	    || stat_info.st_ino != ino
#endif
	    )
	  {
	    char *message = (char *)
	      xmalloc (MESSAGE_BUFFER_SIZE + strlen (current.linkname));

	    sprintf (message, _("Not linked to %s"), current.linkname);
	    report_difference (message);
	    free (message);
	    break;
	  }

#if DOSWIN
	/* Since these are actually two different files, make sure their
	   contents is identical.  */
	diff_two_files (current.name, current.linkname);
#endif

	break;
      }

    case SYMTYPE:
#ifdef S_ISLNK
      {
	char linkbuf[NAME_FIELD_SIZE + 3]; /* FIXME: may be too short.  */

	status = readlink (current.name, linkbuf, (sizeof linkbuf) - 1);

	if (status < 0)
	  {
	    if (errno == ENOENT)
	      report_difference (_("No such file or directory"));
	    else
	      {
		WARN ((0, errno, _("Cannot read link %s"), current.name));
		report_difference (NULL);
	      }
	    break;
	  }

	linkbuf[status] = '\0';	/* null-terminate it */
	if (strncmp (current.linkname, linkbuf, (size_t) status) != 0)
	  report_difference (_("Symlink differs"));
      }
#else  /* not S_ISLNK */

      status = stat (current.linkname, &stat_info);
      if (status < 0)
	{
	  if (errno == ENOENT)
	    report_difference (_("Target of link does not exist"));
	  else
	    {
	      WARN ((0, errno, _("Cannot stat file %s"), current.linkname));
	      report_difference (NULL);
	    }
	  break;
	}
      status = stat (current.name, &stat_info);
      if (status < 0)
	{
	  if (errno == ENOENT)
	    report_difference (_("No such file or directory"));
	  else
	    {
	      WARN ((0, errno, _("Cannot stat file %s"), current.name));
	      report_difference (NULL);
	    }
	}
      else
	diff_two_files (current.name, current.linkname);

#endif /* not S_ISLNK */

      break;

#ifdef S_IFCHR
    case CHRTYPE:
      current.stat.st_mode |= S_IFCHR;
      goto check_node;
#endif /* not S_IFCHR */

#ifdef S_IFBLK
      /* If local system doesn't support block devices, use default case.  */

    case BLKTYPE:
      current.stat.st_mode |= S_IFBLK;
      goto check_node;
#endif /* not S_IFBLK */

#ifdef S_ISFIFO
      /* If local system doesn't support FIFOs, use default case.  */

    case FIFOTYPE:
# ifdef S_IFIFO
      current.stat.st_mode |= S_IFIFO;
# endif
      current.stat.st_rdev = 0;	/* FIXME: do we need this? */
      goto check_node;
#endif /* S_ISFIFO */

    check_node:
      /* FIXME: deal with umask.  */

      if (!get_stat_info (&stat_info))
	break;

      if (current.stat.st_rdev != stat_info.st_rdev)
	{
	  report_difference (_("Device numbers changed"));
	  break;
	}

      if (
#ifdef S_IFMT
	  mode_differs (current.stat.st_mode, stat_info.st_mode)
#else
	  /* POSIX lossage.  */
	  mode_differs ((current.stat.st_mode & 07777),
			(stat_info.st_mode & 07777))
#endif
	  )
	{
	  report_difference (_("Mode or device-type changed"));
	  break;
	}

      break;

    case OLDGNU_DUMPDIR:
      {
	char *dumpdir_buffer = get_directory_contents (current.name, (dev_t) 0);

	if (multi_volume_option)
	  {
	    assign_string (&save_name, current.name);
	    save_totsize = current.stat.st_size;
	    /* save_sizeleft is set in read_and_process.  */
	  }

	if (dumpdir_buffer)
	  {
	    dumpdir_cursor = dumpdir_buffer;
	    read_and_process (current.stat.st_size, process_dumpdir);
	    free (dumpdir_buffer);
	  }
	else
	  read_and_process (current.stat.st_size, process_noop);

	if (multi_volume_option)
	  assign_string (&save_name, NULL);
	/* Fall through.  */
      }

    case DIRTYPE:
      /* Check for trailing slashes.  */
      name_length = strlen (current.name) - 1;

    really_dir:
      while (name_length && current.name[name_length] == '/')
	/* Zap trailing slashes.  */
	current.name[name_length--] = '\0';

      if (!get_stat_info (&stat_info))
	break;

      if (!S_ISDIR (stat_info.st_mode))
	{
	  report_difference (_("No longer a directory"));
	  break;
	}

      if (mode_differs ((stat_info.st_mode & 07777),
			(current.stat.st_mode & 07777)))
	report_difference (_("Mode differs"));
      break;

    case OLDGNU_VOLHDR:
      break;

    case OLDGNU_MULTIVOL:
      {
	off_t offset;

	name_length = strlen (current.name) - 1;
	if (current.name[name_length] == '/')
	  goto really_dir;

	if (!get_stat_info (&stat_info))
	  break;

	if (!S_ISREG (stat_info.st_mode))
	  {
	    report_difference (_("Not a regular file"));
	    skip_file (current.stat.st_size);
	    break;
	  }

	stat_info.st_mode &= 07777;
	offset = get_header_offset (current.block);
	if (stat_info.st_size != current.stat.st_size + offset)
	  {
	    report_difference (_("Size differs"));
	    skip_file (current.stat.st_size);
	    break;
	  }

	diff_handle = open (current.name, O_NDELAY | O_RDONLY | O_BINARY);

	if (diff_handle < 0)
	  {
	    WARN ((0, errno, _("Cannot open file %s"), current.name));
	    report_difference (NULL);
	    skip_file (current.stat.st_size);
	    break;
	  }

	status = lseek (diff_handle, offset, SEEK_SET);
	if (status != offset)
	  {
	    WARN ((0, errno, _("Cannot seek to %ld in file %s"),
		   offset, current.name));
	    report_difference (NULL);
	    break;
	  }

	if (multi_volume_option)
	  {
	    assign_string (&save_name, current.name);
	    save_totsize = stat_info.st_size;
	    /* save_sizeleft is set in read_and_process.  */
	  }

	read_and_process (current.stat.st_size, process_rawdata);

	if (multi_volume_option)
	  assign_string (&save_name, NULL);

	status = close (diff_handle);
	if (status < 0)
	  ERROR ((0, errno, _("Error while closing %s"), current.name));

	break;
      }
    }
}

/*---.
| ?  |
`---*/

void
verify_volume (void)
{
  if (!diff_buffer)
    diff_init ();

  /* Verifying an archive is meant to check if the physical media got it
     correctly, so try to defeat clever in-memory buffering pertaining to this
     particular media.  On Linux, for example, the floppy drive would not even
     be accessed for the whole verification.

     The code was using fsync only when the ioctl is unavailable.  Marty
     Leisner says that the ioctl does not work when not preceded by fsync.  */

#if HAVE_FSYNC
  fsync (archive);
#endif
#ifdef FDFLUSH
  ioctl (archive, FDFLUSH);
#endif
#if DOSWIN
  _flush_disk_cache ();
#endif

#ifdef MTIOCTOP
  {
    struct mtop operation;
    int status;

    operation.mt_op = MTBSF;
    operation.mt_count = 1;
    if (status = rmtioctl (archive, MTIOCTOP, (char *) &operation), status < 0)
      {
	if (errno != EIO
	    || (status = rmtioctl (archive, MTIOCTOP, (char *) &operation),
		status < 0))
	  {
#endif
	    if (rmtlseek (archive, 0L, SEEK_SET) != 0)
	      {
		/* Lseek failed.  Try a different method.  */

		WARN ((0, errno,
		       _("Could not rewind archive file for verify")));
		return;
	      }
#ifdef MTIOCTOP
	  }
      }
  }
#endif

  access_mode = ACCESS_READ;
  now_verifying = true;

  flush_read ();
  while (true)
    {
      enum read_header status = read_header (&current);

      if (status == HEADER_FAILURE)
	{
	  int counter = 0;

	  while (status == HEADER_FAILURE)
	    {
	      counter++;
	      status = read_header (&current);
	    }

	  ERROR ((0, 0, (counter ==1
			 ? _("VERIFY FAILURE: %d invalid header detected")
			 : _("VERIFY FAILURE: %d invalid headers detected")),
		  counter));
	}
      if (status == HEADER_ZERO_BLOCK || status == HEADER_END_OF_FILE)
	break;

      diff_archive ();
    }

  access_mode = ACCESS_WRITE;
  now_verifying = false;
}
