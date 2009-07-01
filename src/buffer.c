/* Buffer management for tar.
   Copyright (C) 1988, 92, 93, 94, 96, 97, 98 Free Software Foundation, Inc.
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

#if !HAVE_PIPE
# if !HAVE_POPEN
#  include "popen.h"
# endif
#endif

#include <signal.h>
#include <time.h>
time_t time ();

#if XENIX
# include <sys/inode.h>
#endif

#ifndef FNM_LEADING_DIR
# include <fnmatch.h>
#endif

#include "common.h"
#include "rmt.h"

#define DEBUG_FORK 0		/* if nonzero, childs are born stopped */

#define	PREAD 0			/* read file descriptor from pipe() */
#define	PWRITE 1		/* write file descriptor from pipe() */

/* Number of retries before giving up on read.  */
#define	READ_ERROR_MAX 10

/* Globbing pattern to append to volume label if initial match failed.  */
#define VOLUME_LABEL_APPEND " Volume [1-9]*"

/* Variables.  */

static tarlong total_written;	/* bytes written on all volumes */
static tarlong bytes_written;	/* bytes written on this volume */

/* FIXME: The following four variables should ideally be static to this
   module.  However, this cannot be done yet, as update.c uses the first
   three a lot, and compare.c uses the fourth.  The cleanup continues!  */

union block *record_start;	/* start of record of archive */
union block *record_end;	/* last+1 block of archive record */
union block *current_block;	/* current block of archive */
enum access_mode access_mode;	/* how do we handle the archive */
static struct stat archive_stat; /* stat block for archive file */

/* FIXME: off_t is a bit of an overkill here, because we are counting
   blocks, not bytes.  */
static off_t record_start_block; /* block ordinal at record_start */

/* Where we write list messages (not errors, not interactions) to.  Stdout
   unless we're writing a pipe, in which case stderr.  */
FILE *stdlis;

static void backspace_output PARAMS ((void));
static bool new_volume PARAMS ((enum access_mode));
static void write_error PARAMS ((ssize_t));
static void read_error PARAMS ((void));

/* To check if user is erroneously trying to dump the archive itself.  */
dev_t ar_dev;
ino_t ar_ino;

/* PID of child program, if compress_option or remote archive access.  For
   systems without pipes, it's just a flag saying popen is currently in use.  */
static pid_t child_pid = 0;

/* Error recovery stuff.  */
static int read_error_count;

/* Have we hit EOF yet?  */
static bool hit_eof;

/* Checkpointing counter; flag to know if an incomplete line is pending.  */
static int checkpoint = 0;
static bool checkpoint_dots = false;

int file_to_switch_to = -1;	/* if remote update, close archive, and use
				   this descriptor to write to */

static int volno = 1;		/* current volume of a multi-volume tape */
static int global_volno = 1;	/* volume number to print in messages */

/* The pointer save_name, which is set in function dump_file() of module
   create.c, points to the original long filename instead of the new,
   shorter mangled name that is set in start_header() of module create.c.
   The pointer save_name is only used in multi-volume mode when the file
   being processed is non-sparse; if a file is split between volumes, the
   save_name is used in generating the LF_MULTIVOL record on the second
   volume.  (From Pierce Cantrell, 1991-08-13.)  */

char *save_name;		/* name of the file we are currently writing */
off_t save_totsize;		/* total size of file we are writing, only
				   valid if save_name is non NULL */
off_t save_sizeleft;		/* where we are in the file we are writing,
				   only valid if save_name is nonzero */

bool write_archive_to_stdout = false;

/* Used by flush_read and flush_write to store the real info about saved
   names.  */
static char *real_s_name = NULL;
static off_t real_s_totsize;
static off_t real_s_sizeleft;

/* Kludge for loosing systems.  */
#if !HAVE_PIPE
# if !HAVE_POPEN
static FILE *archive_popened_file;
# endif
#endif

#if DOSWIN

struct dos_pipe
{
  FILE *pipe;
  char *tfile;
};

/* FIXME: A single pipe object is enough only as long as `tar' doesn't support
   updating compressed archives.  */
static struct dos_pipe dos_pipe = { NULL, NULL };

#endif /* DOSWIN */

/* Functions.  */

#if DEBUG_FORK

static pid_t
myfork (void)
{
  pid_t result = fork();

  if (result == 0)
    kill (getpid (), SIGSTOP);
  return result;
}

# define fork myfork

#endif /* DEBUG FORK */

void
init_total_written (void)
{
  clear_tarlong (total_written);
  clear_tarlong (bytes_written);
}

void
print_total_written (void)
{
  fprintf (stderr, _("Total bytes written: "));
  print_tarlong (total_written, stderr);
  fprintf (stderr, "\n");
}

/*--------------------------------------------------------.
| Compute and return the block ordinal at current_block.  |
`--------------------------------------------------------*/

/* FIXME: off_t is a bit of an overkill here, because we are counting
   blocks, not bytes.  */

off_t
current_block_ordinal (void)
{
  return record_start_block + (current_block - record_start);
}

/*------------------------.
| Checkpoint processing.  |
`------------------------*/

void
flush_checkpoint_line (void)
{
  if (checkpoint_dots)
    {
#if 0
      /* FIXME: Also see the comment in output_checkpoint_mark, below.  It
	 does not mean much to print dot counts unless each dot represents
	 something precise, which experimentation does not grant yet.  */
      fprintf (stderr, " [%d]\n", checkpoint);
#else
      fputc ('\n', stderr);
#endif
      checkpoint_dots = false;
    }
}

static void
output_checkpoint_mark (void)
{
  if (!checkpoint_dots)
    {
#if 0
      /* FIXME: Each dot stands for 10 records, yet apparently not when using
	 -z, even with -B forces reblocking.  Some exploration is needed
	 before it really makes sense to print the record size.  */
      fprintf (stderr, "[%d] x ", record_size);
#endif
      checkpoint_dots = true;
    }

  fputc ('.', stderr);
  if (checkpoint % 100 == 0)
    {
      if (checkpoint % 500 == 0)
	flush_checkpoint_line ();
      else
	fputc (' ', stderr);
    }

  fflush (stderr);
}

/*------------------------------------------------------------------.
| If the EOF flag is set, reset it, as well as current_block, etc.  |
`------------------------------------------------------------------*/

void
reset_eof (void)
{
  if (hit_eof)
    {
      hit_eof = false;
      current_block = record_start;
      record_end = record_start + blocking_factor;
      access_mode = ACCESS_WRITE;
    }
}

/*-------------------------------------------------------------------------.
| Return the location of the next available input or output block.	   |
| Return NULL for EOF.  Once we have returned NULL, we just keep returning |
| it, to avoid accidentally going on to the next file on the tape.	   |
`-------------------------------------------------------------------------*/

union block *
find_next_block (void)
{
  if (current_block == record_end)
    {
      if (hit_eof)
	return NULL;
      flush_archive ();
      if (current_block == record_end)
	{
	  hit_eof = true;
	  return NULL;
	}
    }
  return current_block;
}

/*------------------------------------------------------.
| Indicate that we have used all blocks up thru BLOCK.  |
| 						        |
| FIXME: should the arg have an off-by-1?	        |
`------------------------------------------------------*/

void
set_next_block_after (union block *block)
{
  while (block >= current_block)
    current_block++;

  /* Do *not* flush the archive here.  If we do, the same argument to
     set_next_block_after could mean the next block (if the input record is
     exactly one block long), which is not what is intended.  */

  if (current_block > record_end)
    abort ();
}

