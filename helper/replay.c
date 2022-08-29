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
#include "map.h"

#ifndef PRIszt
// POSIX
#define PRIszt "zu"
// Windows is "Iu"
#endif

#define REPLAY_WRITEONLY 1
#define REPLAY_READWRITE 2
#define REPLAY_APPEND    3

#define RANDOM_NULL      0
#define RANDOM_MT19937AR 1
#define RANDOM_STDLIB    2
#define RANDOM_LCG       3

extern char *optarg;
char dstpath[128] = {0};
int rand_gener_type = 0;
int mode = REPLAY_WRITEONLY;
unsigned long max_continuous_4K_blks = 1;  // 4KiB as default
int threads = 1;
int verbose = 0;

void usage() 
{
	printf("Replay file parsed by blkparse written by deadpool\n");
	printf("Description: tools to help reply traces under different threads\n");
	printf("-f blkparse                     <.blkparse filename>\n");
	printf("-d dstpath                      <dst directory to replay>\n");
    printf("-o [a|w|rw|]                    <repley mode, default is write only>\n");
    printf("-g [null|mt19937ar|rand|lcg|]   <random generator, default is null>\n");
    printf("-t threads                      <threads #., default is 1>\n");
    printf("-c blks                         <largest continuous blks, default is 1>\n");
    printf("-v                              <enable verbose?>\n");
    printf("Example: ./replay -f homes-sample.blkparse -d /mnt/pmem0/ -o rw -g null -t 1 -c 1\n");
}
#define DEBUG_INFO(verbose, code)        if (verbose) { code; }
#define BLK_SIZE 4096
#define BLK_SHIFT 12
#define LINE_SIZE 4096

void mt19937ar_seed_wrapper(void *ctx, unsigned int s) {
    init_genrand_r(ctx, s);
}

int32_t mt19937ar_gen_wrapper(void *ctx) {
    return genrand_int32_r(ctx);
}

void stdlib_seed_wrapper(void *ctx, unsigned int s) {
    *(int *)ctx = s;
    return;
}

int32_t stdlib_gen_wrapper(void *ctx) {
    return *(int *)ctx = rand_r(ctx);
}

void lcg_seed_wrapper(void *ctx, unsigned int s) {
    *(int *)ctx = s;
    return;
}

int32_t lcg_gen_wrapper(void *ctx) {
    return *(int *)ctx = lcg_rand_r(*(int *)ctx);
}

typedef void (*randseed_set_func)(void *ctx, unsigned int); 
typedef int32_t (*randint32_gen_func)(void *ctx);

typedef struct 
{
    void *ctx;
    randseed_set_func fedseed;
    randint32_gen_func genrandom;
} rand_gener_t;

