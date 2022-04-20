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
	int block_num;
	struct inode *inode_block = malloc(SECTORSIZE);
	struct inode *res;
	if ((inum + 1) % inode_per_block == 0) {
		block_num = (inum + 1) / inode_per_block;
		int status = ReadSector(block_num, inode_block);
		if (status == ERROR) { 
			TracePrintf(0, "Cannot read the block to get inode.\n");
			return NULL;
		}
		res = &inode_block[inode_per_block - 1];
	} else {
		block_num = (inum + 1) / inode_per_block + 1;
		int status = ReadSector(block_num, inode_block);
		if (status == ERROR) { 
			TracePrintf(0, "Cannot read the block to get inode.\n");
			return NULL;
		}
		res = &inode_block[(inum + 1) % inode_per_block - 1];
	}
	free(inode_block);
	return res;
}

int write_inode(short inum, struct inode *inode) 
{
	int status;
	int block_num;
	struct inode *inode_block = malloc(SECTORSIZE);
	// Change the block's data to what we want
	if ((inum + 1) % inode_per_block == 0) {
		block_num = (inum + 1) / inode_per_block;
		status = ReadSector(block_num, inode_block);
		if (status == ERROR) { 
			TracePrintf(0, "Cannot read the block to get inode.\n");
			return ERROR;
		}
		inode_block[inode_per_block - 1] = *inode;
	} else {
		block_num = (inum + 1) / inode_per_block + 1;
		status = ReadSector(block_num, inode_block);
		if (status == ERROR) { 
			TracePrintf(0, "Cannot read the block to get inode.\n");
			return ERROR;
		}
		inode_block[(inum + 1) % inode_per_block - 1] = *inode;
	}

	// Write to the block with our new inode in it. 
	status = WriteSector(block_num, inode_block);
	if (status == ERROR) {
		TracePrintf(0, "Cannot write to block %d\n", block_num);
		return ERROR;
	}
	
	free(inode_block);

	return 0;
}	

