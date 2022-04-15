#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include <stdbool.h>
#include "hash_table.h"

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
 * Definitions for types of messages. To best use memory, different structs
 * should be defined for different types of requests.
 */
struct msg_send {
    int type;
    int data1;
    int data2;
    void *addr1;
    void *addr2;
};

/**
 * Definitions for helper functions 
 */
extern int InitYFS();
