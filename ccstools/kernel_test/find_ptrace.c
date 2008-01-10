/* find -type f -name '*.c' -print0 | xargs -0 grep -lZ _ptrace | xargs -0 ./a.out */

#include <stdio.h>
#include <string.h>

static void convert(const char *filename) {
	int c = 0, d = 0;
	int in_single_quote = 0, in_double_quote = 0;
	FILE *fp_in = fopen(filename, "r");
	FILE *fp_out = fopen("/dev/shm/source.c", "w");
	if (!fp_in || !fp_out) {
		if (!fp_in) {
			fprintf(stderr, "Can't open %s\n", filename);
		} else {
			fclose(fp_in);
		}
		if (!fp_out) {
			fprintf(stderr, "Can't open /dev/shm/source.c\n");
		} else {
			fclose(fp_out);
		}
		return; 
	}
	while (d = c, (c = fgetc(fp_in)) != EOF) {
		if (c == '\\') {
			if ((c = fgetc(fp_in)) == EOF) break;
		} else if (in_single_quote) {
			if (c == '\'') in_single_quote = 0;
			continue;
		} else if (in_double_quote) {
			if (c == '"') in_double_quote = 0;
			continue;
		} else if (c == '\'') {
			in_single_quote = 1;
			continue;
		} else if (c == '"') {
			in_double_quote = 1;
			continue;
		} else if (c == '/') {
			d = c;
			c = fgetc(fp_in);
			if (c == '/') {
				while (d = c, (c = fgetc(fp_in)) != EOF) {
					if (c == '\n') {
						fputc(c, fp_out);
						break;
					}
				}
				continue;
			} else if (c == '*') {
				c = 0;
				while (d = c, (c = fgetc(fp_in)) != EOF) {
					if (c == '\n') fputc(c, fp_out);
					if (d == '*' && c == '/') break;
				}
				continue;
			} else {
				ungetc(c, fp_in);
				c = d;
			}
		}
		fputc(c, fp_out);
	}
	fclose(fp_in);
	fclose(fp_out);
}

int main(int argc, char *argv[]) {
	static char buffer[4096];
	int pos = 0;
	int i, c, p;
	int line;
	int first;
	for (i = 1; i < argc; i++) {
		FILE *fp;
		if (strstr(argv[i], "/.pc/")) continue;
		if (strstr(argv[i], "/patches/")) continue;
		convert(argv[i]);
		if ((fp = fopen("/dev/shm/source.c", "r")) == NULL) {
			fprintf(stderr, "Can't open /dev/shm/source.c\n");
			continue;
		}
		line = 1;
		first = 1;
		p = 0;
		while ((c = fgetc(fp)) != EOF) {
			long fpos;
			if (c == '\n') line++;
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
				if (pos < sizeof(buffer) - 1) buffer[pos++] = c;
				continue;
			}
			buffer[pos] = '\0';
			if (!pos) continue;
			pos = 0;
			if (strcmp(buffer, "do_sys_ptrace") &&
			    strcmp(buffer, "sys_ptrace") &&
			    strcmp(buffer, "compat_sys_ptrace") &&
			    strcmp(buffer, "do_ptrace") &&
			    strcmp(buffer, "sys32_ptrace") &&
			    strcmp(buffer, "sh64_ptrace") &&
			    strcmp(buffer, "CheckCapabilityACL")) {
				continue;
			}
			if (first) {
				first = 0;
				printf("--- %s ---\n", argv[i]);
			}
			fpos = ftell(fp);
			while ((c = fgetc(fp)) != EOF) {
				if (c == ';') {
					if (strcmp(buffer, "CheckCapabilityACL") == 0) printf("%4d:                                         %s\n", line, buffer);
					break;
				}
				if (c == '{') {
					if (strcmp(buffer, "do_sys_ptrace") == 0 ||
					    strcmp(buffer, "sys_ptrace") == 0 ||
					    strcmp(buffer, "compat_sys_ptrace") == 0 ||
					    strcmp(buffer, "do_ptrace") == 0 ||
					    strcmp(buffer, "sys32_ptrace") == 0 ||
					    strcmp(buffer, "sh64_ptrace") == 0) {
						printf("%4d:                     %s\n", line, buffer);
					} else if (strcmp(buffer, "CheckCapabilityACL") == 0) {
						printf("%4d:                                         %s\n", line, buffer);
					}
					break;
				}
			}
			fseek(fp, fpos, SEEK_SET);
		}
		fclose(fp);
	}
	return 0;
}
