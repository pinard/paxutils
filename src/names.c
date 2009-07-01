/* Various processing of names.
   Copyright (C) 1988, 92, 94, 96, 97, 98 Free Software Foundation, Inc.

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

#ifndef FNM_LEADING_DIR
# include <fnmatch.h>
#endif

#include "common.h"

/* Names from the command call.  */

static const char **name_array;	/* store an array of names */
static int allocated_names;	/* how big is the array? */
static int names;		/* how many entries does it have? */
static int name_index = 0;	/* how many of the entries have we scanned? */

/*------------------------.
| Initialize structures.  |
`------------------------*/

void
init_names (void)
{
  allocated_names = 10;
  name_array = (const char **)
    xmalloc (sizeof (const char *) * allocated_names);
  names = 0;
}

/*--------------------------------------------------------------.
| Add NAME at end of name_array, reallocating it as necessary.  |
`--------------------------------------------------------------*/

void
name_add (const char *name)
{
  if (names == allocated_names)
    {
      allocated_names *= 2;
      name_array = (const char **)
	xrealloc (name_array, sizeof (const char *) * allocated_names);
    }
  name_array[names++] = name;
}

/* Names from external name file.  */

static FILE *name_file;		/* file to read names from */
static char *name_buffer;	/* buffer to hold the current file name */
static size_t name_buffer_length; /* allocated length of name_buffer */

/*---.
| ?  |
`---*/

/* FIXME: I should better check more closely.  It seems at first glance that
   is_pattern is only used when reading a file, and ignored for all
   command line arguments.  */

static inline bool
is_pattern (const char *string)
{
  return (strchr (string, '*') != 0
	  || strchr (string, '[') != 0
	  || strchr (string, '?') != 0);
}

/*-----------------------------------------------------------------------.
| Set up to gather file names for tar.  They can either come from a file |
| or were saved from decoding arguments.				 |
`-----------------------------------------------------------------------*/

void
name_init (int argc, char *const *argv)
{
  name_buffer = xmalloc (NAME_FIELD_SIZE + 2);
  name_buffer_length = NAME_FIELD_SIZE;

  if (files_from_option)
    if (!strcmp (files_from_option, "-"))
      name_file = stdin;
    else if (name_file = fopen (files_from_option, "r"), !name_file)
      FATAL_ERROR ((0, errno, _("Cannot open file %s"), files_from_option));
}

/*---.
| ?  |
`---*/

void
name_term (void)
{
  free (name_buffer);
  free (name_array);
}

/*-------------------------------------------------------------------------.
| Read the next filename from name_file and terminate it with NUL.  Put it |
| into name_buffer, reallocating and adjusting name_buffer_length if       |
| necessary.  Return false at end of file, true otherwise.                 |
`-------------------------------------------------------------------------*/

static bool
read_name_from_file (void)
{
  int character;
  size_t counter = 0;

  /* FIXME: getc may be called even if character was EOF the last time.  */

  /* FIXME: This + 2 allocation might serve no purpose.  */

  while (character = getc (name_file),
	 character != EOF && character != filename_terminator)
    {
      if (counter == name_buffer_length)
	{
	  name_buffer_length += NAME_FIELD_SIZE;
	  name_buffer = xrealloc (name_buffer, name_buffer_length + 2);
	}
      name_buffer[counter++] = character;
    }

  if (counter == 0 && character == EOF)
    return false;

  if (counter == name_buffer_length)
    {
      name_buffer_length += NAME_FIELD_SIZE;
      name_buffer = xrealloc (name_buffer, name_buffer_length + 2);
    }
  name_buffer[counter] = '\0';

  return true;
}

/*------------------------------------------------------------------------.
| Get the next name from ARGV or the file of names.  Result is in static  |
| storage and can't be relied upon across two calls.			  |
| 									  |
| If CHANGE_DIRS is true, treat a filename of the form "-C" as meaning	  |
| that the next filename is the name of a directory to change to.  If	  |
| `filename_terminator' is NUL, CHANGE_DIRS is effectively always false.  |
`------------------------------------------------------------------------*/

