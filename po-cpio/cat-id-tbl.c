/* Automatically generated by po2tbl.sed from cpio.pot.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "libgettext.h"

const struct _msg_ent _msg_tbl[] = {
  {"", 1},
  {"%s: option `%s' is ambiguous\n", 2},
  {"%s: option `--%s' doesn't allow an argument\n", 3},
  {"%s: option `%c%s' doesn't allow an argument\n", 4},
  {"%s: option `%s' requires an argument\n", 5},
  {"%s: unrecognized option `--%s'\n", 6},
  {"%s: unrecognized option `%c%s'\n", 7},
  {"%s: illegal option -- %c\n", 8},
  {"%s: invalid option -- %c\n", 9},
  {"%s: option requires an argument -- %c\n", 10},
  {"%s: option `-W %s' is ambiguous\n", 11},
  {"%s: option `-W %s' doesn't allow an argument\n", 12},
  {"cannot make directory `%s'", 13},
  {"`%s' exists but is not a directory", 14},
  {"Success", 15},
  {"No match", 16},
  {"Invalid regular expression", 17},
  {"Invalid collation character", 18},
  {"Invalid character class name", 19},
  {"Trailing backslash", 20},
  {"Invalid back reference", 21},
  {"Unmatched [ or [^", 22},
  {"Unmatched ( or \\(", 23},
  {"Unmatched \\{", 24},
  {"Invalid content of \\{\\}", 25},
  {"Invalid range end", 26},
  {"Memory exhausted", 27},
  {"Invalid preceding regular expression", 28},
  {"Premature end of regular expression", 29},
  {"Regular expression too big", 30},
  {"Unmatched ) or \\)", 31},
  {"No previous regular expression", 32},
  {"virtual memory exhausted", 33},
  {"can not omit both user and group", 34},
  {"invalid user", 35},
  {"cannot get the login group of a numeric UID", 36},
  {"invalid group", 37},
  {"premature end of archive", 38},
  {"warning: skipped %ld bytes of junk", 39},
  {"standard input is closed", 40},
  {"%s: checksum error (0x%x, should be 0x%x)", 41},
  {"%s not created: newer or same age version exists", 42},
  {"%s already exists; not created", 43},
  {"cannot remove current %s", 44},
  {"cannot link %s to %s", 45},
  {"cannot swap halfwords of %s: odd number of halfwords", 46},
  {"cannot swap bytes of %s: odd number of bytes", 47},
  {"%s: unknown file type", 48},
  {"1 block\n", 49},
  {"%d blocks\n", 50},
  {"cannot open directory %s", 51},
  {"blank line ignored", 52},
  {"standard output is closed", 53},
  {"%s: file name too long", 54},
  {"%s: error resetting file access time", 55},
  {"%s not dumped: not a regular file", 56},
  {"%s: symbolic link too long", 57},
  {"cannot read checksum for %s", 58},
  {"%s linked to %s", 59},
  {"Usage: cpio [OPTION]... [EXTRA]...\n", 60},
  {"\
Mandatory or optional arguments to long options are mandatory or optional\n\
for short options too.\n", 61},
  {"\
\n\
Main operation mode:\n\
 -o, --create             create a new archive (copy-out)\n\
                           archive on stdout unless specified\n\
                           list of filenames on stdin\n\
 -i, --extract            extract files from an archive (copy-in)\n\
                           archive on stdin unless specified\n\
                           EXTRA argumentss are list of patterns to match\n\
 -p, --pass-through       copy files (copy-pass)\n\
                           list of filenames on stdin\n\
                           EXTRA argument is destination directory\n", 62},
  {"\
\n\
Other options:\n", 63},
  {" -0, --null               read null-terminated names\n", 64},
  {" -a, --reset-access-time  reset access time after reading file\n", 65},
  {" -A, --append             append to existing archive (copy-out)\n", 66},
  {" -b, --swap               swap both words and bytes of data\n", 67},
  {" -B                       set block size to 5120 bytes\n", 68},
  {" --block-size=SIZE        set block size to 512 * SIZE bytes\n", 69},
  {" -c                       use old portable (ASCII) format\n", 70},
  {" -C SIZE, --io-size=SIZE  set block size to SIZE bytes\n", 71},
  {" -d, --make-directories   make directories as needed\n", 72},
  {"\
 -E FILE, --pattern-file=FILE\n\
                          read filename patterns from FILE\n", 73},
  {" -f, --nonmatching        only copy files that do not match a pattern\n", 74},
  {" -F FILE, --file=FILE     use FILE for archive\n", 75},
  {" --force-local            archive file is local even if has a colon\n", 76},
  {"\
 -H FORMAT, --format=FORMAT\n\
                          use archive format FORMAT\n", 77},
  {" --help                   print this help and exit\n", 78},
  {" -I FILE                  use FILE as input archive (copy-in)\n", 79},
  {" -l, --link               link files instead of copying\n", 80},
  {"\
 -L, --dereference        don't dump symlinks; dump the files they point to\n", 81},
  {"\
 -m, --preserve-modification-time\n\
                          retain modification time when creating files\n", 82},
  {"\
 -M MESSAGE, --message=MESSAGE\n\
                          print message when end of volume reached\n", 83},
  {" -n, --numeric-uid-gid    show numeric uid and gid\n", 84},
  {" --no-absolute-filenames  create all files relative to cwd (copy-in)\n", 85},
  {" --no-preserve-owner      do not change ownership of files\n", 86},
  {" -O FILE                  use FILE as output archive (copy-out)\n", 87},
  {" --only-verify-crc        verify CRCs but do not extract (copy-out)\n", 88},
  {" --quiet, --silent        do not print number of blocks copied\n", 89},
  {" -r, --rename             interactively rename files\n", 90},
  {"\
 -R [USER][:.][GROUP], --owner=[USER][:.][GROUP]\n\
                          set ownership of new files to USER, GROUP\n", 91},
  {"\
 -s, --swap-bytes         swap bytes of each halfword in files (copy-in)\n", 92},
  {"\
 -S, --swap-halfwords     swap halfwords of each word in files (copy-in)\n", 93},
  {" --sparse                 write sparse files (copy-out, copy-pass)\n", 94},
  {" -t, --list               print table of contents of input\n", 95},
  {" -u, --unconditional      replace all files, even if older\n", 96},
  {" -v, --verbose            list files processed\n", 97},
  {" -V, --dot                print a `.' for each file processed\n", 98},
  {" --version                print version information and exit\n", 99},
  {"\
\n\
Report bugs to <bug-gnu-utils@prep.ai.mit.edu>\n", 100},
  {"invalid block size", 101},
  {"archive format specified twice", 102},
  {"only one of -o, -i, -p allowed", 103},
  {"only one of --no-preserve-owner and -R allowed", 104},
  {"cpio (GNU %s) %s\n", 105},
  {"unrecognized option -- `%c'", 106},
  {"one of -o, -i, -p must be specified", 107},
  {"-n only makes sense with -t and -v", 108},
  {"option conflicts with -i", 109},
  {"option conflicts with -o", 110},
  {"option conflicts with -p", 111},
  {"can't specify archive name with -p", 112},
  {"warning: archive header has reverse byte-order", 113},
  {"%s: truncating inode number", 114},
  {"invalid header: checksum error", 115},
  {"\
invalid archive format `%s'; valid formats are:\n\
crc, newc, odc, bin, ustar, tar, hpodc, hpbin", 116},
  {"error closing archive", 117},
  {"tape operation", 118},
  {"no tape device specified", 119},
  {"%s is not a character special file", 120},
  {"drive type = %d\n", 121},
  {"drive status (high) = %d\n", 122},
  {"drive status (low) = %d\n", 123},
  {"drive status = %d\n", 124},
  {"sense key error = %d\n", 125},
  {"residue count = %d\n", 126},
  {"file number = %d\n", 127},
  {"block number = %d\n", 128},
  {"\
Usage: %s [-V] [-f device] [--file=device] [--help] [--version] operation \
[count]\n", 129},
  {"Usage: pax [OPTION]... [FILE]... [DIRECTORY]\n", 130},
  {"\
\n\
Main operation mode:\n\
 -r, --read               extract files from an archive\n\
 -w, --write              create a new archive\n\
 with neither -r nor -w   list contents of archive\n\
 with both -r and -w      copy files\n", 131},
  {" -a, --append             append to archive\n", 132},
  {" -b N, --block-size=N     set block size to N\n", 133},
  {" -c, --nonmatching        only copy files that do not match a pattern\n", 134},
  {" --debug                  turn on debugging statements\n", 135},
  {"\
 -d, --directories-only   don't recurse into directories while writing\n", 136},
  {" -f FILE, --file=FILE     input or output file\n", 137},
  {" -i, --rename             interactively rename\n", 138},
  {" -k, --no-overwrite       don't overwrite existing file on extraction\n", 139},
  {" -n, --first-pattern      allow each pattern to match only once\n", 140},
  {" -p, --privileges         preserve permissions on extraction\n", 141},
  {"\
 -s REPLACEMENT, --replace=REPLACEMENT\n\
                          rename files according to REPLACEMENT\n", 142},
  {" -t, --reset-access-time  reset access time on extraction\n", 143},
  {" -u, --keep-old-files     only replace file if replacement is newer\n", 144},
  {" -v, --verbose            be verbose\n", 145},
  {"\
 -x FORMAT, --format=FORMAT\n\
                          set input or output format\n", 146},
  {" -X, --one-file-system    don't cross file systems\n", 147},
  {"parse error in blocksize", 148},
  {"block size cannot be 0", 149},
  {"unrecognized flag `%c' for -p; recognized flags are `aemop'", 150},
  {"rename %s -> ", 151},
  {"missing regexp", 152},
  {"null regexp", 153},
  {"%s while compiling pattern", 154},
  {"invalid regexp modifier `%c'", 155},
  {"exec/tcp: service not available", 156},
  {"cannot execute remote shell", 157},
  {"write error", 158},
  {"read error", 159},
  {"premature end of file", 160},
  {"File %s shrunk by %ld bytes, padding with zeros", 161},
  {"Read error at byte %ld in file %s, padding with zeros", 162},
  {"cannot seek on output", 163},
  {"Found end of tape.  Load next tape and press RETURN. ", 164},
  {"Found end of tape.  To continue, type device/file name when ready.\n", 165},
  {"To continue, type device/file name when ready.\n", 166},
  {"internal error: tape descriptor changed from %d to %d", 167},
};

int _msg_tbl_length = 167;
