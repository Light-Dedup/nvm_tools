#define _GNU_SOURCE
#include "stdio.h"
#include <unistd.h>
#include "stdlib.h"
#include "fcntl.h"
#include <stdint.h>
#include "string.h"
#include "mt19937ar.h"
#include "lcg.h"
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "pthread.h"

#ifndef PRIszt
// POSIX
#define PRIszt "zu"
// Windows is "Iu"
#endif

extern char *optarg;

void usage() 
{
	printf("Replay file parsed by blkparse written by deadpool\n");
	printf("Description: tools to help reply traces under different threads\n");
	printf("-f blkparse                     <.blkparse filename>\n");
	printf("-d dstpath                      <dst directory to replay>\n");
    printf("-o [a|w|rw|]                    <repley mode, default is write only>\n");
    printf("-g [null|mt19937ar|rand|lcg|]   <random generator, default is null>\n");
    printf("-t threads                      <threads #., default is 1>\n");
    printf("Example: ./replay -f homes-sample.blkparse -d /mnt/pmem0/ -o rw -g null -t 1\n");
}

#define BLK_SIZE 4096
#define LINE_SIZE 4096

void mt19937ar_seed_wrapper(void *ctx, unsigned int s) {
    init_genrand_r(ctx, s);
}

int32_t mt19937ar_gen_wrapper(void *ctx) {
    return genrand_int32_r(ctx);
}

void stdlib_seed_wrapper(void *ctx, unsigned int s) {
    return;
}

int32_t stdlib_gen_wrapper(void *ctx) {
    return rand_r(ctx);
}

void lcg_seed_wrapper(void *ctx, unsigned int s) {
    return;
}

int32_t lcg_gen_wrapper(void *ctx) {
    return lcg_rand_r(*(int *)ctx);
}

typedef void (*randseed_set_func)(void *ctx, unsigned int); 
typedef int32_t (*randint32_gen_func)(void *ctx);

typedef struct 
{
    void *ctx;
    randseed_set_func fedseed;
    randint32_gen_func genrandom;
} rand_gener_t;

static __always_inline void 
fill_32bytes_rand(uint32_t *buf, char *md5, void *ctx, randseed_set_func fedseed, randint32_gen_func genrandom)
{
    if (genrandom == NULL) {
        memcpy((char *)buf, md5, 32);
    }
    else {
        int i;
        int seed = *(uint32_t *)md5;
        fedseed(ctx, seed);
        for (i = 0; i < 8; i++) {
            buf[i] = (uint32_t)genrandom(ctx);
        }
    }
}