char *
name_next (bool change_dirs)
{
  const char *source;
  char *cursor;
  bool chdir_flag = false;

  if (filename_terminator == '\0')
    change_dirs = false;

  while (true)
    {
      /* Get a name, either from file or from saved arguments.  */

      if (name_file)
	{
	  if (!read_name_from_file ())
	    break;
	}
      else
	{
	  if (name_index == names)
	    break;

	  source = name_array[name_index++];
	  if (strlen (source) > name_buffer_length)
	    {
	      free (name_buffer);
	      name_buffer_length = strlen (source);
	      name_buffer = xmalloc (name_buffer_length + 2);
	    }
	  strcpy (name_buffer, source);
	}

#if DOSWIN
      /* The rest of code depends on getting Unix-style forward slashes and
	 will break otherwise.  All recursions into subdirectories append
	 forward slashes, so we only need to make sure the original
	 file/directory gets its DOS-style backslashes mirrored.  It seems
	 that here's the right place to do that.  Use DJGPP v2 or later.  */

      for (cursor = strchr (name_buffer, '\\');
	   cursor;
	   cursor = strchr (cursor + 1, '\\'))
	*cursor = '/';
#endif

      /* Zap trailing slashes.  */

      cursor = name_buffer + strlen (name_buffer) - 1;
#if DOSWIN
      if (name_buffer[0] >= 'A' && name_buffer[0] <= 'z'
	  && name_buffer[1] == ':' && name_buffer[2] == '/')
	while (cursor > name_buffer + 2 && *cursor == '/')
	  *cursor-- = '\0';
      else
#endif
	while (cursor > name_buffer && *cursor == '/')
	  *cursor-- = '\0';

      if (chdir_flag)
	{
	  if (chdir (name_buffer) < 0)
	    FATAL_ERROR ((0, errno, _("Cannot change to directory %s"),
			  name_buffer));
	  chdir_flag = false;
	}
      else if (change_dirs && strcmp (name_buffer, "-C") == 0)
	chdir_flag = true;
      else
#if 0
	if (!exclude_option || !check_exclude (name_buffer))
#endif
	  {
	    unquote_string (name_buffer);
	    return name_buffer;
	  }
    }

  /* No more names in file.  */

  if (name_file && chdir_flag)
    FATAL_ERROR ((0, 0, _("Missing file name after -C")));

  return NULL;
}

/*------------------------------.
| Close the name file, if any.  |
`------------------------------*/

void
name_close (void)
{
  if (name_file != NULL && name_file != stdin)
    if (fclose (name_file) == EOF)
      ERROR ((0, errno, "%s", name_buffer));
}

/*----------------------------------.
| Add a name to the list of names.  |
`----------------------------------*/

void
add_name (const char *string)
{
  /* FIXME: This is ugly.  How is memory managed?  */
  static char *chdir_name = NULL;

  struct name *name;
  size_t length;

  if (strcmp (string, "-C") == 0)
    {
      chdir_name = xstrdup (name_next (false));
      string = name_next (false);
      if (!chdir_name)
	FATAL_ERROR ((0, 0, _("Missing file name after -C")));

      if (chdir_name[0] != '/'
#if DOSWIN
	  /* The case of drive letter.  */
	  && !(chdir_name[0] && chdir_name[1] == ':')
#endif
	  )
	{
	  char *current_directory = xgetcwd ();

	  if (!current_directory)
	    FATAL_ERROR ((0, 0, _("Could not get current directory")));

	  chdir_name = concat_with_slash (current_directory, chdir_name);
	  free (current_directory);
	}
    }

  length = string ? strlen (string) : 0;
  name = (struct name *) xmalloc (sizeof (struct name) + length);
  memset (name, 0, sizeof (struct name) + length);
  name->next = NULL;

  if (string)
    {
      name->is_chdir = false;
      name->length = length;
      /* FIXME: Possibly truncating a string, here?  Tss, tss, tss!  */
      strncpy (name->name, string, length);
      name->name[length] = '\0';
    }
  else
    name->is_chdir = true;

  /* Make reasonable assumptions.  */
  name->match_found = false;
  name->is_wildcard = false;
  name->first_is_literal = true;
  name->change_dir = chdir_name;
  name->dir_contents = 0;

  if (string && is_pattern (string))
    {
      name->is_wildcard = true;
      if (string[0] == '*' || string[0] == '[' || string[0] == '?')
	name->first_is_literal = false;
    }

  if (name_list_tail)
    name_list_tail->next = name;
  name_list_tail = name;
  if (!name_list_head)
    name_list_head = name;
}

