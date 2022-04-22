#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include "iohelpers.h"
#include "yfs.h"

int num_dir_in_block = BLOCKSIZE / sizeof(struct dir_entry);
int inode_per_block = BLOCKSIZE / INODESIZE;

int compare_filenames(char *f1, char *f2) 
{
	int i;
	for (i = 0; i < DIRNAMELEN; i++) {
		if (f1[i] == f2[i] && f1[i] == '\0') {
			return 1;
		} 
		if (f1[i] != f2[i]) {
			return 0;
		}
	}	
	return 1;
}

int get_num_blocks(struct inode *inode) 
{
	TracePrintf(1, "The inode has a size of %d\n", inode->size);
	if (inode->size % BLOCKSIZE == 0) {
		return inode->size / BLOCKSIZE;
	} else {
		return inode->size / BLOCKSIZE + 1;
	}
}

int get_num_dir(struct inode *inode) 
{
    return inode->size / sizeof(struct dir_entry);
}

struct inode *get_inode(short inum) 
{
	// Check the inode_cache first
	struct inode *inode = (struct inode *) GetFromCache(inode_cache, inum);
	if (inode != NULL) {	
		TracePrintf(1, "The inode %d is in the cache\n", inum);
	 	return inode;
	}
	free(inode);
	int block_num;
	struct inode *inode_block = malloc(SECTORSIZE);
	struct inode *res;
	if ((inum + 1) % inode_per_block == 0) {
		block_num = (inum + 1) / inode_per_block;
		if (ReadSectorWrapper(block_num, inode_block) == ERROR) { 
			free(inode_block);
			TracePrintf(0, "Cannot read the block to get inode.\n");
			return NULL;
		}
		res = &inode_block[inode_per_block - 1];
	} else {
		block_num = (inum + 1) / inode_per_block + 1;
		if (ReadSectorWrapper(block_num, inode_block) == ERROR) {
		   	free(inode_block);	
			TracePrintf(0, "Cannot read the block to get inode.\n");
			return NULL;
		}
		res = &inode_block[(inum + 1) % inode_per_block - 1];
	}
	PutIntoCache(inode_cache, inum, res);
	return res;
}

/* return address of block given its blocknum */
void *get_block(int blocknum)
{
	// check if block is in cache first
	void *addr = GetFromCache(block_cache, blocknum);
	if (addr != NULL) return addr;
	// block not in cache, so cache it
	addr = malloc(BLOCKSIZE);
	if (ReadSectorWrapper(blocknum, addr) == ERROR) {
		TracePrintf(0, "Cannot read to block %d\n", blocknum);
		free(addr);
		return NULL;
	}
	return addr;
}

int write_inode(short inum, struct inode *inode) 
{
	TracePrintf(0, "Starts writing to inode %d\n", inum);
	int status;
	int block_num;
	struct inode *inode_block = malloc(SECTORSIZE);
	// Change the block's data to what we want
	if ((inum + 1) % inode_per_block == 0) {
		block_num = (inum + 1) / inode_per_block;
		status = ReadSectorWrapper(block_num, inode_block);
		if (status == ERROR) { 
			return ERROR;
		}
		inode_block[inode_per_block - 1] = *inode;
	} else {
		block_num = (inum + 1) / inode_per_block + 1;
		status = ReadSectorWrapper(block_num, inode_block);
		if (status == ERROR) { 
			return ERROR;
		}
		inode_block[(inum + 1) % inode_per_block - 1] = *inode;
	}

	// Write to the block with our new inode in it. 
	TracePrintf(1, "Write to block %d\n", block_num);
	status = WriteSectorWrapper(block_num, inode_block);
	if (status == ERROR) {
		return ERROR;
	}
	PutIntoCache(inode_cache, inum, inode);
	
	free(inode_block);

	return 0;
}	