/*------------------------------------------------------------------------.
| Return the number of bytes comprising the space between POINTER through |
| the end of the current buffer of blocks.  This space is available for	  |
| filling with data, or taking data from.  POINTER is usually (but not	  |
| always) the result previous find_next_block call.			  |
`------------------------------------------------------------------------*/

size_t
available_space_after (union block *pointer)
{
  return record_end->buffer - pointer->buffer;
}

/*------------------------------------------------------------------.
| Close file having descriptor FD, and abort if close unsucessful.  |
`------------------------------------------------------------------*/

static void
xclose (int fd)
{
  if (close (fd) < 0)
    FATAL_ERROR ((0, errno, _("Cannot close file #%d"), fd));
}

/*-----------------------------------------------------------------------.
| Duplicate file descriptor FROM into becoming INTO, or else, issue	 |
| MESSAGE.  INTO is closed first and has to be the next available slot.	 |
`-----------------------------------------------------------------------*/

static void
xdup2 (int from, int into, const char *message)
{
  if (from != into)
    {
      int status = close (into);

      if (status < 0 && errno != EBADF)
	FATAL_ERROR ((0, errno, _("Cannot close descriptor %d"), into));
      status = dup (from);
      if (status != into)
	FATAL_ERROR ((0, errno, _("Cannot properly duplicate %s"), message));
      xclose (from);
    }
}

/*-----------------------------------------------------------------------.
| Return true if NAME is the name of a regular file, or if the file does |
| not exist (so it would be created as a regular file).                  |
`-----------------------------------------------------------------------*/

static bool
is_regular_file (const char *name)
{
  struct stat stat_info;

  if (stat (name, &stat_info) < 0)
    return true;

  if (S_ISREG (stat_info.st_mode))
    return true;

  return false;
}

/*-------------------------------------------------------.
| Set ARCHIVE for writing, then compressing an archive.	 |
`-------------------------------------------------------*/

static void
child_open_for_compress (void)
{

#if DOSWIN

  /* Use `popen' to simulate a pipe.  */
  char pipe_cmd[PATH_MAX*5];
  int to_stdout = strcmp (archive_name_array[0], "-") == 0;

  /* I think on DOSWIN we only need special handling for remote files.  */
  if (!_remdev (archive_name_array[0]))
    {
      if (backup_option)
	maybe_backup_file (archive_name_array[0], 1);

      sprintf (pipe_cmd, "%s%s%s",
	       use_compress_program_option,
	       to_stdout ? "" : " > ",
	       to_stdout ? "" : archive_name_array[0]);
      dos_pipe.tfile = NULL;
    }
  else	/* In case somebody will make rtapelib.c work some day...  */
    {
      /*  We cannot write to a pipe and read from its other end at
	  the same time.  So we need a temporary file in-between.  */
      char *tem_file = (char *)xmalloc (L_tmpnam);

      tmpnam (tem_file);
      sprintf (pipe_cmd, "%s > %s", use_compress_program_option, tem_file);
      dos_pipe.tfile = tem_file;
    }

  /* Open the pipe to the compressor.  */
  dos_pipe.pipe = popen (pipe_cmd, "wb");
  if (!dos_pipe.pipe)
    {
      int saved_errno = errno;

      if (backup_option)
	undo_last_backup ();
      dos_pipe.tfile = NULL;
      FATAL_ERROR ((0, saved_errno, _("Cannot open pipe")));
    }

  archive = fileno (dos_pipe.pipe);

#else /* not DOSWIN */
# if HAVE_PIPE

  int parent_pipe[2];
  int child_pipe[2];
  pid_t grandchild_pid;

  if (pipe (parent_pipe) < 0)
    FATAL_ERROR ((0, errno, _("Cannot open pipe")));

  child_pid = fork ();
  if (child_pid < 0)
    FATAL_ERROR ((0, errno, _("Cannot fork process")));

  if (child_pid > 0)
    {
      /* The parent tar is still here!  Just clean up.  */

      archive = parent_pipe[PWRITE];
      xclose (parent_pipe[PREAD]);
      return;
    }

  /* The new born child tar is here!  */

  program_name = _("tar (child)");

  xdup2 (parent_pipe[PREAD], STDIN, _("(child) Pipe to stdin"));
  xclose (parent_pipe[PWRITE]);

  /* Check if we need a grandchild tar.  This happens only if either:
     a) we are writing stdout: to force reblocking;
     b) the file is to be accessed by rmt: compressor doesn't know how;
     c) the file is not a plain file.  */

  if (strcmp (archive_name_array[0], "-") != 0
      && !_remdev (archive_name_array[0])
      && is_regular_file (archive_name_array[0]))
    {
      if (backup_option)
	maybe_backup_file (archive_name_array[0], true);

      /* We don't need a grandchild tar.  Open the archive and launch the
	 compressor.  */

      archive = creat (archive_name_array[0], (mode_t) 0666);
      if (archive < 0)
	{
	  int saved_errno = errno;

	  if (backup_option)
	    undo_last_backup ();
	  FATAL_ERROR ((0, saved_errno, _("Cannot open archive %s"),
			archive_name_array[0]));
	}
      xdup2 (archive, STDOUT, _("Archive to stdout"));
      execlp (use_compress_program_option, use_compress_program_option,
	      (char *) 0);
      FATAL_ERROR ((0, errno, _("Cannot exec %s"),
		    use_compress_program_option));
    }

  /* We do need a grandchild tar.  */

  if (pipe (child_pipe) < 0)
    FATAL_ERROR ((0, errno, _("Cannot open pipe")));

  grandchild_pid = fork ();
  if (grandchild_pid < 0)
    FATAL_ERROR ((0, errno, _("Child cannot fork")));

  if (grandchild_pid > 0)
    {
      /* The child tar is still here!  Launch the compressor.  */

      xdup2 (child_pipe[PWRITE], STDOUT, _("((child)) Pipe to stdout"));
      xclose (child_pipe[PREAD]);
      execlp (use_compress_program_option, use_compress_program_option,
	      (char *) 0);
      FATAL_ERROR ((0, errno, _("Cannot exec %s"),
		    use_compress_program_option));
    }

  /* The new born grandchild tar is here!  */

  program_name = _("tar (grandchild)");

  /* Prepare for reblocking the data from the compressor into the archive.  */

  xdup2 (child_pipe[PREAD], STDIN, _("(grandchild) Pipe to stdin"));
  xclose (child_pipe[PWRITE]);

  if (strcmp (archive_name_array[0], "-") == 0)
    archive = STDOUT;
  else
    archive = rmtcreat (archive_name_array[0], (mode_t) 0666,
			rsh_command_option);
  if (archive < 0)
    FATAL_ERROR ((0, errno, _("Cannot open archive %s"),
		  archive_name_array[0]));

  /* Let's read out of the stdin pipe and write an archive.  */

  while (true)
    {
      size_t size;
      ssize_t size_read = 0;	/* for lint, presumably */
      ssize_t size_written;
      char *buffer_cursor;

      /* Assemble a record.  */

      for (size = 0, buffer_cursor = record_start->buffer;
	   size < record_size;
	   size += size_read, buffer_cursor += size_read)
	{
	  size_t size_to_read = record_size - size;

	  if (size_to_read < BLOCKSIZE)
	    size_to_read = BLOCKSIZE;
	  size_read = full_read (STDIN, buffer_cursor, size_to_read);
	  if (size_read <= 0)
	    break;
	}

      if (size_read < 0)
	FATAL_ERROR ((0, errno, _("Cannot read from compression program")));

      /* Copy the record.  */

      if (size_read == 0)
	{
	  /* We hit the end of the file.  Write last record at full size, as
	     the only role of the grandchild is doing proper reblocking.  */

	  if (size > 0)
	    {
	      memset (record_start->buffer + size, 0, record_size - size);
	      size_written
		= rmtwrite (archive, record_start->buffer, record_size);
	      if (size_written != record_size)
		write_error (size_written);
	    }

	  /* There is nothing else to read, break out.  */
	  break;
	}

      size_written = rmtwrite (archive, record_start->buffer, record_size);
      if (size_written != record_size)
 	write_error (size_written);
    }

#  if 0
  close_archive ();
#  endif
  exit (exit_status);

# else /* not HAVE_PIPE */

  /* While relying on popen emulation, forget about post-compression
     reblocking or pre-decompression deblocking.  */

  char *command = (char *) xmalloc (4 + strlen (use_compress_program_option)
				    + strlen (*archive_name_cursor));

  sprintf (command, "%s > %s",
	   use_compress_program_option, *archive_name_cursor);
  archive_popened_file = popen (command, "w");
  if (!archive_popened_file)
    FATAL_ERROR ((0, errno, _("Cannot run `%s'"), command));
  free (command);

  archive = fileno (archive_popened_file);
  child_pid = 1;

# endif /* not HAVE_PIPE */
#endif /* not DOSWIN */

}

