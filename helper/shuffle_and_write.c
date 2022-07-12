#include "stdio.h"
#include <sys/stat.h>
#include <unistd.h>
#include "stdlib.h"
#include <sys/time.h>
#include "fcntl.h"
#include <string.h>
#include "mt19937ar.h"

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

    char *optstring = "f:g:h"; 
	int opt;
	char filepath[128] = {0};
	char shuffled_path[128] = {0};
	char new_filepath[128] = {0};
	char *records;
	int *map;
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
    records = (char *)malloc(num_blocks);
	memset(records, 0, num_blocks);
	memset(map, 0, num_blocks * sizeof(int));
    
	pos = 0;
	while (pos < num_blocks) {
		value = genrand_int32() % num_blocks;
		if (records[value] == 0) {
			records[value] = 1;
			map[pos] = value;
			pos++;
		}
	}
    
    fd = open(filepath, O_RDONLY);
	sprintf(shuffled_path, "%s-" SHUFFLE_SUFFIX, filepath);
	shuffled_fd = open(shuffled_path , O_RDWR | O_CREAT | O_TRUNC);

	for (int i = 0; i < num_blocks; i++) {
		lseek(fd, map[i] * granularity, SEEK_SET);
		read(fd, records, granularity);
		write(shuffled_fd, records, granularity);	
	}
	
	close(fd);
	close(shuffled_fd);
	
    return 0;
}