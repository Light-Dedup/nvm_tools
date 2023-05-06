#include "fcntl.h"
#include "mt19937ar.h"
#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"
#include <algorithm>
#include <random>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

extern char *optarg;

#define PREDEFINED_RATIO    (1.0 / 3.0)
#define DIVIDER 			   "==========\n"
#define MO_READ_LAT			1
#define MO_WRITE_LAT		2

typedef struct rw_interference_task {
    unsigned long start_magic;
    unsigned long num_blocks;
	unsigned long progress;
    unsigned int id;
    bool dup;
} rw_interf_task_t;

void usage()
{
    printf("Read-Write Interference Evaluation (for Light-Dedup) written by deadpool\n");
    printf("-d dir  <target dir name>\n");
    printf("-g deduplication granularity <B>\n");
    printf("-r reader number for deduplication threads <0+>\n");
    printf("-w writer number for non-deduplication threads <0+>\n");
    printf("-s stage <0 for All, 1 for Prepare, 2 for Dedup/Write>\n");
    printf("-a total I/O <GiB>\n");
    printf("-m measurement object <1 for Read Latency, 2 for Write Latency>\n");
    printf("Example: ./rw_interference -d /mnt/pmem/ -g 4096 -r 0 -w 8 -s 0 -a 1 -m 1\n");
}

int make_tasks(std::vector<rw_interf_task_t *> &task_list, unsigned int reader_num, unsigned int writer_num, unsigned long total_num_blocks, int measurement_object)
{
    unsigned long start_magic = 0;
    unsigned int num_tasks = reader_num + writer_num;
	unsigned long total_reader_num_blocks;
	unsigned long total_writer_num_blocks;
	unsigned long reader_num_blocks;
	unsigned long writer_num_blocks;

	if (num_tasks == 0) {
		printf("reader_num and writer_num cannot be both 0\n");
		return -1;
	} else if (reader_num == 0) {
		total_reader_num_blocks = 0;
		total_writer_num_blocks = total_num_blocks;
	} else if (writer_num == 0) {
		total_reader_num_blocks = total_num_blocks;
		total_writer_num_blocks = 0;
	} else {
		if (measurement_object == MO_READ_LAT) {  /* read */
			/* make sure reader is finished but write not */
			total_reader_num_blocks = total_num_blocks * PREDEFINED_RATIO;
			total_writer_num_blocks = total_num_blocks - total_reader_num_blocks;
		} else {						/* write */
			/* make sure write is finished but reader not */
			total_writer_num_blocks = total_num_blocks * PREDEFINED_RATIO;
			total_reader_num_blocks = total_num_blocks - total_writer_num_blocks;
		}
	}

    reader_num_blocks = reader_num == 0 ? 0 : total_reader_num_blocks / reader_num;
    writer_num_blocks = writer_num == 0 ? 0 : total_writer_num_blocks / writer_num;

    printf("num blocks for each reader: %lu\n", reader_num_blocks);
    printf("num blocks for each writer: %lu\n", writer_num_blocks);

    /* assign read tasks */
    for (unsigned int i = 0; i < reader_num; i++) {
        rw_interf_task_t *task = (rw_interf_task_t *)malloc(sizeof(rw_interf_task_t));
        if (!task) {
            perror("malloc");
            return -1;
        }
        task->start_magic = start_magic;
        task->num_blocks = reader_num_blocks;
        task->dup = true;
        task->id = i;
		task->progress = 0;
        task_list.push_back(task);
        start_magic += reader_num_blocks;
    }

    start_magic = total_num_blocks;
    /* assign write tasks */
    for (unsigned int i = 0; i < writer_num; i++) {
        rw_interf_task_t *task = (rw_interf_task_t *)malloc(sizeof(rw_interf_task_t));
        if (!task) {
            perror("malloc");
            return -1;
        }
        task->start_magic = start_magic;
        task->num_blocks = writer_num_blocks;
        task->dup = false;
        task->id = i + reader_num;
		task->progress = 0;
        task_list.push_back(task);
        start_magic += writer_num_blocks;
    }

    /* dump task */
    for (unsigned int i = 0; i < num_tasks; i++) {
        printf("task %d: start_magic %lu, num_blocks %lu, dup %s\n", i, task_list[i]->start_magic, task_list[i]->num_blocks, task_list[i]->dup ? "true" : "false");
    }

    return 0;
}

