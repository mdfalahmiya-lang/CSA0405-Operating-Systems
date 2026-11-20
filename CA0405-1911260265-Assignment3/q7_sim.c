/* Q7: simulate three tasks on a 2 GB device with LRU and Clock.
   Frames are 1 MB; 1800 frames are available to user tasks after the OS
   reservation. The real-time task is pinned (never evicted) or not pinned.
   A warm-up period fills the caches; only the measured period is counted. */
#include <stdio.h>
#include <string.h>

#define FRAMES      1800
#define WARMUP      5000
#define REFS        20000
#define TOTAL       (WARMUP + REFS)
#define RT_PAGES    200
#define ML_PAGES    1536
#define LOG_PAGES   96
#define PAGES       (RT_PAGES + ML_PAGES + LOG_PAGES)

static unsigned long long rng_state = 424242ULL;
static unsigned next_rand(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned)(rng_state >> 33);
}

static int refs[TOTAL];
static int task[TOTAL];       /* 0 real-time, 1 ML, 2 log */

static void make_refs(void)
{
    int logpos = 0;
    /* Pre-touch every real-time page during the warm-up: after that, a pinned
       real-time task should never fault again. */
    for (int i = 0; i < RT_PAGES; i++) {
        task[i] = 0;
        refs[i] = i;
    }
    for (int i = RT_PAGES; i < TOTAL; i++) {
        unsigned r = next_rand() % 100;
        if (r < 10) {                 /* 10 % periodic real-time references */
            task[i] = 0;
            refs[i] = (int)(next_rand() % RT_PAGES);
        } else if (r < 90) {          /* 80 % machine-learning working set */
            task[i] = 1;
            refs[i] = RT_PAGES + (int)(next_rand() % ML_PAGES);
        } else {                      /* 10 % sequential logging */
            task[i] = 2;
            refs[i] = RT_PAGES + ML_PAGES + (logpos++ % LOG_PAGES);
        }
    }
}

struct result { long fault[3]; };

static int victim_lru(const int f[], const long last[], int frames, int pin)
{
    int v = -1;
    long oldest = 0;
    for (int j = 0; j < frames; j++) {
        if (pin && f[j] < RT_PAGES) continue;
        if (v < 0 || last[j] < oldest) { oldest = last[j]; v = j; }
    }
    if (v < 0) {          /* every frame is pinned: fall back to any frame */
        v = 0;
        for (int j = 1; j < frames; j++) if (last[j] < last[v]) v = j;
    }
    return v;
}

static struct result run_lru(int pin)
{
    static int f[FRAMES];
    static long last[FRAMES];
    struct result res = {{0, 0, 0}};
    int used = 0;
    memset(last, -1, sizeof last);
    for (int i = 0; i < TOTAL; i++) {
        int p = refs[i], hit = -1;
        for (int j = 0; j < used; j++) if (f[j] == p) { hit = j; break; }
        if (hit >= 0) { last[hit] = i; continue; }
        if (i >= WARMUP) res.fault[task[i]]++;
        if (used < FRAMES) { f[used] = p; last[used] = i; used++; }
        else {
            int v = victim_lru(f, last, FRAMES, pin);
            f[v] = p; last[v] = i;
        }
    }
    return res;
}

static int victim_clock(const int f[], int bit[], int *hand, int frames, int pin)
{
    int h = *hand;
    for (int scanned = 0; scanned < 2 * frames; scanned++) {
        if (!(pin && f[h] < RT_PAGES)) {
            if (!bit[h]) { *hand = h; return h; }
            bit[h] = 0;
        }
        h = (h + 1) % frames;
    }
    *hand = h;
    return h;
}

static struct result run_clock(int pin)
{
    static int f[FRAMES], bit[FRAMES];
    struct result res = {{0, 0, 0}};
    int used = 0, hand = 0;
    memset(bit, 0, sizeof bit);
    for (int i = 0; i < TOTAL; i++) {
        int p = refs[i], hit = -1;
        for (int j = 0; j < used; j++) if (f[j] == p) { hit = j; break; }
        if (hit >= 0) { bit[hit] = 1; continue; }
        if (i >= WARMUP) res.fault[task[i]]++;
        if (used < FRAMES) { f[used] = p; bit[used] = 1; used++; }
        else {
            int v = victim_clock(f, bit, &hand, FRAMES, pin);
            f[v] = p; bit[v] = 1; hand = (v + 1) % FRAMES;
        }
    }
    return res;
}

int main(void)
{
    make_refs();
    printf("frames=%d (1 MB), warm-up=%d, measured=%d references\n", FRAMES, WARMUP, REFS);
    printf("task set: real-time %d MB, ML %d MB, log %d MB = %d MB, OS reservation %d MB\n",
           RT_PAGES, ML_PAGES, LOG_PAGES, PAGES, 2048 - FRAMES);
    printf("policy,pinned,rt_faults,ml_faults,log_faults,total\n");
    for (int pin = 0; pin <= 1; pin++) {
        struct result a = run_lru(pin);
        printf("LRU,%d,%ld,%ld,%ld,%ld\n", pin, a.fault[0], a.fault[1], a.fault[2],
               a.fault[0] + a.fault[1] + a.fault[2]);
        struct result b = run_clock(pin);
        printf("Clock,%d,%ld,%ld,%ld,%ld\n", pin, b.fault[0], b.fault[1], b.fault[2],
               b.fault[0] + b.fault[1] + b.fault[2]);
    }
    return 0;
}
