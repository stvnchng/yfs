#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include "yfs.h"

struct opened_file {
    int inum;
    int occupied; // is the current slot in use
    int position; // position that is pointed to in the file
};

struct opened_file opened_files[MAX_OPEN_FILES];

int curr_inum = ROOTINODE;

/**
 *  IOLib helper functions prototypes
 */
int SendMessage(int type);
int SendMessageWithPath(int type, char *pathname);

/*
 *  YFS Message Send Handlers
 */
int Open(char *pathname)
{
    SendMessageWithPath(OPEN, pathname);

    int fd = 0;
    while (fd < MAX_OPEN_FILES) {
        if (opened_files[fd].occupied == 0) {
            opened_files[fd].inum = curr_inum;
            opened_files[fd].occupied = 1;
            opened_files[fd].position = 0;
            break;
        }
        fd++;
    }
    
    return fd;
}

int Close(int fd)
{
    if (fd < 0 || fd > MAX_OPEN_FILES - 1) {
        printf("Invalid fd %d provided for Close.\n", fd);
        return ERROR;
    }

    opened_files[fd].occupied = 0; // fd is not occupied anymore

    return 0;
}

int Create(char *pathname)
{
    SendMessageWithPath(CREATE, pathname);
    
    int fd = 0;
    while (fd < MAX_OPEN_FILES) {
        // find the lowest fd that is not occupied and return it
        if (opened_files[fd].occupied == 0) {
            opened_files[fd].inum = curr_inum;
            opened_files[fd].occupied = 1;
            opened_files[fd].position = 0;
            break;
        }
        fd++;
    }

    if (fd == MAX_OPEN_FILES) {
        printf("Could not create valid fd\n");
        return ERROR;
    }

    return fd;
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

// int SymLink(char *, char *);
// int ReadLink(char *pathname, char *buf, int len)
// {
//     (void)pathname;
//     (void)buf;
//     return len;
// }

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
    return SendMessage(SYNC);
}

int Shutdown(void)
{
    return SendMessage(SHUTDOWN);
}

/* Helper method to send simple message */
int SendMessage(int type)
{
    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = type;
    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for type %d\n", msg->type);
        return ERROR;
    }
    free(msg);
    return 0;
}

/* Helper for requests with pathname param */
int SendMessageWithPath(int type, char *pathname)
{
    if (strlen(pathname) > MAXPATHNAMELEN) {
        printf("Error: pathname %s longer than MAXPATHNAMELEN\n", pathname);
        return ERROR;
    }

    int i = 0;
    while (i < MAX_OPEN_FILES) {
        if (!opened_files[i].occupied) {
            break;
        }
    }

    if (i == MAX_OPEN_FILES) {
        printf("Error: fd for %s out of bounds, index %d\n", pathname, i);
        return ERROR;
    }
    
    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = type;
    msg->data1 = curr_inum; // store inode_num
    msg->ptr1 = pathname;

    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for type %d\n", msg->type);
        return ERROR;
    }
    free(msg);
    return i; // return fd
}