int make_file(char *path, unsigned long num_blocks, unsigned long start_magic, unsigned long granularity, bool dup, unsigned long *progress)
{
    int fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    unsigned long *buf = (unsigned long *)malloc(granularity);
    if (!buf) {
        perror("malloc");
        return -1;
    }

    unsigned long block;
    ssize_t ret;
    for (block = 0; block < num_blocks; block++) {
        /* fill buffer */
		for (unsigned long i = 0; i < granularity / sizeof(unsigned long); i++) {
			buf[i] = start_magic + block;
		}
        
		ret = write(fd, buf, granularity);
        
		if (ret < 0) {
            perror("write");
            return -1;
        }

		if (progress) {
			*progress = block + 1;
		}
    }

    free(buf);
    close(fd);

    printf("make file %s (%s) done\n", path, dup ? "dup" : "non-dup");
    return 0;
}

void prepare_workload(char *dirpath, std::vector<rw_interf_task_t *> &task_list, unsigned long granularity)
{
    unsigned int num_tasks = task_list.size();
    unsigned long num_blocks;
    unsigned long start_magic = 0;
    char path[128] = {0};

	printf(DIVIDER);
	printf("Prepare workload (Stage 1)\n");
    for (unsigned int i = 0; i < num_tasks; i++) {
        num_blocks = task_list[i]->num_blocks;
        sprintf(path, "%s/pre-%lu", dirpath, i);
		printf("Prepare %s, magic at: %lu (%lu)\n", path, start_magic, num_blocks);
        make_file(path, num_blocks, start_magic, granularity, false, NULL);
        start_magic += num_blocks;
    }

    printf("Prepare workload done\n");
	printf(DIVIDER);
}

struct task_param {
    char *dirpath;
    rw_interf_task_t *rw_task_param;
    unsigned long granularity;
};

void dedup_or_write_task(struct task_param *param)
{
    unsigned long num_blocks;
    unsigned long start_magic = 0;
    rw_interf_task_t *rw_task_param = param->rw_task_param;
    char *dirpath = param->dirpath;
    unsigned long granularity = param->granularity;
    char path[128] = {0};

    num_blocks = rw_task_param->num_blocks;
    start_magic = rw_task_param->start_magic;
    sprintf(path, "%s/%lu", dirpath, rw_task_param->id);
    make_file(path, num_blocks, start_magic, granularity, rw_task_param->dup, &rw_task_param->progress);

    return;
}

static inline uint64_t timespec_to_ns(const struct timespec *t)
{
    return (uint64_t)t->tv_sec * 1000000000 + t->tv_nsec;
}

static inline uint64_t get_ns_diff(uint64_t start, uint64_t end)
{
    return end - start;
}

static inline uint64_t timestamp_ns()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return timespec_to_ns(&t);
}

