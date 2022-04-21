#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "yfs.h"
#include "iohelpers.h"

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
    inode_cache = malloc(sizeof(struct cache));
    block_cache = malloc(sizeof(struct cache));
    struct hash_table *inode_ht = hash_table_create(INODE_CACHESIZE);
    struct hash_table *block_ht = hash_table_create(BLOCK_CACHESIZE);
    inode_cache->ht = inode_ht;
    block_cache->ht = block_ht;
	inode_cache->size = 0;
	block_cache->size = 0;
	inode_cache->maxsize = INODE_CACHESIZE;
	block_cache->maxsize = BLOCK_CACHESIZE;
	inode_cache->head = NULL;
	block_cache->head = NULL;
	inode_cache->tail = NULL;
	block_cache->tail = NULL;

	// Get the number of blocks and inodes
	void *temp_ptr = malloc(SECTORSIZE);
	int status = ReadSector(1, temp_ptr);
	struct fs_header *header_ptr = (struct fs_header *)temp_ptr;
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
		free_block_list[i] = 1;
	}
	for (i = 0; i < num_inodes; i++) {
		free_inode_list[i] = 0;
	}

	// Check how many blocks the inodes take
	int num_iblocks = num_inodes * INODESIZE / BLOCKSIZE + 1;
	struct inode *inode_ptr;
	for (i = 1; i <= num_iblocks; i++) {
		int status = ReadSector(i, temp_ptr);
		inode_ptr = (struct inode *)temp_ptr;
		if (status == ERROR) {
			TracePrintf(0, "Unable to read inode block.\n");
			return ERROR;
		}
		// skip file system header
		int blocks_count = 0;
		if (i == 1) {
			blocks_count++;	
		}
		// Loop through all the inode in a block (one sector)
		while (blocks_count < (SECTORSIZE / INODESIZE)) {
			// check root node
			if (i == 1 && blocks_count == 1) {
				TracePrintf(6, "The root inode is of type %d\n", inode_ptr[blocks_count].type);
				TracePrintf(6, "The root inode is of size %d\n", inode_ptr[blocks_count].size);
			}
			// if inode is free, add it to the free inode list
			if (inode_ptr[blocks_count].type == INODE_FREE) {
				free_inode_list[(i - 1) * (SECTORSIZE / INODESIZE) + blocks_count] = 1;
				// add all the blocks it uses to free block list
				// int j;
				// for (j = 0; j < NUM_DIRECT; j++) {
				// 	free_block_list[inode_ptr[blocks_count].direct[j]] = 1;
				// }	
				// free_block_list[inode_ptr[blocks_count].indirect] = 1;
			} else {
				int j;
				for (j = 0; j < NUM_DIRECT; j++) {
					free_block_list[inode_ptr[blocks_count].direct[j]] = 0;
				}
			}	
			blocks_count++;
		}
	}
	free(temp_ptr);
	TracePrintf(2, "There are %d blocks allocated for inodes.\n", num_iblocks);
	// Make sure that the blocks that contain the inodes are not free
	for (i = 0; i <= num_iblocks; i++) {
		free_block_list[i] = 0;
	}


    return 0;
}

/**
 * 	Helpers for caching inodes and blocks
 */
void *GetFromCache(struct cache *cache, int num)
{
	struct cache_item *res = hash_table_lookup(cache->ht, num);
	if (res == NULL) return NULL; //means inode/block not in cache
	// if head is not res, move it to head (LRU)
	if (cache->head != res) {
		// RemoveFromCache(cache, res);
		AssignHead(cache, res);
	}

	return res->value; //value holds the inode/block
}

struct cache_item *GetItemFromCache(struct cache *cache, int num) 
{
	struct cache_item *res = hash_table_lookup(cache->ht, num);
	if (res == NULL) return NULL; //means inode/block not in cache
	// if head is not res, move it to head (LRU)
	if (cache->head != res) {
		// RemoveFromCache(cache, res);
		AssignHead(cache, res);
	}

	return res; //value holds the inode/block
}

