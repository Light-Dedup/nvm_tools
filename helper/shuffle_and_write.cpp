#include "stdio.h"
#include <sys/stat.h>
#include <unistd.h>
#include "stdlib.h"
#include <sys/time.h>
#include "fcntl.h"
#include <string.h>
#include "mt19937ar.h"
#include <random>
#include <algorithm>

extern char *optarg;

#define SHUFFLE_SUFFIX "shuffled" 

void usage() 
{
	printf("Shuffle data blocks in a given file written by deadpool\n");
	printf("Description: shuffle x B data blocks in a given file \n");
	printf("             and write these blocks into a new file\n");
	printf("-f file  <filename>\n");
	printf("-g granularity <GiB>\n");
	printf("Example: ./shuffle_and_write -f /mnt/pmem/hello -g 4096 \n");
}

int main(int argc, char **argv) {

    char *optstring = (char *)"f:g:h"; 
	int opt;
	char filepath[128] = {0};
	char shuffled_path[128] = {0};
	char new_filepath[128] = {0};
	int *map;
	char *very_large_filebuf = NULL;
	int fd, shuffled_fd;
	int pos, value;
    long num_blocks;
    long granularity;
    struct stat st;
	struct timeval start, end;
    off_t size;
	double diff; 

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch(opt) {
		case 'f':
			strcpy(filepath, optarg);
			break;
		case 'g': 
			granularity = atol(optarg);
			break;
		case 'h':
			usage();
			exit(1);
		default:
			printf("Bad usage!\n");
			usage();
			exit(1);
		}
	}

    if (access(filepath, F_OK) == -1) {
        printf("File %s does not exist!\n", filepath);
        exit(1);
    }

    stat(filepath, &st);
    size = st.st_size;
    num_blocks = size / granularity == 0 ? 1 : size / granularity;
    map = (int *)malloc(num_blocks * sizeof(int));
	if (map == NULL) {
		printf("Failed to allocate memory for map!\n");
		exit(1);
	}

	very_large_filebuf = (char *)malloc(granularity);
	memset(map, 0, num_blocks * sizeof(int));
    
	pos = 0;
	while (pos < num_blocks) {
		map[pos] = pos;
		pos++;
	}

	std::random_shuffle(map, map + num_blocks);
    
    fd = open(filepath, O_RDONLY);
	sprintf(shuffled_path, "%s-" SHUFFLE_SUFFIX, filepath);
	shuffled_fd = open(shuffled_path , O_RDWR | O_CREAT | O_TRUNC);

	very_large_filebuf = (char *)malloc(granularity * num_blocks);
	if (very_large_filebuf == NULL) {
		printf("Failed to allocate memory for file buffer!\n");
		exit(1);
	}

	for (int i = 0; i < num_blocks; i++) {
		read(fd, very_large_filebuf + i * granularity, granularity);
	}
	
	gettimeofday(&start, NULL);
	for (int i = 0; i < num_blocks; i++) {
		write(shuffled_fd, very_large_filebuf + map[i] * granularity, granularity);
	}
	gettimeofday(&end, NULL);
	diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
	diff /= 1000000;
	printf("Shuffle and write %ld blocks in %f seconds\n", num_blocks, diff);
	printf("Bandwidth: %f MiB/s\n", num_blocks * granularity / diff / 1024 / 1024);
	
	free(very_large_filebuf);
	free(map);

	close(fd);
	close(shuffled_fd);
    return 0;
}