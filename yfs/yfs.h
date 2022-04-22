#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <stdbool.h>
#include "hash_table.h"


/**
 *  Definition for LRU cache structure for inodes and blocks 
 */
struct cache {
    int size;
    int maxsize;
    struct hash_table *ht;
    struct cache_item *head;
    struct cache_item *tail;
};

struct cache_item {
    bool dirty;
    int num; // hash table key (aka inum or block num)
    void *value; // in this case, the address of the inum or block
    struct cache_item *prev;
    struct cache_item *next;
};

/**
 *  Helper definitions for caching
 */
void *GetFromCache(struct cache *cache, int num);
void PutIntoCache(struct cache *cache, int num, void *value);
void RemoveFromCache(struct cache *cache, struct cache_item *item);
void AssignHead(struct cache *cache, struct cache_item *item);
int ReadSectorWrapper(int sector_num, void *buf);
int WriteSectorWrapper(int sector_num, void *buf);
void PrintCache(struct cache* cache); 

/**
 *  Global Variables
 */
struct cache *inode_cache;
struct cache *block_cache;

int num_blocks, num_inodes; // count of free inodes and blocks
short *free_block_list;
short *free_inode_list;

/**
 *  Definitions for messaging and request types
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
    int type;
    int inum;
    int data1; // [fd] or to hold len of [pathname] or [oldname]
    int data2; // [size] in Read/Write or to hold len of [newname]
    void *ptr1; // [oldname] or [pathname]
    void *ptr2; // [newname], [buf], or [statbuf]
};

// A message struct for seek since it requires 5 int members
struct msg_seek {
    int type; // should always be SEEK
    int inum;
    int position; // position in file
    int offset;
    int whence;
    char padding[12]; // makes this struct 32 bytes
};

/**
 * Definitions for helper functions 
 */
int InitYFS();
int HandleRequest(struct msg *);
char *GetMessagePath(int, void *, int);
void *GetBuf(int srcpid, void *src, int buflen);
int SendBuf(int destpid, void *dest, void *src, int buflen);