/*---------------------------------------------------------.
| Set ARCHIVE for uncompressing, then reading an archive.  |
`---------------------------------------------------------*/

static void
child_open_for_uncompress (void)
{

#if DOSWIN

  /* Use `popen' to simulate a pipe.  */
  char pipe_cmd[PATH_MAX*5];
  int from_stdout = strcmp (archive_name_array[0], "-") == 0;

  /* I think on DOSWIN we only need special handling for remote files.  */
  if (!_remdev (archive_name_array[0]))
    {
      /* We don't need a grandchild tar.  */
      sprintf (pipe_cmd, "%s -d%s%s",
	       use_compress_program_option,
	       from_stdout ? "" : " < ",
	       from_stdout ? "" : archive_name_array[0]);
      dos_pipe.tfile = NULL;
    }
  else	/* In case somebody will make rtapelib.c work some day...  */
    {
      char *tem_file = (char *)xmalloc (L_tmpnam);
      int   remote_archive, tem_fd;

      /* Since we cannot write to a pipe and read its other end, we
	 need first to copy the remote archive to a local temporary file.  */
      remote_archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY,
				0666, rsh_command_option);
      if (remote_archive < 0)
	FATAL_ERROR ((0, errno, _("Cannot open archive %s"),
		      archive_name_array[0]));

      tmpnam (tem_file);
      tem_fd = open (tem_file, O_RDONLY | O_BINARY | O_CREAT | O_EXCL, 0666);
      if (tem_fd < 0)
	FATAL_ERROR ((0, errno, _("Cannot open pipe")));

      /* Let's read the archive and pipe it into temporary file.  */

      while (1)
	{
	  char *cursor;
	  int maximum;
	  int count;
	  int status;

	  read_error_count = 0;

	error_loop:
	  status = rmtread (remote_archive, record_start->buffer,
			    (unsigned int) (record_size));
	  if (status < 0)
	    {
	      read_error ();
	      goto error_loop;
	    }
	  if (status == 0)
	    break;
	  cursor = record_start->buffer;
	  maximum = status;
	  while (maximum)
	    {
	      count = maximum < BLOCKSIZE ? maximum : BLOCKSIZE;
	      status = write (tem_fd, cursor, (size_t) count);
	      if (status < 0)
		FATAL_ERROR ((0, errno, _("\
Cannot write to compression program")));

	      if (status != count)
		{
		  ERROR ((0, 0, _("\
Write to compression program short %d bytes"),
			  count - status));
		  count = status;
		}

	      cursor += count;
	      maximum -= count;
	    }
	}

      close (tem_fd);
      sprintf (pipe_cmd, "%s -d < %s", use_compress_program_option, tem_file);
      dos_pipe.tfile = tem_file;
    }

  /* Open the pipe to the decompressor.  */
  dos_pipe.pipe = popen (pipe_cmd, "rb");
  if (!dos_pipe.pipe)
    {
      int saved_errno = errno;

      if (backup_option)
	undo_last_backup ();
      dos_pipe.tfile = NULL;
      FATAL_ERROR ((0, saved_errno, _("Cannot open pipe")));
    }

  archive = fileno (dos_pipe.pipe);

#else /* not DOSWIN */
# if HAVE_PIPE

  int parent_pipe[2];
  int child_pipe[2];
  pid_t grandchild_pid;

  if (pipe (parent_pipe) < 0)
    FATAL_ERROR ((0, errno, _("Cannot open pipe")));

  child_pid = fork ();
  if (child_pid < 0)
    FATAL_ERROR ((0, errno, _("Cannot fork process")));

  if (child_pid > 0)
    {
      /* The parent tar is still here!  Just clean up.  */

      read_full_records_option = true;
      archive = parent_pipe[PREAD];
      xclose (parent_pipe[PWRITE]);
      return;
    }

  /* The new born child tar is here!  */

  program_name = _("tar (child)");

  xdup2 (parent_pipe[PWRITE], STDOUT, _("(child) Pipe to stdout"));
  xclose (parent_pipe[PREAD]);

  /* Check if we need a grandchild tar.  This happens only if either:
     a) we're reading stdin: to force unblocking;
     b) the file is to be accessed by rmt: compressor doesn't know how;
     c) the file is not a plain file.  */

  if (strcmp (archive_name_array[0], "-") != 0
      && !_remdev (archive_name_array[0])
      && is_regular_file (archive_name_array[0]))
    {
      /* We don't need a grandchild tar.  Open the archive and lauch the
	 uncompressor.  */

      archive = open (archive_name_array[0], O_RDONLY | O_BINARY,
		      (mode_t) 0666);
      if (archive < 0)
	FATAL_ERROR ((0, errno, _("Cannot open archive %s"),
		      archive_name_array[0]));
      xdup2 (archive, STDIN, _("Archive from stdin"));
      execlp (use_compress_program_option, use_compress_program_option,
	      "-d", (char *) 0);
      FATAL_ERROR ((0, errno, _("Cannot exec %s"),
		    use_compress_program_option));
    }

  /* We do need a grandchild tar.  */

  if (pipe (child_pipe) < 0)
    FATAL_ERROR ((0, errno, _("Cannot open pipe")));

  grandchild_pid = fork ();
  if (grandchild_pid < 0)
    FATAL_ERROR ((0, errno, _("Child cannot fork")));

  if (grandchild_pid > 0)
    {
      /* The child tar is still here!  Launch the uncompressor.  */

      xdup2 (child_pipe[PREAD], STDIN, _("((child)) Pipe to stdin"));
      xclose (child_pipe[PWRITE]);
      execlp (use_compress_program_option, use_compress_program_option,
	      "-d", (char *) 0);
      FATAL_ERROR ((0, errno, _("Cannot exec %s"),
		    use_compress_program_option));
    }

  /* The new born grandchild tar is here!  */

  program_name = _("tar (grandchild)");

  /* Prepare for unblocking the data from the archive into the uncompressor.  */

  xdup2 (child_pipe[PWRITE], STDOUT, _("(grandchild) Pipe to stdout"));
  xclose (child_pipe[PREAD]);

  if (strcmp (archive_name_array[0], "-") == 0)
    archive = STDIN;
  else
    archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY,
		       (mode_t) 0666, rsh_command_option);
  if (archive < 0)
    FATAL_ERROR ((0, errno, _("Cannot open archive %s"),
		  archive_name_array[0]));

  /* Let's read the archive and pipe it into stdout.  */

  while (true)
    {
      size_t size;
      ssize_t size_read;
      ssize_t size_written;
      char *buffer_cursor;

      read_error_count = 0;

      size_read = rmtread (archive, record_start->buffer, record_size);
      while (size_read < 0)
	{
	  read_error ();
	  size_read = rmtread (archive, record_start->buffer, record_size);
	}
      if (size_read == 0)
	break;

      for (size = size_read, buffer_cursor = record_start->buffer;
	   size;
	   size -= size_written, buffer_cursor += size_written)
	{
	  size_t size_to_write = size;

	  if (size > BLOCKSIZE)
	    size = BLOCKSIZE;
	  size_written = full_write (STDOUT, buffer_cursor, size_to_write);
	  if (size_written < 0)
	    FATAL_ERROR ((0, errno, _("Cannot write to compression program")));

	  if (size_written != size_to_write)
	    ERROR ((0, 0, _("Write to compression program short %lu bytes"),
		    (unsigned long) (size_to_write - size_written)));
	}
    }

