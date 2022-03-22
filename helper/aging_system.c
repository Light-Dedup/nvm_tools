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

#define BLOCK_SIZE 4096
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
    int size, hole_percent, hole_num, phase;
    int fd;
    unsigned long pos;
    unsigned long step = 0, total = 0;
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

    total = size * 1024 * 1024 * 1024 / BLOCK_SIZE;
    
    if (phase == 1 || phase == 4) {
        diff = 0;
        strcpy(filepath + dirlen, "/file1");
        fd = open(filepath, O_RDWR | O_CREAT);
        if (fd < 0) {
            printf("Create file %s error: %s\n", filepath, strerror(errno));
            exit(1);
        }
        

        init_genrand(MAGIC_RAND);
        
        printf("Phase 1: Filling file %s with %d blocks...\n", filepath, total);
        for (step = 0; step < total; step++)
        {
            fill_buf((uint32_t *)buf);
            gettimeofday(&start, NULL);
            write(fd, buf, BLOCK_SIZE);
            gettimeofday(&end, NULL);
            diff += get_ms_diff(start, end);
        }
        printf("done, time cost: %.6f ms\n", diff);
        printf("NewlyWriteBW: %.2f MiB per second\n", size * 1024 * 1000.0 / diff);
        printf("NewlyWriteLAT: %.6f ms\n", diff / total);
        close(fd);
    }
    
    if (phase == 2 || phase == 4) {
        strcpy(filepath + dirlen, "/file1");
        fd = open(filepath, O_RDWR);
        if (fd < 0) {
            printf("Open file %s error: %s\n", filepath, strerror(errno));
            exit(1);
        }

        hole_num = total * hole_percent / 100;
        printf("Phase 2: Punch %d hole in the %s randomly...\n", hole_num, filepath);
        records = (char *)malloc(sizeof(char) * total);
        gettimeofday(&start, NULL);
        while (hole_num)
        {
            pos = (genrand_int32() % total);
            if (records[pos] == 0)
            {
                records[pos] = 1;
                if(fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, pos * BLOCK_SIZE, BLOCK_SIZE) < 0)
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
        hole_num = total * hole_percent / 100;
        printf("Phase 3: Filling holes by %s with %d blocks...\n", filepath, hole_num);
        fd = open(filepath, O_RDWR | O_CREAT);
        if (fd < 0) {
            printf("Create file %s error: %s\n", filepath, strerror(errno));
            exit(1);
        }
        while (hole_num)
        {
            fill_buf((uint32_t *)buf);
            gettimeofday(&start, NULL);
            write(fd, buf, BLOCK_SIZE);
            gettimeofday(&end, NULL);
            diff += get_ms_diff(start, end);
            hole_num--;
        }    
        hole_num = total * hole_percent / 100;
        printf("done, time cost: %.6f ms\n", diff);
        printf("AgingWriteBW: %.2f MiB per second\n", hole_num * BLOCK_SIZE * 1000.0 / 1024 / 1024 / (diff));
        printf("AgingWriteLAT: %.6f ms\n", diff / hole_num);
        close(fd);
    }

    return 0;
}
