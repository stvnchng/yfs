#include <stdio.h>
#include <stdlib.h>
#include "iohelpers"
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
	struct inode **inode_block = malloc(SECTORSIZE);
	if ((inum + 1) % inode_per_block == 0) {
		block_num = (inum + 1) / inode_per_block;
		int status = ReadSector(block_num, inode_block);
		if (status == ERROR) 
			return ERROR;
		return inode_block[inode_per_block - 1];
	} else {
		block_num = (inum + 1) / inode_per_block + 1;
		int status = ReadSector(block_num, inode_block);
		if (status == ERROR) 
			return ERROR;
		return inode_block[(inum + 1) % inode_per_block - 1];
	}
}

struct inode *process_path(char *path) 
{
	char component[DIRNAMELEN + 1];
	int comp_ptr = 0;
	int path_ptr = 0;
	int num_blocks;

	struct inode *start_inode;
	if (path[0] == '/') {
		start_inode = root_ptr;
		path_ptr = 1;
	} else {
		start_inode = curr_inode;
	}
	num_blocks = get_num_blocks(start_inode);
	if (num_blocks == 0) 
		return ERROR;
	num_dir = start_inode->size / sizeof(struct dir_entry);
	num_dir_in_block = BLOCKSIZE / sizeof(struct dir_entry);

	while (path_ptr < MAXPATHNAMELEN && path[path_ptr] != '\0') {
		if (start_inode->type != INODE_DIRECTORY) {
			TracePrintf(0, "The current component in the path is not a directory.\n");
			return ERROR;
		}
		if (path[path_ptr] != '/') {
			component[comp_ptr++] = path[path_ptr];	
		} else {
			component[comp_ptr] = '\0';
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
						if (compare_file_names(dir_ptr[j]->name, component) == 1) {
							// if we found the right file/dir, move on/open file
							short next_inum = dir_ptr[j]->inum;
							start_inode = get_inode(next_inum);
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
					status = ReadSector(start_inode->direct[i], dir_ptr);
					if (status == ERROR)
						return ERROR;
					int j;
					for (j = 0; j < num_dir_in_block; j++) {
						if (compare_file_names(dir_ptr->name, component) == 1) {
							// if we found the right file/dir, move on/open file
							short next_inum = dir_ptr->inum;
							start_inode = get_inode(next_inum);
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
					status = ReadSector(start_inode->indirect, blocks);
					if (status == ERROR)
						return ERROR;
					for (i = 0; i < num_blocks - NUM_DIRECT; i++) {
						status = ReadSector(blocks[i], dir_ptr);
						if (status == ERROR)
							return ERROR;
						int j;
						for (j = 0; j < num_dir_in_block; j++) {
							if (compare_file_names(dir_ptr[j]->name, component) == 1) {
								// if we found the right file/dir, move on/open file
								short next_inum = dir_ptr[j]->inum;
								start_inode = get_inode(next_inum);
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
			free(dir_ptr);
			// Reset component to be ready for the next component in path
			comp_ptr = 0;
		}
	}

	if (start_inode->type != INODE_REGULAR) {
		TracePrintf(0, "Cannot open a non-file.\n");
		return ERROR;
	}

	return start_inode;
}




