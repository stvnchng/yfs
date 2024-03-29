#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include "yfs.h"
#include "iohelpers.h"


/*
 *  YFS User Request Procedures
 */
int YFSOpen(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Pathname null in YFSOpen\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->inum, OPEN, -1);
    if (inum == ERROR) return ERROR;
    msg->inum = inum;

    if (get_inode(inum) == NULL) {
        return ERROR;
    }

    return inum;
}

int YFSCreate(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Pathname null in YFSCreate\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->inum, CREATE, -1);
    if (inum == ERROR) return ERROR;
    msg->inum = inum;

    return 0;
}

int YFSRead(struct msg *msg, int pid)
{
    void *readbuf = malloc(msg->data2);
	if (readbuf == NULL) {
        printf("Malloc failed for readbuf in YFSRead\n");
		return ERROR;
	}
    int bytes_written = read_helper(msg->inum, msg->data1, msg->data2, readbuf);
    if (bytes_written == ERROR) {
        printf("Error in YFSRead\n");
        return ERROR;
    }
	SendBuf(pid, msg->ptr2, readbuf, msg->data2); 	
    msg->data1 = bytes_written;

    return bytes_written;
}

int YFSWrite(struct msg *msg, int pid)
{
	void *writebuf = GetBuf(pid, msg->ptr2, msg->data2);
    int bytes_written = write_helper(msg->inum, msg->data1, msg->data2, writebuf);
    if (bytes_written == ERROR) {
        printf("Error in YFSWrite\n");
        return ERROR;
    }
	free(writebuf);
    msg->data1 = bytes_written;
    return bytes_written;
}

int YFSSeek(struct msg_seek *msg, int pid)
{
    (void)pid; // might use if we decide to write errors to msg and reply
    struct inode *inode = get_inode(msg->inum);
    if (inode == NULL) {
        printf("Couldn't find inode in YFSSeek\n");
        return ERROR;
    }
    if (msg->position < 0 || msg->position > inode->size) {
        printf("Invalid position passed to YFSSeek\n");
        return ERROR;
    }
    // Check if the size
    int offset = msg->offset;
    switch (msg->whence)
    {
    // file position cannot go before start of file or further than inode
    case SEEK_SET: // from beginning
        if (offset < 0 || offset > inode->size) {
            printf("Invalid seek position for SEEK_SET\n");
            return ERROR;
        }
        msg->position = offset;
        break;
    case SEEK_CUR: // from current
        if (msg->position + offset < 0 || msg->position + offset > inode->size) {
            printf("Invalid seek position for SEEK_CUR\n");
            return ERROR;
        }
        msg->position = msg->position + offset;
        break;
    case SEEK_END: // from end
        if (inode->size + offset < 0) {
            printf("Invalid seek position for SEEK_END\n");
            return ERROR;
        }
        msg->position = inode->size + offset;
        break;
    }
    return msg->position;
}

int YFSLink(struct msg *msg, int pid)
{
    char *oldname = GetMessagePath(pid, msg->ptr1, msg->data1);
    char *newname = GetMessagePath(pid, msg->ptr2, msg->data2);
    if (oldname == NULL || newname == NULL) {
        printf("Invalid oldname or newname passed to YFSLink\n");
        return ERROR;
    }

	int old_inum = process_path(oldname, msg->inum, LINK, 0);
	if (old_inum <= 0) {
		return ERROR;
	}

    // Create the link from newname to existing file oldname
	int status = process_path(newname, msg->inum, LINK, old_inum);

    return status;
}

int YFSUnlink(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Invalid pathname passed to YFSLink\n");
        return ERROR;
    }

    int status = process_path(pathname, msg->inum, UNLINK, -1);

    return status;
}

int YFSMkDir(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Pathname null in YFSMkDir\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->inum, MKDIR, -1);
    if (inum == ERROR) return ERROR;
    msg->inum = inum;

    return 0;
}

int YFSRmDir(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Pathname null in YFSRmDir\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->inum, RMDIR, -1);
    if (inum == ERROR) return ERROR;
    msg->inum = inum;

    return 0;
}

int YFSChDir(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Pathname null in YFSChDir\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->inum, CHDIR, -1);
    if (inum == ERROR) return ERROR;
    msg->inum = inum;

    return 0;
}

int YFSStat(struct msg *msg, int pid)
{
    char *pathname = GetMessagePath(pid, msg->ptr1, msg->data1);
    if (pathname == NULL) {
        printf("Pathname null in YFSStat\n");
        return ERROR;
    }

    int inum = process_path(pathname, msg->inum, STAT, -1);
    if (inum == ERROR) return ERROR;
    msg->inum = inum; // store inum into message

    struct inode *inode = get_inode(inum);
    if (inode == NULL) {
        printf("Null inode in YFSStat\n");
        return ERROR;
    }

    int pathlen = msg->data1;
    struct Stat *statbuf = malloc(sizeof(struct Stat)); 
    statbuf->inum = inum;
    statbuf->type = inode->type;
    statbuf->size = inode->size;
    statbuf->nlink = inode->nlink;

    // copy statbuf info from server back to client 
    if (CopyTo(pid, msg->ptr2, statbuf, pathlen) == ERROR) {
        printf("Error in CopyTo for YFSStat\n");
        return ERROR;
    }

    msg->type = inode->type; // overwrite message type with inode type
    msg->data1 = inode->size; // overwrite pathlen with size
    msg->data2 = inode->nlink; //store nlink into data2
    // printf("YFSStat return message contents: inum %d, nlnk %d, size %d, type %d\n",
        //    statbuf->inum, statbuf->nlink, statbuf->size, statbuf->type);
    return 0;
}

