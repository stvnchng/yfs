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
    if (InitYFS() == ERROR) {
        printf("An error occurred during initialization...\n");
        return ERROR;
    }

    if (Register(FILE_SERVER) == ERROR) {
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

                HandleRequest(msg);
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

