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

#if 0
static int gettid(void)
{
    return syscall(__NR_gettid);
}
#endif

volatile enum {
    SIG_STATE_INIT,
    SIG_STATE_IDLE,
    SIG_STATE_SENDING,
    SIG_STATE_RECEIVED,
} sig_state = SIG_STATE_INIT;

pthread_t threads[2];

static void sig_handler(int sig)
{
//printf("[%d] handler %d\n", gettid(), sig);
    if (sig == SIGUSR1)
        sig_state = SIG_STATE_RECEIVED;
}

static void *waiter_thread(void *opaque)
{
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval tv;
    uint64_t loops = 0;
    struct sigaction sa = {
        .sa_handler = sig_handler,
    };

    sigset_t fSigSet;

    sigemptyset(&fSigSet);
    sigaddset(&fSigSet, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &fSigSet, NULL);

    sigaction(SIGUSR1, &sa, NULL);

    assert(!gettimeofday(&tv_start, NULL));
    tv_end = tv_start;
    tv_end.tv_sec += 1;

    sig_state = SIG_STATE_IDLE;

    while(1) {
        assert(!gettimeofday(&tv, NULL));
        if ((tv.tv_sec >= tv_end.tv_sec) && (tv.tv_usec >= tv_end.tv_usec))
            break;

        loops++;
//printf("[%d] pre-received \n", gettid());
        while (sig_state != SIG_STATE_RECEIVED) ;
//printf("[%d] recevied\n", gettid());
        sig_state = SIG_STATE_IDLE;
    }

    printf("Number of sleeps: %ld\n", loops);
    exit(0);
    return NULL;
}

static void *notifier_thread(void *opaque)
{
    while (sig_state != SIG_STATE_IDLE) ;
    while (1) {
//printf("[%d] trigger\n", gettid());
        sig_state = SIG_STATE_SENDING;
        pthread_kill(threads[1], SIGUSR1);
        while (sig_state != SIG_STATE_IDLE) ;
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int i;
    sigset_t fSigSet;

    sigemptyset(&fSigSet);
    sigaddset(&fSigSet, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &fSigSet, NULL);

    pthread_create(&threads[0], NULL, notifier_thread, NULL);
    pthread_create(&threads[1], NULL, waiter_thread, NULL);

    for (i=0; i<(sizeof(threads) / sizeof(threads[0])); i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
