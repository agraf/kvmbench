/*
 *  TSC vs gettimeofday benchmark
 *
 *  Copyright (c) 2017 Alexander Graf
 *
 *  SPDX-License-Identifier:     GPL-2.0+
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include "tsc.h"

static void benchmark_gettimeofday_fast(void)
{
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval tv;
    uint64_t loops = 0;

    assert(!gettimeofday(&tv_start, NULL));
    tv_end = tv_start;
    tv_end.tv_sec += 1;

    while(1) {
        assert(!gettimeofday(&tv, NULL));
        if ((tv.tv_sec >= tv_end.tv_sec) && (tv.tv_usec >= tv_end.tv_usec))
            break;

        loops++;
    }

    printf("Timer loops finished with optimized gettimeofday(): %ld\n", loops);
}

//////////

static uint64_t get_us_gettimeofday(void)
{
    struct timeval tv;

    assert(!gettimeofday(&tv, NULL));
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static void benchmark_gettimeofday(void)
{
    uint64_t us_start;
    uint64_t us_end;
    uint64_t loops = 0;

    us_start = get_us_gettimeofday();
    us_end = us_start + 1000000ULL;

    while(get_us_gettimeofday() < us_end)
        loops++;

    printf("Timer loops finished with gettimeofday(): %ld\n", loops);
}

//////////

static void benchmark_tsc(void)
{
    uint64_t tsc_start;
    uint64_t tsc_end;
    uint64_t loops = 0;

    tsc_start = rdtsc();
    tsc_end = tsc_start + tsc_per_sec;

    while(rdtsc() < tsc_end)
        loops++;

    printf("Timer loops finished with TSC: %ld\n", loops);
}

//////////

int main(int argc, char **argv)
{
    benchmark_gettimeofday();
    benchmark_gettimeofday_fast();

    setup_tsc();
    benchmark_tsc();

    return 0;
}
