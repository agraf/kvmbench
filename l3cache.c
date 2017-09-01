#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

static void benchmark_sleeps(void)
{
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval tv;
    struct timespec ts_sleep;
    uint64_t loops = 0;

    assert(!gettimeofday(&tv_start, NULL));
    tv_end = tv_start;
    tv_end.tv_sec += 1;

    ts_sleep.tv_nsec = 1;
    ts_sleep.tv_sec = 0;

    while(1) {
        assert(!gettimeofday(&tv, NULL));
        if ((tv.tv_sec >= tv_end.tv_sec) && (tv.tv_usec >= tv_end.tv_usec))
            break;

        loops++;
        nanosleep(&ts_sleep, NULL);
    }

    printf("Number of sleeps: %ld\n", loops);
}

int main(int argc, char **argv)
{
    benchmark_sleeps();

    return 0;
}
