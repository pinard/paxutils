/*
 * verify.c - A program that verifies characteristics of files.  If the
 * characteristics are "OK", verify is silent (unless the -verbose
 * switch is used); otherwise verify prints that actual and expected
 * values that don't match.
 *
 * $ verify -size 12 file		# verify that the size of `file' is
 * $ verify -verbose -size 12 file	# 12.
 * OK: junk size.
 * $ verify -size 11 file
 * file size 12 should be 11.
 * $ verify -mode 0100644 file	# verify that file the file type
 * # and protection mode bits for file
 * # are 0100644 (octal).
 * $ verify -mtime-gt 1 file	# verify that file was modified
 * # after 1/1/70 00:00:01 GMT.
 *
 * These are some of the basic options to verify:
 *
 * -size bytes
 * -mtime seconds-since-epoch
 * -mtime-gt seconds-since-epoch
 * -atime seconds-since-epoch
 * -atime-gt seconds-since-epoch
 * -atime-lt seconds-since-epoch
 * -uid user-id-number
 * -gid group-id-number
 * -ino inode-number
 * -mode file-type-and-protection-modes
 * -nlink links-to-file
 *
 *
 * Verify can also operate on a list of files.
 *
 * $ cat list			# verify that file1, file2 and file3
 * file1				# are all owned by uid 706.
 * file2
 * file3
 * $ ./verify -uid 706 < list
 *
 * Verify can also compare the characteristics of a file to another
 * file, and print any differences:
 *
 * $ ls -l file file2
 * -rw-r--r--   1 foo      bar            12 May 29 11:16 file
 * -rw-r--r--   1 foo      bar            15 May  6 17:15 file2
 * $ ./verify -match-file file2 -size-match -mode-match -uid-match file
 * file size match 12 should be 15.
 *
 * When comparing 2 files with the -match-file option, verify can also
 * take these options:
 *
 * -size-match
 * -mtime-match
 * -atime-match
 * -uid-match
 * -gid-match
 * -ino-match
 * -mode-match
 * -nlink-match
 *
 * When verify is comparing 2 files, the -contents-match option can be
 * used to compare the contents of the 2 files:
 *
 * $ ./verify -match-file file2 -contents-match file
 * file lengths and contents differ at bytes 13.
 * $ ./verify -match-file file -contents-match file
 *
 * If the files are actually directories, instead of regular files, verify
 * compares their enteries (it only verifies their names, not any characteristics
 * of the files themselves):
 *
 * $ ls dir1 dir2
 * dir1:
 * a  b  c
 *
 * dir2:
 * a  d
 * $ ./verify -match-file dir1 -contents-match dir2
 * dir2/b: missing file.
 * dir2/c: missing file.
 * dir2/d: extra file.
 *
 * If the files are symbolic links, verify makes sure that they point
 * to the same file:
 *
 * $ cd dir2
 * $ ln -s a b
 * $ ln -s a c
 * $ ln -s d f
 * $ ../verify -match-file b -contents-match c
 * $ ../verify -match-file f -contents-match c
 * c link contents differ.
 *
 * Verify can compare the characteristics of a file to a file with the same
 * name in another directory.
 *
 * $ ls -l file dir/file
 * -rw-r--r--   1 foo      bar            12 May 29 11:16 file
 * -rw-r--r--   1 foo      bar            12 May 29 11:40 dir/file
 * $ ./verify -match-dir dir -size-match -mode-match -mtime-match file
 * file mtime match 707152592 should be 707154016.
 *
 * Finally, verify can compare the characteristics of a list of files in
 * the current directory and another directory:
 *
 * $ cat list
 * file
 * file2
 * -rw-r--r--   1 foo      bar            12 May 29 11:40 dir/file
 * -rw-------   1 foo      bar            26 May 29 11:42 dir/file2
 * -rw-r--r--   1 foo      bar            12 May 29 11:16 file
 * -rw-r--r--   1 foo      bar            26 May 29 11:43 file2
 * $ ./verify -match-dir dir -mode-match -mtime-match -list < list
 * file mtime match 707152592 should be 707154016.
 * file2 mtime match 707154185 should be 707154132.
 * file2 mode match 0100644 should be 0100600.
 */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#if STDC_HEADERS || HAVE_STRING_H