void PutIntoCache(struct cache *cache, int num, void *value)
{
	PrintCache(cache);
	// First check if key already exists
	struct cache_item *item = GetItemFromCache(cache, num);
	if (item != NULL) {
		TracePrintf(2, "There is already a key %d in the cache\n", num);
		// If so, we reassign its value and move it to the head
	//	item->value = value;
		RemoveFromCache(cache, item);
		// AssignHead(cache, item);
		// return;
	}
	// Otherwise, initialize an item and insert it at the head
	item = malloc(sizeof(struct cache_item));
	item->num = num;
	item->prev = NULL;
	item->next = NULL;
	void *new_val;
	if (cache == block_cache) {
		TracePrintf(2, "Putting block %d into cache\n", num);
		new_val = malloc(BLOCKSIZE);
		memcpy(new_val, value, BLOCKSIZE);
	}
	else { 
		TracePrintf(2, "Putting inode %d into cache\n", num);
		new_val = malloc(INODESIZE);
		memcpy(new_val, value, INODESIZE);
	}
	item->value = new_val;
	hash_table_insert(cache->ht, num, item);

	if (cache->size >= cache->maxsize) {
		// LRU: If the cache is maxcap, start removing from the tail
		hash_table_remove(cache->ht, cache->tail->num); //remove from ht and cache
		RemoveFromCache(cache, cache->tail);
	} else {
		cache->size = cache->size + 1; // increment size
	}

	AssignHead(cache, item);
	TracePrintf(2, "Finished putting the new item into cache. \n");
	PrintCache(cache);
}

void RemoveFromCache(struct cache *cache, struct cache_item *item)
{
	if (cache == block_cache) {
		TracePrintf(2, "Starts removing block %d from block cache\n", item->num);
	} else {
		TracePrintf(2, "Starts removing inode %d from inode cache\n", item->num);
	}

	if (item->prev == NULL) { // we are at the head
		cache->head = item->next; // reassign head
	} else item->prev->next = item->next; // else link to original succ
	if (item->next == NULL) { // we are at the tail
		cache->tail = item->prev; // reassign tail
	} else item->next->prev = item->prev; // else link to original pred
	TracePrintf(3, "Starts freeing the old item from memory\n");
	free(item->value);
	free(item);
}

void AssignHead(struct cache *cache, struct cache_item *item)
{
	TracePrintf(2, "Assigning item %d to head of cache\n", item->num);
	if (cache->head == item) return;
	if (item->prev != NULL) {
		item->prev->next = item->next;
		if (item->next == NULL) {
			cache->tail = item->prev;
		} else {
			item->next->prev = item->prev;
		}
	} 
	item->prev = NULL;
	item->next = cache->head;
	// Check for existing head item
	if (cache->head != NULL) cache->head->prev = item;
	// Assign tail if nonexistent
	if (cache->tail == NULL) cache->tail = item;
	cache->head = item;
}

/**
 * Check the cache before actually read from disk
 */
int ReadSectorWrapper(int sector_num, void *buf) 
{
	// Check the cache first
	void *cache_sector = GetFromCache(block_cache, sector_num);
	if (cache_sector != NULL) {
		TracePrintf(3, "Found block %d in cache\n", sector_num);
		memcpy(buf, cache_sector, SECTORSIZE);
		return 0;
	}

	// if we can't find anything in the cache, do the ReadSector
	if (ReadSector(sector_num, buf) == ERROR) {
		TracePrintf(0, "Failed to read from block %d\n", sector_num);
		return ERROR;
	}
	PutIntoCache(block_cache, sector_num, buf);

	return 0;
}

/**
 * Write to disk and also update the cache
 */
int WriteSectorWrapper(int sector_num, void *buf)
{
	if (WriteSector(sector_num, buf) == ERROR) {
		TracePrintf(0, "Failed to write to block %d\n", sector_num);
		return ERROR;
	}
	PutIntoCache(block_cache, sector_num, buf);

	return 0;
}


void PrintCache(struct cache* cache) 
{
	struct cache_item *head = cache->head;
	while (head != NULL) {
		if (cache == block_cache) {
			TracePrintf(4, "Block Cache next item num %d\n", head->num);
		} else {
			TracePrintf(4, "Inode Cache next item num %d with size %d of type %d\n", head->num, ((struct inode *)head->value)->size, ((struct inode *)head->value)->type);
		}
		head = head->next;
	}
}