/*-------------------------------------------------------------------------.
| Gather names in a list for scanning.  Could hash them later if we really |
| care.									   |
| 									   |
| If the names are already sorted to match the archive, we just read them  |
| one by one.  name_gather reads the first one, and it is called by	   |
| name_match as appropriate to read the next ones.  At EOF, the last name  |
| read is just left in the buffer.  This option lets users of small	   |
| machines extract an arbitrary number of files by doing "tar t" and	   |
| editing down the list of files.					   |
`-------------------------------------------------------------------------*/

void
name_gather (void)
{
  /* Buffer able to hold a single name.  */
  static struct name *buffer;
  static size_t allocated_length = 0;

  char *name;

  if (same_order_option)
    {
      if (allocated_length == 0)
	{
	  allocated_length = sizeof (struct name) + NAME_FIELD_SIZE;
	  buffer = (struct name *) xmalloc (allocated_length);
	  /* FIXME: This memset is overkill, and ugly...  */
	  memset (buffer, 0, allocated_length);
	}
      name = name_next (false);
      if (name)
	{
	  if (strcmp (name, "-C") == 0)
	    {
	      char *copy = xstrdup (name_next (false));

	      name = name_next (false);
	      if (!name)
		FATAL_ERROR ((0, 0, _("Missing file name after -C")));
	      buffer->change_dir = copy;
	    }
	  buffer->length = strlen (name);
	  if (sizeof (struct name) + buffer->length >= allocated_length)
	    {
	      allocated_length = sizeof (struct name) + buffer->length;
	      buffer = (struct name *) xrealloc (buffer, allocated_length);
	    }
	  strncpy (buffer->name, name, buffer->length);
	  buffer->name[buffer->length] = 0;
	  buffer->next = NULL;
	  buffer->match_found = false;

	  /* FIXME: Poorly named globals, indeed...  */
	  name_list_head = buffer;
	  name_list_tail = name_list_head;
	}
      return;
    }

  /* Non sorted names -- read them all in.  */

  while (name = name_next (false), name)
    add_name (name);
}

/*------------------------------------------------------------------------.
| Return true if and only if name PATH (from an archive) matches any name |
| from the list of names.                                                 |
`------------------------------------------------------------------------*/

bool
name_match (const char *path)
{
  size_t length = strlen (path);

  while (true)
    {
      struct name *cursor = name_list_head;

      if (!cursor)
	return true;

      if (cursor->is_chdir)
	{
	  if (cursor->change_dir && chdir (cursor->change_dir))
	    FATAL_ERROR ((0, errno, _("Cannot change to directory %s"),
			  cursor->change_dir));
	  name_list_head = NULL;
	  return true;
	}

      for (; cursor; cursor = cursor->next)
	{
	  /* If first chars don't match, quick skip.  */

	  if (cursor->first_is_literal && cursor->name[0] != path[0])
	    continue;

	  /* Wildcarding..  */

	  if (cursor->is_wildcard)
	    {
	      if (fnmatch (cursor->name, path, FNM_LEADING_DIR) == 0)
		{
		  cursor->match_found = true;
		  if (starting_file_option)
		    {
		      free (name_list_head);
		      name_list_head = NULL;
		    }
		  if (cursor->change_dir && chdir (cursor->change_dir))
		    FATAL_ERROR ((0, errno, _("Cannot change to directory %s"),
				  cursor->change_dir));

		  /* We got a match.  */
		  return true;
		}
	      continue;
	    }

	  /* Plain old Strings.  */

	  if (cursor->length <= length
				/* archive length >= specified */
	      && (path[cursor->length] == '\0'
		  || path[cursor->length] == '/')
				/* full match on file/dirname */
	      && strncmp (path, cursor->name, cursor->length) == 0)
				/* name compare */
	    {
	      cursor->match_found = true;
	      if (starting_file_option)
		{
		  free ((void *) name_list_head);
		  name_list_head = NULL;
		}
	      if (cursor->change_dir && chdir (cursor->change_dir))
		FATAL_ERROR ((0, errno, _("Cannot change to directory %s"),
			      cursor->change_dir));

	      /* We got a match.  */
	      return true;
	    }
	}

      /* Filename from archive not found in the list of names.  If we have the
	 whole list of names here, just return false.  Otherwise, read the
	 next name in and compare it.  If this was the last name,
	 name_list_head->match_found will remain true.  If not, we loop to
	 compare the newly read name.  */

      if (same_order_option && name_list_head->match_found)
	{
	  name_gather ();	/* read one more */
	  if (name_list_head->match_found)
	    return false;
	}
      else
	return false;
    }
}

