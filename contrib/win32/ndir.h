/*
  BSD directory emulation functions for Windows NT
  Written by Drew Bliss, Nov 93
  */
#ifndef __WNT_DIR_H__
#define __WNT_DIR_H__

#define MAXNAMLEN _MAX_PATH

#include <sys\types.h>

struct dirent
{
    ino_t d_ino;
    int d_reclen;
    int d_namlen;
    char d_name[MAXNAMLEN + 1];
};

typedef struct _dirdesc
{
    void *h;
    struct dirent ent;
    int first;
    int no_files;
} DIR;

extern DIR *opendir(const char *path);
extern struct dirent *readdir(register DIR *dirp);
extern int closedir(register DIR *dirp);

#endif
