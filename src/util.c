/* util.c - Several utility routines for cpio.
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

#include <stdio.h>
#include <sys/types.h>
#ifdef HPUX_CDF
#include <sys/stat.h>
#endif
#ifndef __MSDOS__
#include <sys/ioctl.h>
#else
#include <io.h>
#endif

#include "common.h"
#include "rmt.h"

static void tape_fill_input_buffer PARAMS ((int in_des, int num_bytes));
static int disk_fill_input_buffer PARAMS ((int in_des, int num_bytes));
static void hash_insert ();
static void write_nuls_to_file PARAMS ((long num_bytes, int out_des));
static int sparse_write PARAMS ((int fildes, char *buf, unsigned int nbyte));

/*------------------------------------------------------------------------.
| Initialize the input and output buffers to their proper size and        |
| initialize all variables associated with the input and output buffers.  |
`------------------------------------------------------------------------*/

void
initialize_buffers (void)
{
  int in_buf_size, out_buf_size;

  if (copy_function == process_copy_in)
    {
      /* Make sure the input buffer can always hold 2 blocks and that it
	 is big enough to hold 1 tar record (512 bytes) even if it
	 is not aligned on a block boundary.  The extra buffer space
	 is needed by process_copyin and peek_in_buf to automatically
	 figure out what kind of archive it is reading.  */
      if (io_block_size >= 512)	/* FIXME: use a symbol */
	in_buf_size = 2 * io_block_size;
      else
	in_buf_size = 1024;
      out_buf_size = DISK_IO_BLOCK_SIZE;
    }
  else if (copy_function == process_copy_out)
    {
      in_buf_size = DISK_IO_BLOCK_SIZE;
      out_buf_size = io_block_size;
    }
  else
    {
      in_buf_size = DISK_IO_BLOCK_SIZE;
      out_buf_size = DISK_IO_BLOCK_SIZE;
    }

  input_buffer = (char *) xmalloc (in_buf_size);
  input_buffer_cursor = input_buffer;
  input_buffer_size = in_buf_size;
  input_buffer_remaining_size = 0;
  input_bytes = 0;

  output_buffer = (char *) xmalloc (out_buf_size);
  output_buffer_cursor = output_buffer;
  output_buffer_remaining_size = 0;
  output_bytes = 0;
}

/*------------------------------------------------------------------------.
| Write `output_buffer_remaining_size' bytes of `output_buffer' to file descriptor OUT_DES |
| and reset `output_buffer_remaining_size' and `output_buffer_cursor'.                                 |
`------------------------------------------------------------------------*/

void
tape_empty_output_buffer (int out_des)
{
  int bytes_written;

#ifdef BROKEN_LONG_TAPE_DRIVER
  static long output_bytes_before_lseek = 0;

  /* Some tape drivers seem to have a signed internal seek pointer and they
     lose if it overflows and becomes negative (e.g. when writing tapes >
     2Gb).  Doing an lseek (des, 0, SEEK_SET) seems to reset the seek pointer
     and prevent it from overflowing.  */
  if (output_is_special
     && (output_bytes_before_lseek += output_buffer_remaining_size,
	 output_bytes_before_lseek >= 1073741824L))
    {
      lseek(out_des, 0L, SEEK_SET);
      output_bytes_before_lseek = 0;
    }
#endif

  bytes_written = rmtwrite (out_des, output_buffer, output_buffer_remaining_size);
  if (bytes_written != output_buffer_remaining_size)
    {
      int rest_bytes_written;
      int rest_output_buffer_remaining_size;

      if (output_is_special
	  && (bytes_written >= 0
	      || (bytes_written < 0
		  && (errno == ENOSPC || errno == EIO || errno == ENXIO))))
	{
	  get_next_reel (out_des);
	  if (bytes_written > 0)
	    rest_output_buffer_remaining_size = output_buffer_remaining_size - bytes_written;
	  else
	    rest_output_buffer_remaining_size = output_buffer_remaining_size;
	  rest_bytes_written = rmtwrite (out_des, output_buffer,
					 rest_output_buffer_remaining_size);
	  if (rest_bytes_written != rest_output_buffer_remaining_size)
	    error (1, errno, _("write error"));
	}
      else
	error (1, errno, _("write error"));
    }
  output_bytes += output_buffer_remaining_size;
  output_buffer_cursor = output_buffer;
  output_buffer_remaining_size = 0;
}

/*--------------------------------------------------------------------------.
| Write `output_buffer_remaining_size' bytes of `output_buffer' to file descriptor OUT_DES   |
| and reset `output_buffer_remaining_size' and `output_buffer_cursor'.  If `swapping_halfwords' or       |
| `swapping_bytes' is set, do the appropriate swapping first.  Our callers  |
| have to make sure to only set these flags if `output_buffer_remaining_size' is appropriate |
| (a multiple of 4 for `swapping_halfwords', 2 for `swapping_bytes').  The  |
| fact that DISK_IO_BLOCK_SIZE must always be a multiple of 4 helps us (and |
| our callers) insure this.                                                 |
`--------------------------------------------------------------------------*/

