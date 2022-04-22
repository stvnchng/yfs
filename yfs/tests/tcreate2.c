#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

/* After running this, try topen2 and/or tunlink2 */

int
main()
{
	printf("\n%d\n\n", MkDir("/foo"));
	printf("\n%d\n\n", MkDir("/foo/haha"));
	// printf("\n%d\n\n", Create("/foo"));
	printf("\n%d\n\n", Create("/bar"));
	// printf("\n%d\n\n", Create("/foo"));
	printf("\n%d\n\n", Create("/foo/zzz"));
	printf("\n%d\n\n", RmDir("/foo/haha"));
	printf("\n%d\n\n", ChDir("/foo"));
	printf("\n%d\n\n", Create("./happy"));
	printf("\n%d\n\n", Create("/foo/lol"));
	printf("\n%d\n\n", ChDir("../"));
	printf("\n%d\n\n", Open("/bar"));

	Shutdown();
	return 0;
}
