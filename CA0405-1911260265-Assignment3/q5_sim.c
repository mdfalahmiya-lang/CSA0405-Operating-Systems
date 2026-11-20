/* Q5: FIFO, LRU, Optimal and Clock on locality and random strings,
   with fault counts for 3..10 frames and timings for 100,000 references. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PAGES   50
#define REFS    1000
#define TIMEREF 100000
#define MAXF    16

static unsigned long long rng_state = 20261119ULL;
static unsigned next_rand(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned)(rng_state >> 33);
}

static void make_locality(int ref[], int n)
{
    int base = 0;
    for (int i = 0; i < n; i++) {
        if (next_rand() % 100 < 80) base = (base + (int)(next_rand() % 3) - 1 + PAGES) % PAGES;
        else base = (int)(next_rand() % PAGES);
        ref[i] = (base + (int)(next_rand() % 10)) % PAGES;
    }
}

static void make_random(int ref[], int n)
{
    for (int i = 0; i < n; i++) ref[i] = (int)(next_rand() % PAGES);
}

static int find(const int f[], int used, int page)
{
    for (int i = 0; i < used; i++) if (f[i] == page) return i;
    return -1;
}

static int fifo(const int ref[], int n, int frames)
{
    int f[MAXF], used = 0, next = 0, faults = 0;
    for (int i = 0; i < n; i++) {
        if (find(f, used, ref[i]) >= 0) continue;
        faults++;
        if (used < frames) f[used++] = ref[i];
        else { f[next] = ref[i]; next = (next + 1) % frames; }
    }
    return faults;
}

static int lru(const int ref[], int n, int frames)
{
    int f[MAXF], last[MAXF], used = 0, faults = 0;
    for (int i = 0; i < n; i++) {
        int k = find(f, used, ref[i]);
        if (k >= 0) { last[k] = i; continue; }
        faults++;
        if (used < frames) { f[used] = ref[i]; last[used] = i; used++; }
        else {
            int victim = 0;
            for (int j = 1; j < frames; j++) if (last[j] < last[victim]) victim = j;
            f[victim] = ref[i]; last[victim] = i;
        }
    }
    return faults;
}

/* Optimal with next-use preprocessing: O(n * frames), not O(n^2 * frames). */
static int optimal(const int ref[], int n, int frames)
{
    static int nextsame[TIMEREF + 1];
    int lastuse[PAGES];
    for (int i = 0; i < PAGES; i++) lastuse[i] = n;
    for (int i = n - 1; i >= 0; i--) {
        nextsame[i] = lastuse[ref[i]];
        lastuse[ref[i]] = i;
    }
    nextsame[n] = n;

    int f[MAXF], pos[MAXF], used = 0, faults = 0;
    for (int i = 0; i < n; i++) {
        int hit = find(f, used, ref[i]);
        if (hit >= 0) { pos[hit] = i; continue; }
        faults++;
        if (used < frames) { f[used] = ref[i]; pos[used] = i; used++; continue; }
        int victim = 0;
        for (int j = 1; j < frames; j++)
            if (nextsame[pos[j]] > nextsame[pos[victim]]) victim = j;
        f[victim] = ref[i];
        pos[victim] = i;
    }
    return faults;
}

static int clock_alg(const int ref[], int n, int frames)
{
    int f[MAXF], bit[MAXF], used = 0, hand = 0, faults = 0;
    for (int i = 0; i < n; i++) {
        int k = find(f, used, ref[i]);
        if (k >= 0) { bit[k] = 1; continue; }
        faults++;
        if (used < frames) { f[used] = ref[i]; bit[used] = 1; used++; continue; }
        while (bit[hand]) { bit[hand] = 0; hand = (hand + 1) % frames; }
        f[hand] = ref[i]; bit[hand] = 1; hand = (hand + 1) % frames;
    }
    return faults;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void)
{
    static int loc[REFS], rnd[REFS], big[TIMEREF];
    make_locality(loc, REFS);
    make_random(rnd, REFS);
    make_locality(big, TIMEREF);

    printf("string,frames,FIFO,LRU,Optimal,Clock\n");
    for (int fr = 3; fr <= 10; fr++)
        printf("locality,%d,%d,%d,%d,%d\n", fr, fifo(loc, REFS, fr), lru(loc, REFS, fr),
               optimal(loc, REFS, fr), clock_alg(loc, REFS, fr));
    for (int fr = 3; fr <= 10; fr++)
        printf("random,%d,%d,%d,%d,%d\n", fr, fifo(rnd, REFS, fr), lru(rnd, REFS, fr),
               optimal(rnd, REFS, fr), clock_alg(rnd, REFS, fr));

    printf("\ntiming (ms for %d references, 8 frames, median of 7 runs)\n", TIMEREF);
    const char *names[] = {"FIFO", "LRU", "Optimal", "Clock"};
    int (*alg[])(const int *, int, int) = {fifo, lru, optimal, clock_alg};
    for (int a = 0; a < 4; a++) {
        double best[7];
        int check = 0;
        for (int r = 0; r < 7; r++) {
            double t0 = now_ms();
            check += alg[a](big, TIMEREF, 8);
            best[r] = now_ms() - t0;
        }
        for (int i = 0; i < 7; i++) for (int j = i + 1; j < 7; j++)
            if (best[j] < best[i]) { double t = best[i]; best[i] = best[j]; best[j] = t; }
        printf("%-8s %8.2f  (faults %d)\n", names[a], best[3], check);
    }
    return 0;
}