#  if 0
  close_archive ();
#  endif
  exit (exit_status);

# else /* not HAVE_PIPE */

  /* While relying on popen emulation, forget about deblocking.  */

  char *command = (char *) xmalloc (7 + strlen (use_compress_program_option)
				    + strlen (*archive_name_cursor));

  sprintf (command, "%s -d < %s",
	   use_compress_program_option, *archive_name_cursor);
  archive_popened_file = popen (command, "r");
  if (!archive_popened_file)
    FATAL_ERROR ((0, errno, _("Cannot run `%s'"), command));
  free (command);

  archive = fileno (archive_popened_file);
  child_pid = 1;

# endif /* not HAVE_PIPE */
#endif /* not DOSWIN */

}

/*--------------------------------------------------------------------------.
| Check the LABEL block against the volume label, seen as a globbing	    |
| pattern.  Return true if the pattern matches.  In case of failure, retry  |
| matching a volume sequence number before giving up in multi-volume mode.  |
`--------------------------------------------------------------------------*/

static bool
check_label_pattern (union block *label)
{
  char *string;
  bool result;

  if (fnmatch (volume_label_option, label->header.name, 0) == 0)
    return true;

  if (!multi_volume_option)
    return false;

  string = xmalloc (strlen (volume_label_option)
		    + sizeof VOLUME_LABEL_APPEND + 1);
  strcpy (string, volume_label_option);
  strcat (string, VOLUME_LABEL_APPEND);
  result = fnmatch (string, label->header.name, 0) == 0;
  free (string);
  return result;
}

/*------------------------------------------------------------------------.
| Open an archive file.  The argument specifies whether we are reading or |
| writing, or both.							  |
`------------------------------------------------------------------------*/

void
open_archive (enum access_mode wanted_access)
{
  bool backed_up_flag = false;

  if (record_size == 0)
    FATAL_ERROR ((0, 0, _("Record size is zero")));

  if (archive_names == 0)
    FATAL_ERROR ((0, 0, _("No archive name given")));

  /* Allocate enough so we can do POSIX without fearing any overflow.  */

  current.name_allocated = PREFIX_FIELD_SIZE + NAME_FIELD_SIZE + 2;
  current.name = (char *) xmalloc (current.name_allocated);
  current.linkname_allocated = LINKNAME_FIELD_SIZE + 1;
  current.linkname = (char *) xmalloc (current.linkname_allocated);

  /* FIXME: According to POSIX.1, PATH_MAX may well not be a compile-time
     constant, and the value from sysconf (_SC_PATH_MAX) may well not be any
     size that is reasonable to allocate a buffer; in fact, there might be no
     fixed limit.  The only correct thing to do is to use dynamic allocation.
     (Roland McGrath) */

  if (!real_s_name)
    real_s_name = (char *) xmalloc (PATH_MAX);
  /* FIXME: real_s_name is never freed.  */

  save_name = NULL;

  if (multi_volume_option)
    {
      if (verify_option)
	FATAL_ERROR ((0, 0, _("Cannot verify multi-volume archives")));

      record_start = (union block *) valloc (record_size + 2 * BLOCKSIZE);
      if (record_start)
	record_start += 2;
    }
  else
    record_start = (union block *) valloc (record_size);

  if (!record_start)
    FATAL_ERROR ((0, 0, _("Could not allocate memory for blocking factor %d"),
		  blocking_factor));

  current_block = record_start;
  record_end = record_start + blocking_factor;
  /* When updating the archive, we start with reading.  */
  access_mode = wanted_access == ACCESS_UPDATE ? ACCESS_READ : wanted_access;

  stdlis = to_stdout_option ? stderr : stdout;
  checkpoint = 0;
  checkpoint_dots = false;

  if (use_compress_program_option)
    {
      if (multi_volume_option)
	FATAL_ERROR ((0, 0, _("Cannot use multi-volume compressed archives")));
      if (verify_option)
	FATAL_ERROR ((0, 0, _("Cannot verify compressed archives")));

      switch (wanted_access)
	{
	case ACCESS_READ:
	  child_open_for_uncompress ();
	  break;

	case ACCESS_WRITE:
	  child_open_for_compress ();
	  break;

	case ACCESS_UPDATE:
	  FATAL_ERROR ((0, 0, _("Cannot update compressed archives")));
	  break;
	}

      if (wanted_access == ACCESS_WRITE
	  && strcmp (archive_name_array[0], "-") == 0)
	stdlis = stderr;
    }
  else if (strcmp (archive_name_array[0], "-") == 0)
    {
      read_full_records_option = true; /* could be a pipe, be safe */
      if (verify_option)
	FATAL_ERROR ((0, 0, _("Cannot verify stdin/stdout archive")));

      switch (wanted_access)
	{
	case ACCESS_READ:
	  archive = STDIN;
	  break;

	case ACCESS_WRITE:
	  archive = STDOUT;
	  stdlis = stderr;
	  break;

	case ACCESS_UPDATE:
	  archive = STDIN;
	  stdlis = stderr;
	  write_archive_to_stdout = true;
	  break;
	}
    }
  else if (verify_option)
    archive = rmtopen (archive_name_array[0], O_RDWR | O_CREAT | O_BINARY,
		       (mode_t) 0666, rsh_command_option);
  else
    switch (wanted_access)
      {
      case ACCESS_READ:
	archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY,
			   (mode_t) 0666, rsh_command_option);
	break;

      case ACCESS_WRITE:
	if (backup_option)
	  {
	    maybe_backup_file (archive_name_array[0], true);
	    backed_up_flag = true;
	  }
	archive = rmtcreat (archive_name_array[0], (mode_t) 0666,
			    rsh_command_option);
	break;

      case ACCESS_UPDATE:
	archive = rmtopen (archive_name_array[0], O_RDWR | O_CREAT | O_BINARY,
			   (mode_t) 0666, rsh_command_option);
	break;
      }

  if (archive < 0)
    {
      int saved_errno = errno;

      if (backed_up_flag)
	undo_last_backup ();
      FATAL_ERROR ((0, saved_errno, _("Cannot open %s"),
		    archive_name_array[0]));
    }

  /* This is to guard against environments where there can be more than 128
     handles open at any given time.  `tar' currently thinks that any handle
     above 128 belongs to a remote archive.  */
  if (!_remdev (archive_name_array[0]) && _isrmt (archive))
    {
      int saved_errno = errno;

      if (backed_up_flag)
	undo_last_backup ();
      FATAL_ERROR ((0, saved_errno, _("Too many open files, cannot open %s"),
		    archive_name_array[0]));
    }