int process_path(char *path, int curr_inum, int call_type) 
{
	char component[DIRNAMELEN + 1];
	int comp_ptr = 0;
	int path_ptr = 0;
	int num_blocks;
	int return_inum, parent_inum;

	struct inode *start_inode, *parent_inode;
	if (path[0] == '/') {
		start_inode = get_inode(1);
		return_inum = 1;
		path_ptr = 1;
	} else {
		start_inode = get_inode(curr_inum);
		return_inum = curr_inum;
	}
	if (start_inode == NULL) {
		return ERROR;
	}
	TracePrintf(3, "start_inode's addr is: %p\n", start_inode);
	num_blocks = get_num_blocks(start_inode);
	if (num_blocks == 0) {
		TracePrintf(0, "the inode has 0 blocks.\n");	
		return ERROR;
	}
	int num_dir = get_num_dir(start_inode);
	TracePrintf(1, "the inode has %d directories\n", num_dir);

	while (path_ptr < MAXPATHNAMELEN && path[path_ptr] != '\0') {
		if (start_inode->type != INODE_DIRECTORY) {
			TracePrintf(0, "The current inode is not a directory.\n");
			TracePrintf(1, "The current inode is a %d\n", start_inode->type);
			return ERROR;
		}
		if (path[path_ptr] != '/') {
			component[comp_ptr++] = path[path_ptr++];	
		} 

		if (path[path_ptr] == '/' || path[path_ptr] == '\0' || path_ptr == MAXPATHNAMELEN) {
			component[comp_ptr] = '\0';
			TracePrintf(1, "The new component is %s\n", component);
			// if there are nothing between two slashes, continue
			if (comp_ptr == 0) 
				continue;
			// if there are more than DIRNAMELEN characters in the component name, return error
			if (comp_ptr > DIRNAMELEN)
				return ERROR;
			// otherwise, go down the directory tree
			int i;
			int found_next_inode = 0;
			struct dir_entry *dir_ptr = malloc(SECTORSIZE);
			TracePrintf(2, "there are %d blocks in the inode.\n", num_blocks);
			// if there are NUM_DIRECT blocks or less. We don't need to look into the indirect blocks. 
			if (num_blocks <= NUM_DIRECT) {
				for (i = 0; i < num_blocks; i++) {
					int status = ReadSectorWrapper(start_inode->direct[i], dir_ptr);
					if (status == ERROR)
						return ERROR;
					int j;
					for (j = 0; j < num_dir_in_block; j++) {
						TracePrintf(1, "the compared component is %s\n", dir_ptr[j].name); // remove this later
						if (compare_filenames(dir_ptr[j].name, component) == 1) {
							// if we found the right file/dir, move on/open file
							parent_inum = return_inum;
							return_inum = dir_ptr[j].inum;
							parent_inode = start_inode;
							start_inode = get_inode(return_inum);
							num_blocks = get_num_blocks(start_inode);
							num_dir = start_inode->size / sizeof(struct dir_entry);
							TracePrintf(1, "we have found a matching dir_entry with inum %d. %d blocks, %d dir entries\n", return_inum, num_blocks, num_dir);
						   	found_next_inode = 1;
							break;	
						}
						// if we already went through every file/directory, return error
						if (--num_dir <= 0) {
							break;
						}
					}
					// if found the next node in hierarchy, go to next while loop
					if (found_next_inode) 
						break;
				}	
			} 
			// We need to look into the direct blocks and the indirect blocks. 
			else {
				for (i = 0; i < NUM_DIRECT; i++) {
					int status = ReadSectorWrapper(start_inode->direct[i], dir_ptr);
					if (status == ERROR)
						return ERROR;
					int j;
					for (j = 0; j < num_dir_in_block; j++) {
						if (compare_filenames(dir_ptr[j].name, component) == 1) {
							// if we found the right file/dir, move on/open file
							parent_inum = return_inum;
							return_inum = dir_ptr[j].inum;
							TracePrintf(1, "we have found a matching dir_entry with inum %d. %d blocks, %d dir entries\n", return_inum, num_blocks, num_dir);
						   	found_next_inode = 1;
							parent_inode = start_inode;
							start_inode = get_inode(return_inum);
							num_blocks = get_num_blocks(start_inode);
							num_dir = start_inode->size / sizeof(struct dir_entry);
						   	found_next_inode = 1;
							break;	
						}
						dir_ptr++;
						--num_dir;	
					}
					// if found the next node in hierarchy, go to next while loop
					if (found_next_inode) 
						break;
				}
				// if we didn't find the next inode in the direct blocks, go to the indirect blocks. 	
				if (!found_next_inode) {
					int *blocks = malloc(SECTORSIZE);
					int status = ReadSectorWrapper(start_inode->indirect, blocks);
					if (status == ERROR)
						return ERROR;
					for (i = 0; i < num_blocks - NUM_DIRECT; i++) {
						status = ReadSectorWrapper(blocks[i], dir_ptr);
						if (status == ERROR)
							return ERROR;
						int j;
						for (j = 0; j < num_dir_in_block; j++) {
							if (compare_filenames(dir_ptr[j].name, component) == 1) {
								// if we found the right file/dir, move on/open file
								parent_inum = return_inum;
								return_inum = dir_ptr[j].inum;
								TracePrintf(1, "we have found a matching dir_entry with inum %d. %d blocks, %d dir entries\n", return_inum, num_blocks, num_dir);
								parent_inode  = start_inode;
								start_inode = get_inode(return_inum);
								num_blocks = get_num_blocks(start_inode);
								num_dir = start_inode->size / sizeof(struct dir_entry);
								found_next_inode = 1;
								break;	
							}
							// if we already went through every file/directory, return error
							if (--num_dir <= 0) {
								break;
							}
						}
						// if found the next node in hierarchy, go to next while loop
						if (found_next_inode) 
							break;
					}
					free(blocks);
				}
			}
			if (!found_next_inode) {
				switch (call_type) {
					case OPEN:
					case CHDIR:
					case RMDIR:
						TracePrintf(0, "The component %s does not exist\n", component);
						return ERROR;
					case CREATE:
						if (path[path_ptr] == '\0' || path_ptr == MAXPATHNAMELEN) {
							return create_stuff(component, return_inum, INODE_REGULAR);
						} else {
							TracePrintf(0, "Cannot find the right file to open\n");
							return ERROR;
						}
					case MKDIR:
						if (path[path_ptr] == '\0' || path_ptr == MAXPATHNAMELEN) {
							return create_stuff(component, return_inum, INODE_DIRECTORY);
						} else {
							TracePrintf(0, "Cannot find the right file to open\n");
							return ERROR;
						}
				}
			} 

			free(dir_ptr);
			// Reset component to be ready for the next component in path
			comp_ptr = 0;
			if (path[path_ptr] == '/') {
				path_ptr++;
			}
		}
	}

	// Exiting the while loop means that all of the component already exists
	switch (call_type) {
		case MKDIR:
			TracePrintf(0, "MKDIR fail. The directory already exists.\n");
			return ERROR;
		case RMDIR:
			return remove_dir_entry(start_inode, parent_inode, return_inum, parent_inum);		
		default:
			break;
	}
	
	return return_inum;
}