static inline void fill_blk(char *blk, char *md5, int md5_len, rand_gener_t *rand_gener)
{
	int step = md5_len / 32;
    int blk_size_per_step = BLK_SIZE / step;
    randseed_set_func fedseed = rand_gener->fedseed;
    randint32_gen_func genrandom = rand_gener->genrandom;
    void *ctx = rand_gener->ctx;

    for (int i = 0; i < step; i++, blk += blk_size_per_step, md5 += 32) {
        if (genrandom == NULL) {
            for (int j = 0; j < blk_size_per_step; j += 32) {
                memcpy(blk + j, md5, 32);
            }
        } else {
            int seed = *(uint32_t *)md5;
            fedseed(ctx, seed);
            for (int j = 0; j < blk_size_per_step; j += sizeof(uint32_t)) {
                *(uint32_t *)(blk + j) = (uint32_t)genrandom(ctx);
            }
        }
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

/* for each trace record */
struct trace_info {         
    unsigned long ts;
    unsigned long pid;
    unsigned long lba;
    unsigned long ofs;
    unsigned long blks;
    char          rw;
    int           major;
    int           minor;
    char          md5[257];
};

struct trace_replay_hint {
    unsigned long continuous_blks; 
    unsigned long start_trace_line;         /* for index trace infos */
    char          rw;                       /* mode */
};

/* a collection of trace info */
struct trace_container {
    struct trace_info  **infos_map;
    void               *meta;
    int                size;
    int                capacity;
};

#define DEFAULT_COLLECTIONS_CAPACITY 64
struct trace_container *trace_container_create(int capacity, void *meta) {
    struct trace_container *tc = malloc(sizeof(struct trace_container));
    tc->infos_map = malloc(sizeof(struct trace_info *) * capacity);
    tc->size = 0;
    tc->capacity = capacity;
    tc->meta = meta;
    return tc;
}

void trace_container_destroy(struct trace_container *tc) {
    if (tc) {
        free(tc->infos_map);
        free(tc);
    }
}

void trace_container_add(struct trace_container *tc, struct trace_info *info) {
    if (tc->size == tc->capacity) {
        tc->capacity *= 2;
        tc->infos_map = realloc(tc->infos_map, sizeof(struct trace_info *) * tc->capacity);
        if (!tc->infos_map) {
            printf("realloc failed\n");
            exit(1);
        }
    }
    tc->infos_map[tc->size++] = info;
}

void trace_container_add_collection(struct trace_container *tc, struct trace_container *tc2) {
    for (int i = 0; i < tc2->size; i++) {
        trace_container_add(tc, tc2->infos_map[i]);
    }
}


unsigned long _partition_by_ts(struct trace_info **infos_map, unsigned long low, unsigned long high)
{
    unsigned long low_index = low;
    struct trace_info *tmp;
    struct trace_info *pivot;

    pivot = infos_map[low];

    for (unsigned long i = low + 1; i < high; i++)
    {
        if (infos_map[i]->ts < pivot->ts)
        {
            low_index++;
            if (i != low_index)
            {
                tmp = infos_map[i];
                infos_map[i] = infos_map[low_index];
                infos_map[low_index] = tmp;
            }
        }
    }
    infos_map[low] = infos_map[low_index];
    infos_map[low_index] = pivot;
    return low_index;
}

void _trace_infomap_qsort_by_ts(struct trace_info **infos_map, unsigned long low, unsigned long high) {
    if (low < high)
    {
        unsigned long pivot = _partition_by_ts(infos_map, low, high);
        if (pivot > 0)
            _trace_infomap_qsort_by_ts(infos_map, low, pivot);
        _trace_infomap_qsort_by_ts(infos_map, pivot + 1, high);
    }
}

void trace_container_sort_infomap_by_ts(struct trace_container *tc) {
    _trace_infomap_qsort_by_ts(tc->infos_map, 0, tc->size);
}

void trace_container_print_info(struct trace_container *tc) {
    printf("ts\tpid\tlba\tofs\tblks\trw\n");
    for (int i = 0; i < tc->size; i++) {
        struct trace_info *info = tc->infos_map[i];
        printf("%lu\t%lu\t%lu\t%lu\t%lu\t%c \n", info->ts, info->pid, info->lba, info->ofs, info->blks, info->rw);
    }
}

typedef struct {
    struct trace_container *tc;
    struct trace_replay_hint *hints;
    unsigned long hints_start;
    unsigned long hints_end;
    char *buf;
    char dstfilepath[128];
    int worker_id;
    rand_gener_t *rand_gener;
    int mode;
} replay_param_t; 

void *replay_worker(void *arg) {
    replay_param_t *param = (replay_param_t *)arg;
    int dst_fd;
    char *dstpath = param->dstfilepath;
    unsigned long i, j;
    int mode = param->mode;
    struct trace_info **infos_map = param->tc->infos_map;
    struct trace_info *info;
    struct trace_replay_hint *hints = param->hints;
    struct trace_replay_hint *hint;
    unsigned long hints_start = param->hints_start;
    unsigned long hints_end = param->hints_end;
    char *blk = param->buf;
    char *p = blk;
    unsigned long blk_size = 0;
    rand_gener_t *rand_gener = param->rand_gener;

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
        for (i = hints_start; i < hints_end; i++) {
            hint = &hints[i];
            blk_size = hint->continuous_blks << BLK_SHIFT;
            p = blk;
            unsigned long end_trace_line =
                hint->start_trace_line + hint->continuous_blks;
            for (j = hint->start_trace_line; j < end_trace_line; j++) {
                info = infos_map[j];
                fill_blk(p, info->md5, strlen(info->md5), rand_gener);
                p += BLK_SIZE;
            }
            pwrite(dst_fd, blk, blk_size, infos_map[hint->start_trace_line]->ofs);
        }
    }
    else if (mode == REPLAY_APPEND) {
        for (i = hints_start; i < hints_end; i++) {
            hint = &hints[i];
            blk_size = hint->continuous_blks << BLK_SHIFT;
            p = blk;
            unsigned long end_trace_line =
                hint->start_trace_line + hint->continuous_blks;
            for (j = hint->start_trace_line; j < end_trace_line; j++) {
                info = infos_map[j];    
                fill_blk(p, info->md5, strlen(info->md5), rand_gener);
                p += BLK_SIZE;
            }
            write(dst_fd, blk, blk_size);
        }
    }
    else if (mode == REPLAY_READWRITE) {
        for (i = hints_start; i < hints_end; i++) {
            hint = &hints[i];
            blk_size = hint->continuous_blks << BLK_SHIFT;
            unsigned long end_trace_line =
                hint->start_trace_line + hint->continuous_blks;
            if (hint->rw == 'W') {
                p = blk;
                for (j = hint->start_trace_line; j < end_trace_line; j++) {
                    info = infos_map[j];    
                    fill_blk(p, info->md5, strlen(info->md5), rand_gener);
                    p += BLK_SIZE;
                }
                pwrite(dst_fd, blk, blk_size, infos_map[hint->start_trace_line]->ofs);
            }
            else {
                pread(dst_fd, blk, blk_size, infos_map[hint->start_trace_line]->ofs);
            }
        }
    }
    
    close(dst_fd);
}

void check_param_per_worker(replay_param_t *param, int thread) {
    unsigned long i;
    unsigned long hints_start = param->hints_start;
    unsigned long hints_end = param->hints_end;
    struct trace_replay_hint *hints = param->hints;

    for (i = hints_start; i < hints_end; i++) {
        if (hints[i].continuous_blks == 0) {
            printf("error: continuous_blks is 0\n");
            exit(1);
        }
        printf("hint[%lu] for container(worker) %d: %d traces, continuous_blks=%lu, start_trace_line_within_container=%lu\n", i, thread, param->tc->size, hints[i].continuous_blks, hints[i].start_trace_line);
    }
}

void build_trace_containers(hashmap *map, struct trace_info *infos, unsigned long valid_lines) {
    unsigned long i;
    struct trace_info *info;
    struct trace_container *ptc;
    bool not_null;

    for (i = 0; i < valid_lines; i++) {
        info = &infos[i];
        not_null = hashmap_get(map, &info->lba, sizeof(unsigned long *), (uintptr_t *)&ptc);
        if (!not_null) {
            ptc = trace_container_create(DEFAULT_COLLECTIONS_CAPACITY, &info->lba);
            hashmap_set(map, &info->lba, sizeof(unsigned long *), (uintptr_t)ptc);
        }
        trace_container_add(ptc, info);
    }
}

int trace_container_cmp(const void *a, const void *b) {
    unsigned long lba_a = *((unsigned long *)a);
    unsigned long lba_b = *((unsigned long *)b);
    return lba_a - lba_b; 
}

void assign_params_per_worker(hashmap *map, unsigned long valid_lines, 
                              struct trace_replay_hint *hints,
                              replay_param_t *params) {

    replay_param_t *param;
    unsigned long start_line = 0;
    unsigned long per_thread_lines = valid_lines / threads;
    int cur_thread = 0;
    unsigned long cur_thread_lines = 0; 
    int probability_assign_current = 0;
    struct trace_container *tc;
    unsigned long i, j;
    struct trace_replay_hint *hint;
    unsigned long hints_idx = 0;
    unsigned long hints_start = 0;
    struct bucket *b;

    for (i = 0; i < threads; i++) {
        param = &params[i];
        param->tc = trace_container_create(DEFAULT_COLLECTIONS_CAPACITY, &param->worker_id);
    }

    hashmap_sort(map, trace_container_cmp);
    foreach_hashmap_bucket(map, b) {
        struct trace_container *_tc = (struct trace_container *)b->value;
        if (cur_thread != threads - 1) {
            if (_tc->size + cur_thread_lines > per_thread_lines) {
                probability_assign_current = (per_thread_lines - cur_thread_lines) * 100 / _tc->size;
                if (rand() % 100 < probability_assign_current) {
                    cur_thread++;
                    cur_thread_lines = 0;
                }
            }
        }

        tc = params[cur_thread].tc;
        trace_container_add_collection(tc, _tc);
        cur_thread_lines += _tc->size;
        trace_container_destroy(_tc);
        
        if (cur_thread_lines >= per_thread_lines) {
            cur_thread++;
            cur_thread_lines = 0;
        }
    }

    
    for (int i = 0; i < threads; i++) {
        tc = params[i].tc;
        trace_container_sort_infomap_by_ts(tc);
        unsigned long _max_continuous_4K_blks = 0;
        unsigned long per_thread_start = 0;
        unsigned long per_thread_end = tc->size;
        unsigned long consecutive_start = per_thread_start;
        hints_start = hints_idx;

        /* extract consecutive  blks per thread*/
        for (j = per_thread_start + 1; j < per_thread_end; j++) {
            struct trace_info *info = tc->infos_map[j];
            struct trace_info *pre = tc->infos_map[j - 1];
            if (info->lba - pre->lba == 8 && 
                info->rw == pre->rw &&
                (j - consecutive_start) < max_continuous_4K_blks) {
                continue;
            } else {
                /* save hints */
                hint = &hints[hints_idx++];
                hint->continuous_blks = j - consecutive_start;
                hint->start_trace_line = consecutive_start;
                hint->rw = tc->infos_map[per_thread_start]->rw;
                if (hint->continuous_blks > _max_continuous_4K_blks) {
                    _max_continuous_4K_blks = hint->continuous_blks;
                }
                /* reset state */
                consecutive_start = j;
            }
        }
        hint = &hints[hints_idx++];
        hint->continuous_blks = per_thread_end - consecutive_start;
        hint->start_trace_line = consecutive_start;
        hint->rw = tc->infos_map[per_thread_start]->rw;
        if (hint->continuous_blks > _max_continuous_4K_blks) {
            _max_continuous_4K_blks = hint->continuous_blks;
        }
        /* assign param */
        param = &params[i];
        param->hints = hints;
        param->hints_start = hints_start;
        param->hints_end = hints_idx;
        param->buf = (char *)malloc(_max_continuous_4K_blks * BLK_SIZE);
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
        case RANDOM_NULL: {
            rand_gener->ctx = NULL;
            rand_gener->fedseed = NULL;
            rand_gener->genrandom = NULL;
            break;
        }
        default:
            break;
        }
        param->rand_gener = rand_gener; 
        DEBUG_INFO(verbose, check_param_per_worker(param, i));
    }
}