void
disk_empty_output_buffer (int out_des)
{
  int bytes_written;

  if (swapping_halfwords || swapping_bytes)
    {
      if (swapping_halfwords)
	{
	  int complete_words;
	  complete_words = output_buffer_remaining_size / 4;
	  swahw_array (output_buffer, complete_words);
	  if (swapping_bytes)
	    swab_array (output_buffer, 2 * complete_words);
	}
      else
	{
	  int complete_halfwords;
	  complete_halfwords = output_buffer_remaining_size /2;
	  swab_array (output_buffer, complete_halfwords);
	}
    }

  if (sparse_option)
    bytes_written = sparse_write (out_des, output_buffer, output_buffer_remaining_size);
  else
    bytes_written = write (out_des, output_buffer, output_buffer_remaining_size);

  if (bytes_written != output_buffer_remaining_size)
    {
      error (1, errno, _("write error"));
    }
  output_bytes += output_buffer_remaining_size;
  output_buffer_cursor = output_buffer;
  output_buffer_remaining_size = 0;
}

/*-----------------------------------------------------------------------.
| Exchange the halfwords of each element of the array of COUNT longs     |
| starting at PTR.  PTR does not have to be aligned at a word boundary.  |
`-----------------------------------------------------------------------*/

void
swahw_array (char *ptr, int count)
{
  char tmp;

  for (; count > 0; count--)
    {
      tmp = *ptr;
      *ptr = *(ptr + 2);
      *(ptr + 2) = tmp;
      ptr++;
      tmp = *ptr;
      *ptr = *(ptr + 2);
      *(ptr + 2) = tmp;
      ptr += 3;
    }
}

/*-------------------------------------------------------------------------.
| Read at most NUM_BYTES or `io_block_size' bytes, whichever is smaller,   |
| into the start of `input_buffer' from file descriptor IN_DES.  Set       |
| `input_buffer_remaining_size' to the number of bytes read and reset `input_buffer_cursor'.  Exit with |
| an error if end of file is reached.                                      |
`-------------------------------------------------------------------------*/

#ifdef BROKEN_LONG_TAPE_DRIVER
static long input_bytes_before_lseek = 0;
#endif

static void
tape_fill_input_buffer (int in_des, int num_bytes)
{
#ifdef BROKEN_LONG_TAPE_DRIVER
  /* Some tape drivers seem to have a signed internal seek pointer and they
     lose if it overflows and becomes negative (e.g. when writing tapes >
     4Gb).  Doing an lseek (des, 0, SEEK_SET) seems to reset the seek pointer
     and prevent it from overflowing.  */
  if (input_is_special
      && (input_bytes_before_lseek += num_bytes,
	  input_bytes_before_lseek >= 1073741824L))
    {
      lseek(in_des, 0L, SEEK_SET);
      input_bytes_before_lseek = 0;
    }
#endif
  input_buffer_cursor = input_buffer;
  num_bytes = (num_bytes < io_block_size) ? num_bytes : io_block_size;
  input_buffer_remaining_size = rmtread (in_des, input_buffer, num_bytes);
  if (input_buffer_remaining_size == 0 && input_is_special)
    {
      get_next_reel (in_des);
      input_buffer_remaining_size = rmtread (in_des, input_buffer, num_bytes);
    }
  if (input_buffer_remaining_size < 0)
    error (1, errno, _("read error"));
  if (input_buffer_remaining_size == 0)
    {
      error (0, 0, _("premature end of file"));
      exit (1);
    }
  input_bytes += input_buffer_remaining_size;
}

/*------------------------------------------------------------------------.
| Read at most NUM_BYTES or `DISK_IO_BLOCK_SIZE' bytes, whichever is      |
| smaller, into the start of `input_buffer' from file descriptor IN_DES.  |
| Set `input_buffer_remaining_size' to the number of bytes read and reset `input_buffer_cursor'.  Exit |
| with an error if end of file is reached.                                |
`------------------------------------------------------------------------*/

static int
disk_fill_input_buffer (int in_des, int num_bytes)
{
  input_buffer_cursor = input_buffer;
  num_bytes = (num_bytes < DISK_IO_BLOCK_SIZE) ? num_bytes : DISK_IO_BLOCK_SIZE;
  input_buffer_remaining_size = read (in_des, input_buffer, num_bytes);
  if (input_buffer_remaining_size < 0)
    {
      input_buffer_remaining_size = 0;
      return -1;
    }
  else if (input_buffer_remaining_size == 0)
    return 1;
  input_bytes += input_buffer_remaining_size;
  return 0;
}

/*--------------------------------------------------------------------------.
| Copy NUM_BYTES of buffer IN_BUF to `output_buffer_cursor', which may be partly full.  |
| When `output_buffer_cursor' fills up, flush it to file descriptor OUT_DES.            |
`--------------------------------------------------------------------------*/

void
tape_buffered_write (char *in_buf, int out_des, long num_bytes)
{
  long bytes_left = num_bytes;	/* bytes needing to be copied */
  long space_left;		/* room left in output buffer */

  while (bytes_left > 0)
    {
      space_left = io_block_size - output_buffer_remaining_size;
      if (space_left == 0)
	tape_empty_output_buffer (out_des);
      else
	{
	  if (bytes_left < space_left)
	    space_left = bytes_left;
	  memcpy (output_buffer_cursor, in_buf, (unsigned) space_left);
	  output_buffer_cursor += space_left;
	  output_buffer_remaining_size += space_left;
	  in_buf += space_left;
	  bytes_left -= space_left;
	}
    }
}

/*--------------------------------------------------------------------------.
| Copy NUM_BYTES of buffer IN_BUF to `output_buffer_cursor', which may be partly full.  |
| When `output_buffer_cursor' fills up, flush it to file descriptor OUT_DES.            |
`--------------------------------------------------------------------------*/

