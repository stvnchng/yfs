#include <stdlib.h>
#include <string.h>
#include "yfs.h"

/*
 *  Function prototypes for YFS calls:
 */
int Open(char *pathname)
{
    (void)pathname;
    return 0;
}

int Close(int fd)
{
    
    return fd;
}

int Create(char *pathname)
{
    (void)pathname;
    return 0;
}

int Read(int fd, void *buf, int size)
{
    (void)fd;
    (void)buf;
    (void)size;
    return 0;
}

int Write(int fd, void *buf, int size)
{
    (void)fd;
    (void)buf;
    (void)size;
    return 0;
}

int Seek(int fd, int offset, int whence)
{
    return fd && offset && whence;
}

int Link(char *oldname, char *newname)
{
    (void)oldname;
    (void)newname;
    return 0;
}

int Unlink(char *pathname)
{   
    (void)pathname;
    return 0;
}

int SymLink(char *, char *);
int ReadLink(char *pathname, char *buf, int len)
{
    (void)pathname;
    (void)buf;
    return len;
}

int MkDir(char *pathname)
{
    (void)pathname;
    return 0;
}

int RmDir(char *pathname)
{
    (void)pathname;
    return 0;
}

int ChDir(char *pathname)
{
    (void)pathname;
    return 0;
}

int Stat(char *pathname, struct Stat *statbuf)
{
    (void)pathname;
    (void)statbuf;
    return 0;
}

int Sync(void)
{
    return 0;
}

int Shutdown(void)
{
    return 0;
}
