#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <stdbool.h>
#include "hash_table.h"


/**
 *  Definition for cache structure for INodes and Blocks 
 */
struct cache {
    int size;
    struct hash_table *ht;
    struct cache_item *head;
    struct cache_item *tail;
};

struct cache_item {
    bool dirty;
    int num; // hash table key
    void *value; // hash table values
    struct cache_item *prev;
    struct cache_item *next;
};


/**
 *  Definition for free block or inode
 */
struct free_list_item {
    int id;
    bool free;
    struct free_block *next;
};

/**
 *  Definitions for request types
 */
#define OPEN            0
#define CLOSE          1
#define CREATE        2
#define READ            3
#define WRITE          4
#define SEEK            5
#define LINK             6
#define UNLINK        7
#define SYMLINK      8
#define READLINK    9
#define MKDIR         10
#define RMDIR         11
#define CHDIR          12
#define STAT            13
#define SYNC            14
#define SHUTDOWN 15

/**
 * Definition for a 32-byte length message that should cover any valid request.
 * Each generic data member describes what var it can potentially hold.
 */
struct msg {
    int type; // the type of request
    int data1; // use this for storing inode_num or [fd] (for Seek) 
    int data2; // use this for [fd], [len] (for ReadLink), or [offset] (for Seek)
    int data3; // use this for [size] or [whence] (for Seek)
    void *ptr1; // use this for [pathname] or [oldname] 
    void *ptr2; // use this for [newname], [buf], or [statbuf]
};

/**
 * Definitions for helper functions 
 */
int InitYFS();
void HandleRequest(struct msg *);
char *GetMessagePath(int srcpid, void *src);


/*
 *  Function prototypes for YFS calls:
 */
extern int YFSOpen(struct msg *msg, int pid);
extern int YFSCreate(struct msg *msg, int pid);
extern int YFSRead(int, void *, int);
extern int YFSWrite(int, void *, int);
extern int YFSSeek(int, int, int);
extern int YFSLink(char *, char *);
extern int YFSUnlink(char *);
// extern int SymLink(char *, char *);
// extern int ReadLink(char *, char *, int);
extern int YFSMkDir(char *);
extern int YFSRmDir(char *);
extern int YFSChDir(char *);
extern int YFSStat(char *, struct Stat *);
extern int YFSSync(void);
extern int YFSShutdown(void);