void run(char *dirpath, std::vector<rw_interf_task_t *> &task_list, unsigned long granularity, int measurement_object)
{
    unsigned int num_tasks = task_list.size();
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * num_tasks);
	uint64_t start, end;
	uint64_t time_usage = 0;
	unsigned int num_readers = 0;
	unsigned int num_writers = 0;
	unsigned long total_processed_blocks = 0;

	for (unsigned int i = 0; i < num_tasks; i++) {
		if (task_list[i]->dup) {
			num_readers++;
		} else {
			num_writers++;
		}
	} 

	struct task_param *params = (struct task_param *)malloc(sizeof(struct task_param) * num_tasks);
	if (!params) {
		perror("malloc");
		return;
	}	
	
	printf(DIVIDER);
	printf("Run (Stage 2)\n");
	start = timestamp_ns();
    /* running thread  */
    for (unsigned int i = 0; i < num_tasks; i++) {
		struct task_param *param = &params[i];

		param->dirpath = dirpath;
		param->rw_task_param = task_list[i];
		param->granularity = granularity;

        pthread_create(&threads[i], NULL, (void *(*)(void *))dedup_or_write_task, (void *)param);
    }

	if (measurement_object == MO_READ_LAT) {
		/* wait for all target threads to finish */
		for (unsigned int i = 0; i < num_readers; i++) {
			pthread_join(threads[i], NULL);
		}
		/* cancel remains */
		for (unsigned int i = num_readers; i < num_tasks; i++) {
			pthread_cancel(threads[i]);	
		}
	} else if (measurement_object == MO_WRITE_LAT) {
		/* wait for all target threads to finish */
		for (unsigned int i = num_readers; i < num_tasks; i++) {
			pthread_join(threads[i], NULL);
		}
		/* cancel remains */
		for (unsigned int i = 0; i < num_readers; i++) {
			pthread_cancel(threads[i]);	
		}
	}

	end = timestamp_ns();

	time_usage = get_ns_diff(start, end);

	for (unsigned int i = 0; i < num_tasks; i++) {
		struct task_param *param = &params[i];
		total_processed_blocks += param->rw_task_param->progress;
	}

	printf("Process %ld blocks in %f seconds\n", total_processed_blocks, time_usage / 1000000000.0);
	printf("Bandwidth: %f MiB/s\n", total_processed_blocks * granularity / (time_usage / 1000000000.0) / 1024 / 1024);

	free(threads);
	free(params);
	printf("Run done\n");
	printf(DIVIDER);
}

int main(int argc, char **argv)
{
    char *optstring = (char *)"d:g:r:w:a:s:m:h";
    int opt;
    char dirpath[128] = {0};
    unsigned long granularity;
    unsigned long total_io;
    unsigned long total_num_blocks;
    unsigned int reader_num;
    unsigned int writer_num;
	int measurement_object = MO_READ_LAT;
	int stage = 0;

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'd':
            strcpy(dirpath, optarg);
			/* remove last '/' */
			if (dirpath[strlen(dirpath) - 1] == '/') {
				dirpath[strlen(dirpath) - 1] = '\0';
			}
            break;
        case 'g':
            granularity = atol(optarg);
            break;
        case 'r':
            reader_num = atoi(optarg);
            break;
        case 'w':
            writer_num = atoi(optarg);
            break;
        case 'a':
            total_io = atol(optarg);
            break;
		case 's':
			stage = atoi(optarg);
			break;
		case 'm':
			measurement_object = atoi(optarg);
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
	
    total_num_blocks = total_io * 1024 * 1024 * 1024 / granularity;
	
	printf("\nParameters:\n");
	printf("\tdirpath: %s\n", dirpath);
	printf("\tgranularity: %lu B\n", granularity);
	printf("\treader_num: %u\n", reader_num);
	printf("\twriter_num: %u\n", writer_num);
	printf("\ttotal_io: %lu GiB\n", total_io);
	printf("\ttotal_num_blocks: %lu\n", total_num_blocks);
	printf("\tmeasurement_object: %s\n", measurement_object == MO_READ_LAT ? "Read Latency" : (measurement_object == MO_WRITE_LAT ? "Write Latency" : "None"));
	printf("\tstage: %d for %s\n\n", stage, stage == 0 ? "All" : (stage == 1 ? "Prepare" : "Dedup/Write"));
    
	std::vector<rw_interf_task_t *> task_list;
    make_tasks(task_list, reader_num, writer_num, total_num_blocks, measurement_object);
	
	switch (stage)
	{
	case 0:
		prepare_workload(dirpath, task_list, granularity);
		run(dirpath, task_list, granularity, measurement_object);
		break;
	case 1:
		prepare_workload(dirpath, task_list, granularity);
		break;
	case 2:
		run(dirpath, task_list, granularity, measurement_object);
		break;
	default:
		break;
	}

	/* tasks */
	task_list.clear();

    return 0;
}