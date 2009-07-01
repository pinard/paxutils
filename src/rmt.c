/* Remote connection server.
   Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

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

/* Copyright (C) 1983 Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms are permitted provided
   that the above copyright notice and this paragraph are duplicated in all
   such forms and that any documentation, advertising materials, and other
   materials related to such distribution and use acknowledge that the
   software was developed by the University of California, Berkeley.  The
   name of the University may not be used to endorse or promote products
   derived from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
   MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.  */

#include "system.h"

#include <sys/socket.h>

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif

/* File descriptors for standard input and standard output.  */
#define INPUT 0
#define OUTPUT 1

/* Maximum size of a string from the requesting program.  */
#define	STRING_SIZE 64

/* Name of executing program.  */
const char *program_name;

/* File descriptor of the tape device, or negative if none open.  */
static int tape = -1;

/* Buffer containing transferred data, and its allocated size.  */
static char *record_buffer = NULL;
static size_t allocated_size = 0;

/* Buffer for constructing the reply.  */
static char reply_buffer[BUFSIZ];

/* Debugging tools.  */

static FILE *debug_file = NULL;

#define	DEBUG(File) \
  if (debug_file) fprintf(debug_file, File)

#define	DEBUG1(File, Arg) \
  if (debug_file) fprintf(debug_file, File, Arg)

#define	DEBUG2(File, Arg1, Arg2) \
  if (debug_file) fprintf(debug_file, File, Arg1, Arg2)

/*------------------------------------------------.
| Return an error string, given an error number.  |
`------------------------------------------------*/

#if HAVE_STRERROR
# ifndef strerror
char *strerror ();
# endif
#else
static const char *
private_strerror (int errnum)
{
  extern const char *const sys_errlist[];
  extern int sys_nerr;

  if (errnum > 0 && errnum <= sys_nerr)
    return sys_errlist[errnum];
  return N_("Unknown system error");
}
# define strerror private_strerror
#endif

/*---.
| ?  |
`---*/

static void
return_response (int status)
{
  DEBUG1 ("rmtd: A %d\n", status);
  sprintf (reply_buffer, "A%d\n", status);
  full_write (OUTPUT, reply_buffer, strlen (reply_buffer));
}

/*---.
| ?  |
`---*/

static void
return_error_message (const char *string)
{
  DEBUG1 ("rmtd: E 0 (%s)\n", string);
  sprintf (reply_buffer, "E0\n%s\n", string);
  full_write (OUTPUT, reply_buffer, strlen (reply_buffer));
}

/*---.
| ?  |
`---*/

static void
return_numbered_error (int num)
{
  DEBUG2 ("rmtd: E %d (%s)\n", num, strerror (num));
  sprintf (reply_buffer, "E%d\n%s\n", num, strerror (num));
  full_write (OUTPUT, reply_buffer, strlen (reply_buffer));
}

/*---.
| ?  |
`---*/

static void
get_string (char *string)
{
  int counter;

  for (counter = 0; counter < STRING_SIZE; counter++)
    {
      if (full_read (INPUT, string + counter, 1) != 1)
	exit (EXIT_SUCCESS);

      if (string[counter] == '\n')
	break;
    }
  string[counter] = '\0';
}

/*---.
| ?  |
`---*/

static void
prepare_record_buffer (size_t size)
{
  if (size <= allocated_size)
    return;

  if (record_buffer)
    free (record_buffer);

  record_buffer = malloc (size);

  if (record_buffer == NULL)
    {
      return_error_message (N_("Cannot allocate buffer space"));
      /* Exit status used to be 4.  */
      exit (EXIT_FAILURE);
    }

  allocated_size = size;

#ifdef SO_RCVBUF
  /* FIXME: Should &size be (int *) ?  */
  while (size > 1024 &&
   setsockopt (INPUT, SOL_SOCKET, SO_RCVBUF, (char *) &size, sizeof (size)) < 0)
    size -= 1024;
#else
  /* FIXME: I do not see any purpose to the following line...  Sigh! */
  size = 1 + ((size - 1) % 1024);
#endif
}

/*---.
| ?  |
`---*/