void
disk_buffered_write (char *in_buf, int out_des, long num_bytes)
{
  long bytes_left = num_bytes;	/* bytes needing to be copied */
  long space_left;		/* room left in output buffer */

  while (bytes_left > 0)
    {
      space_left = DISK_IO_BLOCK_SIZE - output_buffer_remaining_size;
      if (space_left == 0)
	disk_empty_output_buffer (out_des);
      else
	{
	  if (bytes_left < space_left)
	    space_left = bytes_left;
	  memcpy (output_buffer_cursor, in_buf, (unsigned) space_left);
	  output_buffer_cursor += space_left;
	  output_buffer_remaining_size += space_left;
	  in_buf += space_left;
	  bytes_left -= space_left;
	}
    }
}

/*------------------------------------------------------------------------.
| Copy COUNT bytes of buffer `input_buffer_cursor' into BUFFER.           |
| `input_buffer_cursor' may be partly full (whatever that means? FIXME).  |
| When `input_buffer_cursor' is exhausted, refill it from file descriptor |
| HANDLE.                                                                 |
`------------------------------------------------------------------------*/

void
tape_buffered_read (char *buffer, int handle, long count)
{
  long space_left;		/* bytes to copy from input buffer */

  while (count > 0)
    {
      if (input_buffer_remaining_size == 0)
	tape_fill_input_buffer (handle, io_block_size);
      space_left = MINIMUM (count, input_buffer_remaining_size);
      memcpy (buffer, input_buffer_cursor, (unsigned) space_left);
      input_buffer_cursor += space_left;
      buffer += space_left;
      input_buffer_remaining_size -= space_left;
      count -= space_left;
    }
}

/*-------------------------------------------------------------------------.
| Copy the the next NUM_BYTES bytes of `input_buffer' into PEEK_BUF.  If   |
| NUM_BYTES bytes are not available, read the next `io_block_size' bytes   |
| into the end of `input_buffer' and update `input_buffer_remaining_size'.                  |
|                                                                          |
| Return the number of bytes copied into PEEK_BUF.  If the number of bytes |
| returned is less than NUM_BYTES, then EOF has been reached.              |
`-------------------------------------------------------------------------*/

int
tape_buffered_peek (char *peek_buf, int in_des, int num_bytes)
{
  long tmp_input_buffer_remaining_size;
  long got_bytes;
  char *append_buf;

#ifdef BROKEN_LONG_TAPE_DRIVER
  /* Some tape drivers seem to have a signed internal seek pointer and they
     lose if it overflows and becomes negative (e.g. when writing tapes >
     4Gb).  Doing an lseek (des, 0, SEEK_SET) seems to reset the seek pointer
     and prevent it from overflowing.  */
  if (input_is_special
      && (input_bytes_before_lseek += num_bytes,
	  input_bytes_before_lseek>= 1073741824L))
    {
      lseek(in_des, 0L, SEEK_SET);
      input_bytes_before_lseek = 0;
    }
#endif

  while (input_buffer_remaining_size < num_bytes)
    {
      append_buf = input_buffer_cursor + input_buffer_remaining_size;
      if (append_buf - input_buffer >= input_buffer_size)
	{
	  /* We can keep up to 2 "blocks" (either the physical block size
	     or 512 bytes(the size of a tar record), which ever is
	     larger) in the input buffer when we are peeking.  We
	     assume that our caller will never be interested in peeking
	     ahead at more than 512 bytes, so we know that by the time
	     we need a 3rd "block" in the buffer we can throw away the
	     first block to make room.  */
	  int half;
	  half = input_buffer_size / 2;
	  memcpy (input_buffer, input_buffer + half, half);
	  input_buffer_cursor = input_buffer_cursor - half;
	  append_buf = append_buf - half;
	}
      tmp_input_buffer_remaining_size = rmtread (in_des, append_buf, io_block_size);
      if (tmp_input_buffer_remaining_size == 0)
	if (input_is_special)
	  {
	    get_next_reel (in_des);
	    tmp_input_buffer_remaining_size = rmtread (in_des, append_buf, io_block_size);
	  }
	else
	  break;
      if (tmp_input_buffer_remaining_size < 0)
	error (1, errno, _("read error"));
      input_bytes += tmp_input_buffer_remaining_size;
      input_buffer_remaining_size += tmp_input_buffer_remaining_size;
    }
  if (num_bytes <= input_buffer_remaining_size)
    got_bytes = num_bytes;
  else
    got_bytes = input_buffer_remaining_size;
  memcpy (peek_buf, input_buffer_cursor, (unsigned) got_bytes);
  return got_bytes;
}

/*----------------------------------------------------------.
| Skip the next NUM_BYTES bytes of file descriptor IN_DES.  |
`----------------------------------------------------------*/

void
tape_toss_input (int in_des, long num_bytes)
{
  long bytes_left = num_bytes;	/* bytes needing to be copied */
  long space_left;		/* bytes to copy from input buffer */

  while (bytes_left > 0)
    {
      if (input_buffer_remaining_size == 0)
	tape_fill_input_buffer (in_des, io_block_size);
      if (bytes_left < input_buffer_remaining_size)
	space_left = bytes_left;
      else
	space_left = input_buffer_remaining_size;

      if (crc_i_flag && only_verify_crc_option)
	{
 	  int k;

	  for (k = 0; k < space_left; k++)
	    crc += input_buffer_cursor[k] & 0xff;
	}

      input_buffer_cursor += space_left;
      input_buffer_remaining_size -= space_left;
      bytes_left -= space_left;
    }
}

