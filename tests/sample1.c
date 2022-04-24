#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int fd, i;

	fd = Create("a");
	Write(fd, "aaaaaaaaaaaaaaaa", 16);
	Close(fd);

	fd = Create("b");
	for (i = 0; i < 340; i++) {
	Write(fd, "bbbbbbbbbbbbbbbb", 19);
	}
	Close(fd);

	fd = Create("c");
	Write(fd, "cccccccccccccccc", 16);
	Close(fd);

	MkDir("dir");

	fd = Create("/dir/x");
	Write(fd, "xxxxxxxxxxxxxxxx", 16);
	Close(fd);

	fd = Create("/dir/y");
	Write(fd, "yyyyyyyyyyyyyyyy", 16);
	Close(fd);

	fd = Create("/dir/z");
	Write(fd, "zzzzzzzzzzzzzzzz", 16);
	Close(fd);

	Shutdown();

    return (0);
}
