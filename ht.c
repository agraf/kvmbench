#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

volatile uint32_t running = 0;

volatile struct {
    uint64_t result;
} *results;

static void *thread_alu(void *opaque)
{
    uint64_t idx = (unsigned long)opaque;
    volatile uint64_t d = idx;
    uint64_t loops = 0;

    while (!running);

    while (1) {
        d = d * 11;
        loops++;
        results[idx].result = loops;
    } 

    return (void*)d;
}

static void *thread_fpu(void *opaque)
{
    uint64_t idx = (unsigned long)opaque;
    volatile double f = (double)idx;
    uint64_t loops = 0;

    while (!running);

    while (1) {
        f = f * 1.01;
        loops++;
        results[idx].result = loops;
    } 

    return (void*)(unsigned long)f;
}

int pin_thread(pthread_t t, int core_id) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    return pthread_setaffinity_np(t, sizeof(cpu_set_t), &cpuset);
}

static void spawn_thread(pthread_t *pt, char *spawned, int is_alu, uintptr_t idx)
{
    pthread_create(pt, NULL, is_alu ? thread_alu : thread_fpu, (void*)idx);
    pin_thread(*pt, idx);
    *spawned = 1;
}

int main(int argc, char **argv)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int i;
    char thread_spawned[num_cores];
    pthread_t thread[num_cores];
    uint64_t result = 0;

    results = malloc(num_cores * sizeof(*results));
    memset((void*)results, 0, num_cores * sizeof(*results));
    memset(thread_spawned, 0, num_cores);

#if 0
    char path[512];

    for (i = 0; i < num_cores; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/topology/thread_siblings");
    }
#else
    for (i = 0; i < num_cores; i++) {
        int thread0 = i;
        int thread1 = i + 1;

        if (thread_spawned[i]) continue;

        spawn_thread(&thread[thread0], &thread_spawned[thread0], 1, thread0);
        spawn_thread(&thread[thread1], &thread_spawned[thread1], 0, thread1);
    }

    running = 1;
    sleep(5);
#endif

    for (i = 0; i < num_cores; i++) {
        result += results[i].result;
    }
    printf("Total MOps: %ld\n", result / 1000000 / 5);

    return 0;
}