#include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
#if !STDC_HEADERS && HAVE_MEMORY_H
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#define index strchr
#define rindex strrchr
#define bcopy(s, d, n) memcpy ((d), (s), (n))
#define bcmp(s1, s2, n) memcmp ((s1), (s2), (n))
#define bzero(s, n) memset ((s), 0, (n))
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
/* memory.h and strings.h conflict on some systems.  */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#if 0
/* portable directory stuff stolen from tar */
#ifdef BSD42
#include <sys/dir.h>
#else
#ifdef MSDOS
#include <sys/dir.h>
#else
#ifdef USG
#ifdef NDIR
#include <ndir.h>
#else
#include <dirent.h>
#endif
#ifndef DIRECT
#define direct dirent
#endif
#define DP_NAMELEN(x) strlen((x)->d_name)
#else
#include "ndir.h"
#endif
#endif
#endif

#ifndef DP_NAMELEN
#define DP_NAMELEN(x)	(x)->d_namlen
#endif

#endif /* 0 */


#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif



#if 0

#ifdef USG
#define bcmp(s1,s2,n)	memcmp((s1),(s2),(n))
#endif

#endif /* 0 */

#include "filetypes.h"

#define MIN(a,b)	(((a)<(b))?(a):(b))

char **read_a_dir ();

int size_flag = 0;
int size_value = 0;
int size_match_flag = 0;
int mtime_flag = 0;
int mtime_gt_flag = 0;
int mtime_value = 0;
int mtime_gt_value = 0;
int mtime_match_flag = 0;
int atime_flag = 0;
int atime_gt_flag = 0;
int atime_lt_flag = 0;
int atime_value = 0;
int atime_gt_value = 0;
int atime_lt_value = 0;
int atime_match_flag = 0;
int ignore_dir_atime_flag = 0;
int ignore_dir_mtime_flag = 0;
int ignore_dot_mtime_flag = 0;
int ignore_link_mtime_flag = 1;
int ignore_dir_ino_flag = 0;
int ignore_link_ino_flag = 0;
int contents_match_flag = 0;
int ino_flag = 0;
int ino_value = 0;
int ino_match_flag = 0;
int nlink_flag = 0;
int nlink_value = 0;
int nlink_match_flag = 0;
int uid_flag = 0;
int uid_value = 0;
int uid_match_flag = 0;
int gid_flag = 0;
int gid_value = 0;
int gid_match_flag = 0;
int mode_flag = 0;
int mode_value = 0;
int mode_match_flag = 0;
int reg_flag = 0;
int dir_flag = 0;
int symlink_flag = 0;
int other_flag = 0;
int list_flag = 0;
int not_flag = 0;
int exist_flag = 0;
int non_exist_flag = 0;
char *match_dir = NULL;
char *match_file = NULL;

int verbose_flag = 0;
int errors = 0;

