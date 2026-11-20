/* Q3: external fragmentation after loading and removing 20 segments (first fit). */
#include <stdio.h>
#include <stdlib.h>

#define MEMORY  (4u * 1024 * 1024)
#define SEGS    20u
#define MAXF    64

struct block { unsigned start, size; int used; };

static struct block mem[MAXF];
static int nblocks;

static unsigned long long rng_state = 987654321ULL;
static unsigned next_rand(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned)(rng_state >> 33);
}

static void init(void)
{
    mem[0].start = 0; mem[0].size = MEMORY; mem[0].used = 0;
    nblocks = 1;
}

static int first_fit(unsigned size)
{
    for (int i = 0; i < nblocks; i++) {
        if (!mem[i].used && mem[i].size >= size) return i;
    }
    return -1;
}

static void split_and_use(int i, unsigned size)
{
    if (mem[i].size > size) {
        for (int j = nblocks; j > i; j--) mem[j] = mem[j - 1];
        nblocks++;
        mem[i + 1].start = mem[i].start + size;
        mem[i + 1].size = mem[i].size - size;
        mem[i + 1].used = 0;
        mem[i].size = size;
    }
    mem[i].used = 1;
}

static void release(unsigned start)
{
    for (int i = 0; i < nblocks; i++) {
        if (mem[i].used && mem[i].start == start) { mem[i].used = 0; break; }
    }
    /* coalesce neighbours */
    for (int i = 0; i + 1 < nblocks; ) {
        if (!mem[i].used && !mem[i + 1].used) {
            mem[i].size += mem[i + 1].size;
            for (int j = i + 1; j + 1 < nblocks; j++) mem[j] = mem[j + 1];
            nblocks--;
        } else i++;
    }
}

static void report(const char *label)
{
    unsigned free_total = 0, largest = 0, free_blocks = 0;
    for (int i = 0; i < nblocks; i++) {
        if (!mem[i].used) {
            free_blocks++;
            free_total += mem[i].size;
            if (mem[i].size > largest) largest = mem[i].size;
        }
    }
    printf("%-22s free %7u KB in %2u blocks, largest %6u KB, external fragmentation %5.1f%%\n",
           label, free_total / 1024, free_blocks, largest / 1024,
           free_total ? 100.0 * (double)(free_total - largest) / (double)free_total : 0.0);
}

int main(void)
{
    unsigned size[SEGS], start[SEGS];
    int ok[SEGS];
    init();
    printf("segment  size_KB  start_KB  result\n");
    for (unsigned i = 0; i < SEGS; i++) {
        size[i] = (50 + next_rand() % 750) * 1024;
        int b = first_fit(size[i]);
        if (b < 0) { ok[i] = 0; start[i] = 0; printf("%7u %8u         -  no block\n", i + 1, size[i] / 1024); continue; }
        start[i] = mem[b].start;
        split_and_use(b, size[i]);
        ok[i] = 1;
        printf("%7u %8u %9u  loaded\n", i + 1, size[i] / 1024, start[i] / 1024);
    }
    report("after 20 loads");

    for (unsigned i = 0; i < SEGS; i += 2) if (ok[i]) release(start[i]);
    report("after removing 10");

    unsigned want = 900 * 1024;
    printf("900 KB request after removals: %s\n", first_fit(want) >= 0 ? "fits" : "FAILS although total free is larger");
    report("final state");
    return 0;
}