int YFSSync(void)
{
    TracePrintf(0, "Writing back all dirty cached inodes and blocks...\n");
    struct cache_item *inode_head = inode_cache->head;
    while (inode_head != NULL) {
        if (inode_head->dirty) {
            int inum = inode_head->num;
            void *block_addr = get_block((inum / (BLOCKSIZE / INODESIZE)) + 1);
            if (block_addr == NULL) {
                printf("Error getting block for inode %d in Sync\n", inum);
                return ERROR;
            }
            // Section 4.1 - write back to block cache for the block holding inode 
            memcpy(block_addr, inode_head->value, sizeof(struct inode));
            TracePrintf(0, "Wrote back inum %d\n", inum);
            // TODO: do we need to WriteSector to the blocknum here?
            WriteSector(((inum / (BLOCKSIZE / INODESIZE)) + 1), block_addr);
            inode_head->dirty = false;
        }   
        inode_head = inode_head->next;
    }
    TracePrintf(0, "Now writing back all dirty cached blocks...\n");
    struct cache_item *block_head = block_cache->head;
    while (block_head != NULL) {
        // If inode is dirty, Sync writes it back
        if (block_head->dirty) {
            if (WriteSector(block_head->num, block_head->value) == ERROR) {
                printf("WriteSector error in Sync, block %d\n", block_head->num);
                return ERROR;
            }
            block_head->dirty = false;
        }
        block_head = block_head->next;
    }
    return 0;
}

int YFSShutdown(struct msg *msg, int pid)
{
    YFSSync(); // write back all dirty cached inodes and disk blocks
    Reply(msg, pid); // reply to the calling process and exit
    Exit(0);
}

int HandleRequest(struct msg *msg)
{
    int pid = Receive(msg);
    if (pid == ERROR) {
        printf("Receive message failed! YFS is shutting down...\n");
        YFSShutdown(msg, pid);
    }

    switch (msg->type)
    {
    case OPEN:
        TracePrintf(0, "[OPEN]\n");
        printf("[OPEN]\n");
        YFSOpen(msg, pid);
        break;
    case CREATE:
        TracePrintf(0, "[CREATE]\n");
        printf("[CREATE]\n");
        YFSCreate(msg, pid);
        break;
    case READ:
        TracePrintf(0, "[READ]\n");
        printf("[READ]\n");
        YFSRead(msg, pid);
        break;
    case WRITE:
        TracePrintf(0, "[WRITE]\n");
        printf("[WRITE]\n");
        YFSWrite(msg, pid);
        break;
    case SEEK:
        TracePrintf(0, "[SEEK]\n");
        printf("[SEEK]\n");
        YFSSeek((struct msg_seek *) msg, pid);
        break;
    case LINK:
        TracePrintf(0, "[LINK]\n");
        printf("[LINK]\n");
        YFSLink(msg, pid);
        break;
    case UNLINK:
        TracePrintf(0, "[UNLINK]\n");
        printf("[UNLINK]\n");
        YFSUnlink(msg, pid);
        break;
    case SYMLINK:
        TracePrintf(0, "[SYMLINK]\n");
        printf("[SYMLINK]\n");
        break;
    case READLINK:
        TracePrintf(0, "[READLINK]\n");
        printf("[READLINK]\n");
        break;
    case MKDIR:
        TracePrintf(0, "[MKDIR]\n");
        printf("[MKDIR]\n");
        YFSMkDir(msg, pid);
        break;
    case RMDIR:
        TracePrintf(0, "[RMDIR]\n");
        printf("[RMDIR]\n");
        YFSRmDir(msg, pid);
        break;
    case CHDIR:
        TracePrintf(0, "[CHDIR]\n");
        printf("[CHDIR]\n");
        YFSChDir(msg, pid);
        break;
    case STAT:
        TracePrintf(0, "[STAT]\n");
        printf("[STAT]\n");
        YFSStat(msg, pid);
        break;
    case SYNC:
        TracePrintf(0, "[SYNC]\n");
        printf("[SYNC]\n");
        YFSSync();
        break;
    case SHUTDOWN:
        TracePrintf(0, "[SHUTDOWN]\n");
        printf("[SHUTDOWN]\n");
        YFSShutdown(msg, pid);
        break;
    default:
        printf("Message with type %d was not recognized!\n", msg->type);
        break;
    }

    // send reply to the blocked process
    if (Reply((void *) msg, pid) == ERROR) {
        printf("An error occurred during reply for pid %d\n", pid);
		return ERROR;
    }
	return 0;
}

char *GetMessagePath(int srcpid, void *src, int pathlen)
{
    char *dest = malloc(sizeof(char) *  pathlen);
    if (CopyFrom(srcpid, dest, src, pathlen) == ERROR) {
        printf("Error occurred at CopyFrom in GetMessagePath\n");
        return NULL;
    }
    return dest;
}

void *GetBuf(int srcpid, void *src, int buflen) 
{
	void *dest = malloc(buflen);
	if (CopyFrom(srcpid, dest, src, buflen) == ERROR) {
		printf("Error occurred at CopyFrom in GetBuf\n");
		return NULL;
	}
	return dest;
}

int SendBuf(int destpid, void *dest, void *src, int buflen)
{
	if (CopyTo(destpid, dest, src, buflen) == ERROR) {
		printf("Error occurred at CopyTo in SendBufWrite\n");
		return ERROR;
	}
	return 0;
}
