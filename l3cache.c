#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_cond_t cv;
static pthread_mutex_t mutex;

static void *waiter_thread(void *opaque)
{
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval tv;
    uint64_t loops = 0;

    assert(!gettimeofday(&tv_start, NULL));
    tv_end = tv_start;
    tv_end.tv_sec += 1;

    pthread_mutex_lock(&mutex);

    while(1) {
        assert(!gettimeofday(&tv, NULL));
        if ((tv.tv_sec >= tv_end.tv_sec) && (tv.tv_usec >= tv_end.tv_usec))
            break;

        loops++;
        pthread_cond_wait(&cv, &mutex);
    }

    pthread_mutex_unlock(&mutex);

    printf("Number of sleeps: %ld\n", loops);
    exit(0);
    return NULL;
}

static void *notifier_thread(void *opaque)
{
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads[2];
    int i;

    pthread_create(&threads[0], NULL, notifier_thread, NULL);
    pthread_create(&threads[1], NULL, waiter_thread, NULL);

    for (i=0; i<(sizeof(threads) / sizeof(threads[0])); i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
