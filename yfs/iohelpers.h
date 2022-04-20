/**
 * Helper functions for iolib.c
 */
int compare_filenames(char *f1, char *f2);
int get_num_blocks(struct inode *inode);
int get_num_dir(struct inode *inode);
struct inode *get_inode(short inum);
int process_path(char *path, int curr_inum, int call_type);
void add_free_inode(int inum);
int remove_free_inode(short type);
void add_free_block(int blocknum);
int remove_free_block();
int create_file(char *name, int parent_inum);