/*---------------------------------------------------------------------------.
| Copy a file using the input and output buffers, which may start out partly |
| full.  After the copy, the files are not closed nor the last block flushed |
| to output, and the input buffer may still be partly full.  If `crc_i_flag' |
| is set, add each byte to `crc'.  IN_DES is the file descriptor for input;  |
| OUT_DES is the file descriptor for output; NUM_BYTES is the number of      |
| bytes to copy.                                                             |
`---------------------------------------------------------------------------*/

void
copy_files_tape_to_disk (int in_des, int out_des, long num_bytes)
{
  long size;
  long k;

  while (num_bytes > 0)
    {
      if (input_buffer_remaining_size == 0)
	tape_fill_input_buffer (in_des, io_block_size);
      size = (input_buffer_remaining_size < num_bytes) ? input_buffer_remaining_size : num_bytes;
      if (crc_i_flag)
	for (k = 0; k < size; k++)
	  crc += input_buffer_cursor[k] & 0xff;
      disk_buffered_write (input_buffer_cursor, out_des, size);
      num_bytes -= size;
      input_buffer_remaining_size -= size;
      input_buffer_cursor += size;
    }
}

/*---------------------------------------------------------------------------.
| Copy a file using the input and output buffers, which may start out partly |
| full.  After the copy, the files are not closed nor the last block flushed |
| to output, and the input buffer may still be partly full.  If `crc_i_flag' |
| is set, add each byte to `crc'.  IN_DES is the file descriptor for input;  |
| OUT_DES is the file descriptor for output; NUM_BYTES is the number of      |
| bytes to copy.                                                             |
`---------------------------------------------------------------------------*/

void
copy_files_disk_to_tape (int in_des, int out_des, long num_bytes,
			 char *filename)
{
  long size;
  long k;
  int rc;
  long original_num_bytes;

  original_num_bytes = num_bytes;

  while (num_bytes > 0)
    {
      if (input_buffer_remaining_size == 0)
	if (rc = disk_fill_input_buffer (in_des, DISK_IO_BLOCK_SIZE), rc)
	  {
	    if (rc > 0)
	      error (0, 0,
		     _("File %s shrunk by %ld bytes, padding with zeros"),
		     filename, num_bytes);
	    else
	      error (0, 0,
		     _("Read error at byte %ld in file %s, padding with zeros"),
		     original_num_bytes - num_bytes, filename);
	    write_nuls_to_file (num_bytes, out_des);
	    break;
	  }
      size = (input_buffer_remaining_size < num_bytes) ? input_buffer_remaining_size : num_bytes;
      if (crc_i_flag)
	for (k = 0; k < size; k++)
	  crc += input_buffer_cursor[k] & 0xff;
      tape_buffered_write (input_buffer_cursor, out_des, size);
      num_bytes -= size;
      input_buffer_remaining_size -= size;
      input_buffer_cursor += size;
    }
}

/*---------------------------------------------------------------------------.
| Copy a file using the input and output buffers, which may start out partly |
| full.  After the copy, the files are not closed nor the last block flushed |
| to output, and the input buffer may still be partly full.  If `crc_i_flag' |
| is set, add each byte to `crc'.  IN_DES is the file descriptor for input;  |
| OUT_DES is the file descriptor for output; NUM_BYTES is the number of      |
| bytes to copy.                                                             |
`---------------------------------------------------------------------------*/

void
copy_files_disk_to_disk (int in_des, int out_des, long num_bytes,
			 char *filename)
{
  long size;
  long k;
  long original_num_bytes;
  int rc;

  original_num_bytes = num_bytes;
  while (num_bytes > 0)
    {
      if (input_buffer_remaining_size == 0)
	if (rc = disk_fill_input_buffer (in_des, DISK_IO_BLOCK_SIZE), rc)
	  {
	    if (rc > 0)
	      error (0, 0,
		     _("File %s shrunk by %ld bytes, padding with zeros"),
		     filename, num_bytes);
	    else
	      error (0, 0,
		     _("Read error at byte %ld in file %s, padding with zeros"),
		     original_num_bytes - num_bytes, filename);
	    write_nuls_to_file (num_bytes, out_des);
	    break;
	  }
      size = (input_buffer_remaining_size < num_bytes) ? input_buffer_remaining_size : num_bytes;
      if (crc_i_flag)
	for (k = 0; k < size; k++)
	  crc += input_buffer_cursor[k] & 0xff;
      disk_buffered_write (input_buffer_cursor, out_des, size);
      num_bytes -= size;
      input_buffer_remaining_size -= size;
      input_buffer_cursor += size;
    }
}

/*--------------------------------------------------------------------------.
| Create all directories up to but not including the last part of NAME.  Do |
| not destroy any nondirectories while creating directories.                |
`--------------------------------------------------------------------------*/

void
create_all_directories (char *name)
{
  char *dir;
  int mode;

  dir = dir_name (name);
  mode = 0700;
#ifdef HPUX_CDF
  if (is_last_parent_cdf (name))
    {
      dir [strlen (dir) - 1] = '\0';	/* remove final + */
      mode = 04700;
    }

#endif

  if (dir == NULL)
    error (2, 0, _("virtual memory exhausted"));

  if (dir[0] != '.' || dir[1] != '\0')
    make_path (dir, mode, 0700, -1, -1, 0, (char *) NULL);

  free (dir);
}