void add_free_inode(int inum)
{
	free_inode_list[inum - 1] = 1;	
	struct inode free_inode;
	free_inode.type = INODE_FREE;
	free_inode.size = 0;
	write_inode(inum, &free_inode);
}

int remove_free_inode(short type) 
{
	int i;
	for (i = 0; i < num_inodes; i++) {
		// if we found a free inode in our list, remove it
		if (free_inode_list[i] == 1) {
			struct inode *free_inode = get_inode(i + 1);
			// verify the type first
			if (free_inode->type != INODE_FREE) {
				TracePrintf(0, "ERROR: the inode is suppose to be a free inode. Skip.\n");
				continue;
			}	
			free_inode_list[i] = 0;
			free_inode->type = type;
			TracePrintf(2, "The free inode removed is %d\n", i+1);
			return i + 1;
		}
	}
	return ERROR;
}

void add_free_block(int blocknum)
{
	free_block_list[blocknum - 1] = 1;
}

int remove_free_block() 
{
	int i;
	for (i = 0; i < num_blocks; i++) {
		// if we found a free block, remove it
		if (free_block_list[i] == 1) {
			free_block_list[i] = 0;
			TracePrintf(2, "The free block removed is %d\n", i+1);
			return i + 1;
		}
	}
	TracePrintf(0, "Cannot find a free block.\n");
	return ERROR;
}

