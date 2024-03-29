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
	int file_inum;
    if ((file_inum = SendMessageWithPath(OPEN, pathname)) == ERROR) {
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

    opened_files[fd].inum = file_inum;
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
    if (opened_files[fd].occupied == 0) {
        printf("Error: Attempted to close a non-open file in this process\n");
        return ERROR;
    }

    opened_files[fd].occupied = 0; // fd is not occupied anymore

    return 0;
}

int Create(char *pathname)
{
	int file_inum = SendMessageWithPath(CREATE, pathname);
    if (file_inum == ERROR) {
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

    opened_files[fd].inum = file_inum;
    opened_files[fd].occupied = 1;
    opened_files[fd].position = 0;

    return fd;
}

int Read(int fd, void *buf, int size) // TODO might need to read over
{
   if (fd < 0 || fd > MAX_OPEN_FILES - 1 || buf == NULL || size < 0) {
        printf("Invalid arguments to Read\n");
        return ERROR;
    }

    struct opened_file curr_file = opened_files[fd];
    if (curr_file.occupied == 0) {
        printf("Error: Read to a nonexistent file\n");
        return ERROR;
    }

    int bytes_read = SendMessageRW(READ, curr_file, buf, size);
    if (bytes_read == ERROR) {
        printf("Error in SendRWMessage for Read\n");
        return ERROR;
    }

    // increment position of file to be written
    opened_files[fd].position += bytes_read;
    return bytes_read;
}

int Write(int fd, void *buf, int size)
{
    if (fd < 0 || fd > MAX_OPEN_FILES - 1 || buf == NULL || size < 0) {
        printf("Invalid arguments to Write\n");
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
    opened_files[fd].position += bytes_written;
    return bytes_written;
}

int Seek(int fd, int offset, int whence)
{
    if (fd < 0 || fd > MAX_OPEN_FILES - 1 || (whence != SEEK_CUR && whence != SEEK_END && whence != SEEK_SET)) {
        printf("Invalid args to Seek\n");
        return ERROR;
    }

    if (opened_files[fd].occupied == 0) {
        printf("Error: Seek called on a nonexistent file\n");
        return ERROR;
    }

    struct msg_seek *msg = malloc(sizeof(struct msg_seek));
    msg->type = SEEK;
    msg->inum = opened_files[fd].inum; //store inum
    msg->position = opened_files[fd].position; //store file position
    msg->offset = offset; //store len for server CopyFrom operation 
    msg->whence = whence; //store len for server CopyFrom operation 
    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for SEEK\n");
        return ERROR;
    }
    int new_pos = msg->position; //return new offset from msg
    free(msg);
    return new_pos; 
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
    msg->inum = curr_inum; //store inum
    msg->data1 = GetPathLen(oldname) + 1; //store len for server CopyFrom operation 
    msg->data2 = GetPathLen(newname) + 1; //store len for server CopyFrom operation 
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

int MkDir(char *pathname)
{
    if (SendMessageWithPath(MKDIR, pathname) == ERROR) {
        printf("Error in send message for MkDir\n");
        return ERROR;
    }
    return 0;
}

int RmDir(char *pathname)
{
    if (SendMessageWithPath(RMDIR, pathname) == ERROR) {
        printf("Error in send message for RmDir\n");
        return ERROR;
    }
    return 0;
}

int ChDir(char *pathname)
{
    int new_inum = SendMessageWithPath(CHDIR, pathname);
    if (new_inum == ERROR) {
        printf("Error in send message for ChDir\n");
        return ERROR;
    }
    curr_inum = new_inum; // point curr inum to inum of chdir
    return 0;
}

int Stat(char *pathname, struct Stat *statbuf)
{    
    if (pathname == NULL || statbuf == NULL || GetPathLen(pathname)== ERROR ) {
        printf("Path length not valid in Stat\n");
        return ERROR;
    }
    
    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = STAT; // will be overwritten with inode type after Send
    msg->inum = curr_inum; //store inum
    msg->data1 = GetPathLen(pathname) + 1; // will be overwritten with size 
    msg->ptr1 = pathname;
    msg->ptr2 = statbuf;
    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for STAT\n");
        return ERROR;
    }
    // update statbuf with contents of reply message
    statbuf->inum = msg->inum; // should be the same
    statbuf->type = msg->type; // need to overwrite message type with inode type
    statbuf->size = msg->data1; // need to overwrite with size
    statbuf->nlink = msg->data2; // need to write nlink into data2
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
	int len;
    if ((len = GetPathLen(pathname)) == ERROR) {
        printf("Error: pathname has invalid length\n");
        return ERROR;
    }
	
    // modify pathname if needed
    if (pathname[len - 2] == '/' && pathname[len - 1] == '/') {
        // printf("Before modify name is [%s]\n", pathname);
        pathname[len - 2] = '\0';
	}
    
    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = type;
    msg->inum = curr_inum; // store inode_num
    msg->data1 = pathname[len - 1] == '\0' ? len : len + 1; // store len for server CopyFrom operation
    msg->ptr1 = pathname;

    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for type %d\n", msg->type);
        return ERROR;
    }
    int inum = msg->inum;
    free(msg);
    return inum;
}

int SendMessageRW(int type, struct opened_file curr_file, void* buf, int size)
{
    if (buf == NULL || size < 0) {
        printf("Invalid args to SendMessageRW\n");
        return ERROR;
    }

    struct msg *msg = malloc(sizeof(struct msg));
    msg->type = type;
    msg->inum = curr_file.inum; // store inode_num
    msg->data1 = curr_file.position; // store position in curr file
    msg->data2 = size; // store len for server CopyFrom operation
    msg->ptr2 = buf;

    if (Send((void *) msg, -FILE_SERVER) == ERROR) {
        free(msg);
        printf("Error during Send for type %d\n", msg->type);
        return ERROR;
    }
    int bytes_written = msg->data1; // collect reply data
    free(msg);
    return bytes_written;
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

