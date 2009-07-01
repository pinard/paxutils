/*
  BSD directory emulation functions for Windows NT
  Written by Drew Bliss, Nov 93
  */
#include <stdlib.h>
#include <stdio.h>
#include <sys\types.h>
#include <errno.h>
#include <string.h>

#include <windows.h>

#include "ndir.h"

static void
find_data_to_dirent(WIN32_FIND_DATA *fd, struct dirent *ent)
{
    /* Inode zero means non-existent, so use one instead */
    ent->d_ino = 1;

    ent->d_namlen = strlen(fd->cFileName);
    strncpy(ent->d_name, fd->cFileName, MAXNAMLEN);

    ent->d_reclen = sizeof(struct dirent)-(MAXNAMLEN+1)+ent->d_namlen;
}

DIR *
opendir(const char *filename)
{
    DIR *dirp;
    char path[_MAX_PATH + 4];
    int ln;
    WIN32_FIND_DATA fd;

    dirp = (DIR *)malloc(sizeof(DIR));
    if (dirp == NULL)
    {
        errno = ENOMEM;
        goto EH_Fail;
    }

    strcpy(path, filename);
    ln = strlen(path)-1;
    if (path[ln] != '/' && path[ln] != '\\' && path[ln] != ':')
    {
        ln++;
        path[ln] = '\\';
    }
    strcpy(path+ln+1, "*.*");

    dirp->no_files = FALSE;

    dirp->h = FindFirstFile(path, &fd);
    if (dirp->h == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_NO_MORE_FILES ||
            GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            dirp->no_files = TRUE;
        }
        else
        {
            errno = EINVAL;
            goto EH_dirp;
        }
    }
    else
    {
        find_data_to_dirent(&fd, &dirp->ent);
    }

    dirp->first = TRUE;

    return dirp;

 EH_dirp:
    free(dirp);
 EH_Fail:
    return NULL;
}

int
closedir(register DIR *dirp)
{
    if (dirp->h != INVALID_HANDLE_VALUE)
        FindClose(dirp->h);
    free(dirp);
	return 0;
}

struct dirent *
readdir(register DIR *dirp)
{
    WIN32_FIND_DATA fd;
    char *name;
    int no_read;

    if (dirp->no_files)
    {
        errno = 0;
        return NULL;
    }

    if (dirp->first)
    {
        name = dirp->ent.d_name;
        no_read = FALSE;
        dirp->first = FALSE;
    }
    else
    {
        name = NULL;
        no_read = TRUE;
    }

    /* Don't return . or .. since it doesn't look like any of the
       readdir callers expect them */
    while (no_read ||
           strcmp(name, ".") == 0 ||
           strcmp(name, "..") == 0)
    {
        if (!FindNextFile(dirp->h, &fd))
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
                errno = 0;
            else
                errno = ENOENT;
            return NULL;
        }

        name = fd.cFileName;
        no_read = FALSE;
    }

    if (name != dirp->ent.d_name)
        find_data_to_dirent(&fd, &dirp->ent);

    return &dirp->ent;
}