int create_stuff(char *name, int parent_inum, short type) 
{
	if (type == INODE_REGULAR) {
		TracePrintf(0, "Creating a file named %s\n", name);
	} else if (type == INODE_DIRECTORY) {
		TracePrintf(0, "Creating a directory named %s\n", name);
	}

	// Get the parent inode
	struct inode *parent_inode = get_inode(parent_inum);
	if (parent_inode->type != INODE_DIRECTORY) {
		TracePrintf(0, "The parent inode should be a directory.\n");
		return ERROR;
	}

	// if we exceeds the size limit, then return ERROR
	int parent_size = parent_inode->size;
	if (parent_size + sizeof(struct dir_entry) > (NUM_DIRECT + (BLOCKSIZE/sizeof(int))) * BLOCKSIZE) {
		TracePrintf(0, "There are not enough free space left in the parent inode.\n");
		return ERROR;
	}

	// Create a new dir_entry
	struct dir_entry *new_entry = malloc(SECTORSIZE);
	new_entry->inum = remove_free_inode(type);
	memcpy(new_entry->name, name, DIRNAMELEN);
	
	// Get how many blocks and directories the parent inode already have. 
	int num_blocks = get_num_blocks(parent_inode);
	int num_dir = get_num_dir(parent_inode);
	TracePrintf(1, "The parent inode has %d blocks, %d dir entries.\n", num_blocks, num_dir);

	if (num_blocks == NUM_DIRECT && num_dir % num_dir_in_block == 0) {
		TracePrintf(1, "Needs to create an indirect block for parent inode\n");
		parent_inode->indirect = remove_free_block();
		num_blocks++;
	}

	int status;
	if (num_blocks <= NUM_DIRECT) {
		// allocate the new inode in a direct block
		if (num_dir % num_dir_in_block == 0) {
			// all the old blocks are filled up. need a new block
			int free_block = remove_free_block();
			if (free_block == ERROR) {
				free(new_entry);
				return ERROR;
			}
			status = WriteSectorWrapper(free_block, new_entry);
			if (status == ERROR) {
				add_free_block(free_block);
				free(new_entry);
				TracePrintf(0, "Failed to write to sector %d\n", free_block);
				return ERROR;
			}
			parent_inode->direct[num_blocks] = free_block;
		} else {
			struct dir_entry last_block[SECTORSIZE/sizeof(struct dir_entry)];
			// read the last block
			status = ReadSectorWrapper(parent_inode->direct[num_blocks - 1], last_block);
			if (status == ERROR) { 
				free(new_entry);
				TracePrintf(0, "Failed to read from sector %d\n", num_blocks - 1);
				return ERROR;
			}
			// put the new dir_entry at the end of the last block
			int new_dir_index = num_dir % num_dir_in_block;
			TracePrintf(2, "the new entry will be put at index %d\n", new_dir_index);
			last_block[new_dir_index] = *new_entry;
			// Write back the block
			status = WriteSectorWrapper(parent_inode->direct[num_blocks - 1], last_block);
			if (status == ERROR) {
				free(new_entry);
				return ERROR;
			}
		}
	} else {
		// Allocate a block in the indirect block
		int *indirect_blocks = malloc(SECTORSIZE);
		status = ReadSectorWrapper(parent_inode->indirect, indirect_blocks);
		if (status == ERROR) {
			free(new_entry);
			free(indirect_blocks);
			TracePrintf(0, "Failed to read from sector %d\n", parent_inode->indirect);
			return ERROR;
		}
		// allocate the new inode in a direct block
		if (num_dir % num_dir_in_block == 0) {
			// all the old blocks are filled up. need a new block
			int free_block = remove_free_block();
			if (free_block == ERROR) {
				free(new_entry);
				free(indirect_blocks);
				return ERROR;
			}
			// Write the new dir_entry to the newly allocated free block
			status = WriteSectorWrapper(free_block, new_entry);
			if (status == ERROR) {
				add_free_block(free_block);
				free(new_entry);
				free(indirect_blocks);
				TracePrintf(0, "Failed to write to sector %d\n", free_block);
				return ERROR;
			}
			// Add a new block to the end of the indirect block. 
			indirect_blocks[num_blocks - NUM_DIRECT] = free_block;
			status = WriteSectorWrapper(parent_inode->indirect, indirect_blocks);
			if (status == ERROR) {
				add_free_block(free_block);
				free(new_entry);
				free(indirect_blocks);
				TracePrintf(0, "Failed to write to sector %d\n", parent_inode->indirect);
				return ERROR;
			}
		} else {
			struct dir_entry last_block[SECTORSIZE/sizeof(struct dir_entry)];
			// Read the last block
			status = ReadSectorWrapper(indirect_blocks[num_blocks - 1 - NUM_DIRECT], last_block);
			if (status == ERROR) {
				free(new_entry);
				free(indirect_blocks);
				TracePrintf(0, "Failed to read from sector %d\n", indirect_blocks[num_blocks - 1 - NUM_DIRECT]);
				return ERROR;
			}
			// put the new dir_entry at the end of the last block
			last_block[num_dir % num_dir_in_block] = *new_entry;
			// Write back the block
			status = WriteSectorWrapper(indirect_blocks[num_blocks - 1 - NUM_DIRECT], last_block);
			if (status == ERROR) {
				free(new_entry);
				free(indirect_blocks);
				TracePrintf(0, "Failed to write to sector %d\n", indirect_blocks[num_blocks - 1 - NUM_DIRECT]);
				return ERROR;
			}
		}	
		free(indirect_blocks);
	}


	// Increase the size of the parent inode
	parent_inode->size += sizeof(struct dir_entry);
	if (write_inode(parent_inum, parent_inode) == ERROR) {
		free(new_entry);
		return ERROR;
	}
	TracePrintf(1, "Finished writing the updated parent inode to disk.\n");

	int return_inum = new_entry->inum;
	// Set up the new inode
	struct inode *new_inode = malloc(sizeof(struct inode));
	new_inode->type = type;
	// if the newly created inode is a directory, we need to put special dir_entry . and ..
	if (type == INODE_DIRECTORY) {
		new_inode->size = 2 * sizeof(struct dir_entry);
		int free_block = remove_free_block();
		new_inode->direct[0] = free_block;
		struct dir_entry *entries = malloc(SECTORSIZE);
		entries[0].inum = return_inum;
		entries[0].name[0] = '.';
		entries[0].name[1] = '\0';
		entries[1].inum = parent_inum;
		entries[1].name[0] = '.';
		entries[1].name[1] = '.';
		entries[1].name[2] = '\0';
		status = WriteSectorWrapper(free_block, entries);
		free(entries);
		if (status == ERROR) {
			free(new_entry);
			TracePrintf(0, "Failed to write to block %d\n", free_block);
			return ERROR;
		}
	} else {
		TracePrintf(2, "type: %d\n", type);
		new_inode->size = 0;
	}
	write_inode(return_inum, new_inode);
	free(new_entry);

	TracePrintf(0, "the returning inode from create is %d\n", return_inum);
	return return_inum;
}

