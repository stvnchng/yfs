#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;

	status = Create("/a");
	printf("Create status %d\n", status);

	status = Link("/a", "/b");
	printf("Link status %d\n", status);

	status = MkDir("/d");
	printf("MkDir status %d\n", status);

	status = ChDir("/d/");
	printf("ChDir status %d\n", status);

	status = Create("./e");
	printf("Create status %d\n", status);

	status = Link("./e", "/f");
	printf("Link status %d\n", status);

	status = Unlink("/");
	printf("UnLink status %d\n", status);

	status = Write(0, "hahaha", 6);
	printf("Write status %d\n", status);

	Shutdown();
	return 0;
}
