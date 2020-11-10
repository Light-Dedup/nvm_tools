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

#define STEP 4096

void fill_buf(uint32_t buf[]) {
	for (int i = 0; i < STEP / sizeof(uint32_t); ++i) {
		buf[i] = genrand_int32();
	}
}

int main() {
	static char buf[STEP];
	int ret;
	size_t block_cnt = 0;
	int fd = open("/mnt/pmem/test0", O_WRONLY);
	if (fd != -1) {
		printf("Error: Not an empty file system.");
		return 0;
	}
	fd = open("/mnt/pmem/test0", O_WRONLY | O_CREAT);
	IF_ERR_EXIT(fd, "open");

	init_genrand(2333);
	while (1) {
		fill_buf((uint32_t *)buf);
		ret = write(fd, buf, STEP);
		if (ret == STEP) {
			++block_cnt;
		} else {
			break;
		}
	}
	printf("After writing %ld blocks, write return %d\n", block_cnt, ret);

	return 0;
}