int remove_dir_entry(struct inode *child_inode, struct inode *parent_inode, int child_inum,  int parent_inum) 
{
	TracePrintf(1, "the parent's inum is %d\n", parent_inum);
	// both inodes needs to be directories
	if (parent_inode->type != INODE_DIRECTORY) {
		TracePrintf(0, "The parent inode needs to be a directory\n");
		return ERROR;
	}

	if (child_inode->type == INODE_DIRECTORY) {
		// if there are more than . and .. in the inode to be removed, then return an ERROR
		int num_dir = get_num_dir(child_inode);
		if (num_dir > 2) {
			TracePrintf(0, "RMDIR. There are more than . and .. in the directory.\n");
			return ERROR;
		}
	}

	// remove the child from its parent.
	int parent_num_blocks = get_num_blocks(parent_inode);
   	int parent_num_dir = get_num_dir(parent_inode);
	
	unsigned int i;
	int status;
	struct dir_entry entries[BLOCKSIZE/sizeof(struct dir_entry)];
	if (parent_num_dir == 3) {
		TracePrintf(1, "The parent inode only has one removable dir_entry\n");
	} else {
		// if the parent inode have more than one removable inode. Remove the child inode, and shift
		// the last possible inode to there. 
		TracePrintf(1, "The parent inode has %d removable dir_entry\n", parent_num_dir - 2);
		// Get the last dir_entry
		if (parent_num_blocks <= NUM_DIRECT) {
			status = ReadSectorWrapper(parent_inode->direct[parent_num_blocks-1], entries);
		} else {
			int *indirect_blocks = malloc(SECTORSIZE);
			status = ReadSectorWrapper(parent_inode->indirect, indirect_blocks);
			if (status == ERROR) {
				TracePrintf(0, "Failed to read from block %d\n", parent_inode->indirect);
				return ERROR;
			}
			int indirect_index = parent_num_blocks - NUM_DIRECT - 1;
			status = ReadSectorWrapper(indirect_blocks[indirect_index], entries);
			free(indirect_blocks);
		}
		if (status == ERROR) {
			TracePrintf(0, "Failed to read from block something in remove_dir_entry\n");
			return ERROR;
		}
		int last_dir_index;
		if (parent_num_dir % num_dir_in_block == 0) {
			last_dir_index = num_dir_in_block - 1;
		} else {
			last_dir_index = parent_num_dir % num_dir_in_block - 1;	
		}
		// Get teh last dir_entry's inum and name
		int last_dir_inum = entries[last_dir_index].inum;
		char *last_dir_name = malloc(DIRNAMELEN);
		memcpy(last_dir_name, entries[last_dir_index].name, DIRNAMELEN);
		TracePrintf(1, "The last dir_entry is inode %d, with name %s\n", last_dir_inum, last_dir_name);
		
		// Now find the child inode
		int child_block = find_dir_entry_block(child_inum, parent_inode, parent_num_blocks, parent_num_dir);
		if (child_block == ERROR) {
			free(last_dir_name);
			return ERROR;
		}		
		status = ReadSectorWrapper(child_block, entries);
		if (status == ERROR) {
			free(last_dir_name);
			TracePrintf(0, "Cannot read from block %d\n", child_block);
		 	return ERROR;
		}
		for (i = 0; i < BLOCKSIZE/sizeof(struct dir_entry); i++) {
			if (entries[i].inum == child_inum) {
				TracePrintf(1, "Found the to-be-removed inode\n");
				entries[i].inum = last_dir_inum;
				memcpy(entries[i].name, last_dir_name, DIRNAMELEN);
				free(last_dir_name);
			}
		}
		status = WriteSectorWrapper(child_block, entries);
	}	
	// decrease the parent inode's size
	parent_inode->size -= sizeof(struct dir_entry);
	write_inode(parent_inum, parent_inode);

	// free the inode and the block it is allocated
	int child_num_blocks = get_num_blocks(child_inode);
	if (child_num_blocks <= NUM_DIRECT) {
		for (i = 0; i < (unsigned int) child_num_blocks; i++) {
			add_free_block(child_inode->direct[i]);
		}
	} else {
		for (i = 0; i < NUM_DIRECT; i++) {
			add_free_block(child_inode->direct[i]);
		}
		int num_indirect = child_num_blocks - NUM_DIRECT;
		int *indirect_blocks = malloc(SECTORSIZE);
		status = ReadSectorWrapper(child_inode->indirect, indirect_blocks);
		if (status == ERROR) {
			free(indirect_blocks);
			return ERROR;		
		}
		for (i = 0; i < (unsigned int) num_indirect; i++) {
			add_free_block(indirect_blocks[i]);
		}
		add_free_block(child_inode->indirect);
	}
	add_free_inode(child_inum);
	// add_free_block(free_block);

	return ERROR;
}

