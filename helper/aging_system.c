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
    printf("Aging the system written by deadpool\n");
    printf("Description: write a file in specific size\n");
    printf("             punch several holes in the file randomly\n");
    printf("             write another file to fill the holes\n");
    printf("-d dir  <dirname>\n");
    printf("-s size <GiB>\n");
    printf("-o hole <Percentage(e.g. 50, 60)>\n");
    printf("-p phase <1. Create 2. Punching 3. Re-Create 4. All>\n");
    printf("Example: ./aging_system -d /mnt/dir -s 50 -o 50 -p 4\n");
}

#define BLOCK_SIZE (2 * 1024 * 1024)
#define MAGIC_RAND 2333

void fill_buf(uint32_t buf[]) {
	int i;
    for (i = 0; i < BLOCK_SIZE / sizeof(uint32_t); i++) 
    {
		buf[i] = genrand_int32();
	}
}

double get_ms_diff(struct timeval tvBegin, struct timeval tvEnd)
{
    return 1000 * (tvEnd.tv_sec - tvBegin.tv_sec) + ((tvEnd.tv_usec - tvBegin.tv_usec) / 1000.0);
}

int main(int argc, char **argv)
{
    char *optstring = "d:s:o:p:h"; 
    int opt;
    unsigned long size;
    int hole_percent, phase;
    int fd;
    unsigned long hole_num, fill_num;
    unsigned long pos;
    unsigned long step = 0, total_blks = 0, total_4K_blks = 0;
    int dirlen = 0;
    char filepath[128] = {0};
    char buf[BLOCK_SIZE];
    char *records;
    struct timeval start, end;
    double diff; 

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch(opt) {
        case 'd':
            dirlen = strlen(optarg);
            if (dirlen > 1 && optarg[dirlen - 1] == '/') {
                dirlen -= 1; 
            }
            strcpy(filepath, optarg);
            break;
        case 's': 
            size = atol(optarg);
            break;
        case 'o': 
            hole_percent = atoi(optarg);
            break;
        case 'p':
            phase = atoi(optarg);
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

    total_blks = size * 1024 * 1024 * 1024 / BLOCK_SIZE;
    total_4K_blks = size * 1024 * 1024 * 1024 / 4096;

    init_genrand(MAGIC_RAND + phase);

    if (phase == 1 || phase == 4) {
        diff = 0;
        strcpy(filepath + dirlen, "/file1");
        fd = open(filepath, O_RDWR | O_CREAT);
        if (fd < 0) {
            printf("Create file %s error: %s\n", filepath, strerror(errno));
            exit(1);
        }

        printf("Phase 1: Filling file %s with %lu %ldK blocks...\n", filepath, total_blks, BLOCK_SIZE / 1024);
        for (step = 0; step < total_blks; step++)
        {
            fill_buf((uint32_t *)buf);
            gettimeofday(&start, NULL);
            write(fd, buf, BLOCK_SIZE);
            gettimeofday(&end, NULL);
            diff += get_ms_diff(start, end);
        }
        printf("done, time cost: %.6f ms\n", diff);
        printf("NewlyWriteBW: %.2f MiB per second\n", size * 1024 * 1000.0 / diff);
        printf("NewlyWriteLAT: %.6f ms\n", diff / total_blks);
        close(fd);
    }
    
    if (phase == 2 || phase == 4) {
        strcpy(filepath + dirlen, "/file1");
        fd = open(filepath, O_RDWR);
        if (fd < 0) {
            printf("Open file %s error: %s\n", filepath, strerror(errno));
            exit(1);
        }
        
        hole_num = total_4K_blks * hole_percent / 100;
        printf("Phase 2: Punching %lu 4K hole in %s randomly...\n", hole_num, filepath);
        records = (char *)calloc(total_4K_blks, sizeof(char));
        gettimeofday(&start, NULL);
        while (hole_num)
        {
            pos = (genrand_int32() % total_4K_blks);
            if (records[pos] == 0)
            {
                records[pos] = 1;
                if(fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, pos * 4096, 4096) < 0)
                {
                    perror("could not deallocate");
                }
                
                hole_num--;
            }
        }
        gettimeofday(&end, NULL);
        diff = get_ms_diff(start, end);
        printf("done, time cost: %.2f ms\n", diff);
        free(records);
        close(fd);
    }

    if (phase == 3 || phase == 4) {
        diff = 0;
        strcpy(filepath + dirlen, "/file2");
        fill_num = total_blks * hole_percent / 100;
        printf("Phase 3: Filling holes by %s with %lu %ldK blocks...\n", filepath, fill_num, BLOCK_SIZE / 1024);
        fd = open(filepath, O_RDWR | O_CREAT);
        if (fd < 0) {
            printf("Create file %s error: %s\n", filepath, strerror(errno));
            exit(1);
        }
        while (fill_num)
        {
            fill_buf((uint32_t *)buf);
            gettimeofday(&start, NULL);
            write(fd, buf, BLOCK_SIZE);
            gettimeofday(&end, NULL);
            diff += get_ms_diff(start, end);
            fill_num--;
        }    
        fill_num = total_blks * hole_percent / 100;
        printf("done, time cost: %.6f ms\n", diff);
        printf("AgingWriteBW: %.2f MiB per second\n", fill_num * BLOCK_SIZE * 1000.0 / 1024 / 1024 / (diff));
        printf("AgingWriteLAT: %.6f ms\n", diff / fill_num);
        close(fd);
    }

    return 0;
}
