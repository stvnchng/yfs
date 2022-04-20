#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include "yfs.h"
#include "iohelpers.h"


/*
 *  YFS User Request Procedures
 */
int YFSOpen(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data2);
    if (pathname == NULL) {
        printf("Bad arguments passed to YFSCreate\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->data1, OPEN);
    if (inum == ERROR) return ERROR;
    msg->data1 = inum;

    if (get_inode(inum) == NULL) {
        return ERROR;
    }

    return inum;
}

int YFSCreate(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data2);
    if (pathname == NULL) {
        printf("Bad arguments passed to YFSCreate\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->data1, CREATE);
    if (inum == ERROR) return ERROR;
    msg->data1 = inum;

    return 0;
}

int YFSRead(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

int YFSWrite(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

int YFSSeek(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

int YFSLink(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

int YFSUnlink(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

// int SymLink(char *, char *);
// int ReadLink(char *pathname, char *buf, int len);

int YFSMkDir(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data2);
    if (pathname == NULL) {
        printf("Bad arguments passed to YFSCreate\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->data1, MKDIR);
    if (inum == ERROR) return ERROR;
    msg->data1 = inum;

    return 0;
}

int YFSRmDir(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data2);
    if (pathname == NULL) {
        printf("Bad arguments passed to YFSCreate\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->data1, RMDIR);
    if (inum == ERROR) return ERROR;
    msg->data1 = inum;

    return 0;
}

int YFSChDir(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

int YFSStat(struct msg *msg, int pid)
{
    (void)msg;
    (void)pid;
    return 0;
}

int YFSSync(void)
{
    TracePrintf(0, "Writing back all dirty cached inodes and blocks...\n");
    struct cache_item *inode_head = inode_cache->head;
    while (inode_head != NULL) {
        if (inode_head->dirty) {
            // TODO Section 4.1 last paragraph

        }
        inode_head = inode_head->next;
    }
    TracePrintf(0, "Now writing back all dirty cached blocks...\n");
    struct cache_item *block_head = block_cache->head;
    while (block_head != NULL) {
        // If inode is dirty, Sync writes it back
        if (block_head->dirty) {
            if (WriteSector(block_head->num, block_head->value) == ERROR) {
                printf("WriteSector error in Sync, block %d\n", block_head->num);
                return ERROR;
            }
        }
        block_head = block_head->next;
    }
    return 0;
}

int YFSShutdown(struct msg *msg, int pid)
{
    YFSSync(); // write back all dirty cached inodes and disk blocks
    Reply(msg, pid); // reply to the calling process and exit
    Exit(0);
}

void HandleRequest(struct msg *msg)
{
    int pid = Receive(msg);
    if (pid == ERROR) {
        printf("Receive message failed! YFS is shutting down...\n");
        YFSShutdown(msg, pid);
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
        YFSRead(msg, pid);
        break;
    case WRITE:
        TracePrintf(0, "[WRITE]\n");
        printf("[WRITE]\n");
        YFSWrite(msg, pid);
        break;
    case SEEK:
        TracePrintf(0, "[SEEK]\n");
        printf("[SEEK]\n");
        YFSSeek(msg, pid);
        break;
    case LINK:
        TracePrintf(0, "[LINK]\n");
        printf("[LINK]\n");
        YFSLink(msg, pid);
        break;
    case UNLINK:
        TracePrintf(0, "[UNLINK]\n");
        printf("[UNLINK]\n");
        YFSUnlink(msg, pid);
        break;
    case SYMLINK:
        TracePrintf(0, "[SYMLINK]\n");
        printf("[SYMLINK]\n");
        // SymLink(msg, pid);
        break;
    case READLINK:
        TracePrintf(0, "[READLINK]\n");
        printf("[READLINK]\n");
        // ReadLink(msg, pid);
        break;
    case MKDIR:
        TracePrintf(0, "[MKDIR]\n");
        printf("[MKDIR]\n");
        YFSMkDir(msg, pid);
        break;
    case RMDIR:
        TracePrintf(0, "[RMDIR]\n");
        printf("[RMDIR]\n");
        YFSRmDir(msg, pid);
        break;
    case CHDIR:
        TracePrintf(0, "[CHDIR]\n");
        printf("[CHDIR]\n");
        YFSChDir(msg, pid);
        break;
    case STAT:
        TracePrintf(0, "[STAT]\n");
        printf("[STAT]\n");
        YFSStat(msg, pid);
        break;
    case SYNC:
        TracePrintf(0, "[SYNC]\n");
        printf("[SYNC]\n");
        YFSSync();
        break;
    case SHUTDOWN:
        TracePrintf(0, "[SHUTDOWN]\n");
        printf("[SHUTDOWN]\n");
        YFSShutdown(msg, pid);
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

char *GetMessagePath(int srcpid, void *src, int pathlen)
{
    char *dest = malloc(sizeof(char) *  pathlen);
    if (CopyFrom(srcpid, dest, src, pathlen) == ERROR) {
        printf("Error occurred at CopyFrom in GetMessagePath\n");
        return NULL;
    }
    return dest;
}
