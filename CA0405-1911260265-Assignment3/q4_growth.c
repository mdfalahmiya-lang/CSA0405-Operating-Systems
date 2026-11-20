/* Q4: run time as the touched memory grows beyond the 7.6 GB of RAM.
   Each size is touched once (first pass) and then read again (second pass).
   When the working set no longer fits, the second pass must fault pages back
   from swap, which is where the run time breaks away from linear growth. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static long minflt(long *maj)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    *maj = ru.ru_majflt;
    return ru.ru_minflt;
}

int main(void)
{
    const size_t step = 512UL * 1024 * 1024;
    const int steps = 16;                 /* 512 MB .. 8 GB, beyond 7.6 GB RAM */
    printf("total_MB  pass1_ms  pass2_ms  minor_faults  major_faults\n");
    for (int k = 1; k <= steps; k++) {
        size_t size = step * k;
        unsigned char *p = malloc(size);
        if (!p) { printf("%8d  malloc failed\n", k * 512); break; }

        long maj, f0 = minflt(&maj);
        double t0 = now_ms();
        for (size_t i = 0; i < size; i += 4096) p[i] = (unsigned char)i;
        double t1 = now_ms();

        volatile unsigned long long sum = 0;
        double t2 = now_ms();
        for (size_t i = 0; i < size; i += 4096) sum += p[i];
        double t3 = now_ms();

        long maj2, f1 = minflt(&maj2);
        printf("%8d %9.1f %9.1f %13ld %13ld\n", k * 512, t1 - t0, t3 - t2,
               f1 - f0, maj2 - maj);
        free(p);
    }
    return 0;
}
