/* Q4: page faults for sequential and random first touch of 256 MB, and the
   cost of a second pass over the same pages. Each test uses a fresh mapping;
   the random order is a permutation, so both tests touch every page once. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
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
    const size_t size = 256UL * 1024 * 1024;
    const size_t ps = 4096;
    const size_t pages = size / ps;
    unsigned char *p;
    unsigned *order;
    long f0, f1, maj;
    double t0, t1;
    volatile unsigned long long sum;

    p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    f0 = minflt(&maj); t0 = now_ms();
    for (size_t i = 0; i < pages; i++) p[i * ps] = (unsigned char)i;
    t1 = now_ms(); f1 = minflt(&maj);
    printf("sequential first touch : %ld minor, %ld major, %8.2f ms (%4.0f ns/page)\n",
           f1 - f0, maj, t1 - t0, (t1 - t0) * 1e6 / (double)pages);
    f0 = minflt(&maj); t0 = now_ms(); sum = 0;
    for (size_t i = 0; i < pages; i++) sum += p[i * ps];
    t1 = now_ms(); f1 = minflt(&maj);
    printf("sequential second pass : %ld minor, %ld major, %8.2f ms (%4.0f ns/page)\n",
           f1 - f0, maj, t1 - t0, (t1 - t0) * 1e6 / (double)pages);
    munmap(p, size);

    p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    order = malloc(pages * sizeof *order);
    if (!order) { perror("malloc"); return 1; }
    for (size_t i = 0; i < pages; i++) order[i] = (unsigned)i;
    srand(20261126);
    for (size_t i = pages - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        unsigned t = order[i]; order[i] = order[j]; order[j] = t;
    }
    f0 = minflt(&maj); t0 = now_ms();
    for (size_t i = 0; i < pages; i++) p[(size_t)order[i] * ps] = (unsigned char)i;
    t1 = now_ms(); f1 = minflt(&maj);
    printf("random     first touch : %ld minor, %ld major, %8.2f ms (%4.0f ns/page)\n",
           f1 - f0, maj, t1 - t0, (t1 - t0) * 1e6 / (double)pages);
    f0 = minflt(&maj); t0 = now_ms(); sum = 0;
    for (size_t i = 0; i < pages; i++) sum += p[(size_t)order[i] * ps];
    t1 = now_ms(); f1 = minflt(&maj);
    printf("random     second pass : %ld minor, %ld major, %8.2f ms (%4.0f ns/page)\n",
           f1 - f0, maj, t1 - t0, (t1 - t0) * 1e6 / (double)pages);
    printf("checksum %llu\n", sum);

    free(order);
    munmap(p, size);
    return 0;
}
