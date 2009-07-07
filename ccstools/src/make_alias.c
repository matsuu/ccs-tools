#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/vfs.h>

static void printf_encoded(const char *str)
{
	while (1) {
		unsigned char c = *(const unsigned char *) str++;
		if (!c)
			break;
		if (c == '\\') {
			putchar('\\');
			putchar('\\');
		} else if (c > ' ' && c < 127) {
			putchar(c);
		} else {
			printf("\\%c%c%c", (c >> 6) + '0',
			       ((c >> 3) & 7) + '0', (c & 7) + '0');
		}
	}
}

static void scan_symlink(char *path)
{
	char *base1;
	char *base2;
	char *cp = strrchr(path, '/');
	struct statfs buf;
	int ret;
	if (!cp)
		return;
	*cp = '\0';
	ret = statfs(path, &buf);
	*cp = '/';
	if (ret)
		return;
	switch (buf.f_type) {
	case 0x00009FA0: /* proc */
	case 0x62656572: /* sys */
	case 0x64626720: /* debug */
	case 0x73636673: /* security */
	case 0x00009FA2: /* usb */
	case 0x0027E0EB: /* cgroup */
	case 0x0BAD1DEA: /* futex */
	case 0x2BAD1DEA: /* inotify */
	case 0x00001373: /* device */
	case 0x00001CD1: /* devpts */
	case 0x42494E4D: /* binfmt_misc */
	case 0x67596969: /* rpc_pipefs */
	case 0x19800202: /* mqueue */
	case 0xABABABAB: /* vmblock */
		return;
	}
	if (statfs(path, &buf))
		return;
	switch (buf.f_type) {
	case 0x00009FA0: /* proc */
	case 0x62656572: /* sys */
	case 0x64626720: /* debug */
	case 0x73636673: /* security */
	case 0x00009FA2: /* usb */
	case 0x0027E0EB: /* cgroup */
	case 0x0BAD1DEA: /* futex */
	case 0x2BAD1DEA: /* inotify */
	case 0x00001373: /* device */
	case 0x00001CD1: /* devpts */
	case 0x42494E4D: /* binfmt_misc */
	case 0x67596969: /* rpc_pipefs */
	case 0x19800202: /* mqueue */
	case 0xABABABAB: /* vmblock */
		return;
	}
	cp = realpath(path, NULL);
	if (!cp)
		return;
	base1 = strrchr(path, '/');
	base2 = strrchr(cp, '/');
	if (strcmp(base1, base2) && strncmp(base1, "/lib", 4) &&
	    !strstr(base1, ".so")) {
		printf("alias ");
		printf_encoded(cp);
		putchar(' ');
		printf_encoded(path);
		putchar('\n');
	}
	free(cp);
}

static int scandir_filter(const struct dirent *buf)
{
	return buf->d_type == DT_LNK || buf->d_type == DT_DIR;
}

static void scan_dir(const char *dir)
{
	static struct stat buf;
	static char path[8192];
	static _Bool first = 1;
	struct dirent **namelist;
	int i;
	int n = scandir(dir, &namelist, scandir_filter, 0);
	if (n < 0)
		return;
	if (first) {
		memset(path, 0, sizeof(path));
		if (strcmp(dir, "/"))
			strncpy(path, dir, sizeof(path) - 1);
		first = 0;
	}
	for (i = 0; i < n; i++) {
		const char *cp = namelist[i]->d_name;
		const unsigned char type = namelist[i]->d_type;
		const int len = strlen(path);
		snprintf(path + len, sizeof(path) - len - 1, "/%s", cp);
		if (type == DT_LNK && !stat(path, &buf) &&
		    S_ISREG(buf.st_mode) && (buf.st_mode & 0111))
			scan_symlink(path);
		else if (type == DT_DIR && strcmp(cp, ".") && strcmp(cp, ".."))
			scan_dir(path);
		path[len] = '\0';
		free((void *) namelist[i]);
	}
	free((void *) namelist);
}

int main(int argc, char *argv[])
{
	if (argc > 1 && (chroot(argv[1]) || chdir("/")))
		return 1;
	scan_dir("/");
	return 0;
}
