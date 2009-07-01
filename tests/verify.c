#include "system.h"

/*--------------------------------------------------------------------------.
| A program that verifies characteristics of files.  If the characteristics |
| are OK, verify is silent (unless the -verbose switch is used); otherwise  |
| verify prints that actual and expected values that don't match.           |
`--------------------------------------------------------------------------*/

/* $ verify -size 12 file		# verify that the size of `file' is
   $ verify -verbose -size 12 file	# 12.
   OK: junk size.
   $ verify -size 11 file
   file size 12 should be 11.
   $ verify -mode 0100644 file		# verify that file the file type
					# and protection mode bits for file
					# are 0100644 (octal).
   $ verify -mtime-ge 1 file		# verify that file was modified
					# at or after 1/1/70 00:00:01 GMT.

   These are some of the basic options to verify:

   -atime seconds-since-epoch
   -atime-ge seconds-since-epoch
   -atime-le seconds-since-epoch
   -gid group-id-number
   -ino inode-number
   -mode file-type-and-protection-modes
   -mtime seconds-since-epoch
   -mtime-le seconds-since-epoch
   -mtime-ge seconds-since-epoch
   -nlink links-to-file
   -size bytes
   -uid user-id-number

   Verify can also operate on a list of files.

   $ cat list				# verify that file1, file2 and file3
   file1				# are all owned by uid 706.
   file2
   file3
   $ ./verify -uid 706 < list

   Verify can also compare the characteristics of a file to another
   file, and print any differences:

   $ ls -l file file2
   -rw-r--r--   1 foo      bar            12 May 29 11:16 file
   -rw-r--r--   1 foo      bar            15 May  6 17:15 file2
   $ ./verify -match-file file2 -size-match -mode-match -uid-match file
   file size match 12 should be 15.

   When comparing 2 files with the -match-file option, verify can also
   take these options:

   -atime-match
   -gid-match
   -ino-match
   -mode-match
   -mtime-match
   -nlink-match
   -size-match
   -uid-match

   When verify is comparing 2 files, the -contents-match option can be
   used to compare the contents of the 2 files:

   $ ./verify -match-file file2 -contents-match file
   file lengths and contents differ at bytes 13.
   $ ./verify -match-file file -contents-match file

   If the files are actually directories, instead of regular files, verify
   compares their entries (it only verifies their names, not any
   characteristics of the files themselves):

   $ ls dir1 dir2
   dir1:
   a  b  c

   dir2:
   a  d
   $ ./verify -match-file dir1 -contents-match dir2
   dir2/b: missing file.
   dir2/c: missing file.
   dir2/d: extra file.

   If the files are symbolic links, verify makes sure that they point
   to the same file:

   $ cd dir2
   $ ln -s a b
   $ ln -s a c
   $ ln -s d f
   $ ../verify -match-file b -contents-match c
   $ ../verify -match-file f -contents-match c
   c link contents differ.

   Verify can compare the characteristics of a file to a file with the same
   name in another directory.

   $ ls -l file dir/file
   -rw-r--r--   1 foo      bar            12 May 29 11:16 file
   -rw-r--r--   1 foo      bar            12 May 29 11:40 dir/file
   $ ./verify -match-dir dir -size-match -mode-match -mtime-match file
   file mtime match 707152592 should be 707154016.

   Finally, verify can compare the characteristics of a list of files in
   the current directory and another directory:

   $ cat list
   file
   file2
   -rw-r--r--   1 foo      bar            12 May 29 11:40 dir/file
   -rw-------   1 foo      bar            26 May 29 11:42 dir/file2
   -rw-r--r--   1 foo      bar            12 May 29 11:16 file
   -rw-r--r--   1 foo      bar            26 May 29 11:43 file2
   $ ./verify -match-dir dir -mode-match -mtime-match -list < list
   file mtime match 707152592 should be 707154016.
   file2 mtime match 707154185 should be 707154132.
   file2 mode match 0100644 should be 0100600.
 */

#include "filetypes.h"
#include <getopt.h>

typedef int TIME_T;

#define MIN(A,B)	((A) < (B) ? (A) : (B))