/*---------------------------------------------------------------------------.
| Prepare to append to an archive.  We have been in process_copy_in, keeping |
| track of the position where the last header started in                     |
| `last_header_start'.  Now we have the starting position of the last header |
| (the TRAILER!!!  header, or blank record for tar archives) and we want to  |
| start writing (appending) over the last header.  The last header may be in |
| the middle of a block, so to keep the buffering in sync we lseek back to   |
| the start of the block, read everything up to but not including the last   |
| header, lseek back to the start of the block, and then do a copy_buf_out   |
| of what we read.  Actually, we probably don't have to worry so much about  |
| keeping the buffering perfect since you can only append to archives that   |
| are disk files.                                                            |
`---------------------------------------------------------------------------*/

void
prepare_append (int out_file_des)
{
  int start_of_header;
  int start_of_block;
  int useful_bytes_in_block;
  char *tmp_buf;

  start_of_header = last_header_start;
  /* Figure out how many bytes we will rewrite, and where they start.  */
  useful_bytes_in_block = start_of_header % io_block_size;
  start_of_block = start_of_header - useful_bytes_in_block;

  if (lseek (out_file_des, start_of_block, SEEK_SET) < 0)
    error (1, errno, _("cannot seek on output"));
  if (useful_bytes_in_block > 0)
    {
      tmp_buf = (char *) xmalloc (useful_bytes_in_block);
      read (out_file_des, tmp_buf, useful_bytes_in_block);
      if (lseek (out_file_des, start_of_block, SEEK_SET) < 0)
	error (1, errno, _("cannot seek on output"));
      /* fix juo -- is this copy_tape_buf_out?  or copy_disk? */
      tape_buffered_write (tmp_buf, out_file_des, useful_bytes_in_block);
      free (tmp_buf);
    }

  /* We are done reading the archive, so clear these since they will now be
     used for reading in files that we are appending to the archive.  */
  input_buffer_remaining_size = 0;
  input_bytes = 0;
  input_buffer_cursor = input_buffer;
}

/* Support for remembering inodes with multiple links.  Used in the
   "copy in" and "copy pass" modes for making links instead of copying
   the file.  */

struct inode_val
{
  unsigned long inode;
  unsigned long major_num;
  unsigned long minor_num;
  char *file_name;
};

/* Inode hash table.  Allocated by first call to add_inode.  */
static struct inode_val **hash_table = NULL;

/* Size of current hash table.  Initial size is 47.  (47 = 2*22 + 3) */
static int hash_size = 22;

/* Number of elements in current hash table.  */
static int hash_num;

/*------------------------------------------------------------------.
| Find the file name associated with NODE_NUM.  If there is no file |
| associated with NODE_NUM, return NULL.                            |
`------------------------------------------------------------------*/

char *
find_inode_file (unsigned long node_num, unsigned long major_num,
		 unsigned long minor_num)
{
#ifndef __MSDOS__
  int start;			/* initial hash location */
  int temp;			/* rehash search variable */

  if (hash_table != NULL)
    {
      /* Hash function is node number modulo the table size.  */
      start = node_num % hash_size;

      /* Initial look into the table.  */
      if (hash_table[start] == NULL)
	return NULL;
      if (hash_table[start]->inode == node_num
	  && hash_table[start]->major_num == major_num
	  && hash_table[start]->minor_num == minor_num)
	return hash_table[start]->file_name;

      /* The home position is full with a different inode record.
	 Do a linear search terminated by a NULL pointer.  */
      for (temp = (start + 1) % hash_size;
	   hash_table[temp] != NULL && temp != start;
	   temp = (temp + 1) % hash_size)
	{
	  if (hash_table[temp]->inode == node_num
	      && hash_table[temp]->major_num == major_num
	      && hash_table[temp]->minor_num == minor_num)
	    return hash_table[temp]->file_name;
	}
    }
#endif
  return NULL;
}

/*--------------------------------------------------------------------------.
| Associate FILE_NAME with the inode NODE_NUM, by inserting it into an hash |
| table.                                                                    |
`--------------------------------------------------------------------------*/

void
add_inode (unsigned long node_num,char *file_name, unsigned long major_num,
	   unsigned long minor_num)
{
#ifndef __MSDOS__
  struct inode_val *temp;

  /* Create new inode record.  */
  temp = (struct inode_val *) xmalloc (sizeof (struct inode_val));
  temp->inode = node_num;
  temp->major_num = major_num;
  temp->minor_num = minor_num;
  temp->file_name = xstrdup (file_name);

  /* Do we have to increase the size of (or initially allocate) the hash
     table?  */
  if (hash_num == hash_size || hash_table == NULL)
    {
      struct inode_val **old_table;	/* Pointer to old table.  */
      int i;			/* Index for re-insert loop.  */

      /* Save old table.  */
      old_table = hash_table;
      if (old_table == NULL)
	hash_num = 0;

      /* Calculate new size of table and allocate it.
         Sequence of table sizes is 47, 97, 197, 397, 797, 1597, 3197, 6397 ...
	 where 3197 and most of the sizes after 6397 are not prime.  The other
	 numbers listed are prime.  */
      hash_size = 2 * hash_size + 3;
      hash_table = (struct inode_val **)
	xmalloc (hash_size * sizeof (struct inode_val *));
      memset (hash_table, 0, hash_size * sizeof (struct inode_val *));

      /* Insert the values from the old table into the new table.  */
      for (i = 0; i < hash_num; i++)
	hash_insert (old_table[i]);

      if (old_table != NULL)
	free (old_table);
    }

  /* Insert the new record and increment the count of elements in the
      hash table.  */
  hash_insert (temp);
  hash_num++;
#endif /* __MSDOS__ */
}

