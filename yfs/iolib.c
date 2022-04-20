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
int GetPathLen(char *pathname);
int SendMessage(int type);
int SendMessageWithPath(int type, char *pathname);
int SendMessageRW(int type, struct opened_file curr_file, void* buf, int size);

/*
 *  YFS Message Send Handlers
 */
int Open(char *pathname)
{
    if (SendMessageWithPath(OPEN, pathname) == ERROR) {
        printf("Error in send message for Open\n");
        return ERROR;
    }

    int fd = 0;
    while (fd < MAX_OPEN_FILES) {
        if (opened_files[fd].occupied == 0) {
            break;
        }
        fd++;
    }

    if (fd == MAX_OPEN_FILES) {
        printf("Error: fd for %s out of bounds\n", pathname);
        return ERROR;
    }

    opened_files[fd].inum = curr_inum;
    opened_files[fd].occupied = 1;
    opened_files[fd].position = 0;
    
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
    if (SendMessageWithPath(CREATE, pathname) == ERROR) {
        printf("Error in send message for Create\n");
        return ERROR;
    }
    
    int fd = 0;
    while (fd < MAX_OPEN_FILES) {
        if (opened_files[fd].occupied == 0) {
            break;
        }
        fd++;
    }

    if (fd == MAX_OPEN_FILES) {
        printf("Error: fd for %s out of bounds\n", pathname);
        return ERROR;
    }

    opened_files[fd].inum = curr_inum;
    opened_files[fd].occupied = 1;
    opened_files[fd].position = 0;

    return fd;
}

int Read(int fd, void *buf, int size) // TODO might need to read over
{
   if (fd < 0 || fd > MAX_OPEN_FILES - 1 || buf == NULL || size < 0) {
        printf("Bad arguments to Read\n");
        return ERROR;
    }

    struct opened_file curr_file = opened_files[fd];
    if (curr_file.occupied == 0) {
        printf("Error: Write to a nonexistent file\n");
        return ERROR;
    }

    int bytes_read = SendMessageRW(READ, curr_file, buf, size);
    if (bytes_read == ERROR) {
        printf("Error in SendRWMessage for Write\n");
        return ERROR;
    }

    // increment position of file to be written
    curr_file.position += bytes_read;
    return bytes_read;
}

int Write(int fd, void *buf, int size)
{
    if (fd < 0 || fd > MAX_OPEN_FILES - 1 || buf == NULL || size < 0) {
        printf("Bad arguments to Write\n");
        return ERROR;
    }

    struct opened_file curr_file = opened_files[fd];
    if (curr_file.occupied == 0) {
        printf("Error: Write to a nonexistent file\n");
        return ERROR;
    }

    int bytes_written = SendMessageRW(WRITE, curr_file, buf, size);
    if (bytes_written == ERROR) {
        printf("Error in SendRWMessage for Write\n");
        return ERROR;
    }

    // increment position of file to be written
    curr_file.position += bytes_written;
    return bytes_written;
}

int Seek(int  fd, int offset, int whence) // TODO: needs review
{
    if (fd < 0 || fd > MAX_OPEN_FILES - 1 || (whence != SEEK_CUR && whence != SEEK_END && whence != SEEK_SET)) {
        printf("Invalid args to Seek\n");
        return ERROR;
    }

    if (opened_files[fd].occupied == 0) {
        printf("Error: Seek called on a nonexistent file\n");
        return ERROR;
    }

    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = SEEK;
    msg->data1 = opened_files[fd].inum; //store inum
    msg->data2 = offset; //store len for server CopyFrom operation 
    msg->data3 = whence; //store len for server CopyFrom operation 
    // msg->ptr1 = ; 
    //TODO need to update offset and store more members depending on yfsseek
    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for STAT\n");
        return ERROR;
    }
    free(msg);
    return opened_files[fd].position; //return new offset 
}

int Link(char *oldname, char *newname)
{
    if (oldname == NULL || newname == NULL) {
        printf("Bad arguments to Link\n");
        return ERROR;
    }
    if (GetPathLen(oldname) == ERROR || GetPathLen(newname) == ERROR) {
        printf("Invalid arg length to Link\n");
        return ERROR;
    }

    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = LINK;
    msg->data1 = curr_inum; //store inum
    msg->data2 = GetPathLen(oldname); //store len for server CopyFrom operation 
    msg->data3 = GetPathLen(newname); //store len for server CopyFrom operation 
    msg->ptr1 = oldname;
    msg->ptr2 = newname;
    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for STAT\n");
        return ERROR;
    }
    free(msg);
    return 0;
}

int Unlink(char *pathname)
{   
    if (SendMessageWithPath(UNLINK, pathname) == ERROR) {
        printf("Error in send message for Unlink\n");
        return ERROR;
    }
    return 0;
}

// int SymLink(char *, char *);
// int ReadLink(char *, char *, int);

int MkDir(char *pathname)
{
    if (SendMessageWithPath(MKDIR, pathname) == ERROR) {
        printf("Error in send message for Unlink\n");
        return ERROR;
    }
    return 0;
}

int RmDir(char *pathname)
{
    if (SendMessageWithPath(RMDIR, pathname) == ERROR) {
        printf("Error in send message for Unlink\n");
        return ERROR;
    }
    return 0;
}

int ChDir(char *pathname)
{
    if (SendMessageWithPath(CHDIR, pathname) == ERROR) {
        printf("Error in send message for Unlink\n");
        return ERROR;
    }
    return 0;
}

int Stat(char *pathname, struct Stat *statbuf)
{    
    if (pathname == NULL || statbuf == NULL || GetPathLen(pathname)== ERROR ) {
        printf("Path length not valid in Stat\n");
        return ERROR;
    }
    
    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = STAT;
    msg->data1 = curr_inum; //store inum
    msg->data2 = GetPathLen(pathname); //store len for server CopyFrom operation 
    msg->ptr1 = pathname;
    msg->ptr2 = statbuf;
    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for STAT\n");
        return ERROR;
    }
    free(msg);
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
    if (GetPathLen(pathname) == ERROR) {
        printf("Error: pathname has invalid length\n");
        return ERROR;
    }
    
    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = type;
    msg->data1 = curr_inum; // store inode_num
    msg->data2 = GetPathLen(pathname); // store len for server CopyFrom operation
    msg->ptr1 = pathname;

    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for type %d\n", msg->type);
        return ERROR;
    }
    free(msg);
    return 0;
}

int SendMessageRW(int type, struct opened_file curr_file, void* buf, int size)
{
    if (buf == NULL || size < 0) {
        printf("Invalid args to SendMessageRW\n");
        return ERROR;
    }

     struct msg *msg = malloc(sizeof(struct msg));
    msg->type = type;
    msg->data1 = curr_file.inum; // store inode_num
    msg->data2 = curr_file.position; // store position in curr file
    msg->data3 = size; // store len for server CopyFrom operation
    msg->ptr2 = buf;

    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for type %d\n", msg->type);
        return ERROR;
    }
    free(msg);
    return 0;
}

int GetPathLen(char *pathname)
{
    int len = 0;
    while (len < MAXPATHNAMELEN && pathname[len] != '\0') {
        len++;
    }
    if (len == MAXPATHNAMELEN || len == 0) return ERROR;
    return len;
}