/**
 * Find a block that contains the dir_entry with inum
 */
int find_dir_entry_block(int inum, struct inode *parent_inode, int parent_num_blocks, int parent_num_dir)
{
	unsigned int i, j;
	int status;
	struct dir_entry return_block[SECTORSIZE/sizeof(struct dir_entry)];
	if (parent_num_blocks <= NUM_DIRECT) { // if the parent inode only needs direct blocks
		for (i = 0; i < (unsigned int) parent_num_blocks; i++) {
			status = ReadSectorWrapper(parent_inode->direct[i], return_block);
			if (status == ERROR) {
				TracePrintf(0, "Failed to read from block %d\n", parent_inode->direct[i]);
				return ERROR;
			}
			for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
				if (return_block[j].inum == inum) {
					TracePrintf(0, "Block %d contains inode %d\n", parent_inode->direct[i], inum);
					return parent_inode->direct[i];
				}
				if (--parent_num_dir <= 0) {
					TracePrintf(0, "Unable to find a block that contains inode %d\n", inum);
					return ERROR;
				}
			}
		}
	} else { // if searching for indirect locks might also be needed
		for (i = 0; i < NUM_DIRECT; i++) {
			status = ReadSectorWrapper(parent_inode->direct[i], return_block);
			if (status == ERROR) {
				TracePrintf(0, "Failed to read from block %d\n", parent_inode->direct[i]);
				return ERROR;
			}
			for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
				if (return_block[j].inum == inum) {
					TracePrintf(0, "Block %d contains inode %d", parent_inode->direct[i], inum);
					return parent_inode->direct[i];
				}
				--parent_num_dir;
			}
		}
		// Read from the indirect block
		int *indirect_block = malloc(SECTORSIZE);
		status = ReadSectorWrapper(parent_inode->indirect, indirect_block);
		if (status == ERROR) {
			TracePrintf(0, "Failed to read from block %d\n", parent_inode->indirect);
			return ERROR;
		}
		for (i = 0; i < SECTORSIZE/sizeof(int); i++) {
			int block_num = indirect_block[i];
			status = ReadSectorWrapper(block_num, return_block);
			if (status == ERROR) {
				TracePrintf(0, "Failed to read from block %d\n", block_num);
				return ERROR;
			}
			for (j = 0; j < SECTORSIZE/sizeof(struct dir_entry); j++) {
				if (return_block[j].inum == inum) {
					TracePrintf(0, "Block %d contains inode %d", block_num, inum);
					return block_num;
				}
				if (--parent_num_dir <= 0) {
					TracePrintf(0, "Unable to find a block that contains inode %d\n", inum);
					return ERROR;
				}
			}
		}
	}
	
	TracePrintf(0, "Unable to find a block that contains inode %d\n", inum);
	return ERROR;
}


