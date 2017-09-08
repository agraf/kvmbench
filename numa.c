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

#define MB (1024 * 1024)

#define NR_CHILDREN 100
#define RUN_SECS 5

enum child_type {
    CHILD_READER = 0,
    CHILD_WRITER = 1
};

struct {
    uint64_t bytes;
    uint8_t padding[64 - 8];
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

    for (i = 0; i < NR_CHILDREN; i++) {
        int cpid;

        if (!(i & 1)) {
            if (pipe(pipefd)) {
                printf("Error while creating pipe: %s\n", strerror(errno));
                exit_all(childpid);
            }
        }

        cpid = fork();
        if (!cpid) {
            /* We're a child now, figure out what we should do */

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
            /* Parent, clean up and remember child pid */
            childpid[i] = cpid;
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