/*--------------------------------------------------------------------------.
| Do the hash insert.  Used in normal inserts and resizing the hash table.  |
| It is guaranteed that there is room to insert the item.  NEW_VALUE is the |
| pointer to the previously allocated inode, file name association record.  |
`--------------------------------------------------------------------------*/

static void
hash_insert (struct inode_val *new_value)
{
  int start;			/* Home position for the value.  */
  int temp;			/* Used for rehashing.  */

  /* Hash function is node number modulo the table size.  */
  start = new_value->inode % hash_size;

  /* Do the initial look into the table.  */
  if (hash_table[start] == NULL)
    {
      hash_table[start] = new_value;
      return;
    }

  /* If we get to here, the home position is full with a different inode
     record.  Do a linear search for the first NULL pointer and insert the new
     item there.  */
  temp = (start + 1) % hash_size;
  while (hash_table[temp] != NULL)
    temp = (temp + 1) % hash_size;

  /* Insert at the NULL.  */
  hash_table[temp] = new_value;
}

/*--------------------------------------------------------------------------.
| Open FILE in the mode specified by the command line options and return an |
| open file descriptor for it, or -1 if it can't be opened.                 |
`--------------------------------------------------------------------------*/

int
open_archive (char *file)
{
  int fd;
  void (*copy_in) ();		/* workaround for pcc bug */

#define force_local_option false

  copy_in = process_copy_in;

  if (copy_function == copy_in)
    fd = rmtopen (file, O_RDONLY | O_BINARY, 0666, NULL);
  else if (append_option)
    fd = rmtopen (file, O_RDWR | O_BINARY, 0666, NULL);
  else
    fd = rmtopen (file, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666, NULL);

#undef force_local_option

  return fd;
}

/*-------------------------------------------------------------------------.
| Attempt to rewind the tape drive on file descriptor TAPE_DES and take it |
| offline.                                                                 |
`-------------------------------------------------------------------------*/

void
tape_offline (int tape_des)
{
#if defined(MTIOCTOP) && defined(MTOFFL)
  struct mtop control;

  control.mt_op = MTOFFL;
  control.mt_count = 1;
  /* Don't care if the following call fails.  */
  rmtioctl (tape_des, MTIOCTOP, (char *) &control);
#endif
}

/*---------------------------------------------------------------------------.
| The file on file descriptor TAPE_DES is assumed to be magnetic tape (or    |
| floppy disk or other device) and the end of the medium has been reached.   |
| Ask the user for to mount a new "tape" to continue the processing.  If the |
| user specified the device name on the command line (with the -I, -O, -F or |
| --file options), then we can automatically re-open the same device to use  |
| the next medium.  If the user did not specify the device name, then we     |
| have to ask them which device to use.                                      |
`---------------------------------------------------------------------------*/

void
get_next_reel (int tape_des)
{
  static int reel_number = 1;
  FILE *tty_in;			/* File for interacting with user.  */
  FILE *tty_out;		/* File for interacting with user.  */
  int old_tape_des;
  char *next_archive_name;
  dynamic_string new_name;
  char *str_res;

  ds_init (&new_name, 128);

  /* Open files for interactive communication.  */
  tty_in = fopen (CONSOLE, "r");
  if (tty_in == NULL)
    error (2, errno, "%s", CONSOLE);
  tty_out = fopen (CONSOLE, "w");
  if (tty_out == NULL)
    error (2, errno, "%s", CONSOLE);

  old_tape_des = tape_des;
  tape_offline (tape_des);
  rmtclose (tape_des);

  /* Give message and wait for carrage return.  User should hit carrage return
     only after loading the next tape.  */
  reel_number++;
  if (new_media_message)
    fprintf (tty_out, "%s", new_media_message);
  else if (new_media_message_with_number)
    fprintf (tty_out, "%s%d%s", new_media_message_with_number, reel_number,
	     new_media_message_after_number);
  else if (archive_name)
    fprintf (tty_out,
	     _("Found end of tape.  Load next tape and press RETURN. "));
  else
    fprintf (tty_out,
	     _("Found end of tape.  To continue, type device/file name when ready.\n"));

  fflush (tty_out);

  if (archive_name)
    {
      int c;

      do
	c = getc (tty_in);
      while (c != EOF && c != '\n');

      tape_des = open_archive (archive_name);
      if (tape_des == -1)
	error (1, errno, "%s", archive_name);
    }
  else
    {
      do
	{
	  if (tape_des < 0)
	    {
	      fprintf (tty_out,
		       _("To continue, type device/file name when ready.\n"));
	      fflush (tty_out);
	    }

	  str_res = ds_fgets (tty_in, &new_name);
	  if (str_res == NULL || str_res[0] == '\0')
	    exit (1);
	  next_archive_name = str_res;

	  tape_des = open_archive (next_archive_name);
	  if (tape_des == -1)
	    error (0, errno, "%s", next_archive_name);
	}
      while (tape_des < 0);
    }

  /* We have to make sure that `tape_des' has not changed its value even
     though we closed it and reopened it, since there are local copies of it
     in other routines.  This works fine on Unix (even with rmtread and
     rmtwrite) since open will always return the lowest available file
     descriptor and we haven't closed any files (e.g., stdin, stdout or
     stderr) that were opened before we originally opened the archive.  */

  if (tape_des != old_tape_des)
    error (1, 0, _("internal error: tape descriptor changed from %d to %d"),
	   old_tape_des, tape_des);

  free (new_name.string);
  fclose (tty_in);
  fclose (tty_out);
}

