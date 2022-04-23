# COMP421 Lab 3: Yalnix File System

## Team
Yujian Ou (yo11) and Steven Chung (sc111)

## How to run
First, run 'make'. Then, run ~comp421/pub/bin/yalnix yfs <userprogram> where <userprogram> can be any program that calls YFS user methods such as Create and Write.

## Implementation
### Data structures
- The hash table implementation is borrowed from the 321 lab, and it is modified to reflect the more simple requirements for this lab's hashing.
- cache: designed to be an LRU cache and contains information such as the current size of the cache, the max size of cache (defined in filesystem.h), the hash table associated with the cache, and the head and tail item in the cache. We store the head item to identify which item was most recently accessed, and the tail to identify where to start removing from once the cache hits the max size.
- cache_item: contains information about whether the item is dirty, pointers to the next and previous cache_item, and a key and value. The key is used to associate the item with the cache's hash table, where key is inum or blocknum and value is an inode or void pointer depending on which cache we are accessing.
- msg: a 32-byte message struct that should cover any valid request. Holds important information including the message type, which identifies the user call, and inum. Each generic data member describes what info it can potentially hold, which is detailed in yfs.h. 
- msg_seek: a 32-byte custom message built for handling Seek as it requires 5 int members rather than the 4 in the generic msg struct. It is possible to store the 5th int member's address into the generic msg's void pointer, but this was defined for more clarity and ease of implementation.

### Global variables
- count of free blocks and inodes and a representation of the free lists
- caches for inodes and blocks
- helper definitions for the # of inodes and directories in a block

### Helper Functions
- methods to process the hierarchy tree and get inodes or blocks based on number and vice versa
- methods to allocate, write to, and remove inodes and blocks
- methods to handle modifying the cache and setting items as dirty
- methods to encapsulate the process of copying data from the client to the server and vice versa (using CopyFrom and CopyTo for buffers and strings, for example)

## Testing
We have run all tests provided in the samples/lab3 directory on CLEAR. We have also written our own custom tests to test strain on memory.
