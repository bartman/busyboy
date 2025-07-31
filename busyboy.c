/* this program reports number of loops it was able to execute compared to the baseline */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sched.h>
#include <pthread.h>
#include <malloc.h>
#include <math.h>

static void die(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "ERROR: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

static void busy(unsigned loops)
{
	int i;
	for (i=0; i<loops; i++)
        asm volatile("" : : : "memory");
}

struct counter {
	unsigned long long value;
	unsigned long long padding[7];
};

struct kid {
	int       id;
	pthread_t thread;
	int       cpu;

	unsigned  busy_loops;

	struct counter *counter;
};

static void* kid_func(void *arg)
{
	struct kid *kid = arg;

	while (1) {
		busy(kid->busy_loops);
		kid->counter->value ++;
	}

	pthread_exit(0);
}

static void start_kid(struct kid *kid, int id, int cpu,
			unsigned busy_loops, struct counter *counter)
{
	cpu_set_t cpuset;
	int rc;

	kid->id = id;
	kid->cpu = cpu;
	kid->busy_loops = busy_loops;
	kid->counter = counter;

	rc = pthread_create(&kid->thread,
			NULL, kid_func, kid);
	if (rc)
		die("failed to start thread for kid %u\n", kid->id);

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	rc = pthread_setaffinity_np(kid->thread, sizeof(cpu_set_t), &cpuset);
	if (rc)
		die("failed to bind kid %u to cpu %u\n", kid->id, kid->cpu);

	printf("\rcpu %u started", kid->cpu);

}

#define min(a,b) ({ \
		typeof(a) _a = (a); \
		typeof(b) _b = (b); \
		_a < _b ? _a : _b; \
		})
#define max(a,b) ({ \
		typeof(a) _a = (a); \
		typeof(b) _b = (b); \
		_a > _b ? _a : _b; \
		})


int main(int argc, char *argv[])
{
	unsigned long base_cpu = 0, num_kids = 0, k;
	struct kid *kids;
	unsigned busy_loops = 1000;
	struct counter *counters, *curr, *last;
	double best_rate = 0;
	bool show_summary = false;
	time_t last_time;
	int argi;

	for(argi=1; argi<argc; argi++) {
		const char *arg = argv[argi];

		if (arg[0] != '-' || !arg[1] || arg[2])
			die("bad option: %s\n", arg);

		switch(arg[1]) {
		case 'h':
			printf(	"busyboy [-h] [-n <processes>]\n"
				"\n"
				"Options:\n"
				" -n <processes>         number of busy processes to start\n"
				" -c <base-cpu-number>   which CPU is first\n"
				" -b <loops>             busy loops before increment\n"
				" -s                     show min/avg/max/std.div of all processes\n"
				"\n");
			exit(0);
			break;
		case 'n':
			arg = argv[++argi];
			if (!arg || !*arg)
				die("option -n requires prcoesses count\n");
			num_kids = strtoul(argv[argi], NULL, 10);
			break;
		case 'c':
			arg = argv[++argi];
			if (!arg || !*arg)
				die("option -c requires base cpu number\n");
			base_cpu = strtoul(argv[argi], NULL, 10);
			break;
		case 'b':
			arg = argv[++argi];
			if (!arg || !*arg)
				die("option -b requires loop count\n");
			busy_loops = strtoul(argv[argi], NULL, 10);
			break;
		case 's':
			show_summary = true;
			break;
		default:
			die("unknown option: %s\n", arg);
			break;
		}
	}

	if (num_kids < 1 || num_kids > 128)
		die("number of processes (-n option) must be in [1,128]\n");

	kids = calloc(num_kids, sizeof(struct kid));
	if (!kids)
		die("failed to allocate %lu kids (of size %u bytes each)\n",
			num_kids, sizeof(struct kid));

	counters = memalign(4096, num_kids * 3 * sizeof(struct counter));
	if (!counters)
		die("failed to allocate %lu counters (of size %u bytes each)\n",
			num_kids, 3 * sizeof(struct counter));
	
	last = counters + num_kids;
	curr = last + num_kids;
	
	for (k=0; k<num_kids; k++)
		start_kid(kids+k, k, base_cpu + k, busy_loops, counters+k);
	printf("\rall cpus started.     \n");
	
	last_time = time(NULL);
	memset(last, 0, num_kids * sizeof(struct counter));
	while(1) {
		time_t now;
		double score[num_kids];
		double total_score = 0;
		double min_score = best_rate;
		double max_score = 0;

		sleep(1);

		now = time(NULL);
		memcpy(curr, counters, num_kids * sizeof(struct counter));

		for(k=0; k<num_kids; k++) {
			unsigned long long diff = curr[k].value - last[k].value;
			double lps;

			lps = (double)diff / (now - last_time);

			if (best_rate < lps)
				best_rate = lps;

			score[k] = 100 * lps / best_rate;

			if (show_summary) {
				total_score += score[k];
				min_score = min(min_score, score[k]);
				max_score = max(max_score, score[k]);
			} else
				printf(" %-7.3lg", score[k]);
		}

		if (show_summary) {
			double mean = total_score / num_kids;
			double total_sq_deltas = 0;
			for(k=0; k<num_kids; k++) {
				double diff = score[k] - mean;
				total_sq_deltas += diff * diff;
			}

			printf("%7.3lf   %7.3lf   %7.3lf   %7.3lf\n",
					min_score,
					mean,
					max_score,
					sqrt( total_sq_deltas / num_kids )
					);
		} else
			printf("\n");

		fflush(stdout);
	
		memcpy(last, curr, num_kids * sizeof(struct counter));
		last_time = now;
	}

	pthread_exit(0);
}