#if DOSWIN
  /* When we are creating an archive, DOS won't assign it disk space until we
     actually write something.  DJGPP uses the number of the first disk block
     as the simulated inode number, which will only work if disk space is
     already allocated for this file.  Otherwise, the ar_ino trick below (and
     in create.c) won't work.  So we need to write something and fsync the
     handle, to get meaningful ar_ino.  */

  if (!isatty (archive) && wanted_access == ACCESS_WRITE && !_isrmt (archive))
    {
      write (archive, "xyzzy", 5);
      fsync (archive);
      lseek (archive, 0L, SEEK_SET); /* get back to the beginning */
    }
#endif

  /* Detect if outputting to "/dev/null".  */
  if (rmtfstat (archive, &archive_stat) == 0)
    {
      struct stat dev_null_stat;

      stat ("/dev/null", &dev_null_stat);
      dev_null_output = (S_ISCHR (archive_stat.st_mode)
#if DOSWIN
			 /* No st_rdev in DOSWIN.  All character devices
			    `live' on the same st_dev, so check st_ino also.  */
			 && archive_stat.st_dev == dev_null_stat.st_dev
			 && archive_stat.st_ino == dev_null_stat.st_ino
#else
			 && archive_stat.st_rdev == dev_null_stat.st_rdev
#endif
			 );
    }
  /* else what?  FIXME!  */

  if (!_isrmt (archive) && S_ISREG (archive_stat.st_mode))
    {
      ar_dev = archive_stat.st_dev;
      ar_ino = archive_stat.st_ino;
    }

#if DOSWIN
  if (!isatty (archive))
    setmode (archive, O_BINARY);
#endif

  switch (wanted_access)
    {
    case ACCESS_READ:
    case ACCESS_UPDATE:
      record_end = record_start; /* set up for 1st record = # 0 */
      find_next_block ();	/* read it in, check for EOF */

      if (volume_label_option)
	{
	  union block *label = find_next_block ();

	  if (!label)
	    FATAL_ERROR ((0, 0, _("Archive not labelled to match `%s'"),
			  volume_label_option));
	  if (!check_label_pattern (label))
	    FATAL_ERROR ((0, 0, _("Volume `%s' does not match `%s'"),
			  label->header.name, volume_label_option));
	}
      break;

    case ACCESS_WRITE:
      if (volume_label_option)
	{
	  memset ((void *) record_start, 0, BLOCKSIZE);
	  if (multi_volume_option)
	    sprintf (record_start->header.name, "%s Volume 1",
		     volume_label_option);
	  else
	    strcpy (record_start->header.name, volume_label_option);

	  assign_string (&current.name, record_start->header.name);

	  record_start->header.typeflag = OLDGNU_VOLHDR;
	  set_header_mtime (record_start, time (0));
	  finish_header (record_start);
#if 0
	  current_block++;
#endif
	}
      break;
    }
}

/*--------------------------------------.
| Perform a write to flush the buffer.  |
`--------------------------------------*/

void
flush_write (void)
{
  int copy_back;
  ssize_t size_written;

  if (checkpoint_option && ++checkpoint % 10 == 0)
    output_checkpoint_mark ();

  if (!zerop_tarlong (tape_length_option)
      && !lessp_tarlong (bytes_written, tape_length_option))
    {
      errno = ENOSPC;		/* FIXME: errno should be read-only */
      size_written = 0;
    }
  else if (dev_null_output)
    size_written = record_size;
  else
    size_written = rmtwrite (archive, record_start->buffer, record_size);

  if (size_written != record_size && !multi_volume_option)
    write_error (size_written);
  else if (totals_option)
    add_to_tarlong (total_written, record_size);

  if (size_written > 0)
    add_to_tarlong (bytes_written, size_written);

  if (size_written == record_size)
    {
      if (multi_volume_option)
	{
	  char *cursor;

	  if (!save_name)
	    {
	      real_s_name[0] = '\0';
	      real_s_totsize = 0;
	      real_s_sizeleft = 0;
	      return;
	    }

	  cursor = save_name;
#if DOSWIN
	  if (cursor[0] >= 'A' && cursor[0] <= 'z' && cursor[1] == ':')
	    cursor += 2;
#endif
	  while (*cursor == '/')
	    cursor++;

	  strcpy (real_s_name, cursor);
	  real_s_totsize = save_totsize;
	  real_s_sizeleft = save_sizeleft;
	}
      return;
    }

  /* We're multivol.  Panic if we didn't get the right kind of response.  */

  /* ENXIO is for the UNIX PC.  */
  if (size_written < 0 && errno != ENOSPC && errno != EIO && errno != ENXIO)
    write_error (size_written);

  /* If error indicates a short write, we just move to the next tape.  */

  if (!new_volume (ACCESS_WRITE))
    return;

  clear_tarlong (bytes_written);

  if (volume_label_option && real_s_name[0])
    {
      copy_back = 2;
      record_start -= 2;
    }
  else if (volume_label_option || real_s_name[0])
    {
      copy_back = 1;
      record_start--;
    }
  else
    copy_back = 0;

  if (volume_label_option)
    {
      memset ((void *) record_start, 0, BLOCKSIZE);
      sprintf (record_start->header.name, "%s Volume %d",
	       volume_label_option, volno);
      set_header_mtime (record_start, time (0));
      record_start->header.typeflag = OLDGNU_VOLHDR;
      finish_header (record_start);
    }

  if (real_s_name[0])
    {
      int saved_verbose_option;

      if (volume_label_option)
	record_start++;

      memset ((void *) record_start, 0, BLOCKSIZE);

      /* FIXME: Michael P Urban writes: [a long name file] is being written
	 when a new volume rolls around [...]  Looks like the wrong value is
	 being preserved in real_s_name, though.  */

      strcpy (record_start->header.name, real_s_name);
      record_start->header.typeflag = OLDGNU_MULTIVOL;
      set_header_size (record_start, real_s_sizeleft);
      set_header_offset (record_start, real_s_totsize - real_s_sizeleft);
      saved_verbose_option = verbose_option;
      verbose_option = 0;
      finish_header (record_start);
      verbose_option = saved_verbose_option;

      if (volume_label_option)
	record_start--;
    }

  size_written = rmtwrite (archive, record_start->buffer, record_size);
  if (size_written != record_size)
    write_error (size_written);
  else if (totals_option)
    add_to_tarlong (total_written, record_size);

  add_to_tarlong (bytes_written, record_size);
  if (copy_back != 0)
    {
      record_start += copy_back;
      memcpy ((void *) current_block,
	      (void *) (record_start + blocking_factor - copy_back),
	      (size_t) (copy_back * BLOCKSIZE));
      current_block += copy_back;

      if (real_s_sizeleft >= copy_back * BLOCKSIZE)
	real_s_sizeleft -= copy_back * BLOCKSIZE;
      else if ((real_s_sizeleft + BLOCKSIZE - 1) / BLOCKSIZE <= copy_back)
	real_s_name[0] = '\0';
      else
	{
	  char *cursor = save_name;

#if DOSWIN
	  if (cursor[0] >= 'A' && cursor[0] <= 'z' && cursor[1] == ':')
	    cursor += 2;
#endif
	  while (*cursor == '/')
	    cursor++;

	  strcpy (real_s_name, cursor);
	  real_s_sizeleft = save_sizeleft;
	  real_s_totsize = save_totsize;
	}
      copy_back = 0;
    }
}

