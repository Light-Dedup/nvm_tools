#define _GNU_SOURCE
#include "stdio.h"
#include <unistd.h>
#include "stdlib.h"
#include "fcntl.h"
#include <stdint.h>
#include "string.h"
#include "mt19937ar.h"
#include <errno.h>
#include <time.h>
#include <assert.h>

#ifndef PRIszt
// POSIX
#define PRIszt "zu"
// Windows is "Iu"
#endif

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

static inline uint64_t timespec_to_ns(const struct timespec *t) {
	return (uint64_t)t->tv_sec * 1000000000 + t->tv_nsec;
}

static inline uint64_t get_ns_diff(uint64_t start, uint64_t end) {
    return end - start;
}

static inline uint64_t timestamp_ns() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return timespec_to_ns(&t);
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
	uint64_t start, end;
    unsigned long size_in_total = 0;
    unsigned long write_size_in_total = 0;
    unsigned long read_size_in_total = 0;
    unsigned long blks_start;
    unsigned long blks_end;
    unsigned long blks_replayed = 0; 
    uint64_t time_usage = 0;
    uint64_t write_time_usage = 0;
    uint64_t read_time_usage = 0;
    
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
    
    char pname[128];

    struct trace_info {
        unsigned long ts;
        unsigned long pid;
        unsigned long lba;
        unsigned long blks;
        char          rw;
        int           major;
        int           minor;
        char          md5[256];
    };

    unsigned long lines = 0;
    unsigned long valid_lines = 0;
    unsigned long i = 0;
    struct trace_info *infos;
    struct trace_info *info;

    while (fgets(line, LINE_SIZE, src_fp)) {
        lines++;
    }

    infos = (struct trace_info *)malloc(sizeof(struct trace_info) * lines);
    if (infos == NULL) {
        perror("malloc");
        exit(1);
    }
    
    fseek(src_fp, 0, SEEK_SET);
    
    while (fgets(line, LINE_SIZE, src_fp)) {
        info = &infos[i];
        /* Strip '\n' */
        line[strlen(line) - 1] = '\0';
        if (sscanf(line, "%lu %lu %s %lu %lu %c %d %d %s", &info->ts, &info->pid, pname, &info->lba, &info->blks, &info->rw, &info->major, &info->minor, info->md5) == 9) {
            valid_lines++;
            i++;
        }
    }
    
    for (i = 0; i < valid_lines; i++) {
        info = &infos[i];
        if (info->rw == 'W') {    
            start = timestamp_ns();
            fill_blk(blk, info->md5, strlen(info->md5));
            write(dst_fd, blk, BLK_SIZE);
            end = timestamp_ns();
            write_time_usage += get_ns_diff(start, end);
            write_size_in_total += BLK_SIZE;
            size_in_total += BLK_SIZE;
        }
        // else {
        //     start = timestamp_ns();
        //     read(dst_fd, blk, BLK_SIZE);
        //     end = timestamp_ns();
        //     read_time_usage += get_ns_diff(start, end);
        //     read_size_in_total += BLK_SIZE;
        // }
        // size_in_total += BLK_SIZE;
    }
        
    time_usage = write_time_usage + read_time_usage;
    
    free(infos);
    close(dst_fd);
    fclose(src_fp);
    
    printf("Replay time: %.2f ms, Size: %ld MiB, Bandwidth: %.2f MiB/s, \
            Write time: %.2f ms, Write Size: %ld MiB, Write Bandwidth: %.2f MiB/s, \
            Read time: %.2f ms, Read Size: %ld MiB, Read Bandwidth: %.2f MiB/s\n", 
            time_usage / 1000.0 / 1000, size_in_total / 1024 / 1024, (size_in_total / 1024 / 1024) / (time_usage / 1000.0 / 1000 / 1000),
            write_time_usage / 1000.0 / 1000, write_size_in_total / 1024 / 1024, (write_size_in_total / 1024 / 1024) / (write_time_usage / 1000.0 / 1000 / 1000),
            read_time_usage / 1000.0 / 1000, read_size_in_total / 1024 / 1024, (read_size_in_total / 1024 / 1024) / (read_time_usage / 1000.0 / 1000 / 1000)
            );
    return 0;
}