/*------------------------------------------------------------------------.
| If MESSAGE does not contain the string "%d", make `new_media_message' a |
| copy of MESSAGE.  If MESSAGES does contain the string "%d", make        |
| `new_media_message_with_number' a copy of MESSAGE up to, but not        |
| including, the string "%d", and make `new_media_message_after_number' a |
| copy of MESSAGE after the string "%d".                                  |
`------------------------------------------------------------------------*/

void
set_new_media_message (char *message)
{
  char *p;
  int prev_was_percent;

  p = message;
  prev_was_percent = 0;
  while (*p != '\0')
    {
      if (*p == 'd' && prev_was_percent)
	break;
      prev_was_percent = (*p == '%');
      p++;
    }
  if (*p == '\0')
    {
      new_media_message = xstrdup (message);
    }
  else
    {
      int length = p - message - 1;

      new_media_message_with_number = xmalloc (length + 1);
      strncpy (new_media_message_with_number, message, length);
      new_media_message_with_number[length] = '\0';
      length = strlen (p + 1);
      new_media_message_after_number = xmalloc (length + 1);
      strcpy (new_media_message_after_number, p + 1);
    }
}

#ifdef SYMLINK_USES_UMASK
/*---------------------------------------------------------------------------.
| Most machines always create symlinks with rwxrwxrwx protection, but some   |
| (HP/UX 8.07; maybe DEC's OSF on MIPS, too?) use the umask when creating    |
| symlinks, so if your umask is 022 you end up with rwxr-xr-x symlinks       |
| (although HP/UX seems to completely ignore the protection).  There doesn't |
| seem to be any way to manipulate the modes once the symlinks are created   |
| (e.g.  a hypothetical "lchmod"), so to create them with the right modes we |
| have to set the umask first.                                               |
`---------------------------------------------------------------------------*/

int
umasked_symlink (char *name1, char *name2, int mode)
{
  int	old_umask;
  int	rc;
  mode = ~(mode & 0777) & 0777;
  old_umask = umask (mode);
  rc = symlink (name1, name2);
  umask (old_umask);
  return rc;
}
#endif /* SYMLINK_USES_UMASK */

#if defined(__MSDOS__) && !defined(__GNUC__)
int
chown (char *path, int owner, int group)
{
  return 0;
}
#endif

#ifdef __TURBOC__
#include <time.h>
#include <fcntl.h>
#include <io.h>

int
utime (char *filename, struct utimbuf *utb)
{
  extern int errno;
  struct tm *tm;
  struct ftime filetime;
  time_t when;
  int fd;
  int status;

  if (utb == 0)
      when = time (0);
  else
      when = utb->modtime;

    fd = _open (filename, O_RDWR);
  if (fd == -1)
      return -1;

    tm = localtime (&when);
  if (tm->tm_year < 80)
      filetime.ft_year = 0;
  else
      filetime.ft_year = tm->tm_year - 80;
    filetime.ft_month = tm->tm_mon + 1;
    filetime.ft_day = tm->tm_mday;
  if (tm->tm_hour < 0)
      filetime.ft_hour = 0;
  else
      filetime.ft_hour = tm->tm_hour;
    filetime.ft_min = tm->tm_min;
    filetime.ft_tsec = tm->tm_sec / 2;

    status = setftime (fd, &filetime);
    _close (fd);
    return status;
}
#endif

#define DISKBLOCKSIZE 512

enum sparse_write_states { begin, in_zeros, not_in_zeros };

static bool
buf_all_zeros (char *buf, int bufsize)
{
  int i;

  for (i = 0; i < bufsize; i++)
    if (*buf++ != '\0')
      return false;
  return true;
}

int delayed_seek_count = 0;

/*-------------------------------------------------------------------------.
| Write NBYTE bytes from BUF to remote tape connection FILDES.  Return the |
| number of bytes written on success, -1 on error.                         |
`-------------------------------------------------------------------------*/

