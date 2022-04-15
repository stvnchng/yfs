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
        printf("Registering as file server failed...\n");
        return ERROR;
    }

    if (argc > 1) {
        if (Fork() == 0) {
            Exec(argv[1], argv + 1);
        } 
        else {
            int status;
            while (1) {
                struct msg *msg = malloc(sizeof(struct msg));
                // TODO: Receive the message and process
                if ((status = Receive(msg)) < 0) {
                    printf("Receive failed for msg with type %d...\n", msg->type);
                    return ERROR;
                }
                
                // TODO: listen for messages here
                switch (msg->type)
                {
                case OPEN:
                    // Open((char *)(msg->addr1));
                    break;
                default:
                    // printf("Message with type %d was not recognized!\n", msg->type);
                    break;
                }
                
                // TODO: remember to free memory used by msg on each new request
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