int
main (int argc, char *const *argv)
{
  char command;

  /* FIXME: Localisation is meaningless, unless --help and --version are
     locally used.  Localisation would be best accomplished by the calling
     tar, on messages found within error packets.  */

#if DOSWIN
  program_name = get_program_base_name (argv[0]);
#else
  program_name = argv[0];
#endif

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* FIXME: Implement --help and --version as per GNU standards.  */

  argc--, argv++;
  if (argc > 0)
    {
      debug_file = fopen (*argv, "w");
      if (debug_file == 0)
	{
	  return_numbered_error (errno);
	  exit (EXIT_FAILURE);
	}
      setbuf (debug_file, NULL);
    }

  while (full_read (INPUT, &command, 1) == 1)
    {
      errno = 0;		/* FIXME: errno should be read-only */

      switch (command)
	{
	  /* FIXME: Maybe 'H' and 'V' for --help and --version output?  */

	case 'O':
	  {
	    char device_string[STRING_SIZE];
	    char mode_string[STRING_SIZE];

	    get_string (device_string);
	    get_string (mode_string);
	    DEBUG2 ("rmtd: O %s %s\n", device_string, mode_string);

	    if (tape >= 0)
	      close (tape);

#if defined (i386) && defined (AIX)

	    /* This is alleged to fix a byte ordering problem.  I'm quite
	       suspicious if it's right. -- mib.  */

	    {
	      mode_t old_mode = atoi (mode_string);
	      mode_t new_mode = 0;

	      if ((old_mode & 3) == 0)
		new_mode |= O_RDONLY;
	      if (old_mode & 1)
		new_mode |= O_WRONLY;
	      if (old_mode & 2)
		new_mode |= O_RDWR;
	      if (old_mode & 0x0008)
		new_mode |= O_APPEND;
	      if (old_mode & 0x0200)
		new_mode |= O_CREAT;
	      if (old_mode & 0x0400)
		new_mode |= O_TRUNC;
	      if (old_mode & 0x0800)
		new_mode |= O_EXCL;
	      tape = open (device_string, new_mode, 0666);
	    }
#else
	    tape = open (device_string, atoi (mode_string), 0666);
#endif
	    if (tape < 0)
	      return_numbered_error (errno);
	    else
	      return_response (0);
	    break;
	  }

	case 'C':
	  {
	    char device_string[STRING_SIZE];

	    get_string (device_string); /* discard */
	    DEBUG ("rmtd: C\n");

	    if (close (tape) < 0)
	      return_numbered_error (errno);
	    else
	      {
		tape = -1;
		return_response (0);
	      }
	    break;
	  }

	case 'L':
	  {
	    int status = 0;
	    char count_string[STRING_SIZE];
	    char position_string[STRING_SIZE];

	    get_string (count_string);
	    get_string (position_string);
	    DEBUG2 ("rmtd: L %s %s\n", count_string, position_string);

	    status = lseek (tape, (off_t) atol (count_string),
			    atoi (position_string));
	    if (status < 0)
	      return_numbered_error (errno);
	    else
	      return_response (status);
	    break;
	  }

	case 'W':
	  {
	    char count_string[STRING_SIZE];
	    ssize_t size_read = 0;
	    ssize_t size_written;
	    size_t counter;
	    int size;

	    get_string (count_string);
	    size = atoi (count_string);
	    DEBUG1 ("rmtd: W %s\n", count_string);

	    prepare_record_buffer (size);
	    for (counter = 0; counter < size; counter += size_read)
	      {
		size_read
		  = full_read (INPUT, record_buffer + counter, size - counter);
		if (size_read <= 0)
		  {
		    return_error_message (N_("Premature end of file"));
		    /* Exit status used to be 2.  */
		    exit (EXIT_FAILURE);
		  }
	      }
	    size_written = full_write (tape, record_buffer, size);
	    if (size_written < 0)
	      return_numbered_error (errno);
	    else
	      return_response (size_written);
	    break;
	  }

	case 'R':
	  {
	    char count_string[STRING_SIZE];
	    size_t size;
	    ssize_t read_size;

	    get_string (count_string);
	    DEBUG1 ("rmtd: R %s\n", count_string);

	    size = atoi (count_string);
	    prepare_record_buffer (size);
	    read_size = full_read (tape, record_buffer, size);
	    if (read_size < 0)
	      return_numbered_error (errno);
	    else
	      {
		sprintf (reply_buffer, "A%d\n", read_size);
		full_write (OUTPUT, reply_buffer, strlen (reply_buffer));
		full_write (OUTPUT, record_buffer, read_size);
	      }
	    break;
	  }

	case 'I':
	  {
	    int status = 0;
	    char operation_string[STRING_SIZE];
	    char count_string[STRING_SIZE];

	    get_string (operation_string);
	    get_string  (count_string);
	    DEBUG2 ("rmtd: I %s %s\n", operation_string, count_string);

#ifdef MTIOCTOP
	    {
	      struct mtop mtop;

	      mtop.mt_op = atoi (operation_string);
	      mtop.mt_count = atoi (count_string);
	      if (ioctl (tape, MTIOCTOP, (char *) &mtop) < 0)
		{
		  return_numbered_error (errno);
		  break;
		}
	      status = mtop.mt_count;
	    }
#endif
	    return_response (status);
	    break;
	  }

	case 'S':
	  {
	    int status = 0;

	    DEBUG ("rmtd: S\n");

#ifdef MTIOCGET
	    {
	      struct mtget operation;

	      if (ioctl (tape, MTIOCGET, (char *) &operation) < 0)
		{
		  return_numbered_error (errno);
		  break;
		}
	      status = sizeof (operation);
	      sprintf (reply_buffer, "A%d\n", status);
	      full_write (OUTPUT, reply_buffer, strlen (reply_buffer));
	      full_write (OUTPUT, (char *) &operation, sizeof (operation));
	    }
#endif
	    /* FIXME: No reply at all, here?  */
	    break;
	  }

	default:
	  DEBUG1 (_("rmtd: Garbage command %c\n"), command);
	  return_error_message (N_("Garbage command"));
	  /* Exit status used to be 3.  */
	  exit (EXIT_FAILURE);
	}
    }

  exit (EXIT_SUCCESS);
}