/*---------------------------------------------------------------------.
| Handle write errors on the archive.  Write errors are always fatal.  |
| Hitting the end of a volume does not cause a write error unless the  |
| write was the first record of the volume.			       |
`---------------------------------------------------------------------*/

static void
write_error (ssize_t size_written)
{
  int saved_errno = errno;

  /* It might be useful to know how much was written before the error
     occurred.  Beware that mere printing maybe change errno value.  */
  if (totals_option)
    print_total_written ();

  if (size_written < 0)
    FATAL_ERROR ((0, saved_errno, _("Cannot write to %s"),
		  *archive_name_cursor));
  else
    FATAL_ERROR ((0, 0, _("Only wrote %lu of %lu bytes to %s"),
		  (unsigned long) size_written, (unsigned long) record_size,
		  *archive_name_cursor));
}

/*-------------------------------------------------------------------.
| Handle read errors on the archive.  If the read should be retried, |
| returns to the caller.					     |
`-------------------------------------------------------------------*/

static void
read_error (void)
{
  WARN ((0, errno, _("Read error on %s"), *archive_name_cursor));

  if (record_start_block == 0)
    FATAL_ERROR ((0, 0, _("At beginning of tape, quitting now")));

  /* Read error in mid archive.  We retry up to READ_ERROR_MAX times and
     then give up on reading the archive.  */

  if (read_error_count++ > READ_ERROR_MAX)
    FATAL_ERROR ((0, 0, _("Too many errors, quitting")));
  return;
}

/*-------------------------------------.
| Perform a read to flush the buffer.  |
`-------------------------------------*/

