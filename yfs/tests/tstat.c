#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>
#include "../yfs.h"

int
main()
{
    // int fd1 = Create("/a/i.txt");
    // printf("returned from Create with fd: %d\n", fd1);
    // int fd2 = Open("/a/i.txt");
    // printf("opened file with fd: %d\n", fd2);    
    // int val = Close(fd1);
    // printf("close file with fd: %d, return %d\n", fd1, val);    
    // val = Close(fd2);
    // printf("close file with fd: %d, return %d\n", fd2, val);    
    // printf("called mkdir, return %d\n", MkDir("/a/j"));
    // printf("called chdir, return %d\n", ChDir("/a/j"));
    int fd = Create("x.txt");
   TracePrintf(1, "creating file x.txt, got fd %d\n", fd);
//    TracePrintf(1, "opened file /a/j/x.txt, got fd %d\n", Open("/a/j/x.txt"));
//    TracePrintf(1, "opened file x.txt, got fd %d\n", Open("x.txt"));
//    TracePrintf(1, "unlinking /a/j/x.txt, got %d\n", Unlink("/a/j/x.txt"));
//    TracePrintf(1, "removing directory /a/j, got %d\n", RmDir("/a/j"));
    // printf("writing to file, got %d\n", Write(fd, "hello world\n", 13));
    int fd0 = Open("x.txt");
    printf("opening file, got fd %d\n", fd0);
    // printf("calling seek, got %d\n", Seek(fd, -14, SEEK_END));
    // char buf[13];
    // printf("reading from file, got %d\n", Read(fd, buf, 13));
    // printf("got %s\n", buf);
    // TracePrintf(1, "calling link, got %d\n", Link("x.txt", "d.txt"));
    // fd1 = Open("d.txt");
    // TracePrintf(1, "opening linked file, got fd %d\n", fd1);
    // char buf2[13];
    // printf( "reading from file, got %d\n", Read(fd1, buf2, 13));
    // printf( "got %s\n", buf2);
    struct Stat stat;
    printf("Stat returns %d\n", Stat("x.txt", &stat));
    printf("inum %d, nlnk %d, size %d, type %d\n",
           stat.inum, stat.nlink, stat.size, stat.type);
    Shutdown();
    Exit(0);
}