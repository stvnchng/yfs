#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include "iohelpers.h"
#include "yfs.h"

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

struct inode *get_inode(short inum) 
{
	int inode_per_block = BLOCKSIZE / INODESIZE;
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
	int num_dir = start_inode->size / sizeof(struct dir_entry);
	int num_dir_in_block = BLOCKSIZE / sizeof(struct dir_entry);

	while (path_ptr < MAXPATHNAMELEN && path[path_ptr] != '\0') {
		if (start_inode->type != INODE_DIRECTORY) {
			TracePrintf(0, "The current inode is not a directory.\n");
			TracePrintf(1, "The current inode is a %d\n", start_inode->type);
			return ERROR;
		}
		if (path[path_ptr] != '/') {
			printf("%c\n", path[path_ptr]);
			component[comp_ptr++] = path[path_ptr];	
		} else {
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
			struct dir_entry **dir_ptr = malloc(SECTORSIZE);
			if (num_blocks <= NUM_DIRECT) {
				for (i = 0; i < num_blocks; i++) {
					int status = ReadSector(start_inode->direct[i], dir_ptr);
					if (status == ERROR)
						return ERROR;
					int j;
					for (j = 0; j < num_dir_in_block; j++) {
						if (compare_filenames(dir_ptr[j]->name, component) == 1) {
							// if we found the right file/dir, move on/open file
							return_inum = dir_ptr[j]->inum;
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
						if (compare_filenames(dir_ptr[j]->name, component) == 1) {
							// if we found the right file/dir, move on/open file
							return_inum = dir_ptr[j]->inum;
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
							if (compare_filenames(dir_ptr[j]->name, component) == 1) {
								// if we found the right file/dir, move on/open file
								return_inum = dir_ptr[j]->inum;
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
				switch (call_type) {
					case 0:
						TracePrintf(0, "Cannot find the right file to open\n");
						return ERROR;
					case 1:
						// if it is create calling process path, then create a file that is named component.
						create_file(component);
						break;
					default:
						break;
				}
			} 
			free(dir_ptr);
			// Reset component to be ready for the next component in path
			comp_ptr = 0;
		}
	}

	return return_inum;
}

void add_free_inode(int inum)
{
	free_inode_list[inum - 1] = 1;	
}

short remove_free_inode(short type) 
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

short remove_free_block() 
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

int create_file(char *name) 
{
	(void) name;
	return ERROR;
}



