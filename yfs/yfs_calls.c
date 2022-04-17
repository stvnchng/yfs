#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include "yfs.h"


/*
 *  YFS User Request Procedures
 */
int YFSOpen(struct msg *msg, int pid)
{
    if (msg == NULL) {
        return ERROR;
    }

    return pid;
}

int YFSCreate(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1);
    int inum = msg->data1;

    if (inum < 0 || pathname == NULL) {
        printf("Bad arguments passed to Create\n");
        return ERROR;
    }


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
    TracePrintf(0, "Writing back all dirty cached inodes and blocks...\n");
    // define head of block cache
    // while head != null, check if cache_item = dirty
    // WriteSector back

    // define head of inode cache
    // while head != null, check if cache_item = dirty
    // WriteSector back

    return 0;
}

int Shutdown(void)
{
    TracePrintf(0, "Shutting down YFS server...\n");
    Sync(); // write back all dirty cached inodes and disk blocks
    Exit(0);
}

void HandleRequest(struct msg *msg)
{
    int pid = Receive(msg);
    if (pid == ERROR) {
        printf("Receive message failed! YFS is shutting down...\n");
        Shutdown();
    }

    switch (msg->type)
    {
    case OPEN:
        TracePrintf(0, "[OPEN]\n");
        printf("[OPEN]\n");
        YFSOpen(msg, pid);
        break;
    case CREATE:
        TracePrintf(0, "[CREATE]\n");
        printf("[CREATE]\n");
        YFSCreate(msg, pid);
        break;
    case READ:
        TracePrintf(0, "[READ]\n");
        printf("[READ]\n");
        Read(msg->data2, (char *)(msg->ptr1), msg->data3);
        break;
    case WRITE:
        TracePrintf(0, "[WRITE]\n");
        printf("[WRITE]\n");
        Write(msg->data2, (char *)(msg->ptr1), msg->data3);
        break;
    case SEEK:
        TracePrintf(0, "[SEEK]\n");
        printf("[SEEK]\n");
        Seek(msg->data1, msg->data2, msg->data3);
        break;
    case LINK:
        TracePrintf(0, "[LINK]\n");
        printf("[LINK]\n");
        Link((char *)(msg->ptr1), (char *)(msg->ptr2));
        break;
    case UNLINK:
        TracePrintf(0, "[UNLINK]\n");
        printf("[UNLINK]\n");
        Unlink((char *)(msg->ptr1));
        break;
    case SYMLINK:
        TracePrintf(0, "[SYMLINK]\n");
        printf("[SYMLINK]\n");
        // SymLink((char *)(msg->addr1), (char *)(msg->addr2));
        break;
    case READLINK:
        TracePrintf(0, "[READLINK]\n");
        printf("[READLINK]\n");
        // ReadLink((char *)(msg->ptr1), (char *)(msg->ptr2), msg->data2);
        break;
    case MKDIR:
        TracePrintf(0, "[MKDIR]\n");
        printf("[MKDIR]\n");
        MkDir((char *)(msg->ptr1));
        break;
    case RMDIR:
        TracePrintf(0, "[RMDIR]\n");
        printf("[RMDIR]\n");
        RmDir((char *)(msg->ptr1));
        break;
    case CHDIR:
        TracePrintf(0, "[CHDIR]\n");
        printf("[CHDIR]\n");
        ChDir((char *)(msg->ptr1));
        break;
    case STAT:
        TracePrintf(0, "[STAT]\n");
        printf("[STAT]\n");
        Stat((char *)(msg->ptr1), (struct Stat *)(msg->ptr2));
        break;
    case SYNC:
        TracePrintf(0, "[SYNC]\n");
        printf("[SYNC]\n");
        Sync();
        break;
    case SHUTDOWN:
        TracePrintf(0, "[SHUTDOWN]\n");
        printf("[SHUTDOWN]\n");
        Shutdown();
        break;
    default:
        printf("Message with type %d was not recognized!\n", msg->type);
        break;
    }

    // send reply to the blocked process
    if (Reply((void *) msg, pid) == ERROR) {
        printf("An error occurred during reply for pid %d\n", pid);
    }
}

char *GetMessagePath(int srcpid, void *src)
{
    char *dest = malloc(sizeof(char) *  MAXPATHNAMELEN);
    if (CopyFrom(srcpid, dest, src, MAXPATHNAMELEN) == ERROR) {
        printf("Error occurred during CopyFrom in GetMessagePath\n");
        return NULL;
    }
    return dest;
}