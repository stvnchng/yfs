#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "yfs.h"
#include "iohelpers.h"

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
                free(msg);
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

	// Get the number of blocks and inodes
	struct fs_header *header_ptr = malloc(SECTORSIZE);
	int status = ReadSector(1, header_ptr);
	if (status == ERROR) { // Error handling
		TracePrintf(0, "Unable to read file system header.\n");
		return ERROR;
	}
	num_blocks = header_ptr->num_blocks;
	num_inodes = header_ptr->num_inodes;
	// Allocate space for free block/inode list (both arrays with entries of value 1 or 0)
	free_block_list = malloc(sizeof(short) * num_blocks);
	free_inode_list = malloc(sizeof(short) * num_inodes);
	// Initialize free_block_list and free_inode_list
	int i;
	for (i = 0; i < num_blocks; i++) {
		free_block_list[i] = 0;
	}
	for (i = 0; i < num_inodes; i++) {
		free_inode_list[i] = 0;
	}
	free(header_ptr);

	// Check how many blocks the inodes take
	int num_iblocks = num_inodes * INODESIZE / BLOCKSIZE + 1;
	struct inode **inode_ptr = malloc(SECTORSIZE);
	for (i = 1; i <= num_iblocks; i++) {
		int status = ReadSector(i, inode_ptr);
		if (status == ERROR) {
			TracePrintf(0, "Unable to read inode block.\n");
			return ERROR;
		}
		// skip file system header
		int blocks_count = 0;
		if (i == 1) {
			inode_ptr++;
			blocks_count++;	
		}
		// Loop through all the inode in a block (one sector)
		while (blocks_count < (SECTORSIZE / INODESIZE)) {
			// if inode is free, add it to the free inode list
			if (inode_ptr[blocks_count]->type == INODE_FREE) {
				free_inode_list[(i - 1) * (SECTORSIZE / INODESIZE) + blocks_count] = 1;
				// add all the blocks it uses to free block list
				int j;
				for (j = 0; j < NUM_DIRECT; j++) {
					free_block_list[inode_ptr[blocks_count]->direct[j]] = 1;
				}	
				free_block_list[inode_ptr[blocks_count]->indirect] = 1;
			}
			blocks_count++;
		}
	}
	free(inode_ptr);
	// Make sure that the blocks that contain the inodes are not free
	for (i = 0; i <= num_iblocks; i++) {
		free_block_list[i] = 0;
	}


    return 0;
}

