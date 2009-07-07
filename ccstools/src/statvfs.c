#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <sys/vfs.h>

int main(int argc, char *argv[])
{
	struct statfs buf;
	if (statfs(argv[1], &buf))
		return 1;
	printf("0x%08X\n", buf.f_type);
	return 0;
}