int read_helper(int inum, int pos, int size, void *buf)
{
	struct inode *inode = get_inode(inum);
	
	// Check if the remaining content of the file is bigger than size
	int res, remain_size;
	if (inode->size - pos > size) {
		res = size;
	} else {
		res = inode->size - pos;
	}
	remain_size = res;

	// Get the block where the position is at
	int block_num = pos / BLOCKSIZE;	
	if (block_num < NUM_DIRECT) {
		block_num = inode->direct[block_num];
	} else {
		int *indirect = (int *)get_block(inode->indirect);
		block_num = indirect[block_num - NUM_DIRECT];
		free(indirect);
	}

	void *block_ptr;
	if ((block_ptr = get_block(block_num)) == NULL) {
		return ERROR;
	}
	TracePrintf(1, "The first block is at %d with %d space left.\n", block_num, BLOCKSIZE - (pos % BLOCKSIZE));

	memcpy(buf, block_ptr + (pos % BLOCKSIZE), BLOCKSIZE - (pos % BLOCKSIZE));
	buf += BLOCKSIZE - (pos % BLOCKSIZE);
	remain_size -= BLOCKSIZE - (pos % BLOCKSIZE);
	free(block_ptr);

	// Deal with the fully loaded blocks in the middle
	while (remain_size >= BLOCKSIZE) {
		++block_num;
		if (block_num < NUM_DIRECT) {
			block_num = inode->direct[block_num];
		} else {
			int *indirect = (int *)get_block(inode->indirect);
			block_num = indirect[block_num - NUM_DIRECT];
			free(indirect);
		}

		if ((block_ptr = get_block(block_num)) == NULL) {
			return ERROR;
		}
		memcpy(buf, block_ptr, BLOCKSIZE);
		buf += BLOCKSIZE;
		remain_size -= BLOCKSIZE;
		free(block_ptr);
	}	

	// copy over the last block
	if ((block_ptr = get_block(++block_num)) == NULL) {
		return ERROR;
	}
	memcpy(buf, block_ptr, remain_size);	
	free(block_ptr);

	return res;
}


