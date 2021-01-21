#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv) {
	int i, n;
	char name[1000];
	char *base;
	int fd;
	if (argc != 3) {
		printf("Usage: %s basename count\n", argv[0]);
		return 0;
	}
	strcpy(name, argv[1]);
	base = name + strlen(name);
	n = atoi(argv[2]);
	for (i = 0; i < n; ++i) {
		sprintf(base, "%d", i);
		//puts(name);
		fd = creat(name, S_IRWXU);
		if (-1 == fd) {
			perror("creat");
			return 0;
		}
		if (-1 == close(fd)) {
			perror("close");
			return 0;
		}
	}

	return 0;
}