static bool atime_flag = false;
static bool atime_ge_flag = false;
static TIME_T atime_ge_value = 0;
static bool atime_le_flag = false;
static TIME_T atime_le_value = 0;
static bool atime_match_flag = false;
static TIME_T atime_value = 0;
static bool contents_match_flag = false;
static bool dir_flag = false;
static bool exist_flag = false;
static bool gid_flag = false;
static bool gid_match_flag = false;
static gid_t gid_value = 0;
static bool ignore_dir_atime_flag = false;
static bool ignore_dir_ino_flag = false;
static bool ignore_dir_mtime_flag = false;
static bool ignore_dot_mtime_flag = false;
static bool ignore_link_ino_flag = false;
static bool ignore_link_mtime_flag = 1;
static bool ino_flag = false;
static bool ino_match_flag = false;
static ino_t ino_value = 0;
static bool list_flag = false;
static bool mode_flag = false;
static bool mode_match_flag = false;
static mode_t mode_value = 0;
static bool mtime_flag = false;
static bool mtime_ge_flag = false;
static TIME_T mtime_ge_value = 0;
static bool mtime_le_flag = false;
static TIME_T mtime_le_value = 0;
static bool mtime_match_flag = false;
static TIME_T mtime_value = 0;
static bool nlink_flag = false;
static bool nlink_match_flag = false;
static unsigned nlink_value = 0;
static bool non_exist_flag = false;
static bool not_flag = false;
static bool other_flag = false;
static bool reg_flag = false;
static bool size_flag = false;
static bool size_match_flag = false;
static unsigned size_value = 0;
static bool symlink_flag = false;
static bool uid_flag = false;
static bool uid_match_flag = false;
static uid_t uid_value = 0;
static char *match_dir = NULL;
static char *match_file = NULL;

static bool verbose_flag = false;
static unsigned errors = 0;

static char **
read_a_dir (const char *dirname, size_t *num_entries)
{
  DIR *dirp;
  /* struct direct *dp; */
  struct dirent *dp;
  size_t alloced;
  size_t used;
  char **list;
  size_t len;

  alloced = 10;
  list = (char **) malloc (alloced * sizeof (char *));
  used = 0;

  dirp = opendir (dirname);
  if (dirp == NULL)
    {
      perror (dirname);
      errors++;
      *num_entries = -1;
      return ((char **) NULL);
    }
  while (dp = readdir (dirp))
    {
      if ((used + 1) >= alloced)
	{
	  alloced += 10;
	  list = (char **) realloc (list, alloced * sizeof (char *));
	}
      len = NAMLEN(dp);
      list[used] = (char *) malloc (len + 1);
      strncpy (list[used], dp->d_name, len);
      list[used][len] = '\0';
      used++;
    }
  list[used] = NULL;
  closedir (dirp);
  *num_entries = used;
  return list;
}

static int
sortfun (const void *void_p, const void *void_q)
{
  const char **p = (const char **) void_p;
  const char **q = (const char **) void_q;

  return strcmp (*p, *q);
}

static void
content_match_dirs (const char *dirname1, const char *dirname2)
{
  char **list1, **list2;
  size_t len1, len2;
  bool matches;
  size_t j, k;
  int cmp;

  matches = true;

  list1 = read_a_dir (dirname1, &len1);
  if (len1 <= 0)
    return;
  qsort (list1, len1, sizeof (char *), sortfun);

  list2 = read_a_dir (dirname2, &len2);
  if (len2 <= 0)
    return;
  qsort (list2, len2, sizeof (char *), sortfun);

  for (j = 0, k = 0; j < len1 && k < len2; )
    {
      cmp = strcmp (list1[j], list2[k]);
      if (cmp == 0)
	{
	  j++;
	  k++;
	}
      else if (cmp < 0)
	{
	  matches = false;
	  printf ("%s/%s: extra file\n", dirname1, list1[j]);
	  j++;
	  while (j < len1 && (cmp = strcmp (list1[j], list2[k]), cmp < 0))
	    {
	      printf ("%s/%s: extra file\n", dirname1, list1[j]);
	      j++;
	    }
	}
      else
	{
	  matches = false;
	  printf ("%s/%s: missing file\n", dirname1, list2[k]);
	  k++;
	  while (k < len2 && (cmp = strcmp (list1[j], list2[k]), cmp > 0))
	    {
	      printf ("%s/%s: missing file\n", dirname1, list2[k]);
	      k++;
	    }
	}
    }
  while (j < len1)
    printf ("%s/%s: extra file\n", dirname1, list1[j++]);
  while (k < len2)
    printf ("%s/%s: missing file\n", dirname2, list2[k++]);

  if (matches && verbose_flag)
    printf ("OK: %s dir contents match\n", dirname1);
}

