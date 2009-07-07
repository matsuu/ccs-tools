#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int print_tail = 1;
	if (argc == 3) {
		if (!strcmp(argv[1], "-n"))
			print_tail = 0;
		argv++;
		argc--;
	}
	if (argc == 2) {
		char *cp = realpath(argv[1], NULL);
		if (cp)
			printf("%s%s", cp, print_tail ? "\n" : "");
		free(cp);
	}
	return 0;
}
