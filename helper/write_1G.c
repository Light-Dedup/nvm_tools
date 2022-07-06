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

#define STEP (2 * 1024 * 1024)
#define COUNT (1024 * 1024 * 1024 / STEP)
#define NUM_4K_BLKS (1024 * 1024 * 1024 / 4096)

void fill_buf(uint64_t buf[], uint64_t base, uint64_t value) {
	uint64_t num_4k_blks_per_batch = STEP / 4096;
	value = value * num_4k_blks_per_batch;
	for (uint64_t i = 0; i < STEP / sizeof(uint64_t); ++i) {
		buf[i] = base + value;	// genrand_int32();
		if (i != 0 && i % 4096 == 0) {
			value += 1;
		}
	}
}

#define FNAME_BASE ("/mnt/pmem0/test")
int main(int argc, char **argv) {
	static char fname[100] = FNAME_BASE;
	static char buf[STEP];
	int ret;
	int fd;
	uint64_t base;

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
	base = atoi(argv[1]) * NUM_4K_BLKS;
	for (uint64_t i = 0; i < COUNT; ++i) {
		fill_buf((uint64_t *)buf, base, i);
		ret = write(fd, buf, STEP);
		IF_ERR_EXIT(ret, "write");
	}
	if (ret != STEP) {
		printf("Write failed with return code %d\n", ret);
		return 1;
	} else {
		return 0;
	}
}

