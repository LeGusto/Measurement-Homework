#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sched.h>

#define ITERATIONS 100000
#define TRIALS     11

static int cmp_long(const void *a, const void *b) {
    long x = *(long *)a, y = *(long *)b;
    return (x > y) - (x < y);
}

int main(void) {
    // cpu_set_t set;
    // CPU_ZERO(&set);
    // CPU_SET(0, &set);
    // if (sched_setaffinity(0, sizeof(set), &set) < 0) {
    //     perror("sched_setaffinity");
    //     return 1;
    // }

    long samples[TRIALS];
    char buf[1] = {0};

    for (int t = 0; t < TRIALS; t++) {
        int p1[2], p2[2];
        pipe(p1);
        pipe(p2);

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }

        if (pid == 0) {
            close(p1[1]); close(p2[0]);
            for (int i = 0; i < ITERATIONS; i++) {
                read(p1[0], buf, 1);
                write(p2[1], buf, 1);
            }
            close(p1[0]); close(p2[1]);
            exit(0);
        }

        close(p1[0]); close(p2[1]);

        struct timeval t1, t2;
        gettimeofday(&t1, NULL);
        for (int i = 0; i < ITERATIONS; i++) {
            write(p1[1], buf, 1);
            read(p2[0], buf, 1);
        }
        gettimeofday(&t2, NULL);

        close(p1[1]); close(p2[0]);
        wait(NULL);

        samples[t] = (t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_usec - t1.tv_usec);
    }

    qsort(samples, TRIALS, sizeof(long), cmp_long);
    long median_us = samples[TRIALS / 2];
    double cost_ns = (double)median_us * 1000.0 / (ITERATIONS * 2);

    printf("Iterations (round trips): %d\n", ITERATIONS);
    printf("Trials                  : %d\n", TRIALS);
    printf("Context switch cost (median): %.1f ns\n", cost_ns);

    return 0;
}
