#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#define MAX_JOBS 64

typedef struct {
    int id;
    int arrival;
    int burst;
    int remaining;
    int first_run;
    int finish;
} Job;

static int rr_queue[1 << 20];

static int cmp_arrival(const void *a, const void *b) {
    return ((Job *)a)->arrival - ((Job *)b)->arrival;
}

static void simulate_fifo(Job *jobs, int n) {
    qsort(jobs, n, sizeof(Job), cmp_arrival);
    int time = 0;
    for (int i = 0; i < n; i++) {
        if (time < jobs[i].arrival) time = jobs[i].arrival;
        jobs[i].first_run = time;
        time += jobs[i].burst;
        jobs[i].finish = time;
    }
}

static void simulate_sjf(Job *jobs, int n) {
    int done = 0, time = 0;
    int scheduled[MAX_JOBS] = {0};
    while (done < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (scheduled[i] || jobs[i].arrival > time) continue;
            if (best == -1 || jobs[i].burst < jobs[best].burst) best = i;
        }
        if (best == -1) {
            int next = INT_MAX;
            for (int i = 0; i < n; i++)
                if (!scheduled[i] && jobs[i].arrival < next) next = jobs[i].arrival;
            time = next;
            continue;
        }
        jobs[best].first_run = time;
        time += jobs[best].burst;
        jobs[best].finish = time;
        scheduled[best] = 1;
        done++;
    }
}

static void simulate_stcf(Job *jobs, int n) {
    int done = 0, time = 0;
    for (int i = 0; i < n; i++) { jobs[i].remaining = jobs[i].burst; jobs[i].first_run = -1; }
    while (done < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (jobs[i].remaining == 0 || jobs[i].arrival > time) continue;
            if (best == -1 || jobs[i].remaining < jobs[best].remaining) best = i;
        }
        if (best == -1) { time++; continue; }
        if (jobs[best].first_run == -1) jobs[best].first_run = time;
        time++;
        if (--jobs[best].remaining == 0) { jobs[best].finish = time; done++; }
    }
}

static void simulate_rr(Job *jobs, int n, int quantum) {
    for (int i = 0; i < n; i++) { jobs[i].remaining = jobs[i].burst; jobs[i].first_run = -1; }
    int in_queue[MAX_JOBS] = {0};
    int qhead = 0, qtail = 0, done = 0, time = 0;

    for (int i = 0; i < n; i++)
        if (jobs[i].arrival == 0) { rr_queue[qtail++] = i; in_queue[i] = 1; }

    while (done < n) {
        if (qhead == qtail) {
            int next = INT_MAX;
            for (int i = 0; i < n; i++)
                if (jobs[i].remaining > 0 && !in_queue[i] && jobs[i].arrival < next)
                    next = jobs[i].arrival;
            time = next;
            for (int i = 0; i < n; i++)
                if (jobs[i].remaining > 0 && !in_queue[i] && jobs[i].arrival <= time)
                    { rr_queue[qtail++] = i; in_queue[i] = 1; }
            continue;
        }
        int idx = rr_queue[qhead++];
        if (jobs[idx].first_run == -1) jobs[idx].first_run = time;
        int run = jobs[idx].remaining < quantum ? jobs[idx].remaining : quantum;
        int end = time + run;
        for (int i = 0; i < n; i++)
            if (!in_queue[i] && jobs[i].remaining > 0 && jobs[i].arrival > time && jobs[i].arrival <= end)
                { rr_queue[qtail++] = i; in_queue[i] = 1; }
        time = end;
        jobs[idx].remaining -= run;
        if (jobs[idx].remaining == 0) { jobs[idx].finish = time; done++; }
        else rr_queue[qtail++] = idx;
    }
}

static void print_results(Job *jobs, int n, const char *policy, int quantum) {
    if (strcmp(policy, "RR") == 0)
        printf("\nPolicy: %s (quantum=%d)\n", policy, quantum);
    else
        printf("\nPolicy: %s\n", policy);

    printf("%-6s %-8s %-8s %-10s %-12s %-12s\n",
           "Job", "Arrival", "Burst", "Finish", "Turnaround", "Response");
    double avg_ta = 0, avg_resp = 0;
    for (int i = 0; i < n; i++) {
        int ta   = jobs[i].finish    - jobs[i].arrival;
        int resp = jobs[i].first_run - jobs[i].arrival;
        avg_ta   += ta;
        avg_resp += resp;
        printf("%-6d %-8d %-8d %-10d %-12d %-12d\n",
               jobs[i].id, jobs[i].arrival, jobs[i].burst,
               jobs[i].finish, ta, resp);
    }
    printf("Average turnaround : %.2f\n", avg_ta   / n);
    printf("Average response   : %.2f\n", avg_resp / n);
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s -p <FIFO|SJF|STCF|RR> [-q quantum] -j arrival,burst [-j ...]\n"
        "  -p  scheduling policy (default: FIFO)\n"
        "  -q  time quantum for RR (default: 1)\n"
        "  -j  job spec as arrival,burst (repeat for multiple jobs)\n"
        "Example: %s -p RR -q 2 -j 0,10 -j 2,4 -j 4,6\n",
        prog, prog);
}

int main(int argc, char *argv[]) {
    Job jobs[MAX_JOBS];
    int n = 0, quantum = 1;
    char policy[16] = "FIFO";
    int opt;

    while ((opt = getopt(argc, argv, "p:q:j:")) != -1) {
        switch (opt) {
        case 'p': strncpy(policy, optarg, sizeof(policy) - 1); break;
        case 'q': quantum = atoi(optarg); break;
        case 'j':
            if (n >= MAX_JOBS) { fprintf(stderr, "Too many jobs (max %d)\n", MAX_JOBS); return 1; }
            if (sscanf(optarg, "%d,%d", &jobs[n].arrival, &jobs[n].burst) != 2) {
                fprintf(stderr, "Bad job format (expected arrival,burst): %s\n", optarg); return 1;
            }
            jobs[n].id = n; jobs[n].remaining = jobs[n].burst; jobs[n].first_run = -1; jobs[n].finish = 0;
            n++;
            break;
        default: usage(argv[0]); return 1;
        }
    }

    if (n == 0) { usage(argv[0]); return 1; }

    if      (strcmp(policy, "FIFO") == 0) simulate_fifo(jobs, n);
    else if (strcmp(policy, "SJF")  == 0) simulate_sjf(jobs, n);
    else if (strcmp(policy, "STCF") == 0) simulate_stcf(jobs, n);
    else if (strcmp(policy, "RR")   == 0) simulate_rr(jobs, n, quantum);
    else { fprintf(stderr, "Unknown policy: %s\n", policy); usage(argv[0]); return 1; }

    print_results(jobs, n, policy, quantum);
    return 0;
}