main (argc, argv)
  int	argc;
  char	*argv[];
{
  char	*file1 = NULL;
  char	*file2 = NULL;
  int		optind;

  for (optind = 1; optind < argc; ++optind)
    {
      if (argv [optind][0] != '-')
	break;
      if (!strcmp (argv [optind], "-match-dir") )
	{
	  ++optind;
	  if (optind < argc)
	    match_dir = argv [optind];
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-match-file") )
	{
	  ++optind;
	  if (optind < argc)
	    match_file = argv [optind];
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-size") )
	{
	  size_flag = 1;
	  ++optind;
	  if (optind < argc)
	    size_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-mtime") )
	{
	  mtime_flag = 1;
	  ++optind;
	  if (optind < argc)
	    mtime_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-mtime-gt") )
	{
	  mtime_gt_flag = 1;
	  ++optind;
	  if (optind < argc)
	    mtime_gt_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-atime") )
	{
	  atime_flag = 1;
	  ++optind;
	  if (optind < argc)
	    atime_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-atime-gt") )
	{
	  atime_gt_flag = 1;
	  ++optind;
	  if (optind < argc)
	    atime_gt_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-atime-lt") )
	{
	  atime_lt_flag = 1;
	  ++optind;
	  if (optind < argc)
	    atime_lt_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-list") )
	{
	  list_flag = 1;
	}
      else if (!strcmp (argv [optind], "-size-match") )
	{
	  size_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-mtime-match") )
	{
	  mtime_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-atime-match") )
	{
	  atime_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ignore-dir-atime") )
	{
	  ignore_dir_atime_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ignore-dir-mtime") )
	{
	  ignore_dir_mtime_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ignore-dot-mtime") )
	{
	  ignore_dot_mtime_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ignore-link-mtime") )
	{
	  ignore_link_mtime_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ignore-dir-ino") )
	{
	  ignore_dir_ino_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ignore-link-ino") )
	{
	  ignore_link_ino_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-contents-match") )
	{
	  contents_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-non-exist") )
	{
	  non_exist_flag = 1;
	}
      else if (!strcmp (argv [optind], "-uid") )
	{
	  uid_flag = 1;
	  ++optind;
	  if (optind < argc)
	    uid_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-uid-match") )
	{
	  uid_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-gid") )
	{
	  gid_flag = 1;
	  ++optind;
	  if (optind < argc)
	    gid_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-gid-match") )
	{
	  gid_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-ino") )
	{
	  ino_flag = 1;
	  ++optind;
	  if (optind < argc)
	    ino_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-ino-match") )
	{
	  ino_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-mode") )
	{
	  mode_flag = 1;
	  ++optind;
	  if (optind < argc)
	    mode_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-mode-match") )
	{
	  mode_match_flag = 1 ^ not_flag;
	}
      else if (!strcmp (argv [optind], "-nlink") )
	{
	  nlink_flag = 1;
	  ++optind;
	  if (optind < argc)
	    nlink_value = atoi (argv [optind]);
	  else
	    usage ();
	}
      else if (!strcmp (argv [optind], "-nlink-match") )
	{
	  nlink_match_flag = 1 ^ not_flag;
	}

      else if (!strcmp (argv [optind], "-not") )
	not_flag = not_flag ? 0 : 1;
      else if (!strcmp (argv [optind], "-all-match") )
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
      else if (!strcmp (argv [optind], "-reg") )
	reg_flag = 1;
      else if (!strcmp (argv [optind], "-dir") )
	dir_flag = 1;
      else if (!strcmp (argv [optind], "-symlink") )
	symlink_flag = 1;
      else if (!strcmp (argv [optind], "-verbose") )
	verbose_flag = 1;
      else
	{
	  fprintf (stderr, "illegal switch %s\n", argv [optind]);
	  usage ();
	}
    }
  if (list_flag)
    {
      char filename1 [4096], filename2 [4096];
      strcpy (filename2, "/missing/filename/junk");
      if ( (optind < argc)
            || (!match_dir && (size_match_flag || mtime_match_flag
				|| atime_match_flag
				|| contents_match_flag || ino_match_flag
				|| nlink_match_flag || uid_match_flag
				|| gid_match_flag || mode_match_flag) ) )
	usage ();

      if (match_dir)
	{
	  while (gets (filename1) )
	    {
	      sprintf (filename2, "%s/%s", match_dir, filename1);
	      verify (filename1, filename2);
	    }
	}
      else
	{
	  while (gets (filename1) )
	    {
	      verify (filename1, NULL);
	    }
	}
      return (errors);
    }
  else
    {
      if (optind < argc)
	file1 = argv [optind++];
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
      return (errors);
    }
}


verify (file1, file2)
  char	*file1;
  char	*file2;
{
  struct stat	stat1;
  struct stat	stat2;
  int		rc;

  rc = lstat (file1, &stat1);
  if (rc < 0)
    {

      if (non_exist_flag)
	{
	  if ( (errno == ENOENT) || (errno == ENOTDIR) )
	    {
	      if (verbose_flag)
		fprintf (stderr, "OK: %s does not exist.\n", file1);
	      return (0);
	    }
	  else
	    {
	      perror (file1);
	      fprintf (stderr, "%s error, but might exist.\n", file1);
	      ++errors;
	      return (1);
	    }
	}
      perror (file1);
      ++errors;
      return (1);
    }

  if (non_exist_flag)
    {
      fprintf (stderr, "%s exists, should not.\n", file1);
      ++errors;
      return (1);
    }

  if (file2)
    {
      rc = lstat (file2, &stat2);
      if (rc < 0)
	{
	  perror (file2);
	  ++errors;
	  return (1);
	}
    }

  if (reg_flag)
    {

      if (S_ISREG (stat1.st_mode) )
	{
	  fprintf (stderr, "%s not regular file.\n", file1);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s is a regular file.\n", file1);
    }

  if (dir_flag)
    {
      if (S_ISDIR (stat1.st_mode) )
	{
	  fprintf (stderr, "%s not directory.\n", file1);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s is a directory.\n", file1);
    }

#ifdef S_ISLNK
  if (symlink_flag)
    {
      if (S_ISLNK (stat1.st_mode) )
	{
	  fprintf (stderr, "%s not a sym link.\n", file1);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s is a a sym link.\n", file1);
    }
#endif

  if (size_flag)
    {
      if (stat1.st_size != size_value)
	{
	  fprintf (stderr, "%s size %d should be %d.\n", file1, stat1.st_size, size_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s size.\n", file1);
    }

  if (size_match_flag)
    {
      if ( (stat1.st_size != stat2.st_size) && !S_ISDIR (stat1.st_mode) )
	{
	  fprintf (stderr, "%s size match %d should be %d.\n", file1, stat1.st_size, stat2.st_size);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s size match.\n", file1);
    }

  if (mtime_flag)
    {
      if (stat1.st_mtime != mtime_value)
	{
	  fprintf (stderr, "%s mtime %d should be %d.\n", file1, stat1.st_mtime, mtime_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s mtime.\n", file1);
    }

  if (mtime_gt_flag)
    {
      if (stat1.st_mtime <= mtime_gt_value)
	{
	  fprintf (stderr, "%s mtime %d should be > %d.\n", file1, stat1.st_mtime, mtime_gt_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s mtime gt.\n", file1);
    }

  if (mtime_match_flag)
    {
      if ( (stat1.st_mtime != stat2.st_mtime)
	    && !(ignore_dir_mtime_flag && S_ISDIR (stat1.st_mode) )
#ifdef S_ISLNK
	    && !(ignore_link_mtime_flag && S_ISLNK (stat1.st_mode) )
#endif
	    && !(ignore_dot_mtime_flag && ( (strcmp (file1, ".") == 0)
					     || (strcmp (file1, "./") == 0) ) )
)
	{
	  fprintf (stderr, "%s mtime match %d should be %d.\n", file1, stat1.st_mtime, stat2.st_mtime);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s mtime match.\n", file1);
    }

  if (atime_flag)
    {
      if (stat1.st_atime != atime_value)
	{
	  fprintf (stderr, "%s atime %d should be %d.\n", file1, stat1.st_atime, atime_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s atime.\n", file1);
    }

  if (atime_gt_flag)
    {
      if (stat1.st_atime <= atime_gt_value)
	{
	  fprintf (stderr, "%s atime %d should be > %d.\n", file1, stat1.st_atime, atime_gt_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s atime gt.\n", file1);
    }

  if (atime_lt_flag)
    {
      if ( (stat1.st_atime >= atime_lt_value)
	    && (!ignore_dir_atime_flag && S_ISDIR (stat1.st_mode) ) )
	{
	  fprintf (stderr, "%s atime %d should be < %d.\n", file1, stat1.st_atime, atime_gt_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s atime lt.\n", file1);
    }

  if (atime_match_flag)
    {
      if ( (stat1.st_atime != stat2.st_atime)
	    && (!ignore_dir_atime_flag && S_ISDIR (stat1.st_mode) ) )
	{
	  fprintf (stderr, "%s atime match %d should be %d.\n", file1, stat1.st_atime, stat2.st_atime);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s atime match.\n", file1);
    }

  if (uid_flag)
    {
      if (stat1.st_uid != uid_value)
	{
	  fprintf (stderr, "%s uid %d should be %d.\n", file1, stat1.st_uid, uid_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s uid.\n", file1);
    }

  if (uid_match_flag)
    {
      if (stat1.st_uid != stat2.st_uid)
	{
	  fprintf (stderr, "%s uid match %d should be %d.\n", file1, stat1.st_uid, stat2.st_uid);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s uid match.\n", file1);
    }

  if (gid_flag)
    {
      if (stat1.st_gid != gid_value)
	{
	  fprintf (stderr, "%s gid %d should be %d.\n", file1, stat1.st_gid, gid_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s gid.\n", file1);
    }

  if (gid_match_flag)
    {
      if (stat1.st_gid != stat2.st_gid)
	{
	  fprintf (stderr, "%s gid match %d should be %d.\n", file1, stat1.st_gid, stat2.st_gid);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s gid match.\n", file1);
    }

  if (ino_flag)
    {
      if (stat1.st_ino != ino_value)
	{
	  fprintf (stderr, "%s ino %d should be %d.\n", file1, stat1.st_ino, ino_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s ino.\n", file1);
    }

  if (ino_match_flag)
    {
      if ( (stat1.st_ino != stat2.st_ino)
	    && !(ignore_dir_ino_flag && S_ISDIR (stat1.st_mode) )
#ifdef S_ISLNK
	    && !(ignore_link_ino_flag && S_ISLNK (stat1.st_mode) ) )
#endif
	{
	  fprintf (stderr, "%s ino match %d should be %d.\n", file1, stat1.st_ino, stat2.st_ino);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s ino match.\n", file1);
    }

  if (nlink_flag)
    {
      if (stat1.st_nlink != nlink_value)
	{
	  fprintf (stderr, "%s nlink %d should be %d.\n", file1, stat1.st_nlink, nlink_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s nlink.\n", file1);
    }

  if (nlink_match_flag)
    {
      if (stat1.st_nlink != stat2.st_nlink)
	{
	  fprintf (stderr, "%s nlink match %d should be %d.\n", file1, stat1.st_nlink, stat2.st_nlink);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s nlink match.\n", file1);
    }

  if (mode_flag)
    {
      if (stat1.st_mode != mode_value)
	{
	  fprintf (stderr, "%s mode 0%o should be 0%o.\n", file1, stat1.st_mode, mode_value);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s mode.\n", file1);
    }

  if (mode_match_flag)
    {
      if (stat1.st_mode != stat2.st_mode)
	{
	  fprintf (stderr, "%s mode match 0%o should be 0%o.\n", file1, stat1.st_mode, stat2.st_mode);
	  ++errors;
	}
      else if (verbose_flag)
	 fprintf (stderr, "OK: %s mode match.\n", file1);
    }

  if (contents_match_flag)
    {
      if (S_ISREG (stat1.st_mode) )
	{
	  int	fd1, fd2;
	  char	buf1 [BUFSIZ], buf2 [BUFSIZ];
	  int	rc1, rc2;
	  int	bytes;

	  fd1 = open (file1, 0);
	  if (fd1 < 0)
	    {
	      perror (file1);
	      ++errors;
	      return (1);
	    }
	  fd2 = open (file2, 0);
	  if (fd2 < 0)
	    {
	      perror (file2);
	      ++errors;
	      return (1);
	    }
	  bytes = 0;
	  while (1)
	    {
	      rc1 = read (fd1, buf1, BUFSIZ);
	      rc2 = read (fd2, buf2, BUFSIZ);
	      if (rc1 < 0)
		{
		  perror (file1);
		  ++errors;
		  return (1);
		}
	      if (rc2 < 0)
		{
		  perror (file2);
		  ++errors;
		  return (1);
		}
	      if ( (rc1 != rc2) || (rc1 != BUFSIZ) )
		{
		  if (rc1 != rc2)
		    {
		      fprintf (stderr, "%s lengths and contents differ at bytes %d.\n", file1, bytes + MIN (rc1, rc2) + 1);
		      ++errors;
		    }
		  else
		    {
		      if (bcmp (buf1, buf2, rc1) )
			{
			  fprintf (stderr, "%s contents differ at byte %d.\n", file1,
			    bytes + first_diff (buf1, buf2, rc1) + 1);
			  ++errors;
			}
		      else if (verbose_flag)
			fprintf (stderr, "OK: %s contents match.\n", file1);
		    }
		  break;
		}
	      if (bcmp (buf1, buf2, rc1) )
		{
		  fprintf (stderr, "%s contents differ at byte %d.\n", file1,
		    bytes + first_diff (buf1, buf2, rc1) + 1);
		  ++errors;
		  break;
		}
	      bytes += BUFSIZ;
	    }
	  close (fd1);
	  close (fd2);
	}
#ifdef S_ISLNK
      else if (S_ISLNK (stat1.st_mode) )
	{
	  char	link1 [4096], link2 [4096];
	  int	rc1, rc2;
	  rc1 = readlink (file1, link1, 4096);
	  rc2 = readlink (file2, link2, 4096);
	  if (rc1 < 0)
	    {
	      perror (file1);
	      ++errors;
	      return (1);
	    }
	  if (rc2 < 0)
	    {
	      perror (file2);
	      ++errors;
	      return (1);
	    }
	  if (strcmp (link1, link2) )
	    {
	      fprintf (stderr, "%s link contents differ.\n", file1);
	      ++errors;
	    }
	  else if (verbose_flag)
	      fprintf (stderr, "OK: %s link contents.\n", file1);
	}
#endif	/* S_ISLNK */
      else if (S_ISDIR (stat1.st_mode) )
	{
	  content_match_dirs (file1, file2);
	}
      else if (S_ISCHR (stat1.st_mode) )
	{
	  if (stat1.st_rdev != stat2.st_rdev)
	    {
	      fprintf (stderr, "%s char dev contents differ, 0x%x, should be 0x%x.\n", file1, stat1.st_rdev, stat2.st_rdev);
	      ++errors;
	    }
	  else if (verbose_flag)
	      fprintf (stderr, "OK: %s char dev contents.\n", file1);
	}
      else if (S_ISBLK (stat1.st_mode) )
	{
	  if (stat1.st_rdev != stat2.st_rdev)
	    {
	      fprintf (stderr, "%s block dev contents differ, 0x%x, should be 0x%x.\n", file1, stat1.st_rdev, stat2.st_rdev);
	      ++errors;
	    }
	  else if (verbose_flag)
	      fprintf (stderr, "OK: %s block dev contents.\n", file1);
	}
#ifdef S_IFIFO
      else if (S_ISFIFO (stat1.st_mode) )
	{
	  /* is this the right check? */
	  if (stat1.st_rdev != stat2.st_rdev)
	    {
	      fprintf (stderr, "%s fifo dev contents differ, 0x%x, should be 0x%x.\n", file1, stat1.st_rdev, stat2.st_rdev);
	      ++errors;
	    }
	  else if (verbose_flag)
	      fprintf (stderr, "OK: %s fifo dev contents.\n", file1);
	}
#endif
      else
	{
	  fprintf (stderr, "can't contents-match file type 0%o yet\n", stat1.st_mode);
	  ++errors;
	  return (1);
	}
    }

  return errors;
}

first_diff (buf1, buf2, len)
  char	*buf1;
  char	*buf2;
  int	len;
{
  int	k;

  for (k = 0; k < len; ++k)
    if (buf1 [k] != buf2 [k])
      return k;
  return len;
}

usage()
{
  fprintf (stderr, "usage: verify [-attributes] file\n");
  fprintf (stderr, "usage: verify [-attributes] -list < file\n");
  fprintf (stderr, "usage: verify -match-file file [-attributes] [-attributes-match] file\n");
  fprintf (stderr, "usage: verify -match-dir dir [-attributes] [-attributes-match] file\n");
  fprintf (stderr, "usage: verify -match-dir dir [-attributes] [-attributes-match] -list < file\n");
  exit (1);
}

atoi (s)
  char	*s;
{
  int	val = 0;
  int	sign = 1;

  while (*s == ' ')
    ++s;
  if (*s == '0')
    {
      ++s;
      if (*s == 'x')
	{
	  fprintf (stderr, "hex not supported yet\n");
	  usage ();
	}
      while ( (*s >= '0') && (*s <= '7') )
	val = 8 * val + *s++ - '0';
      if (*s != '\0')
	fprintf (stderr, "atoi: ignoring \"%s\"\n", s);
      return (val);
    }
  while ( (*s >= '0') && (*s <= '9') )
    val = 10 * val + *s++ - '0';
  if (*s != '\0')
    fprintf (stderr, "atoi: ignoring \"%s\"\n", s);
  return (val);
}
Bcmp (b1, b2, len)
  char	*b1, *b2;
  int	len;
{
  while (len > 0)
    {
      if (*b1 != *b2)
	return (*b1 - *b2);
      ++b1;
      ++b2;
      --len;
    }
  return (0);
}


int
sortfun (p, q)
  char	**p;
  char	**q;
{
  return (strcmp (*p, *q) );
}

content_match_dirs (dirname1, dirname2)
  char	*dirname1;
  char	*dirname2;
{
  char 	**list1, **list2;
  char  **p;
  int	len1, len2;
  int	dontmatch;
  int	j, k, cmp;

  dontmatch = 0;

  list1 = read_a_dir (dirname1, &len1);
  if (len1 <= 0)
    return (1);
  qsort (list1, len1, sizeof (char *), sortfun);

  list2 = read_a_dir (dirname2, &len2);
  if (len2 <= 0)
    return (1);
  qsort (list2, len2, sizeof (char *), sortfun);
  for (j = 0, k = 0; j < len1 && k < len2; )
    {
      if ( (cmp = strcmp (list1 [j], list2 [k]) ) == 0)
	{
	  ++j;
	  ++k;
	}
      else if (cmp < 0)
	{
	  dontmatch = 1;
	  fprintf (stderr, "%s/%s: extra file.\n", dirname1, list1 [j]);
	  ++j;
	  while ( (j < len1) && ( (cmp = strcmp (list1 [j], list2 [k]) ) < 0) )
	    {
	      fprintf (stderr, "%s/%s: extra file.\n", dirname1, list1 [j]);
	      ++j;
	    }
	}
      else
	{
	  dontmatch = 1;
	  fprintf (stderr, "%s/%s: missing file.\n", dirname1, list2 [k]);
	  ++k;
	  while ( (k < len2) && ( (cmp = strcmp (list1 [j], list2 [k]) ) > 0) )
	    {
	      fprintf (stderr, "%s/%s: missing file.\n", dirname1, list2 [k]);
	      ++k;
	    }
	}
    }
  if (j < len1)
    {
      for ( ;j < len1; ++j)
	fprintf (stderr, "%s/%s: extra file.\n", dirname1, list1 [j]);
    }
  if (k < len2)
    {
      for ( ; k < len2; ++k)
	fprintf (stderr, "%s/%s: missing file.\n", dirname2, list2 [k]);
    }

  if (dontmatch == 0 && verbose_flag)
    fprintf (stderr, "OK: %s dir contents match\n", dirname1);
}

char **
read_a_dir (dirname, num_entries)
  char	*dirname;
  int	*num_entries;
{
  DIR *dirp;
  /* struct direct *dp; */
  struct dirent *dp;
  int  alloced;
  int  used;
  char **list;
  int len;

  alloced = 10;
  list = (char **) malloc (alloced * sizeof (char *) );
  used = 0;

  dirp = opendir (dirname);
  if (dirp == NULL)
    {
      perror (dirname);
      ++errors;
      *num_entries = -1;
      return ( (char **) NULL);
    }
  while (dp = readdir (dirp) )
    {
      if ( (used + 1) >= alloced)
	{
	  alloced += 10;
	  list = (char **) realloc (list, alloced * sizeof (char *) );
	}
      len = NAMLEN(dp);
      list [used] = (char *) malloc (len + 1);
      strncpy (list [used], dp->d_name, len);
      list [used][len] = '\0';
      ++used;
    }
  list [used] = NULL;
  closedir (dirp);
  *num_entries = used;
  return (list);
}
