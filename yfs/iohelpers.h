/**
 * variables for iohelper
 */
struct inode *curr_inode;

/**
 * Helper functions for iolib.c
 */
int compare_filenames(char *f1, char *f2);
int get_num_blocks(struct inode *inode);
struct inode *get_inode(short inum);
struct inode *process_path(char *path);