static size_t
first_diff (const char *buf1, const char *buf2, size_t len)
{
  size_t k;

  for (k = 0; k < len; k++)
    if (buf1[k] != buf2[k])
      return k;
  return len;
}

static bool
verify (const char *file1, const char *file2)
{
  struct stat stat1;
  struct stat stat2;
  int rc;

  rc = lstat (file1, &stat1);
  if (rc < 0)
    {

      if (non_exist_flag)
	{
	  if ((errno == ENOENT) || (errno == ENOTDIR))
	    {
	      if (verbose_flag)
		printf ("OK: %s does not exist\n", file1);
	      return true;
	    }
	  else
	    {
	      perror (file1);
	      printf ("%s error, but might exist\n", file1);
	      errors++;
	      return false;
	    }
	}
      perror (file1);
      errors++;
      return false;
    }

  if (non_exist_flag)
    {
      printf ("%s exists, should not\n", file1);
      errors++;
      return false;
    }

  if (file2)
    {
      rc = lstat (file2, &stat2);
      if (rc < 0)
	{
	  perror (file2);
	  errors++;
	  return false;
	}
    }

  if (reg_flag)
    {

      if (S_ISREG (stat1.st_mode))
	{
	  printf ("%s not regular file\n", file1);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s is a regular file\n", file1);
    }

  if (dir_flag)
    {
      if (S_ISDIR (stat1.st_mode))
	{
	  printf ("%s not directory\n", file1);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s is a directory\n", file1);
    }

#ifdef S_ISLNK
  if (symlink_flag)
    {
      if (S_ISLNK (stat1.st_mode))
	{
	  printf ("%s not a sym link\n", file1);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s is a a sym link\n", file1);
    }
#endif

  if (size_flag)
    {
      if (stat1.st_size != size_value)
	{
	  printf ("%s size %d should be %d\n",
		   file1, stat1.st_size, size_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s size\n", file1);
    }

  if (size_match_flag)
    {
      if ((stat1.st_size != stat2.st_size) && !S_ISDIR (stat1.st_mode))
	{
	  printf ("%s size match %d should be %d\n",
		  file1, stat1.st_size, stat2.st_size);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s size match\n", file1);
    }

  if (mtime_flag)
    {
      if (stat1.st_mtime != mtime_value)
	{
	  printf ("%s mtime %d should be %d\n",
		  file1, stat1.st_mtime, mtime_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s mtime\n", file1);
    }

  if (mtime_ge_flag)
    {
      if (stat1.st_mtime < mtime_ge_value)
	{
	  printf ("%s mtime %d should be >= %d\n",
		  file1, stat1.st_mtime, mtime_ge_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s mtime ge\n", file1);
    }

  if (mtime_le_flag)
    {
      if (stat1.st_mtime > mtime_le_value)
	{
	  printf ("%s mtime %d should be <= %d\n",
		  file1, stat1.st_mtime, mtime_le_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s mtime le\n", file1);
    }

  if (mtime_match_flag)
    {
      if ((stat1.st_mtime != stat2.st_mtime)
	  && !(ignore_dir_mtime_flag && S_ISDIR (stat1.st_mode))
#ifdef S_ISLNK
	  && !(ignore_link_mtime_flag && S_ISLNK (stat1.st_mode))
#endif
	  && !(ignore_dot_mtime_flag && ((strcmp (file1, ".") == 0)
					 || (strcmp (file1, "./") == 0)))
	  )
	{
	  printf ("%s mtime match %d should be %d\n",
		  file1, stat1.st_mtime, stat2.st_mtime);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s mtime match\n", file1);
    }

  if (atime_flag)
    {
      if (stat1.st_atime != atime_value)
	{
	  printf ("%s atime %d should be %d\n",
		  file1, stat1.st_atime, atime_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s atime\n", file1);
    }

  if (atime_ge_flag)
    {
      if (stat1.st_atime < atime_ge_value)
	{
	  printf ("%s atime %d should be >= %d\n",
		  file1, stat1.st_atime, atime_ge_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s atime ge\n", file1);
    }

  if (atime_le_flag)
    {
      if ((stat1.st_atime > atime_le_value)
	   && (!ignore_dir_atime_flag && S_ISDIR (stat1.st_mode)))
	{
	  printf ("%s atime %d should be <= %d\n",
		  file1, stat1.st_atime, atime_ge_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s atime le\n", file1);
    }

  if (atime_match_flag)
    {
      if ((stat1.st_atime != stat2.st_atime)
	   && (!ignore_dir_atime_flag && S_ISDIR (stat1.st_mode)))
	{
	  printf ("%s atime match %d should be %d\n",
		  file1, stat1.st_atime, stat2.st_atime);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s atime match\n", file1);
    }

  if (uid_flag)
    {
      if (stat1.st_uid != uid_value)
	{
	  printf ("%s uid %d should be %d\n",
		  file1, stat1.st_uid, uid_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s uid\n", file1);
    }

  if (uid_match_flag)
    {
      if (stat1.st_uid != stat2.st_uid)
	{
	  printf ("%s uid match %d should be %d\n",
		  file1, stat1.st_uid, stat2.st_uid);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s uid match\n", file1);
    }

  if (gid_flag)
    {
      if (stat1.st_gid != gid_value)
	{
	  printf ("%s gid %d should be %d\n",
		  file1, stat1.st_gid, gid_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s gid\n", file1);
    }

  if (gid_match_flag)
    {
      if (stat1.st_gid != stat2.st_gid)
	{
	  printf ("%s gid match %d should be %d\n",
		  file1, stat1.st_gid, stat2.st_gid);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s gid match\n", file1);
    }

  if (ino_flag)
    {
      if (stat1.st_ino != ino_value)
	{
	  printf ("%s ino %d should be %d\n",
		  file1, stat1.st_ino, ino_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s ino\n", file1);
    }

  if (ino_match_flag)
    {
      if ((stat1.st_ino != stat2.st_ino)
	   && !(ignore_dir_ino_flag && S_ISDIR (stat1.st_mode))
#ifdef S_ISLNK
	   && !(ignore_link_ino_flag && S_ISLNK (stat1.st_mode)))
#endif
	{
	  printf ("%s ino match %d should be %d\n",
		  file1, stat1.st_ino, stat2.st_ino);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s ino match\n", file1);
    }

  if (nlink_flag)
    {
      if (stat1.st_nlink != nlink_value)
	{
	  printf ("%s nlink %d should be %d\n",
		  file1, stat1.st_nlink, nlink_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s nlink\n", file1);
    }

  if (nlink_match_flag)
    {
      if (stat1.st_nlink != stat2.st_nlink)
	{
	  printf ("%s nlink match %d should be %d\n",
		  file1, stat1.st_nlink, stat2.st_nlink);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s nlink match\n", file1);
    }

  if (mode_flag)
    {
      if (stat1.st_mode != mode_value)
	{
	  printf ("%s mode 0%o should be 0%o\n",
		  file1, stat1.st_mode, mode_value);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s mode\n", file1);
    }

  if (mode_match_flag)
    {
      if (stat1.st_mode != stat2.st_mode)
	{
	  printf ("%s mode match 0%o should be 0%o\n",
		  file1, stat1.st_mode, stat2.st_mode);
	  errors++;
	}
      else if (verbose_flag)
	printf ("OK: %s mode match\n", file1);
    }

  if (contents_match_flag)
    {
      if (S_ISREG (stat1.st_mode))
	{
	  int fd1, fd2;
	  char buf1[BUFSIZ], buf2[BUFSIZ];
	  int rc1, rc2;
	  size_t bytes;

	  fd1 = open (file1, 0);
	  if (fd1 < 0)
	    {
	      perror (file1);
	      errors++;
	      return false;
	    }
	  fd2 = open (file2, 0);
	  if (fd2 < 0)
	    {
	      perror (file2);
	      errors++;
	      return false;
	    }
	  bytes = 0;
	  while (true)
	    {
	      rc1 = read (fd1, buf1, BUFSIZ);
	      rc2 = read (fd2, buf2, BUFSIZ);
	      if (rc1 < 0)
		{
		  perror (file1);
		  errors++;
		  return false;
		}
	      if (rc2 < 0)
		{
		  perror (file2);
		  errors++;
		  return false;
		}
	      if ((rc1 != rc2) || (rc1 != BUFSIZ))
		{
		  if (rc1 != rc2)
		    {
		      printf ("%s lengths and contents differ at bytes %d\n",
			      file1, bytes + MIN (rc1, rc2) + 1);
		      errors++;
		    }
		  else
		    {
		      if (memcmp (buf1, buf2, rc1))
			{
			  printf ("%s contents differ at byte %d\n", file1,
				   bytes + first_diff (buf1, buf2, rc1) + 1);
			  errors++;
			}
		      else if (verbose_flag)
			printf ("OK: %s contents match\n", file1);
		    }
		  break;
		}
	      if (memcmp (buf1, buf2, rc1))
		{
		  printf ("%s contents differ at byte %d\n", file1,
			  bytes + first_diff (buf1, buf2, rc1) + 1);
		  errors++;
		  break;
		}
	      bytes += BUFSIZ;
	    }
	  close (fd1);
	  close (fd2);
	}
#ifdef S_ISLNK
      else if (S_ISLNK (stat1.st_mode))
	{
	  char link1[4096], link2[4096];
	  int rc1, rc2;
	  rc1 = readlink (file1, link1, 4096);
	  rc2 = readlink (file2, link2, 4096);
	  if (rc1 < 0)
	    {
	      perror (file1);
	      errors++;
	      return false;
	    }
	  if (rc2 < 0)
	    {
	      perror (file2);
	      errors++;
	      return false;
	    }
	  if (strcmp (link1, link2))
	    {
	      printf ("%s link contents differ\n", file1);
	      errors++;
	    }
	  else if (verbose_flag)
	    printf ("OK: %s link contents\n", file1);
	}
#endif	/* S_ISLNK */
      else if (S_ISDIR (stat1.st_mode))
	content_match_dirs (file1, file2);
      else if (S_ISCHR (stat1.st_mode))
	{
	  if (stat1.st_rdev != stat2.st_rdev)
	    {
	      printf ("%s char dev contents differ, 0x%x, should be 0x%x\n",
		      file1, stat1.st_rdev, stat2.st_rdev);
	      errors++;
	    }
	  else if (verbose_flag)
	    printf ("OK: %s char dev contents\n", file1);
	}
      else if (S_ISBLK (stat1.st_mode))
	{
	  if (stat1.st_rdev != stat2.st_rdev)
	    {
	      printf ("%s block dev contents differ, 0x%x, should be 0x%x\n",
		      file1, stat1.st_rdev, stat2.st_rdev);
	      errors++;
	    }
	  else if (verbose_flag)
	    printf ("OK: %s block dev contents\n", file1);
	}
#ifdef S_IFIFO
      else if (S_ISFIFO (stat1.st_mode))
	{
	  /* is this the right check? */
	  if (stat1.st_rdev != stat2.st_rdev)
	    {
	      printf ("%s fifo dev contents differ, 0x%x, should be 0x%x\n",
		      file1, stat1.st_rdev, stat2.st_rdev);
	      errors++;
	    }
	  else if (verbose_flag)
	    printf ("OK: %s fifo dev contents\n", file1);
	}
#endif
      else
	{
	  printf ("can't contents-match file type 0%o yet\n", stat1.st_mode);
	  errors++;
	  return false;
	}
    }

  return errors > 0;
}

/* Option decoding and main control.  */

#define OPTION_ATIME      1
#define OPTION_ATIME_GE   2
#define OPTION_ATIME_LE   3
#define OPTION_GID        4
#define OPTION_INO        5
#define OPTION_MATCH_DIR  6
#define OPTION_MATCH_FILE 7
#define OPTION_MODE       8
#define OPTION_MTIME      9
#define OPTION_MTIME_GE  10
#define OPTION_MTIME_LE  11
#define OPTION_NLINK     12
#define OPTION_SIZE      13
#define OPTION_UID       14

static struct option long_options[] =
{
  {"atime", required_argument, NULL, OPTION_ATIME},
  {"atime-ge", required_argument, NULL, OPTION_ATIME_GE},
  {"atime-le", required_argument, NULL, OPTION_ATIME_LE},
  {"atime-match", no_argument, (int *) &atime_match_flag, (int) true},
  {"contents-match", no_argument, (int *) &contents_match_flag, (int) true},
  {"dir", no_argument, (int *) &dir_flag, (int) true},
  {"gid", required_argument, NULL, OPTION_GID},
  {"gid-match", no_argument, (int *) &gid_match_flag, (int) true},
  {"ignore-dir-atime", no_argument, (int *) &ignore_dir_atime_flag, (int) true},
  {"ignore-dir-ino", no_argument, (int *) &ignore_dir_ino_flag, (int) true},
  {"ignore-dir-mtime", no_argument, (int *) &ignore_dir_mtime_flag, (int) true},
  {"ignore-dot-mtime", no_argument, (int *) &ignore_dot_mtime_flag, (int) true},
  {"ignore-link-ino", no_argument, (int *) &ignore_link_ino_flag, (int) true},
  {"ignore-link-mtime", no_argument, (int *) &ignore_link_mtime_flag, (int) true},
  {"ino", required_argument, NULL, OPTION_INO},
  {"ino-match", no_argument, (int *) &ino_match_flag, (int) true},
  {"list", no_argument, (int *) &list_flag, (int) true},
  {"match-dir", required_argument, NULL, OPTION_MATCH_DIR},
  {"match-file", required_argument, NULL, OPTION_MATCH_FILE},
  {"mode", required_argument, NULL, OPTION_MODE},
  {"mode-match", no_argument, (int *) &mode_match_flag, (int) true},
  {"mtime", required_argument, NULL, OPTION_MTIME},
  {"mtime-ge", required_argument, NULL, OPTION_MTIME_GE},
  {"mtime-le", required_argument, NULL, OPTION_MTIME_LE},
  {"mtime-match", no_argument, (int *) &mtime_match_flag, (int) true},
  {"nlink", required_argument, NULL, OPTION_NLINK},
  {"nlink-match", no_argument, (int *) &nlink_match_flag, (int) true},
  {"non-exist", no_argument, (int *) &non_exist_flag, (int) true},
  {"reg", no_argument, (int *) &reg_flag, (int) true},
  {"size", required_argument, NULL, OPTION_SIZE},
  {"size-match", no_argument, (int *) &size_match_flag, (int) true},
  {"symlink", no_argument, (int *) &symlink_flag, (int) true},
  {"uid", required_argument, NULL, OPTION_UID},
  {"uid-match", no_argument, (int *) &uid_match_flag, (int) true},
  {"verbose", no_argument, (int *) &verbose_flag, (int) true},
};

static void
usage (void)
{
  fputs ("\
Usage: verify [-attributes] file\n\
  or : verify [-attributes] -list < file\n\
  or : verify -match-file file [-attributes] [-attributes-match] file\n\
  or : verify -match-dir dir [-attributes] [-attributes-match] file\n\
  or : verify -match-dir dir [-attributes] [-attributes-match] -list < file\n",
	 stderr);
  exit (1);
}

static int
decode_int (const char *s)
{
  int val = 0;
  int sign = 1;

  while (*s == ' ')
    s++;
  if (*s == '0')
    {
      s++;
      if (*s == 'x')
	{
	  fprintf (stderr, "hex not supported yet\n");
	  usage ();
	}
      while (*s >= '0' && *s <= '7')
	val = 8 * val + *s++ - '0';
      if (*s != '\0')
	fprintf (stderr, "decode_int: ignoring \"%s\"\n", s);
      return val;
    }
  while (*s >= '0' && *s <= '9')
    val = 10 * val + *s++ - '0';
  if (*s != '\0')
    fprintf (stderr, "decode_int: ignoring \"%s\"\n", s);
  return val;
}

int
main (int argc, const char *const *argv)
{
  const char *file1 = NULL;
  char *file2 = NULL;
  int optchar;

  while (optchar =  getopt_long_only (argc, argv, "", long_options, NULL),
	 optchar != EOF)
    switch (optchar)
     {
     case '?':
       usage ();

     case 0:
       break;

     case OPTION_ATIME:
       atime_flag = 1;
       atime_value = decode_int (optarg);
       break;

     case OPTION_ATIME_GE:
       atime_ge_flag = 1;
       atime_ge_value = decode_int (optarg);
       break;

     case OPTION_ATIME_LE:
       atime_le_flag = 1;
       atime_le_value = decode_int (optarg);
       break;

     case OPTION_GID:
       gid_flag = 1;
       gid_value = decode_int (optarg);
       break;

     case OPTION_INO:
       ino_flag = 1;
       ino_value = decode_int (optarg);
       break;

     case OPTION_MATCH_DIR:
       match_dir = optarg;
       break;

     case OPTION_MATCH_FILE:
       match_file = optarg;
       break;

     case OPTION_MODE:
       mode_flag = 1;
       mode_value = decode_int (optarg);
       break;

     case OPTION_MTIME:
       mtime_flag = 1;
       mtime_value = decode_int (optarg);
       break;

     case OPTION_MTIME_GE:
       mtime_ge_flag = 1;
       mtime_ge_value = decode_int (optarg);
       break;

     case OPTION_MTIME_LE:
       mtime_le_flag = 1;
       mtime_le_value = decode_int (optarg);
       break;

     case OPTION_NLINK:
       nlink_flag = 1;
       nlink_value = decode_int (optarg);
       break;

     case OPTION_SIZE:
       size_value = decode_int (optarg);
       break;

     case OPTION_UID:
       uid_flag = 1;
       uid_value = decode_int (optarg);
       break;
     }

#if 0
  if (!strcmp (argv[optind], "-not"))
    not_flag = not_flag ? 0 : 1;
  else if (!strcmp (argv[optind], "-all-match"))
    {
      mode_match_flag = 1;
      ino_match_flag = 1;
      nlink_match_flag = 1;
      size_match_flag = 1;
      mtime_match_flag = 1;
      /* atime_match_flag = 1; */ /* fix JUO! */
      uid_match_flag = 1;
      gid_match_flag = 1;
      size_match_flag = 1;
      contents_match_flag = 1;
    }
#endif

  if (list_flag)
    {
      char filename1[4096], filename2[4096];
      strcpy (filename2, "/missing/filename/junk");
      if ((optind < argc)
	  || (!match_dir && (size_match_flag || mtime_match_flag
			     || atime_match_flag
			     || contents_match_flag || ino_match_flag
			     || nlink_match_flag || uid_match_flag
			     || gid_match_flag || mode_match_flag)))
	usage ();

      if (match_dir)
	{
	  while (gets (filename1))
	    {
	      sprintf (filename2, "%s/%s", match_dir, filename1);
	      verify (filename1, filename2);
	    }
	}
      else
	while (gets (filename1))
	  verify (filename1, NULL);
      exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  if (optind < argc)
    file1 = argv[optind++];
  else
    usage ();

  if (match_dir)
    {
      file2 = (char *) malloc (strlen (match_dir) + strlen (file1) + 3);
      sprintf (file2, "%s/%s", match_dir, file1);
    }
  else if (match_file)
    file2 = match_file;

  if (!file1)
    usage ();
  verify (file1, file2);
  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