static int
sparse_write (int fildes, char *buf, unsigned int nbyte)
{
  int complete_block_count;
  int leftover_bytes_count;
  int seek_count;
  int write_count;
  char *cur_write_start;
  int lseek_rc;
  int write_rc;
  int i;
  enum sparse_write_states state;

  complete_block_count = nbyte / DISKBLOCKSIZE;
  leftover_bytes_count = nbyte % DISKBLOCKSIZE;

  if (delayed_seek_count != 0)
    state = in_zeros;
  else
    state = begin;

  seek_count = delayed_seek_count;

  for (i = 0; i < complete_block_count; i++)
    {
      switch (state)
	{
	  case begin :
	    if (buf_all_zeros (buf, DISKBLOCKSIZE))
	      {
		seek_count = DISKBLOCKSIZE;
		state = in_zeros;
	      }
	    else
	      {
		cur_write_start = buf;
		write_count = DISKBLOCKSIZE;
		state = not_in_zeros;
	      }
	    buf += DISKBLOCKSIZE;
	    break;
	  case in_zeros :
	    if (buf_all_zeros (buf, DISKBLOCKSIZE))
	      seek_count += DISKBLOCKSIZE;
	    else
	      {
		lseek (fildes, seek_count, SEEK_CUR);
		cur_write_start = buf;
		write_count = DISKBLOCKSIZE;
		state = not_in_zeros;
	      }
	    buf += DISKBLOCKSIZE;
	    break;
	  case not_in_zeros :
	    if (buf_all_zeros (buf, DISKBLOCKSIZE))
	      {
		write_rc = write (fildes, cur_write_start, write_count);
		seek_count = DISKBLOCKSIZE;
		state = in_zeros;
	      }
	    else
	      write_count += DISKBLOCKSIZE;
	    buf += DISKBLOCKSIZE;
	    break;
	}
    }

  switch (state)
    {
      case begin :
      case in_zeros :
	delayed_seek_count = seek_count;
	break;
      case not_in_zeros :
	write_rc = write (fildes, cur_write_start, write_count);
	delayed_seek_count = 0;
	break;
    }

  if (leftover_bytes_count != 0)
    {
      if (delayed_seek_count != 0)
	{
	  lseek_rc = lseek (fildes, delayed_seek_count, SEEK_CUR);
	  delayed_seek_count = 0;
	}
      write_rc = write (fildes, buf, leftover_bytes_count);
    }
  return nbyte;
}

static void
write_nuls_to_file (long num_bytes, int out_des)
{
  long	blocks;
  long	extra_bytes;
  long	i;

  blocks = num_bytes / BLOCKSIZE;
  extra_bytes = num_bytes % BLOCKSIZE;
  for (i = 0; i < blocks; i++)
    if (write (out_des, zero_block, BLOCKSIZE) != BLOCKSIZE)
      error (1, errno, "error writing NUL's");
  if (extra_bytes != 0)
    if (write (out_des, zero_block, extra_bytes) != extra_bytes)
      error (1, errno, "error writing NUL's");
}

#if DOSWIN

# include <errno.h>
# include <limits.h>
# include <sys/stat.h>

/*-------------------------------------------------.
| A dummy version of `readlink' which just fails.  |
`-------------------------------------------------*/

/* FIXME: it might be nice to support `symlinks' to executable programs.  */

int
readlink (const char *path, char *linkname, int name_buf_size)
{
  errno = ENOSYS;
  return -1;
}

/*--------------------------------------------------------------------------.
| If the original file name includes characters illegal for MS-DOS and      |
| MS-Windows, massage it to make it suitable and return a pointer to static |
| storage with a new name.  If the original name is legit, return it        |
| instead.  Return NULL if the rename failed (shouldn't happen).            |
`--------------------------------------------------------------------------*/

/* The file name changes are: (1) any name that is reserved by a DOS device
   driver (such as `prn.txt' or `aux.c') is prepended with a `_'; and (2)
   illegal characters are replaced with `_' or `-' (well, almost; look at the
   code below for details).  */

char *
maybe_rename_file (char *original_name)
{
  static char dosified_name[PATH_MAX];
  static char illegal_chars_dos[] = ".+, ;=[]|<>\\\":?*";
  static char *illegal_chars_w95 = &illegal_chars_dos[8];
  int idx, dot_idx;
  char *s = original_name, *d = dosified_name;
  char *illegal_aliens = illegal_chars_dos;
  size_t len = sizeof (illegal_chars_dos) - 1;
  int windows9x = _use_lfn (original_name);

  /* Support for Windows 9X VFAT systems, when available.  */
  if (windows9x)
    {
      illegal_aliens = illegal_chars_w95;
      len -= (illegal_chars_w95 - illegal_chars_dos);
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
	  /* Dots are special on DOS: it doesn't allow them as the leading
	     character, and a file name cannot have more than a single dot.
	     We leave the first non-leading dot alone, unless it comes too
	     close to the beginning of the name: we want sh.lex.c to become
	     sh_lex.c, not sh.lex-c.  */
	  if (*s == '.')
	    {
	      if (idx == 0
		  && (s[1] == '/' || s[1] == '\0'
		      || (s[1] == '.' && (s[2] == '/' || s[2] == '\0'))))
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
		  if (dot_idx < 5) /* 5 is merely a heuristic ad-hoc'ery */
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

  /* We could have a file in an archive whose name is reserved
     on MS-DOS by a device driver.  Trying to extract such a
     file would fail at best and wedge us at worst.  We need to
     rename such files.  */

  if (idx > 0)
    {
      struct stat st_buf;
      char *base = d - idx;
      int i = 0;

      /* The list of character devices is not constant: it depends on
	 what device drivers did they install in their CONFIG.SYS.
	 `stat' will tell us if the basename of the file name is a
	 characer device.  */
      while (stat (base, &st_buf) == 0 && S_ISCHR (st_buf.st_mode))
	{
	  size_t blen = strlen (base);

	  /* I don't believe any DOS character device names begin with a
	     `_'.  But in case they invent such a device, let us try twice.  */
	  if (++i > 2)
	    return (char *)0;

	  /* Prepend a '_'.  */
	  memmove (base + 1, base, blen + 1);
	  base[0] = '_';
	}
    }

  if (strcmp (original_name, dosified_name))
    return dosified_name;
  return original_name;
}

#endif /* DOSWIN */