unsigned long parse_trace_info(FILE *src_fp, struct trace_info **infos, int mode) {
    char line[LINE_SIZE];
    char pname[128];
    unsigned long valid_lines = 0;
    unsigned long i = 0;
    unsigned long lines = 0;
    
    while (fgets(line, LINE_SIZE, src_fp)) {
        lines++;
    }

    *infos = (struct trace_info *)malloc(sizeof(struct trace_info) * lines);
    if (*infos == NULL) {
        perror("malloc");
        exit(1);
    }
    
    fseek(src_fp, 0, SEEK_SET);
    
    while (fgets(line, LINE_SIZE, src_fp)) {
        struct trace_info *info = &(*infos)[i];
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
    return valid_lines;
}

int main(int argc, char **argv)
{
	char *optstring = "f:d:o:g:t:c:vh"; 
	int opt;
    FILE *src_fp;
	char filepath[128] = {0};
    char mode_str[128] = {0};
    char rand_gener_str[128] = {0};
	uint64_t start, end;
    unsigned long size_in_total = 0;
    unsigned long blks_start;
    unsigned long blks_end;
    uint64_t time_usage = 0;

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
        case 'c':
            max_continuous_4K_blks = atoi(optarg);
            break;
        case 't':
            threads = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
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

    int i;
    unsigned long valid_lines = 0;
    struct trace_info *infos;
    
    valid_lines = parse_trace_info(src_fp, &infos, mode);
    
    hashmap *map = hashmap_create();
    replay_param_t *params = (replay_param_t *)malloc(sizeof(replay_param_t) * threads);
    replay_param_t *param;
    struct trace_replay_hint *hints = malloc(sizeof(struct trace_replay_hint) * valid_lines);

    build_trace_containers(map, infos, valid_lines);
    assign_params_per_worker(map, valid_lines, hints, params);

    pthread_t *tids = (pthread_t *)malloc(threads * sizeof(pthread_t));
    start = timestamp_ns();
    for (i = 0; i < threads; i++) {
        param = &params[i];
        pthread_create(&tids[i], NULL, replay_worker, param);
    }

    /* wait workers */
    for (i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }
    end = timestamp_ns();
    
    size_in_total = valid_lines * BLK_SIZE;
    time_usage = get_ns_diff(start, end);
        
end:
    free(tids);
    free(infos);
    free(hints);
    for (i = 0; i < threads; i++) {
        param = &params[i];
        if (param->rand_gener) {
            if (param->rand_gener->ctx) {
                free(param->rand_gener->ctx);
            }
            free(param->rand_gener);
            free(param->buf);
        }
        trace_container_destroy(param->tc);
    }
    hashmap_free(map);
    free(params);
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