#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
	char **file = NULL;
	int file_len = 0;
	char *block = NULL;
	{
		static const int PAGE_SIZE = 4096;
		int block_len;
		int pos = 0;
		do {
			block = realloc(block, pos + PAGE_SIZE);
			block_len = read(0, block + pos, PAGE_SIZE);
			pos += block_len;
		} while (block_len == PAGE_SIZE);
		block[pos] = '\0';
	}
	{
		char *cp, *cp0 = block;
		while ((cp = strstr(cp0, "\n--- ")) != NULL) {
			*cp = '\0';
			file = (char **) realloc(file, (file_len + 1) * sizeof(char *));
			file[file_len++] = cp0;
			cp0 = cp + 1;
		}
		file = (char **) realloc(file, (file_len + 1) * sizeof(char *));
		file[file_len++] = cp0;
	}
	{
		int i, j;
		for (i = 1; i < file_len; i++) {
			for (j = i; j < file_len; j++) {
				if (strcmp(file[i], file[j]) > 0) {
					char *tmp = file[i];
					file[i] = file[j];
					file[j] = tmp;
				}
			}
		}
		for (i = 0; i < file_len; i++) {
			printf("%s", file[i]);
			if (i < file_len - 1) putchar('\n');
		}
	}
	return 0;
}
