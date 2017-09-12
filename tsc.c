#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include "tsc.h"

uint64_t tsc_per_sec;

static uint64_t get_us_gettimeofday(void)
{
    struct timeval tv;

    assert(!gettimeofday(&tv, NULL));
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

void setup_tsc(void)
{
    uint64_t before, after;
    uint64_t before_tod, after_tod;

    before_tod = get_us_gettimeofday();
    before = rdtsc();

    usleep(10);

    after_tod = get_us_gettimeofday();
    after = rdtsc();

    tsc_per_sec = ((after - before) * 1000000ULL) / (after_tod - before_tod);
}

