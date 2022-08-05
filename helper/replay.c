#define _GNU_SOURCE
#include "stdio.h"
#include <unistd.h>
#include "stdlib.h"
#include "fcntl.h"
#include <stdint.h>
#include "string.h"
#include "mt19937ar.h"
#include <errno.h>
#include <sys/time.h>

extern char *optarg;

void usage() 
{
	printf("Replay file parsed by blkparse written by deadpool\n");
	printf("Description: tools to help reply traces\n");
	printf("-f blkparse   <.blkparse filename>\n");
	printf("-d dstpath    <dst filename to replay>\n");
    printf("Example: ./replay -f homes-sample.blkparse -d /mnt/pmem0/test \n");
}

#define BLK_SIZE 4096
#define LINE_SIZE 4096

static inline void fill_blk(char *blk, char *md5, int n)
{
	int step = n / 32;
    int i = 0, j;
    int blk_size_per_step = BLK_SIZE / step;
    int md5_per_step = blk_size_per_step / 32;
    char *cur_md5 = md5;
    char *cur_blk = blk;

    for (i = 0; i < step; i++) {
        j = 0;
        for (j = 0; j < md5_per_step; j++) {
            memcpy(cur_blk + j * 32, cur_md5, 32);
        }
        cur_blk += blk_size_per_step;
        cur_md5 += 32;
    }
}

double get_ms_diff(struct timeval tvBegin, struct timeval tvEnd)
{
	return 1000 * (tvEnd.tv_sec - tvBegin.tv_sec) + ((tvEnd.tv_usec - tvBegin.tv_usec) / 1000.0);
}

int main(int argc, char **argv)
{
	char *optstring = "f:d:h"; 
	int opt;
    FILE *src_fp;
	int dst_fd;
	char filepath[128] = {0};
	char dstpath[128] = {0};
	char blk[BLK_SIZE];
    char line[LINE_SIZE];
	struct timeval start, end;
    unsigned long size_in_total = 0;
    unsigned long blks_start;
    unsigned long blks_end;
    unsigned long blks_replayed = 0;
	double diff; 
    double time_usage = 0;

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch(opt) {
		case 'f':
			strcpy(filepath, optarg);
			break;
		case 'd': 
			strcpy(dstpath, optarg);
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

    if (strlen(filepath) == 0) {
        printf("Please specify the blkparse file\n");
        usage();
        exit(1);
    }

    if (strlen(dstpath) == 0) {
        printf("Please specify the destination file path\n");
        usage();
        exit(1);
    }

    src_fp = fopen(filepath, "r");
    if (src_fp == NULL) {
        perror("open");
        exit(1);
    }

    dst_fd = open(dstpath, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("open");
        exit(1);
    }
 
    unsigned long ts;
    unsigned long pid;
    char          pname[128];
    unsigned long lba;
    unsigned long blks;
    char          rw;
    int           major, minor;
    char          md5[256];
    
    while (fgets(line, LINE_SIZE, src_fp)) {
        memset(blk, 0, BLK_SIZE);
        memset(pname, 0, 128);
        memset(md5, 0, 256);
        
        /* Strip '\n' */
        line[strlen(line) - 1] = '\0';

        sscanf(line, "%lu %lu %s %lu %lu %c %d %d %s", &ts, &pid, pname, &lba, &blks, &rw, &major, &minor, md5);
        
        if (rw == 'W') {    
            fill_blk(blk, md5, strlen(md5));
            gettimeofday(&start, NULL);
            write(dst_fd, blk, BLK_SIZE); 
            gettimeofday(&end, NULL);
            time_usage += get_ms_diff(start, end);
            blks_replayed += 1;
        }
        else {
            gettimeofday(&start, NULL);
            read(dst_fd, blk, BLK_SIZE);
            gettimeofday(&end, NULL);
            time_usage += get_ms_diff(start, end);
        }
        size_in_total += BLK_SIZE;
    }

    close(dst_fd);
    fclose(src_fp);
    printf("Replay time: %.2f ms, Bandwidth: %.2f MiB/s\n", time_usage, (size_in_total / 1024 / 1024) / (time_usage / 1000));
    return 0;
}