/*-----------------------------------------------------------------------.
| Print the names of things in the list of names that were not matched.  |
`-----------------------------------------------------------------------*/

void
names_notfound (void)
{
  struct name *cursor;
  struct name *next;

  for (cursor = name_list_head; cursor; cursor = next)
    {
      next = cursor->next;
      if (!cursor->match_found && !cursor->is_chdir)
	ERROR ((0, 0, _("%s: Not found in archive"), cursor->name));

      /* We could free the list, but the process is about to die anyway, so
	 save some CPU time.  Amigas and other similarly broken software
	 will need to waste the time, though.  */

#ifdef amiga
      if (!same_order_option)
	free (cursor);
#endif
    }
  name_list_head = (struct name *) NULL;
  name_list_tail = (struct name *) NULL;

  if (same_order_option)
    {
      char *name;

      while (name = name_next (true), name)
	ERROR ((0, 0, _("%s: Not found in archive"), name));
    }
}

/*---.
| ?  |
`---*/

void
name_expand (void)
{
}

/*-------------------------------------------------------------------------.
| This is like name_match, except that it returns a pointer to the name it |
| matched, and doesn't set MATCH_FOUND in structure.  The caller will have |
| to do that if it wants to.  Oh, and if the list of names is empty, it    |
| returns NULL, unlike name_match, which returns true.                     |
`-------------------------------------------------------------------------*/

struct name *
name_scan (const char *path)
{
  size_t length = strlen (path);

  while (true)
    {
      struct name *cursor = name_list_head;

      if (!cursor)
	return NULL;

      for (; cursor; cursor = cursor->next)
	{
	  /* If first chars don't match, quick skip.  */

	  if (cursor->first_is_literal && cursor->name[0] != path[0])
	    continue;

	  /* Wildcards.  */

	  if (cursor->is_wildcard)
	    {
	      if (fnmatch (cursor->name, path, FNM_LEADING_DIR) == 0)
		return cursor;	/* we got a match */
	      continue;
	    }

	  /* Plain Old Strings.  */

	  if (cursor->length <= length
				/* archive length >= specified */
	      && (path[cursor->length] == '\0'
		  || path[cursor->length] == '/')
				/* full match on file/dirname */
	      && strncmp (path, cursor->name, cursor->length) == 0)
				/* name compare */
	    return cursor;	/* we got a match */
	}

      /* Filename from archive not found in the list of names.  If we have the
	 whole list of names here, just return NULL.  Otherwise, read the next
	 name in and compare it.  If this was the last name,
	 name_list_head->match_found will remain on.  If not, we loop to
	 compare the newly read name.  */

      if (same_order_option && name_list_head->match_found)
	{
	  name_gather ();	/* read one more */
	  if (name_list_head->match_found)
	    return NULL;
	}
      else
	return NULL;
    }
}

/* Accumulated list of names.  */

/* Head for the list of names.  */
struct name *name_list_head;

/* Last name in the list of names.  */
struct name *name_list_tail;

/* Cursor in the list of names, corresponding to the name last returned by
   `name_from_list'.  */
struct name *name_list_current = NULL;

/*--------------------------------------------------------------------------.
| This returns a name from the list of names which doesn't have MATCH_FOUND |
| set.  It sets MATCH_FOUND before returning, so successive calls will find |
| and return all the non-found names in the list of names.                  |
`--------------------------------------------------------------------------*/

char *
name_from_list (void)
{
  if (!name_list_current)
    name_list_current = name_list_head;
  while (name_list_current && name_list_current->match_found)
    name_list_current = name_list_current->next;
  if (name_list_current)
    {
      name_list_current->match_found = true;
      if (name_list_current->change_dir)
	if (chdir (name_list_current->change_dir) < 0)
	  FATAL_ERROR ((0, errno, _("Cannot change to directory %s"),
			name_list_current->change_dir));
      return name_list_current->name;
    }
  return NULL;
}

