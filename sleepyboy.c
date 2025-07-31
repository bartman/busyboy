/* this program runs compute (PRNG) for X msec, then sleep for Y msec */
#include "sched.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#define MSEC_PER_SEC 1000ull
#define USEC_PER_SEC 1000000ull
#define NSEC_PER_SEC 1000000000ull

#define USEC_PER_MSEC 1000ull
#define NSEC_PER_MSEC 1000000ull

#define NSEC_PER_USEC 1000ull

static inline double get_double_time(void)
{
	struct timespec ts;
	double sec;

	clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

	sec = (double)ts.tv_sec
	    + (double)ts.tv_nsec / NSEC_PER_SEC;

	return sec;
}

static void print_help(const char *prog_name) {
    printf("Usage: %s -c <compute-msec> -s <sleep-msec>\n", prog_name);
    printf("  -c <compute-msec> : Time in milliseconds to spend computing (PRNG calls)\n");
    printf("  -s <sleep-msec>   : Time in milliseconds to sleep\n");
    printf("  -h                : Print this help message\n");
}

int main(int argc, char *argv[]) {
    double compute_msec = 9;
    double sleep_msec = 1;
    int opt;

    while ((opt = getopt(argc, argv, "c:s:h")) != -1) {
        switch (opt) {
            case 'c':
                compute_msec = atof(optarg);
                break;
            case 's':
                sleep_msec = atof(optarg);
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    if (compute_msec <= 0.000001) {
        fprintf(stderr, "Compute time must be at least one nanosecond.\n");
        return 1;
    }

    if (sleep_msec < 0) {
        fprintf(stderr, "Sleep time must be a positive integers.\n");
        return 1;
    }

    srand(time(NULL));

    double start = get_double_time();

    size_t count = 0;
    double elapsed = 0.0;

    do {
        const size_t test_loop_count = 100000;
        for (size_t i = 0; i < test_loop_count; i++)
            rand();
        count += test_loop_count;

        double now = get_double_time();
        elapsed = now - start;
    } while (elapsed < 1.0);

    double calls_per_second = count / elapsed;
    size_t calls_per_loop = (size_t)(calls_per_second * (compute_msec / 1000.0));

    printf("Calibration: %zu calls in %.3f seconds, rate: %.0f calls/sec, for %f ms: %zu calls\n",
           count, elapsed, calls_per_second, compute_msec, calls_per_loop);

    struct timespec sleep_time = {
        .tv_sec = 0,
        .tv_nsec = (long)sleep_msec * 1000000LL};

    start = get_double_time();
    double next = start + 1;
    size_t total_calls = 0;

    while (1) {
        for (size_t i = 0; i < calls_per_loop; i++)
            rand();

        if (sleep_time.tv_sec || sleep_time.tv_nsec) {
            nanosleep(&sleep_time, NULL);
        } else {
            sched_yield();
        }
        total_calls += calls_per_loop;

        double now = get_double_time();
        if (now < next)
            continue;
        elapsed = now - start;

        double expected_calls = calls_per_second * elapsed;
        double efficiency = (double)total_calls / expected_calls;

        printf("%zu in %.3f sec, efficiency %.3f\n",
               total_calls, elapsed, efficiency);

        next = now + 1;
    }

    return 0;
}


