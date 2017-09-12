#include "tsc.h"
#include <immintrin.h>
#include <string.h>

extern int avxChecksum(const char * const target, size_t targetLength);
extern int avxChecksumV2(const char * const target, size_t targetLength);
extern int naiveChecksum(const char * const target, size_t targetLength);

static void benchmark(int (*func)(const char * const target, size_t targetLength),
                      size_t len, const char *test_data, const char *type)
{
    uint64_t tsc_start;
    uint64_t tsc_end;
    uint64_t loops;
    int csum;

    loops = 0;
    tsc_start = rdtsc();
    tsc_end = tsc_start + tsc_per_sec;
    while (rdtsc() < tsc_end) {
        csum = func(test_data, len);
        asm("" : : "r"(csum));
        loops++;
    }

    printf("%s loops: %ld\n", type, loops);
}

int main(int argc, char **argv)
{
    char test_data[2048 * 1024];

    setup_tsc();
    memset(test_data, 'd', sizeof(test_data));

    benchmark(naiveChecksum, sizeof(test_data), test_data, "Generic");
    benchmark(avxChecksum, sizeof(test_data), test_data, "AVX2");
    benchmark(avxChecksumV2, sizeof(test_data), test_data, "AVX2 (optimized)");

    return 0;
}
