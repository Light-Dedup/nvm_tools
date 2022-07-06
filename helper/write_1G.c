#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mt19937ar.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define IF_ERR_EXIT(err, msg)  \
	if (err < 0) {				\
		perror(msg);		\
		exit(1);			\
	}

#define SIZE (1024 * 1024 * 1024)
#define STEP (2 * 1024 * 1024)
#define PAGE_SIZE 4096

static inline void fill_page(char *page, uint64_t v)
{
	uint64_t *buf = (uint64_t *)page;
	uint64_t *limit = (uint64_t *)(page + PAGE_SIZE);
	while (buf != limit) {
		*buf = v;
		buf += 1;
	}
}

#define FNAME_BASE ("/mnt/pmem0/test")
int main(int argc, char **argv) {
	static char fname[100] = FNAME_BASE;
	static char buf[STEP];
	int ret;
	int fd;
	uint64_t v;

	if (argc != 2) {
		printf("Usage: %s rand_seed\n", argv[0]);
		return 1;
	}

	strcpy(fname + sizeof(FNAME_BASE) - 1, argv[1]);
	puts(fname);
	fd = open(fname, O_WRONLY);
	if (fd != -1) {
		printf("Error: Not an empty file system.");
		return 1;
	}
	fd = open(fname, O_WRONLY | O_CREAT);
	IF_ERR_EXIT(fd, "open");

	v = atoi(argv[1]) * (SIZE / PAGE_SIZE);
	for (uint64_t i = 0; i < SIZE / STEP; ++i) {
		for (uint64_t j = 0; j < STEP; j += PAGE_SIZE) {
			fill_page(buf + j, v);
			v += 1;
		}
		ret = write(fd, buf, STEP);
		IF_ERR_EXIT(ret, "write");
		if (ret != STEP) {
			printf("Write failed with return code %d\n", ret);
			return 1;
		}
	}
	return 0;
}