/*-------------------------------------------------------------------------.
| Clear MATCH_FOUND on all names in the list of names, so they will all be |
| returned again by successive calls to `name_from_list'.                  |
`-------------------------------------------------------------------------*/

void
blank_name_list (void)
{
  struct name *name;

  for (name = name_list_head; name; name = name->next)
    name->match_found = false;
  name_list_current = NULL;
}

/* Exclusions.  */

static char *exclude_pool = NULL;
static size_t exclude_pool_size = 0;
static size_t allocated_exclude_pool_size = 0;

static char **simple_exclude_array = NULL;
static int simple_excludes = 0;
static int allocated_simple_excludes = 0;

static char **pattern_exclude_array = NULL;
static int pattern_excludes = 0;
static int allocated_pattern_excludes = 0;

/*---.
| ?  |
`---*/

void
add_exclude (char *name)
{
  size_t name_size;

  unquote_string (name);	/* FIXME: unquote in all cases?  If ever? */
  name_size = strlen (name) + 1;

  if (exclude_pool_size + name_size > allocated_exclude_pool_size)
    {
      char *previous_exclude_pool = exclude_pool;
      char **cursor;

      allocated_exclude_pool_size = exclude_pool_size + name_size + 1024;
      exclude_pool = (char *)
	xrealloc (exclude_pool, allocated_exclude_pool_size);

      for (cursor = simple_exclude_array;
	   cursor < simple_exclude_array + simple_excludes;
	   cursor++)
	*cursor = exclude_pool + (*cursor - previous_exclude_pool);
      for (cursor = pattern_exclude_array;
	   cursor < pattern_exclude_array + pattern_excludes;
	   cursor++)
	*cursor = exclude_pool + (*cursor - previous_exclude_pool);
    }

  if (is_pattern (name))
    {
      if (pattern_excludes == allocated_pattern_excludes)
	{
	  allocated_pattern_excludes += 32;
	  pattern_exclude_array = (char **)
	    xrealloc (pattern_exclude_array,
		      allocated_pattern_excludes * sizeof (char *));
	}
      pattern_exclude_array[pattern_excludes++]
	= exclude_pool + exclude_pool_size;
    }
  else
    {
      if (simple_excludes == allocated_simple_excludes)
	{
	  allocated_simple_excludes += 32;
	  simple_exclude_array = (char **)
	    xrealloc (simple_exclude_array,
		      allocated_simple_excludes * sizeof (char *));
	}
      simple_exclude_array[simple_excludes++]
	= exclude_pool + exclude_pool_size;
    }

  strcpy (exclude_pool + exclude_pool_size, name);
  exclude_pool_size += name_size;
}

/*---.
| ?  |
`---*/

void
add_exclude_file (const char *name)
{
  FILE *file;
  char buffer[1024];

  if (!strcmp (name, "-"))
    file = stdin;
  else if (file = fopen (name, "r"), !file)
    FATAL_ERROR ((0, errno, _("Cannot open %s"), name));

  while (fgets (buffer, 1024, file))
    {
      char *end_of_line = strrchr (buffer, '\n');

      if (end_of_line)
	*end_of_line = '\0';
      add_exclude (buffer);
    }
  if (fclose (file) == EOF)
    ERROR ((0, errno, "%s", name));
}

/*------------------------------------------------------------------.
| Returns true if the file NAME should not be added nor extracted.  |
`------------------------------------------------------------------*/

bool
check_exclude (const char *name)
{
  int counter;
  int namelength;

  for (counter = 0; counter < pattern_excludes; counter++)
    if (fnmatch (pattern_exclude_array[counter], name, FNM_LEADING_DIR) == 0)
      return true;

  namelength = strlen (name);
  for (counter = 0; counter < simple_excludes; counter++)
    {
      /* Check if the string matches the tail of name. */
      int headlength = namelength - strlen (simple_exclude_array[counter]);

      if (headlength >= 0
	  && (headlength == 0 || name[headlength - 1] == '/')
	  && strcmp (simple_exclude_array[counter], name + headlength) == 0)
	return true;
    }
  return false;
}
