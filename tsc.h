void setup_tsc(void);
extern uint64_t tsc_per_sec;

static __inline__ unsigned long long rdtsc(void)
{
#ifdef __x86_64__
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
#elif defined(__aarch64__)
    unsigned long long ret;
    asm volatile ("mrs %0, cntvct_el0" : "=r" (ret));
    return ret;
#else
#error Unsupported architecture
#endif
}
