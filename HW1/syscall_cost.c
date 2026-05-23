#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define ITERATIONS 1000000
#define TRIALS     11

static int cmp_long(const void *a, const void *b) {
    long x = *(long *)a, y = *(long *)b;
    return (x > y) - (x < y);
}

int main(void) {
    int fd[2];
    pipe(fd);

    long res_samples[TRIALS];
    for (int t = 0; t < TRIALS; t++) {
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);
        gettimeofday(&t2, NULL);
        res_samples[t] = (t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_usec - t1.tv_usec);
    }
    qsort(res_samples, TRIALS, sizeof(long), cmp_long);
    printf("Timer resolution (median of %d): %ld us\n", TRIALS, res_samples[TRIALS / 2]);

    long cost_samples[TRIALS];
    for (int t = 0; t < TRIALS; t++) {
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);
        for (int i = 0; i < ITERATIONS; i++)
            read(fd[0], NULL, 0);
        gettimeofday(&t2, NULL);
        cost_samples[t] = (t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_usec - t1.tv_usec);
    }
    qsort(cost_samples, TRIALS, sizeof(long), cmp_long);
    long median_us = cost_samples[TRIALS / 2];
    double cost_ns = (double)median_us * 1000.0 / ITERATIONS;

    printf("Iterations : %d\n", ITERATIONS);
    printf("Trials     : %d\n", TRIALS);
    printf("Syscall cost (median): %.2f ns\n", cost_ns);

    close(fd[0]);
    close(fd[1]);
    return 0;
}
