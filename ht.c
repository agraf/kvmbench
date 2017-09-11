#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

volatile uint32_t running = 0;
int opt_pinned = 1;
int opt_optimal = 1;
int opt_only_add = 0;
int opt_only_mul = 0;

volatile struct {
    uint64_t result;
    int is_add;
} *results;

static void *thread_add(void *opaque)
{
    uint64_t idx = (unsigned long)opaque;
    int64_t a = idx;
    int64_t b = idx + 1;
    int64_t c = idx + 2;
    int64_t d = idx + 3;
    int64_t e = idx + 4;
    int64_t f = idx + 5;
    int64_t g = idx + 6;
    int64_t h = idx + 7;
    uint64_t loops = 0;
    int i;

    while (!running);

    while (1) {
        for (i = 0; i < 16; i++) {
            a += idx;
            b += idx;
            c += idx;
            d += idx;
            e += idx;
            f += idx;
            g += idx;
            h += idx;
            asm("" : : "r"(a), "r"(b), "r"(c), "r"(d), "r"(e), "r"(f), "r"(g), "r"(h));
        }
        loops += 16;
        if (!(loops & 0xffff)) {
            results[idx].result = loops;
        }
    }

    return (void*)d;
}

static void *thread_mul(void *opaque)
{
    uint64_t idx = (unsigned long)opaque;
    int64_t d = idx;
    uint64_t loops = 0;

    while (!running);

    while (1) {
        int i;

        for (i = 0; i < 16; i++) {
            d = d * (int64_t)idx;
            asm("" : : "r"(d));
        }
        loops += 16;
        if (!(loops & 0xffff)) {
            results[idx].result = loops;
        }
    }

    return (void*)(unsigned long)d;
}

int pin_thread(pthread_t t, int core_id) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    return pthread_setaffinity_np(t, sizeof(cpu_set_t), &cpuset);
}

static void spawn_thread(pthread_t *pt, char *spawned, int is_add, uintptr_t idx)
{
    results[idx].is_add = is_add;

    pthread_create(pt, NULL, is_add ? thread_add : thread_mul, (void*)idx);
    if (opt_pinned) {
        pin_thread(*pt, idx);
    }
    *spawned = 1;
}

int main(int argc, char **argv)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int i;
    char thread_spawned[num_cores];
    pthread_t thread[num_cores];
    uint64_t result = 0;
    char thread_siblings[1024];
    char *tsp;
    int is_add = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-P")) {
            opt_pinned = 0;
        } else if (!strcmp(argv[i], "-c")) {
            opt_optimal = 0;
        } else if (!strcmp(argv[i], "-d")) {
            opt_only_add = 1;
            is_add = 1;
            opt_optimal = 0;
        } else if (!strcmp(argv[i], "-m")) {
            opt_only_mul = 1;
            opt_optimal = 0;
        } else {
            printf("Syntax: %s\n\n", argv[0]);
            printf("  -c    Pin the threads to collide\n");
            printf("  -P    Don't pin the threads\n");
            printf("\n");
            exit(0);
        }
    }

    results = malloc(num_cores * sizeof(*results));
    memset((void*)results, 0, num_cores * sizeof(*results));
    memset(thread_spawned, 0, num_cores);

    for (i = 0; i < num_cores; i++) {
        char path[512];
        int fd;
        int offset = 0;

        if (thread_spawned[i]) continue;

        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/topology/thread_siblings", i);

        memset(thread_siblings, 0, sizeof(thread_siblings));
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            printf("Error opening %s\n", path);
            return 1;
        }
        read(fd, thread_siblings, sizeof(thread_siblings));
        /* Just hope we read everything */
        close(fd);

        if (strlen(thread_siblings) < 9) {
            uint32_t cur_siblings;

            sscanf(thread_siblings, "%x", &cur_siblings);
            sprintf(thread_siblings, "%08x\n", cur_siblings);
        }

        tsp = thread_siblings + strlen(thread_siblings) - 9;
        while (1) {
            uint32_t cur_siblings;
            int j;

            if (sscanf(tsp, "%08x", &cur_siblings) != 1) {
                printf("Error reading siblings from %s\n", tsp);
                exit(1);
            }

            for (j = 0; j < 32; j++) {
                if (cur_siblings & (1U << j)) {
                    int cpu = j + offset;
                    spawn_thread(&thread[cpu], &thread_spawned[cpu], is_add, cpu);
                    if (opt_optimal) {
                        is_add = (is_add + 1) & 1;
                    }
                }
            }

            if (tsp == thread_siblings) {
                break;
            }

            tsp -= 9;
            offset += 32;
        }

        if (opt_only_add) {
            is_add = 1;
        } else if (opt_only_mul) {
            is_add = 0;
        } else if (!opt_optimal) {
            is_add = (is_add + 1) & 1;
        }
    }

    if (opt_pinned) {
        printf("Running MUL test on CPUs ");
        for (i = 0; i < num_cores; i++) {
            if (!results[i].is_add) {
                printf("%d ", i);
            }
        }
        printf("\n");

        printf("Running ADD test on CPUs ");
        for (i = 0; i < num_cores; i++) {
            if (results[i].is_add) {
                printf("%d ", i);
            }
        }
        printf("\n");
    } else {
        printf("Running ADD and MUL tests without pinning ...\n");
    }

    running = 1;
    sleep(5);

    result = 0;
    for (i = 0; i < num_cores; i++) {
        if (!results[i].is_add) {
            result += results[i].result;
        }
    }
    printf("Total MMuls: %ld\n", result / 1000000 / 5);

    result = 0;
    for (i = 0; i < num_cores; i++) {
        if (results[i].is_add) {
            result += results[i].result;
        }
    }
    printf("Total MAdds: %ld\n", result / 1000000 / 5);

    return 0;
}
