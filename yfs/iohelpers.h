/**
 * Helper functions for iolib.c
 */
int compare_filenames(char *f1, char *f2);
int get_num_blocks(struct inode *inode);
int get_num_dir(struct inode *inode);
struct inode *get_inode(short inum);
void *get_block(int blocknum);
int write_inode(short inum, struct inode *inode);
int process_path(char *path, int curr_inum, int call_type);
void add_free_inode(int inum);
int remove_free_inode(short type);
void add_free_block(int blocknum);
int remove_free_block();
int create_stuff(char *name, int parent_inum, short type);
int remove_dir_entry(struct inode *child_inode, struct inode *parent_inode, int child_inum, int parent_inum);
int find_dir_entry_block(int inum, struct inode *parent_inode, int parent_num_blocks, int parent_num_dir);
int read_helper(int inum, int pos, int size, void *buf);
int write_helper(int inum, int pos, int size, void *buf);
int link_helper(int inum, int curr_inum, char *new_name);