int write_helper(int inum, int pos, int size, void *buf)
{
	struct inode *inode = get_inode(inum);
	int size_left = size;

	// Get the block where the position is at
	int block_num = pos / BLOCKSIZE;	
	if (block_num < NUM_DIRECT) {
		block_num = inode->direct[block_num];
	} else {
		int *indirect = (int *)get_block(inode->indirect);
		block_num = indirect[block_num - NUM_DIRECT];
		free(indirect);
	}

	void *block_ptr;
	if ((block_ptr = get_block(block_num)) == NULL) {
		return ERROR;
	}
	TracePrintf(1, "The first block is at %d with %d space left.\n", block_num, BLOCKSIZE - (pos % BLOCKSIZE));

	// malloc a buffer that is of size SECTORSIZE
 	void *temp_buf = malloc(SECTORSIZE);
	// if size is less than SECTORSIZE, we have to put the buffer into the temp_buf
	if (size < SECTORSIZE) {
		if (size < BLOCKSIZE - (pos % BLOCKSIZE)) {
			memcpy(temp_buf, buf, size);
			if (WriteSectorWrapper(block_num, buf) == ERROR) {
				return ERROR;
			}
			return size;
		} else {
			memcpy(temp_buf, buf, BLOCKSIZE - (pos % BLOCKSIZE));
			if (WriteSectorWrapper(block_num, buf) == ERROR) {
				return ERROR;
			}
			size_left -= BLOCKSIZE - (pos % BLOCKSIZE);
			buf += BLOCKSIZE - (pos % BLOCKSIZE);
		}
	}

	// Write to blocks that are after the first block
	int num_blocks = get_num_blocks(inode);
	while (size_left > 0) {
		++block_num;
		if (block_num >= num_blocks) {   // We need a new block
			if (block_num < NUM_DIRECT) { // We need a new direct block 
				inode->direct[block_num] = remove_free_block();
				block_num = inode->direct[block_num];
			} else { // We need a new indirect block
				if (block_num == NUM_DIRECT) {
					if((inode->indirect = remove_free_block()) == ERROR) {
						return ERROR;
					}
				}
				int *indirects = (int *)get_block(inode->indirect);
				if ((block_num = remove_free_block()) == ERROR) {
					return ERROR;
				}
				indirects[block_num - NUM_DIRECT] = block_num;
			}	
		} else {
			if (block_num < NUM_DIRECT) {
				block_num = inode->direct[block_num];
			} else {
				int *indirect = (int *)get_block(inode->indirect);
				block_num = indirect[block_num - NUM_DIRECT];
				free(indirect);
			}
		}
		
		// Write to the last block needed
		if (size_left < BLOCKSIZE) {
			memcpy(temp_buf, buf, size_left);
			if (WriteSectorWrapper(block_num, buf) == ERROR) {
				return ERROR;
			}
			size_left = 0;
			break;
		}

		memcpy(temp_buf, buf, BLOCKSIZE);
		if (WriteSectorWrapper(block_num, buf) == ERROR) {
			return ERROR;
		}
		size_left -= BLOCKSIZE;
		buf += BLOCKSIZE;
	}

	inode->size += size;
	write_inode(inum, inode);

	free(temp_buf);
	return size;
}