void
flush_read (void)
{
  size_t size;
  ssize_t size_read;
  ssize_t size_written;
  char *buffer_cursor;			/* pointer to next byte to read */

  if (checkpoint_option && ++checkpoint % 10 == 0)
    output_checkpoint_mark ();

  /* Clear the count of errors.  This only applies to a single call to
     flush_read.  */

  read_error_count = 0;		/* clear error count */

  if (write_archive_to_stdout && record_start_block != 0)
    {
      size_written = rmtwrite (1, record_start->buffer, record_size);
      if (size_written != record_size)
	write_error (size_written);
    }

  if (multi_volume_option)
    if (save_name)
      {
	char *cursor = save_name;

#if DOSWIN
	if (cursor[0] >= 'A' && cursor[0] <= 'z' && cursor[1] == ':')
	  cursor += 2;
#endif
	while (*cursor == '/')
	  cursor++;

	strcpy (real_s_name, cursor);
	real_s_sizeleft = save_sizeleft;
	real_s_totsize = save_totsize;
      }
    else
      {
	real_s_name[0] = '\0';
	real_s_totsize = 0;
	real_s_sizeleft = 0;
      }

error_loop:
  size_read = rmtread (archive, record_start->buffer, record_size);
  if (size_read == record_size)
    return;

  if ((size_read == 0
       || (size_read < 0 && errno == ENOSPC)
       || (size_read > 0 && !read_full_records_option))
      && multi_volume_option)
    {
      union block *cursor;

    try_volume:
      switch (subcommand_option)
	{
	case APPEND_SUBCOMMAND:
	case CAT_SUBCOMMAND:
	case UPDATE_SUBCOMMAND:
	  if (!new_volume (ACCESS_UPDATE))
	    return;
	  break;

	default:
	  if (!new_volume (ACCESS_READ))
	    return;
	  break;
	}

    vol_error:
      size_read = rmtread (archive, record_start->buffer, record_size);
      if (size_read < 0)
	{
	  read_error ();
	  goto vol_error;
	}
      if (size_read != record_size)
	goto short_read;

      cursor = record_start;

      if (cursor->header.typeflag == OLDGNU_VOLHDR)
	{
	  if (volume_label_option)
	    {
	      if (!check_label_pattern (cursor))
		{
		  WARN ((0, 0, _("Volume `%s' does not match `%s'"),
			 cursor->header.name, volume_label_option));
		  volno--;
		  global_volno--;
		  goto try_volume;
		}
	    }
	  if (verbose_option)
	    {
	      if (checkpoint_option)
		flush_checkpoint_line ();
	      fprintf (stdlis, _("Reading %s\n"), cursor->header.name);
	    }
	  cursor++;
	}
      else if (volume_label_option)
	WARN ((0, 0, _("WARNING: No volume header")));

      if (real_s_name[0])
	{
	  if (cursor->header.typeflag != OLDGNU_MULTIVOL
	      || strcmp (cursor->header.name, real_s_name))
	    {
	      WARN ((0, 0, _("%s is not continued on this volume"),
		     real_s_name));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  if (real_s_totsize
	      != get_header_size (cursor) + get_header_offset (cursor))
	    {
	      WARN ((0, 0, _("%s is the wrong size (%lu != %lu + %lu)"),
		     cursor->header.name,
		     (unsigned long) save_totsize,
		     (unsigned long) get_header_size (cursor),
		     (unsigned long) get_header_offset (cursor)));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  if (real_s_totsize != real_s_sizeleft + get_header_offset (cursor))
	    {
	      WARN ((0, 0, _("This volume is out of sequence")));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  cursor++;
	}
      current_block = cursor;
      return;
    }
  else if (size_read < 0)
    {
      read_error ();
      goto error_loop;		/* try again */
    }

short_read:
  buffer_cursor = record_start->buffer + size_read;
  size = record_size - size_read;

again:
  if (size % BLOCKSIZE == 0)
    {
      /* FIXME: for size=0, multi-volume support.  On the first record, warn
	 about the problem.  */

      if (!read_full_records_option && verbose_option
	  && record_start_block == 0 && size_read > 0)
	WARN ((0, 0, _("\
Record size of archive appears to be %lu blocks (%lu expected)"),
	       (unsigned long) (size_read / BLOCKSIZE),
	       (unsigned long) (record_size / BLOCKSIZE)));

      record_end = record_start + (record_size - size) / BLOCKSIZE;
      return;
    }

  if (read_full_records_option)
    {
      /* User warned us about this.  Fix up.  */
      /* FIXME: YA example of those useless comments...  Grrr!  */

      if (size)
	{
	  size_read = rmtread (archive, buffer_cursor, size);
	  while (size_read < 0)
	    {
	      read_error ();
	      size_read = rmtread (archive, buffer_cursor, size);
	    }
	  if (size_read == 0)
	    FATAL_ERROR ((0, 0,
			  _("Archive %s end of file not on block boundary"),
			  *archive_name_cursor));
	  size -= size_read;
	  buffer_cursor += size_read;
	  goto again;
	}
    }
  else
    FATAL_ERROR ((0, 0, _("Only read %lu bytes from archive %s"),
		  (unsigned long) size_read, *archive_name_cursor));
}

/*-----------------------------------------------.
| Flush the current buffer to/from the archive.	 |
`-----------------------------------------------*/

void
flush_archive (void)
{
  record_start_block += record_end - record_start;
  current_block = record_start;
  record_end = record_start + blocking_factor;

  if (access_mode == ACCESS_READ && time_to_start_writing)
    {
      access_mode = ACCESS_WRITE;
      time_to_start_writing = false;

      if (file_to_switch_to >= 0)
	{
	  int status = rmtclose (archive);

	  if (status < 0)
	    WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
		   *archive_name_cursor, archive, status));

	  archive = file_to_switch_to;
	}
      else
	backspace_output ();
    }

  switch (access_mode)
    {
    case ACCESS_READ:
      flush_read ();
      break;

    case ACCESS_WRITE:
      flush_write ();
      break;

    case ACCESS_UPDATE:
      abort ();
    }
}

/*-------------------------------------------------------------------------.
| Backspace the archive descriptor by one record worth.  If its a tape,	   |
| MTIOCTOP will work.  If its something else, we try to seek on it.  If we |
| can't seek, we lose!							   |
`-------------------------------------------------------------------------*/

static void
backspace_output (void)
{
#ifdef MTIOCTOP
  {
    struct mtop operation;

    operation.mt_op = MTBSR;
    operation.mt_count = 1;
    if (rmtioctl (archive, MTIOCTOP, (char *) &operation) >= 0)
      return;
    if (errno == EIO && rmtioctl (archive, MTIOCTOP, (char *) &operation) >= 0)
      return;
  }
#endif

  {
    off_t position = rmtlseek (archive, 0L, SEEK_CUR);

    /* Seek back to the beginning of this record and start writing there.  */

    position -= record_size;
    if (rmtlseek (archive, position, SEEK_SET) != position)
      {
	/* Lseek failed.  Try a different method.  */

	WARN ((0, 0, _("\
Could not backspace archive file; it may be unreadable without -i")));

	/* Replace the first part of the record with NULs.  */

	if (record_start->buffer != output_start)
	  memset (record_start->buffer, 0,
		  (size_t) (output_start - record_start->buffer));
      }
  }
}

/*-------------------------.
| Close the archive file.  |
`-------------------------*/

void
close_archive (void)
{
  if (checkpoint_option)
    flush_checkpoint_line ();

  if (time_to_start_writing || access_mode == ACCESS_WRITE)
    flush_archive ();

#if !DOSWIN

  /* Manage to fully drain a pipe we might be reading, so to not break it on
     the producer after the EOF block.  FIXME: one of these days, tar might
     become clever enough to just stop working, once there is no more work to
     do, we might have to revise this area in such time.  */

  if (access_mode == ACCESS_READ && S_ISFIFO (archive_stat.st_mode))
    while (rmtread (archive, record_start->buffer, record_size) > 0)
      continue;

#endif /* not DOSWIN */

  if (subcommand_option == DELETE_SUBCOMMAND)
    {
      off_t position = rmtlseek (archive, 0L, SEEK_CUR);

      ftruncate (archive, position);
      /* rmtwrite (archive, "", 0);  -- for Turbo-C ?  */
    }

  /* FIXME: Does verifying really make sense after extraction?  */

  if (verify_option)
    verify_volume ();

#if DOSWIN

  if (dos_pipe.pipe)
    {
      int status = pclose (dos_pipe.pipe);

      if (status == -1 || (status & 0xff) == 0xff)
	FATAL_ERROR ((0, errno, _("Cannot exec %s"),
		      use_compress_program_option));
      else if (status)
	ERROR ((0, 0, _("Child returned status %d"), (status & 0xff)));

      if (dos_pipe.tfile && access_mode == ACCESS_WRITE)
	{
	  /* We need to copy the temporary file to the remote
	     archive.  */

	  int tem_fd = open (archive_name_array[0], O_RDONLY | O_BINARY, 0666);
	  int remote_archive;

	  if (tem_fd < 0)
	    FATAL_ERROR ((0, errno, _("Cannot open archive %s"),
			  archive_name_array[0]));

	  remote_archive = rmtcreat (archive_name_array[0], 0666,
				     rsh_command_option);
	  if (remote_archive < 0)
	    FATAL_ERROR ((0, errno, _("Cannot open archive %s"),
			  archive_name_array[0]));

	  while (1)
	    {
	      char *cursor;
	      int length;

	      status = 0;
	      /* Assemble a record.  */

	      for (length = 0, cursor = record_start->buffer;
		   length < record_size;
		   length += status, cursor += status)
		{
		  int size = record_size - length;

		  if (size < BLOCKSIZE)
		    size = BLOCKSIZE;
		  status = read (tem_fd, cursor, (size_t) size);
		  if (status <= 0)
		    break;
		}

	      if (status < 0)
		FATAL_ERROR ((0, errno, _("\
Cannot read from compression program")));

	      /* Copy the record.  */

	      if (status == 0)
		{
		  /* We hit the end of the file.  Write last record at
		     full length, as the only role of the grandchild is
		     doing proper reblocking.  */

		  if (length > 0)
		    {
		      memset (record_start->buffer + length, 0,
			      (size_t) record_size - length);
		      status = rmtwrite (remote_archive, record_start->buffer,
					 (unsigned int) record_size);
		      if (status != record_size)
			write_error (status);
		    }

		  /* There is nothing else to read, break out.  */
		  break;
		}

	      status = rmtwrite (remote_archive, record_start->buffer,
				 (unsigned int) record_size);
	      if (status != record_size)
		write_error (status);
	    }
	  close (tem_fd);
	}

      dos_pipe.pipe = NULL;
      if (dos_pipe.tfile)
	{
	  remove (dos_pipe.tfile);
	  free (dos_pipe.tfile);
	  dos_pipe.tfile = NULL;
	}

      if (status < 0)
	{
	  if (access_mode == ACCESS_WRITE)
	    FATAL_ERROR ((0, errno, _("Cannot write to compression program")));
	  else
	    FATAL_ERROR ((0, errno,
			  _("Cannot read from compression program")));
	}
    }
  else

#endif /* DOSWIN */

    {
#if HAVE_PIPE

      {
	int status = rmtclose (archive);

	if (status < 0)
	  WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
		 *archive_name_cursor, archive, status));
      }

      if (child_pid)
	{
	  WAIT_T wait_status;
	  pid_t child;

	  /* Loop waiting for the right child to die, or for no more kids.  */

	  while ((child = wait (&wait_status), child != child_pid)
		 && child != -1)
	    continue;

	  if (child != -1)
	    if (WIFSIGNALED (wait_status)
#if 0
		&& !WIFSTOPPED (wait_status)
#endif
		)
	      {
		/* SIGPIPE is OK, everything else is a problem.  */

		if (WTERMSIG (wait_status) != SIGPIPE)
		  ERROR ((0, 0, _("Child died with signal %d%s"),
			  WTERMSIG (wait_status),
			  WCOREDUMP (wait_status) ? _(" (core dumped)") : ""));
	      }
	    else
	      {
		/* Child voluntarily terminated -- but why?  /bin/sh returns
		   SIGPIPE + 128 if its child, then do nothing.  */

		if (WEXITSTATUS (wait_status) != (SIGPIPE + 128)
		    && WEXITSTATUS (wait_status))
		  ERROR ((0, 0, _("Child returned status %d"),
			  WEXITSTATUS (wait_status)));
	      }
	}

#else /* not HAVE_PIPE */

      if (child_pid)
	{
	  int status = pclose (archive_popened_file);

	  if (status != 0)
	    ERROR ((0, errno, _("Error while executing `%s'"),
		    use_compress_program_option));

	  child_pid = 0;
	}
      else
	{
	  int status = rmtclose (archive);

	  if (status < 0)
	    WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
		   *archive_name_cursor, archive, status));
	}

