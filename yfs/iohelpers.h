/**
 * definitions
 */
#define OPEN 0
#define CREATE 1


/**
 * Helper functions for iolib.c
 */
int compare_filenames(char *f1, char *f2);
int get_num_blocks(struct inode *inode);
struct inode *get_inode(short inum);
int process_path(char *path, int curr_inum, int call_type);
void add_free_inode(int inum);
short remove_free_inode();
void add_free_block(int blocknum);
short remove_free_block();