int process_path(char *path, int curr_inum, int call_type) 
{
	char component[DIRNAMELEN + 1];
	int comp_ptr = 0;
	int path_ptr = 0;
	int num_blocks;
	int return_inum;

	struct inode *start_inode;
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
	TracePrintf(1, "start_inode's addr is: %p\n", start_inode);
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
			// if we are creating a file/directory, and we reached the end of path, break from loop
			if (call_type == CREATE && (path[path_ptr] == '\0' || path_ptr == MAXPATHNAMELEN)) {
				break;
			}
			struct dir_entry *dir_ptr = malloc(SECTORSIZE);
			TracePrintf(2, "there are %d blocks in the inode.\n", num_blocks);
			if (num_blocks <= NUM_DIRECT) {
				for (i = 0; i < num_blocks; i++) {
					int status = ReadSector(start_inode->direct[i], dir_ptr);
					if (status == ERROR)
						return ERROR;
					int j;
					for (j = 0; j < num_dir_in_block; j++) {
						if (compare_filenames(dir_ptr[j].name, component) == 1) {
							// if we found the right file/dir, move on/open file
							return_inum = dir_ptr[j].inum;
							start_inode = get_inode(return_inum);
							num_blocks = get_num_blocks(start_inode);
							num_dir = start_inode->size / sizeof(struct dir_entry);
						   	found_next_inode = 1;
							break;	
						}
						// if we already went through every file/directory, return error
						if (--num_dir <= 0) {
							TracePrintf(0, "Cannot find file that matches the name %s\n", component);
							return ERROR;
						}
					}
					// if found the next node in hierarchy, go to next while loop
					if (found_next_inode) 
						break;
				}	
			} else {
				for (i = 0; i < NUM_DIRECT; i++) {
					int status = ReadSector(start_inode->direct[i], dir_ptr);
					if (status == ERROR)
						return ERROR;
					int j;
					for (j = 0; j < num_dir_in_block; j++) {
						if (compare_filenames(dir_ptr[j].name, component) == 1) {
							// if we found the right file/dir, move on/open file
							return_inum = dir_ptr[j].inum;
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
					int status = ReadSector(start_inode->indirect, blocks);
					if (status == ERROR)
						return ERROR;
					for (i = 0; i < num_blocks - NUM_DIRECT; i++) {
						status = ReadSector(blocks[i], dir_ptr);
						if (status == ERROR)
							return ERROR;
						int j;
						for (j = 0; j < num_dir_in_block; j++) {
							if (compare_filenames(dir_ptr[j].name, component) == 1) {
								// if we found the right file/dir, move on/open file
								return_inum = dir_ptr[j].inum;
								start_inode = get_inode(return_inum);
								num_blocks = get_num_blocks(start_inode);
								num_dir = start_inode->size / sizeof(struct dir_entry);
								found_next_inode = 1;
								break;	
							}
							// if we already went through every file/directory, return error
							if (--num_dir <= 0) {
								TracePrintf(0, "Cannot find file that matches the name %s\n", component);
								return ERROR;
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
				TracePrintf(0, "Cannot find the right file to open\n");
				return ERROR;
			} 
			free(dir_ptr);
			// Reset component to be ready for the next component in path
			comp_ptr = 0;
			if (path[path_ptr] == '/') {
				path_ptr++;
			}
		}
	}
	
	if (call_type == CREATE) {
		return create_stuff(component, return_inum, INODE_REGULAR);
	} else if (call_type == MKDIR) {
		return create_stuff(component, return_inum, INODE_DIRECTORY);
	}

	return return_inum;
}

void add_free_inode(int inum)
{
	free_inode_list[inum - 1] = 1;	
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
			return i + 1;
		}
	}
	TracePrintf(0, "Cannot find a free block.\n");
	return ERROR;
}

int create_stuff(char *name, int parent_inum, short type) 
{
	TracePrintf(0, "Creating a file named %s\n", name);
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
	new_entry->inum = remove_free_inode(INODE_REGULAR);
	memcpy(new_entry->name, name, DIRNAMELEN);
	
	// Get how many blocks and directories the parent inode already have. 
	int num_blocks = get_num_blocks(parent_inode);
	int num_dir = get_num_dir(parent_inode);

	if (num_blocks == NUM_DIRECT && num_dir % num_dir_in_block == 0) {
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
			status = WriteSector(free_block, new_entry);
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
			status = ReadSector(num_blocks - 1, last_block);
			if (status == ERROR) { 
				free(new_entry);
				TracePrintf(0, "Failed to read from sector %d\n", num_blocks - 1);
				return ERROR;
			}
			// put the new dir_entry at the end of the last block
			int new_dir_index = num_dir % num_dir_in_block;
			last_block[new_dir_index] = *new_entry;
			// Write back the block
			status = WriteSector(num_blocks - 1, last_block);
			if (status == ERROR) {
				free(new_entry);
				TracePrintf(0, "Failed to write to sector %d\n", num_blocks - 1);
				return ERROR;
			}
		}
	} else {
		// Allocate a block in the indirect block
		int *indirect_blocks = malloc(SECTORSIZE);
		status = ReadSector(parent_inode->indirect, indirect_blocks);
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
			status = WriteSector(free_block, new_entry);
			if (status == ERROR) {
				add_free_block(free_block);
				free(new_entry);
				free(indirect_blocks);
				TracePrintf(0, "Failed to write to sector %d\n", free_block);
				return ERROR;
			}
			// Add a new block to the end of the indirect block. 
			indirect_blocks[num_blocks - NUM_DIRECT] = free_block;
			status = WriteSector(parent_inode->indirect, indirect_blocks);
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
			status = ReadSector(indirect_blocks[num_blocks - 1 - NUM_DIRECT], last_block);
			if (status == ERROR) {
				free(new_entry);
				free(indirect_blocks);
				TracePrintf(0, "Failed to read from sector %d\n", indirect_blocks[num_blocks - 1 - NUM_DIRECT]);
				return ERROR;
			}
			// put the new dir_entry at the end of the last block
			last_block[num_dir % num_dir_in_block] = *new_entry;
			// Write back the block
			status = WriteSector(indirect_blocks[num_blocks - 1 - NUM_DIRECT], last_block);
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
		status = WriteSector(free_block, entries);
		free(entries);
		if (status == ERROR) {
			free(new_entry);
			TracePrintf(0, "Failed to write to block %d\n", free_block);
			return ERROR;
		}
	} else {
		new_inode->size = 0;
	}
	write_inode(return_inum, new_inode);
	free(new_entry);

	TracePrintf(0, "the returning inode is %d\n", return_inum);
	return return_inum;
}



