#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "yfs.h"

/**
 *  YFS Global Variables
 */



/**
 * Initializes YFS and processes messages.
 * TODO: Run this command: ~comp421/pub/bin/yalnix yfs <args>
 * TODO: Run this to make a new DISK: ~comp421/pub/bin/mkyfs
 */
int
main(int argc, char **argv)
{
    // Initialize LRU cache and free lists
    if (InitYFS() < 0) {
        printf("An error occurred during initialization...\n");
        return ERROR;
    }

    if (Register(FILE_SERVER) < 0) {
        printf("Registering file server failed...\n");
        return ERROR;
    }

    if (argc > 1) {
        if (Fork() == 0) {
            Exec(argv[1], argv + 1);
        } 
        else {
            while (1) {
                struct msg *msg = malloc(sizeof(struct msg));

                if (Receive(msg) < 0) {
                    printf("Receive failed for msg with type %d...\n", msg->type);
                    return ERROR;
                }
                
                switch (msg->type)
                {
                case OPEN:
                    TracePrintf(0, "[OPEN]\n");
                    printf("[OPEN]\n");
                    Open((char *)(msg->addr1));
                    break;
                case CLOSE:
                    TracePrintf(0, "[CLOSE]\n");
                    printf("[CLOSE]\n");
                    Close(msg->data2);
                    break;
                case CREATE:
                    TracePrintf(0, "[CREATE]\n");
                    printf("[CREATE]\n");
                    Create((char *)(msg->addr1));
                    break;
                case READ:
                    TracePrintf(0, "[READ]\n");
                    printf("[READ]\n");
                    Read(msg->data2, (char *)(msg->addr1), msg->data3);
                    break;
                case WRITE:
                    TracePrintf(0, "[WRITE]\n");
                    printf("[WRITE]\n");
                    Write(msg->data2, (char *)(msg->addr1), msg->data3);
                    break;
                case SEEK:
                    TracePrintf(0, "[SEEK]\n");
                    printf("[SEEK]\n");
                    Seek(msg->data1, msg->data2, msg->data3);
                    break;
                case LINK:
                    TracePrintf(0, "[LINK]\n");
                    printf("[LINK]\n");
                    Link((char *)(msg->addr1), (char *)(msg->addr2));
                    break;
                case UNLINK:
                    TracePrintf(0, "[UNLINK]\n");
                    printf("[UNLINK]\n");
                    Unlink((char *)(msg->addr1));
                    break;
                case SYMLINK:
                    TracePrintf(0, "[SYMLINK]\n");
                    printf("[SYMLINK]\n");
                    // SymLink((char *)(msg->addr1), (char *)(msg->addr2));
                    break;
                case READLINK:
                    TracePrintf(0, "[READLINK]\n");
                    printf("[READLINK]\n");
                    ReadLink((char *)(msg->addr1), (char *)(msg->addr2), msg->data2);
                    break;
                case MKDIR:
                    TracePrintf(0, "[MKDIR]\n");
                    printf("[MKDIR]\n");
                    MkDir((char *)(msg->addr1));
                    break;
                case RMDIR:
                    TracePrintf(0, "[RMDIR]\n");
                    printf("[RMDIR]\n");
                    RmDir((char *)(msg->addr1));
                    break;
                case CHDIR:
                    TracePrintf(0, "[CHDIR]\n");
                    printf("[CHDIR]\n");
                    ChDir((char *)(msg->addr1));
                    break;
                case STAT:
                    TracePrintf(0, "[STAT]\n");
                    printf("[STAT]\n");
                    Stat((char *)(msg->addr1), (struct Stat *)(msg->addr2));
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
                // TODO: free memory used by msg on each new request
            }
        }
    }

    return (0);
}

int InitYFS()
{
    // initialize block and inode cache
    struct cache *inode_cache = malloc(sizeof(struct cache));
    struct cache *block_cache = malloc(sizeof(struct cache));

    struct hash_table *inode_ht = hash_table_create(INODE_CACHESIZE);
    struct hash_table *block_ht = hash_table_create(BLOCK_CACHESIZE);
    inode_cache->ht = inode_ht;
    block_cache->ht = block_ht;




    return 0;
}

/*
 *  YFS User Request Procedures
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