static inline void fill_blk(char *blk, char *md5, int n, rand_gener_t *rand_gener)
{
	int step = n / 32;
    int i = 0, j;
    int blk_size_per_step = BLK_SIZE / step;
    int md5_per_step = blk_size_per_step / 32;
    char *cur_md5 = md5;
    char *cur_blk = blk;
    randseed_set_func fedseed = rand_gener->fedseed;
    randint32_gen_func genrandom = rand_gener->genrandom;
    void *ctx = rand_gener->ctx;

    for (i = 0; i < step; i++) {
        j = 0;
        for (j = 0; j < md5_per_step; j++) {
            // memcpy(cur_blk + j * 32, cur_md5, 32);
            fill_32bytes_rand((uint32_t *)(cur_blk + j * 32), cur_md5, ctx, fedseed, genrandom);
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

#define REPLAY_WRITEONLY 1
#define REPLAY_READWRITE 2
#define REPLAY_APPEND    3

#define RANDOM_NULL      0
#define RANDOM_MT19937AR 1
#define RANDOM_STDLIB    2
#define RANDOM_LCG       3

struct trace_info {
    unsigned long ts;
    unsigned long pid;
    unsigned long lba;
    unsigned long ofs;
    unsigned long blks;
    char          rw;
    int           major;
    int           minor;
    char          md5[256];
};

typedef struct {
    struct trace_info *infos;
    unsigned long start;
    unsigned long end;
    char dstfilepath[128];
    int worker_id;
    rand_gener_t *rand_gener;
    int mode;
} replay_param_t; 

void *replay_worker(void *arg) {
    replay_param_t *param = (replay_param_t *)arg;
    int dst_fd;
    char *dstpath = param->dstfilepath;
    unsigned long i;
    int mode = param->mode;
    unsigned long start = param->start;
    unsigned long end = param->end;
    struct trace_info *infos = param->infos;
    struct trace_info *info;
    rand_gener_t *rand_gener = param->rand_gener;
    char blk[BLK_SIZE];

    if (access(dstpath, F_OK) == 0) {
        if (mode == REPLAY_APPEND) {
            dst_fd = open(dstpath, O_RDWR | O_APPEND);
        } else {
            dst_fd = open(dstpath, O_RDWR);
        }
    }
    else {
        if (mode == REPLAY_APPEND) {
            dst_fd = open(dstpath, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0644);
        } else {
            dst_fd = open(dstpath, O_RDWR | O_CREAT | O_TRUNC, 0644);
        }
    }

    if (dst_fd < 0) {
        perror("open");
        exit(1);
    }

    if (mode == REPLAY_WRITEONLY) {
        for (i = start; i < end; i++) {
            info = &infos[i];    
            fill_blk(blk, info->md5, strlen(info->md5), rand_gener);
            pwrite(dst_fd, blk, BLK_SIZE, info->ofs);
        }
    }
    else if (mode == REPLAY_APPEND) {
        for (i = start; i < end; i++) {
            info = &infos[i];    
            fill_blk(blk, info->md5, strlen(info->md5), rand_gener);
            write(dst_fd, blk, BLK_SIZE);
        }
    }
    else if (mode == REPLAY_READWRITE) {
        for (i = start; i < end; i++) {
            info = &infos[i];    
            if (info->rw == 'W') {
                fill_blk(blk, info->md5, strlen(info->md5), rand_gener);
                pwrite(dst_fd, blk, BLK_SIZE, info->ofs);
            }
            else {
                pread(dst_fd, blk, BLK_SIZE, info->ofs);
            }
        }   
    }
    
    close(dst_fd);

    if (param) {
        if (param->rand_gener) {
            if (param->rand_gener->ctx) {
                free(param->rand_gener->ctx);
            }
            free(param->rand_gener);
        }
        free(param);
    }
}

int main(int argc, char **argv)
{
	char *optstring = "f:d:o:g:t:h"; 
	int opt;
    FILE *src_fp;
	char filepath[128] = {0};
	char dstpath[128] = {0};
    char mode_str[128] = {0};
    char rand_gener_str[128] = {0};
    int rand_gener_type = 0;
    int mode = REPLAY_WRITEONLY;
    char line[LINE_SIZE];
	uint64_t start, end;
    unsigned long size_in_total = 0;
    unsigned long blks_start;
    unsigned long blks_end;
    uint64_t time_usage = 0;
    int threads = 1;

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch(opt) {
		case 'f':
			strcpy(filepath, optarg);
			break;
		case 'd': 
			strcpy(dstpath, optarg);
            if (dstpath[strlen(dstpath) - 1] != '/') {
                strcat(dstpath, "/");
            }
			break;
        case 'o':
            strcpy(mode_str, optarg);
            if (strcmp(mode_str, "w") == 0) {
                mode = REPLAY_WRITEONLY;
            } else if (strcmp(mode_str, "rw") == 0) {
                mode = REPLAY_READWRITE;
            } else if (strcmp(mode_str, "a") == 0) {
                mode = REPLAY_APPEND;
            } else {
                usage();
                return -1;
            }
            break;
        case 'g':
            strcpy(rand_gener_str, optarg);
            if (strcmp(rand_gener_str, "mt19937ar") == 0) {
                rand_gener_type = RANDOM_MT19937AR;
            } else if (strcmp(rand_gener_str, "rand") == 0) {
                rand_gener_type = RANDOM_STDLIB;
            } else if (strcmp(rand_gener_str, "lcg") == 0) {
                rand_gener_type = RANDOM_LCG;
            } else if (strcmp(rand_gener_str, "null") == 0) {
                rand_gener_type = RANDOM_NULL;
            } else {
                usage();
                return -1;
            }
            break;
        case 't':
            threads = atoi(optarg);
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
    
    char pname[128];

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
            if (mode == REPLAY_READWRITE) {
                info->ofs = info->lba << 9;
                valid_lines++;
                i++;
            }
            else if (mode == REPLAY_WRITEONLY || mode == REPLAY_APPEND) {
                if (info->rw == 'W') {
                    info->ofs = info->lba << 9;
                    valid_lines++;
                    i++;
                }
            }
        }
    }

    pthread_t *tids = (pthread_t *)malloc(threads * sizeof(pthread_t));
    start = timestamp_ns();
    for (i = 0; i < threads; i++) {
        replay_param_t *param = (replay_param_t *)malloc(sizeof(replay_param_t));
        param->start = i * valid_lines / threads;
        param->end = (i + 1) * valid_lines / threads;
        param->infos = infos;
        memcpy(param->dstfilepath, dstpath, strlen(dstpath));
        sprintf(param->dstfilepath + strlen(dstpath), "trace_%d", i);
        rand_gener_t *rand_gener = (rand_gener_t *)malloc(sizeof(rand_gener_t));
        param->mode = mode;
        switch (rand_gener_type)
        {
        case RANDOM_MT19937AR: {
            struct mt19937ar_state *ctx = (struct mt19937ar_state *)malloc(sizeof(struct mt19937ar_state));
            rand_gener->ctx = ctx;
            rand_gener->fedseed = mt19937ar_seed_wrapper;
            rand_gener->genrandom = mt19937ar_gen_wrapper;
            break;
        }
        case RANDOM_STDLIB: {
            int *ctx = (int *)malloc(sizeof(int)); 
            rand_gener->ctx = ctx;
            rand_gener->fedseed = stdlib_seed_wrapper;
            rand_gener->genrandom = stdlib_gen_wrapper;
            break;
        }
        case RANDOM_LCG: {
            int *ctx = (int *)malloc(sizeof(int)); 
            rand_gener->ctx = ctx;
            rand_gener->fedseed = lcg_seed_wrapper;
            rand_gener->genrandom = lcg_gen_wrapper;
            break;
        }
        default:
            break;
        }
        param->rand_gener = rand_gener; 

        pthread_create(&tids[i], NULL, replay_worker, param);
    }

    /* wait workers */
    for (i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }

    end = timestamp_ns();
    
    size_in_total = valid_lines * BLK_SIZE;
    time_usage = get_ns_diff(start, end);
        
    
    free(infos);
    fclose(src_fp);
    
    // printf("Replay time: %.2f ms, Size: %ld MiB, Bandwidth: %.2f MiB/s, \
    //         Write time: %.2f ms, Write Size: %ld MiB, Write Bandwidth: %.2f MiB/s, \
    //         Read time: %.2f ms, Read Size: %ld MiB, Read Bandwidth: %.2f MiB/s\n", 
    //         time_usage / 1000.0 / 1000, size_in_total / 1024 / 1024, (size_in_total / 1024 / 1024) / (time_usage / 1000.0 / 1000 / 1000),
    //         write_time_usage / 1000.0 / 1000, write_size_in_total / 1024 / 1024, (write_size_in_total / 1024 / 1024) / (write_time_usage / 1000.0 / 1000 / 1000),
    //         read_time_usage / 1000.0 / 1000, read_size_in_total / 1024 / 1024, (read_size_in_total / 1024 / 1024) / (read_time_usage / 1000.0 / 1000 / 1000)
    //         );
    printf("Replay time: %.2f ms, Size: %ld MiB, Bandwidth: %.2f MiB/s\n", 
            time_usage / 1000.0 / 1000, size_in_total / 1024 / 1024, (size_in_total / 1024 / 1024) / (time_usage / 1000.0 / 1000 / 1000));
    return 0;
}