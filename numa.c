#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <numa.h>

#define MB (1024 * 1024)

#define NR_CHILDREN 100
#define RUN_SECS 5

enum child_type {
    CHILD_READER = 0,
    CHILD_WRITER = 1
};

struct {
    uint64_t bytes;
    uint32_t node;
    uint8_t padding[64 - 8 - 4];
} *results;

void exit_all(int *pids)
{
    int i;

    for (i = 0; i < NR_CHILDREN; i++) {
        kill(pids[i], SIGTERM);
    }

    exit(0);
}

static void child_reader(int fd, volatile uint64_t *result)
{
    int size = 100 * MB;
    char *buf = malloc(size);
    uint64_t bytes = 0;

    memset(buf, '\0', size);
    while (1) {
        bytes += read(fd, buf, size);
        *result = bytes;
    }
}

static void child_writer(int fd)
{
    int size = 100 * MB;
    char *buf = malloc(size);

    while (1) {
        memset(buf, ' ', size);
        write(fd, buf, size);
    }
}

int main(int argc, char **argv)
{
    int childpid[NR_CHILDREN];
    int pipefd[2];
    int i;
    uint64_t all_bytes = 0;
    int numa_node = 0;
    int numa_max = numa_max_node() + 1;
    int opt_aligned = 1;
    int opt_numactl = 1;
    int opt_rotate = 1;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-u")) {
            opt_aligned = 0;
        } else if (!strcmp(argv[i], "-N")) {
            opt_numactl = 0;
        } else if (!strcmp(argv[i], "-0")) {
            opt_rotate = 0;
        } else {
            printf("Syntax: %s\n\n", argv[0]);
            printf("  -u    Misalign NUMA nodes for reader/writer\n");
            printf("  -N    Disable NUMA awareness\n");
            printf("  -0    Always use NUMA node 0\n");
            printf("\n");
            exit(0);
        }
    }

    /*
     * We create reader and writer processes. Both are connected via pipes.
     * Writers keep writing 100MB chunks of garbage to the pipe. Readers read
     * that garbage and add up the number of bytes read.
     *
     * To account to full number of bytes read at the end, account them in
     * a shared memory range.
     */

    results = mmap(NULL, sizeof(*results) * NR_CHILDREN, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    printf("Benchmarking message passing on %d NUMA nodes ...\n", numa_max);
    if (opt_numactl) {
        printf("  NUMA aware:        yes\n");
        printf("  NUMA aligned:      %s\n", opt_aligned ? "yes" : "no");
        printf("  NUMA nodes used:   0-%d\n", opt_rotate ? numa_max-1 : 0);
    } else {
        printf("  NUMA aware:        no\n");
    }
    printf("\n");

    for (i = 0; i < NR_CHILDREN; i++) {
        int cpid;
        int new_pair = ((i & 1) == CHILD_READER);

        if (new_pair) {
            if (pipe(pipefd)) {
                printf("Error while creating pipe: %s\n", strerror(errno));
                exit_all(childpid);
            }

            /*
             * Move to the next NUMA node, so we pin reader and writer to
             * the same node
             */
            numa_node++;
            numa_node %= numa_max;
        }

        if (!opt_rotate) {
            numa_node = 0;
        }

        cpid = fork();
        if (!cpid) {
            /*
             * We're a child now, figure out what we should do, but
             * first pin us to the numa node.
             */

            if (opt_numactl) {
                numa_set_preferred(numa_node);
                numa_run_on_node(numa_node);
            }

            results[i].node = numa_node_of_cpu(sched_getcpu());

            if ((i & 1) == CHILD_READER) {
                /* Reader */
                close(pipefd[CHILD_WRITER]);
                child_reader(pipefd[CHILD_READER], &results[i].bytes);
            } else {
                /* Writer */
                close(pipefd[CHILD_READER]);
                child_writer(pipefd[CHILD_WRITER]);
            }
        } else {
            /* Parent, remember child pid */
            childpid[i] = cpid;

            if (!opt_aligned) {
                numa_node++;
                numa_node %= numa_max;
            }
        }
    }

    /* Wait n seconds */
    sleep(RUN_SECS);

    for (i = 0; i < NR_CHILDREN; i++) {
        all_bytes += results[i].bytes;
    }

    printf("Total MB/s read: %ld\n", all_bytes / MB / RUN_SECS);

    /* Then quit */
    exit_all(childpid);

    return 0;
}
