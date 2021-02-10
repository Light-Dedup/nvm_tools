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

#define STEP (1024 * 1024 * 1024)

void fill_buf(uint32_t buf[]) {
	for (int i = 0; i < STEP / sizeof(uint32_t); ++i) {
		buf[i] = genrand_int32();
	}
}

#define FNAME_BASE ("/mnt/pmem/test")
int main(int argc, char **argv) {
	static char fname[100] = FNAME_BASE;
	static char buf[STEP];
	int ret;
	int fd;

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

	init_genrand(atoi(argv[1]));
	fill_buf((uint32_t *)buf);
	ret = write(fd, buf, STEP);
	if (ret != STEP) {
		printf("Write failed with return code %d\n", ret);
		return 1;
	} else {
		return 0;
	}
}

