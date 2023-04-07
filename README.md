# busyboy

This program starts N threads, and tries to loop as fast as possible in each
thread.  It reports how fast the loops are spinning relative to their best
performance.  You'd use it along with a workload that is suspected to use more
compute resources than is reported by Linux.

## history

I have to be honest, I don't remember where this program came from, or who the
original author was.  To the best of my recollection it was written by someone
on LKML, in the early 2000s, and provided as proof that you can get away with
running computation on Linux without it being accounted for in top/ps/etc.  I
have kept it around for ages making small enhancements and eventually put it in
a git repo in 2023.

## usage

Build the program using `make`, there are no dependencies other than a standard
C development environment with posix threads.

You probably want to run as many `busyboy` threads as there are logical processors
in your system.  It is further best to run busy boy with a low scheduling
priority, so that it does not impact the actual workload.

```
$ nice -n19 ./busyboy -n $(nproc)
all cpus started.
 100    100    100    99.7   100    99     100    98.9   96.8   99.4   99.1   98.9   95.6   98.7   97.1   98.7
 100    99.7   100    98.2   99.4   98.6   98.2   100    97.4   99.8   99.9   99.9   97.9   99.5   99.1   95.2
 98.4   99.3   99.6   99     98.9   97.8   95.1   99.2   98.2   99.6   98.9   99.6   95.3   98.5   93.9   97.7
 99.2   99.2   99.7   98.2   98.5   99     98     99.2   95.1   98.8   99.7   99.9   99.4   98     99.8   98.2
^C
```

A line is printed roughly every second, with how fast each thread was able to
run as a percentage of the fastest we have observed to this point.

You can also run `busyboy` in summary mode:
```
$ nice -n19 ./busyboy -n $(nproc) -s
all cpus started.
 0.000   97.053   100.000   97.098
87.127   95.374   100.000   95.427
83.851   96.019   99.176   96.086
94.614   97.068   98.371   97.073
^C
```

Each line shows: lowest score, mean score, max score, and standard deviation


The values could fall bellow 100% if:
* system is using CPU frequency scaling and clock cycle goes up and down
* other threads are using CPU cycles and not letting our busy boy run


