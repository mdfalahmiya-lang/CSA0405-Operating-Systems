/* Q7: worst-case delay of a time-critical loop with and without locked memory.
   The buffer is pre-faulted, then between rounds the unlocked run asks the
   kernel to page it out (MADV_PAGEOUT), so the next round really re-faults it.
   The locked run cannot be paged out, so it shows the delay without faults.
   mode 0 = unlocked, mode 1 = mlockall() + mlock() */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>

static long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static long faults(long *maj)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    *maj = ru.ru_majflt;
    return ru.ru_minflt;
}

static int cmp(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    int lock = argc > 1 ? atoi(argv[1]) : 0;
    int mb = argc > 2 ? atoi(argv[2]) : 32;
    int rounds = argc > 3 ? atoi(argv[3]) : 5;
    int iters = argc > 4 ? atoi(argv[4]) : 200000;
    size_t size = (size_t)mb * 1024 * 1024;

    if (lock && mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { perror("mlockall"); return 1; }
    unsigned char *buf = mmap(NULL, size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); return 1; }

    /* Make every page dirty, so paging it out really writes it to swap. */
    for (size_t i = 0; i < size; i += 4096) buf[i] = (unsigned char)i;
    if (lock && mlock(buf, size) != 0) { perror("mlock"); return 1; }

    printf("mode=%s buffer=%d MB rounds=%d iters=%d\n",
           lock ? "locked" : "unlocked", mb, rounds, iters);

    long *lat = malloc(sizeof(long) * iters);
    if (!lat) return 1;

    for (int r = 0; r < rounds; r++) {
        long maj, f0 = faults(&maj);
        long t_round = now_ns();
        long sum = 0;
        for (int i = 0; i < iters; i++) {
            long a = now_ns();
            volatile unsigned char v = buf[(size_t)(i % (size / 4096)) * 4096];
            (void)v;
            long d = now_ns() - a;
            lat[i] = d;
            sum += d;
        }
        long total = now_ns() - t_round;
        long maj2, f1 = faults(&maj2);
        qsort(lat, iters, sizeof(long), cmp);
        long p999 = lat[(long)(iters * 0.999)];
        printf("round %d: total %.2f ms, mean %.1f ns, p99.9 %ld ns, max %ld ns, minor %ld, major %ld\n",
               r + 1, total / 1e6, (double)sum / iters, p999, lat[iters - 1], f1 - f0, maj2 - maj);

        if (!lock && r + 1 < rounds) {
            int rc = madvise(buf, size, MADV_PAGEOUT);
            printf("          madvise(MADV_PAGEOUT) -> %s, waiting for writeback\n",
                   rc == 0 ? "ok" : strerror(errno));
            usleep(300000);
        } else if (lock && r == 0) {
            int rc = madvise(buf, size, MADV_PAGEOUT);
            printf("          madvise(MADV_PAGEOUT) on locked pages -> %s\n",
                   rc == 0 ? "unexpectedly accepted" : strerror(errno));
        }
    }
    free(lat);
    munmap(buf, size);
    return 0;
}