#endif /* not HAVE_PIPE */
    }

  free (current.name);
  free (current.linkname);

  if (save_name)
    free (save_name);

  free (multi_volume_option ? record_start - 2 : record_start);
}

/*------------------------------------------------.
| Called to initialize the global volume number.  |
`------------------------------------------------*/

void
init_volume_number (void)
{
  FILE *file = fopen (volno_file_option, "r");

  if (file)
    {
      fscanf (file, "%d", &global_volno);
      if (fclose (file) == EOF)
	ERROR ((0, errno, "%s", volno_file_option));
    }
  else if (errno != ENOENT)
    ERROR ((0, errno, "%s", volno_file_option));
}

/*-------------------------------------------------------.
| Called to write out the closing global volume number.	 |
`-------------------------------------------------------*/

void
closeout_volume_number (void)
{
  FILE *file = fopen (volno_file_option, "w");

  if (file)
    {
      fprintf (file, "%d\n", global_volno);
      if (fclose (file) == EOF)
	ERROR ((0, errno, "%s", volno_file_option));
    }
  else
    ERROR ((0, errno, "%s", volno_file_option));
}

/*-----------------------------------------------------------------------.
| We've hit the end of the old volume.  Close it and open the next one.	 |
| Return true on success.						 |
`-----------------------------------------------------------------------*/

static bool
new_volume (enum access_mode wanted_access)
{
  static bool looped = false;
  int status;

  if (now_verifying)
    return false;

  if (verify_option)
    verify_volume ();

#if DOSWIN
  /* Closing stdout or stdin is a bad idea.  */
  if (archive != STDIN && archive != STDOUT)
#endif
    if (status = rmtclose (archive), status < 0)
      WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
	     *archive_name_cursor, archive, status));

  global_volno++;
  volno++;
  archive_name_cursor++;
  if (archive_name_cursor == archive_name_array + archive_names)
    {
      archive_name_cursor = archive_name_array;
      looped = true;
    }

tryagain:
  if (looped)
    {
      /* We have to prompt from now on.  */

      if (info_script_option)
	{
	  /* FIXME: More flexibility is needed, if not more thought.  The info
	     script is currently not called on the first run of initial tapes.
	     Neither the saving of volume number.  I suspect this is a bit
	     rotten.  Strange that nobody complained...  */

	  if (volno_file_option)
	    closeout_volume_number ();
	  if (system (info_script_option) != 0)
	    FATAL_ERROR ((0, errno,
			  _("The info script did not complete successfully")));
	}
      else
	while (true)
	  {
	    char message[80];
	    char input_buffer[80];

	    sprintf (message,
		     _("\007Prepare volume #%d for %s and hit return: "),
		     global_volno, *archive_name_cursor);

	    if (!get_reply (message, input_buffer, sizeof (input_buffer)))
	      {
		fprintf (stderr,
			 _("End of file where user reply was expected"));

		if (subcommand_option != EXTRACT_SUBCOMMAND
		    && subcommand_option != LIST_SUBCOMMAND
		    && subcommand_option != DIFF_SUBCOMMAND)
		  WARN ((0, 0, _("WARNING: Archive is incomplete")));

		exit (TAREXIT_FAILURE);
	      }
	    if (input_buffer[0] == '\n'
		|| input_buffer[0] == 'y'
		|| input_buffer[0] == 'Y')
	      break;

	    switch (input_buffer[0])
	      {
	      case '?':
		{
		  fprintf (stderr, _("\
 n [name]   Give a new file name for the next (and subsequent) volume(s)\n\
 q          Abort tar\n\
 !          Spawn a subshell\n\
 ?          Print this list\n"));
		}
		break;

	      case 'q':
		/* Quit.  */
		fprintf (stdlis, _("No new volume; exiting.\n"));

		if (subcommand_option != EXTRACT_SUBCOMMAND
		    && subcommand_option != LIST_SUBCOMMAND
		    && subcommand_option != DIFF_SUBCOMMAND)
		  WARN ((0, 0, _("WARNING: Archive is incomplete")));

		exit (TAREXIT_FAILURE);

	      case 'n':
		/* Get new file name.  */
		{
		  char *name = &input_buffer[1];
		  char *cursor;

		  while (*name == ' ' || *name == '\t')
		    name++;
		  cursor = name;
		  while (*cursor && *cursor != '\n')
		    cursor++;
		  *cursor = '\0';

		  /* FIXME: the following allocation is never reclaimed.  */
		  *archive_name_cursor = xstrdup (name);
		}
		break;

	      case '!':
#if DOSWIN
		{
		  int save_stdout = -1, save_stdin = -1;

		  /* Exec'ing a child shell with standard steams
		     not connected to the terminal is a VERY bad idea.  */
		  if (!isatty (STDIN))
		    {
		      save_stdin = dup (STDIN);
		      freopen (CONSOLE, "rt", stdin);
		    }
		  if (!isatty (STDOUT))
		    {
		      save_stdout = dup (STDOUT);
		      freopen (CONSOLE, "wt", stdout);
		    }

		  /* `system' is better, since it honors $SHELL.  */
		  if (system ("") == -1)
		    FATAL_ERROR ((0, errno, _("Cannot exec a shell %s"), ""));

		  /* Restore any redirected standard streams.  */
		  if (save_stdin >= 0)
		    dup2 (save_stdin, STDIN);
		  if (save_stdout >= 0)
		    dup2 (save_stdout, STDOUT);
		}

#else /* not DOSWIN */

		switch (fork ())
		  {
		  case -1:
		    WARN ((0, errno, _("Cannot fork process")));
		    break;

		  case 0:
		    {
		      const char *shell = getenv ("SHELL");

		      if (shell == NULL)
			shell = "/bin/sh";
		      execlp (shell, "-sh", "-i", 0);
		      FATAL_ERROR ((0, errno, _("Cannot exec a shell %s"),
				    shell));
		    }

		  default:
		    {
		      WAIT_T wait_status;

		      wait (&wait_status);
		    }
		    break;
		  }

		/* FIXME: I'm not sure if that's all that has to be done
		   here.  (jk)  */

#endif /* not DOSWIN */

		break;
	      }
	  }
    }

  if (verify_option)
    archive = rmtopen (*archive_name_cursor, O_RDWR | O_CREAT, (mode_t) 0666,
		       rsh_command_option);
  else
    switch (wanted_access)
      {
      case ACCESS_READ:
	archive = rmtopen (*archive_name_cursor, O_RDONLY, (mode_t) 0666,
			   rsh_command_option);
	break;

      case ACCESS_WRITE:
	if (backup_option)
	  maybe_backup_file (*archive_name_cursor, true);
	archive = rmtcreat (*archive_name_cursor, (mode_t) 0666,
			    rsh_command_option);
	break;

      case ACCESS_UPDATE:
	archive = rmtopen (*archive_name_cursor, O_RDWR | O_CREAT,
			   (mode_t) 0666, rsh_command_option);
	break;
      }

  if (archive < 0)
    {
      WARN ((0, errno, _("Cannot open %s"), *archive_name_cursor));
      if (!verify_option && wanted_access == ACCESS_WRITE && backup_option)
	undo_last_backup ();
      goto tryagain;
    }

#if DOSWIN
  if (!isatty (archive))
    setmode (archive, O_BINARY);
#endif

  return true;
}
